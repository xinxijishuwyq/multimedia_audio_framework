/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#undef LOG_TAG
#define LOG_TAG "AudioEnhanceChain"

#include "audio_enhance_chain.h"

#include <chrono>

#include "securec.h"
#include "audio_log.h"
#include "audio_errors.h"

namespace OHOS {
namespace AudioStandard {

AudioEnhanceChain::AudioEnhanceChain(const std::string &scene)
{
    setConfigFlag_ = false;
    sceneType_ = scene;
    enhanceMode_ = "EFFECT_DEFAULT";
}

AudioEnhanceChain::~AudioEnhanceChain()
{
    ReleaseEnhanceChain();
}

void AudioEnhanceChain::SetEnhanceMode(const std::string &mode)
{
    enhanceMode_ = mode;
}

void AudioEnhanceChain::ReleaseEnhanceChain()
{
    for (uint32_t i = 0; i < standByEnhanceHandles_.size() && i < enhanceLibHandles_.size(); i++) {
        if (!enhanceLibHandles_[i]) {
            continue;
        }
        if (!standByEnhanceHandles_[i]) {
            continue;
        }
        if (!enhanceLibHandles_[i]->releaseEffect) {
            continue;
        }
        enhanceLibHandles_[i]->releaseEffect(standByEnhanceHandles_[i]);
    }
    standByEnhanceHandles_.clear();
    enhanceLibHandles_.clear();
}

void AudioEnhanceChain::AddEnhanceHandle(AudioEffectHandle handle, AudioEffectLibrary *libHandle)
{
    std::lock_guard<std::mutex> lock(reloadMutex_);
    int32_t ret = 0;
    AudioEffectTransInfo cmdInfo = {};
    AudioEffectTransInfo replyInfo = {};

    ret = (*handle)->command(handle, EFFECT_CMD_INIT, &cmdInfo, &replyInfo);
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with [%{public}s], either one of libs EFFECT_CMD_INIT fail",
        sceneType_.c_str(), enhanceMode_.c_str());

    standByEnhanceHandles_.emplace_back(handle);
    enhanceLibHandles_.emplace_back(libHandle);
}

void GetOneFrameInputData(EnhanceBufferAttr *enhanceBufferAttr, std::vector<uint8_t> &input, uint32_t inputLen)
{
    int32_t ret = 0;
    input.reserve(inputLen);
    std::vector<std::vector<uint8_t>> cache;
    cache.resize(enhanceBufferAttr->batchLen);
    for (auto &it : cache) {
        it.resize(enhanceBufferAttr->byteLenPerFrame);
    }
    // ref channel
    for (uint32_t j = 0; j < enhanceBufferAttr->refNum; ++j) {
        ret = memset_s(cache[j].data(), cache[j].size(), 0, cache[j].size());
        CHECK_AND_RETURN_LOG(ret == 0, "memset error in ref channel memcpy");
    }
    uint32_t index = 0;
    for (uint32_t i = 0; i < enhanceBufferAttr->byteLenPerFrame / enhanceBufferAttr->bitDepth; ++i) {
        // mic channel
        for (uint32_t j = enhanceBufferAttr->refNum; j < enhanceBufferAttr->batchLen; ++j) {
            ret = memcpy_s(&cache[j][i * enhanceBufferAttr->bitDepth], enhanceBufferAttr->bitDepth,
                enhanceBufferAttr->input + index, enhanceBufferAttr->bitDepth);
            CHECK_AND_RETURN_LOG(ret == 0, "memcpy error in mic channel memcpy");
            index += enhanceBufferAttr->bitDepth;
        }
    }
    for (uint32_t i = 0; i < enhanceBufferAttr->batchLen; ++i) {
        input.insert(input.end(), cache[i].begin(), cache[i].end());
    }
}

int32_t AudioEnhanceChain::ApplyEnhanceChain(EnhanceBufferAttr *enhanceBufferAttr)
{
    CHECK_AND_RETURN_RET_LOG(enhanceBufferAttr != nullptr, ERROR, "enhance buffer is null");
    
    enhanceBufferAttr->refNum = REF_CHANNEL_NUM_MAP.find(sceneType_)->second;
    enhanceBufferAttr->batchLen = enhanceBufferAttr->refNum + enhanceBufferAttr->micNum;
    uint32_t inputLen = enhanceBufferAttr->byteLenPerFrame * enhanceBufferAttr->batchLen;
    uint32_t outputLen = enhanceBufferAttr->byteLenPerFrame * enhanceBufferAttr->outNum;
    if (IsEmptyEnhanceHandles()) {
        AUDIO_ERR_LOG("audioEnhanceChain->standByEnhanceHandles is empty");
        CHECK_AND_RETURN_RET_LOG(memcpy_s(enhanceBufferAttr->output, outputLen, enhanceBufferAttr->input,
            outputLen) == 0, ERROR, "memcpy error in IsEmptyEnhanceHandles");
        return ERROR;
    }
    std::vector<uint8_t> input;
    GetOneFrameInputData(enhanceBufferAttr, input, inputLen);
    std::vector<uint8_t> output;
    output.resize(outputLen);

    AudioBuffer audioBufIn_ = {};
    AudioBuffer audioBufOut_ = {};
    audioBufIn_.frameLength = inputLen;
    audioBufOut_.frameLength = outputLen;
    audioBufIn_.raw = static_cast<void *>(input.data());
    audioBufOut_.raw = static_cast<void *>(output.data());

    {
        std::lock_guard<std::mutex> lock(reloadMutex_);
        for (AudioEffectHandle handle : standByEnhanceHandles_) {
            int32_t ret = 0;
            if (!setConfigFlag_) {
                AudioEffectTransInfo cmdInfo = {};
                AudioEffectTransInfo replyInfo = {};
                DataDescription desc = DataDescription(enhanceBufferAttr->frameLength, enhanceBufferAttr->sampleRate,
                    enhanceBufferAttr->dataFormat, enhanceBufferAttr->micNum, enhanceBufferAttr->refNum,
                    enhanceBufferAttr->outNum);
                cmdInfo.data = static_cast<void *>(&desc);
                cmdInfo.size = sizeof(desc);

                ret = (*handle)->command(handle, EFFECT_CMD_SET_CONFIG, &cmdInfo, &replyInfo);
                CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "[%{publc}s] with mode [%{public}s], either one of libs \
                    EFFECT_CMD_SET_CONFIG fai", sceneType_.c_str(), enhanceMode_.c_str());
                setConfigFlag_ = true;
            }
            ret = (*handle)->process(handle, &audioBufIn_, &audioBufOut_);
            CHECK_AND_CONTINUE_LOG(ret == 0, "[%{publc}s] with mode [%{public}s], either one of libs process fail",
                sceneType_.c_str(), enhanceMode_.c_str());
        }
        CHECK_AND_RETURN_RET_LOG(memcpy_s(enhanceBufferAttr->output, outputLen, audioBufOut_.raw, outputLen) == 0,
            ERROR, "memcpy error in audioBufOut_ to enhanceBufferAttr->output");
        return SUCCESS;
    }
}

bool AudioEnhanceChain::IsEmptyEnhanceHandles()
{
    std::lock_guard<std::mutex> lock(reloadMutex_);
    return standByEnhanceHandles_.size() == 0;
}

} // namespace AudioStandard
} // namespace OHOS