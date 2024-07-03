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

const uint32_t BITLENGTH = 8;
const uint32_t MILLISECOND = 1000;
const uint32_t DEFAULT_FRAMELENGTH = 20;
const uint32_t DEFAULT_SAMPLE_RATE = 48000;
const uint32_t DEFAULT_DATAFORMAT = 16;
const uint32_t DEFAULT_REF_NUM = 0;  // if sceneType is voip, refNum is 8
const uint32_t DEFAULT_MIC_NUM = 4;
const uint32_t DEFAULT_OUT_NUM = 4;

AudioEnhanceChain::AudioEnhanceChain(const std::string &scene, const std::string &mode)
{
    sceneType_ = scene;
    enhanceMode_ = mode;
    
    InitAudioEnhanceChain();
}

void AudioEnhanceChain::InitAudioEnhanceChain()
{
    setConfigFlag_ = false;
    enhanceLibHandles_.clear();
    standByEnhanceHandles_.clear();

    algoSupportedConfig_ = {DEFAULT_FRAMELENGTH, DEFAULT_SAMPLE_RATE, DEFAULT_DATAFORMAT, DEFAULT_MIC_NUM,
        DEFAULT_REF_NUM, DEFAULT_OUT_NUM};

    uint32_t batchLen = algoSupportedConfig_.refNum + algoSupportedConfig_.micNum;
    uint32_t bitDepth = algoSupportedConfig_.dataFormat / BITLENGTH;
    uint32_t byteLenPerFrame = algoSupportedConfig_.frameLength * (algoSupportedConfig_.sampleRate / MILLISECOND)
        * bitDepth;
    algoAttr_ = {bitDepth, batchLen, byteLenPerFrame};
}

AudioEnhanceChain::~AudioEnhanceChain()
{
    ReleaseEnhanceChain();
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
    std::lock_guard<std::mutex> lock(chainMutex_);
    int32_t ret = 0;
    AudioEffectTransInfo cmdInfo = {};
    AudioEffectTransInfo replyInfo = {};

    cmdInfo.data = static_cast<void *>(&algoSupportedConfig_);
    cmdInfo.size = sizeof(algoSupportedConfig_);

    ret = (*handle)->command(handle, EFFECT_CMD_INIT, &cmdInfo, &replyInfo);
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with [%{public}s], either one of libs EFFECT_CMD_INIT fail",
        sceneType_.c_str(), enhanceMode_.c_str());

    setConfigFlag_ = true;
    standByEnhanceHandles_.emplace_back(handle);
    enhanceLibHandles_.emplace_back(libHandle);
}

bool AudioEnhanceChain::IsEmptyEnhanceHandles()
{
    std::lock_guard<std::mutex> lock(chainMutex_);
    return standByEnhanceHandles_.size() == 0;
}

void AudioEnhanceChain::GetAlgoConfig(AudioBufferConfig &algoConfig)
{
    algoConfig.samplingRate = algoSupportedConfig_.sampleRate;
    algoConfig.channels = algoSupportedConfig_.micNum;
    algoConfig.format = static_cast<uint8_t>(algoSupportedConfig_.dataFormat);
    return;
}

uint32_t AudioEnhanceChain::GetAlgoBufferSize()
{
    return algoAttr_.byteLenPerFrame * algoSupportedConfig_.micNum;
}

uint32_t AudioEnhanceChain::GetAlgoBufferSizeEc()
{
    return algoAttr_.byteLenPerFrame * algoSupportedConfig_.refNum;
}

} // namespace AudioStandard
} // namespace OHOS