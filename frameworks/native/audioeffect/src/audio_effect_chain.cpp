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
#define LOG_TAG "AudioEffectChain"

#include "securec.h"
#include "audio_effect_chain.h"
#include "audio_effect_chain_adapter.h"
#include "audio_log.h"
#include "audio_errors.h"
#include "audio_effect.h"
#include "audio_utils.h"

namespace OHOS {
namespace AudioStandard {

const uint32_t NUM_SET_EFFECT_PARAM = 5;
const uint32_t DEFAULT_SAMPLE_RATE = 48000;
const uint32_t DEFAULT_NUM_CHANNEL = STEREO;
const uint64_t DEFAULT_NUM_CHANNELLAYOUT = CH_LAYOUT_STEREO;

#ifdef SENSOR_ENABLE
AudioEffectChain::AudioEffectChain(std::string scene, std::shared_ptr<HeadTracker> headTracker)
{
    sceneType = scene;
    effectMode = AUDIO_SUPPORTED_SCENE_MODES.find(EFFECT_DEFAULT)->second;
    audioBufIn.frameLength = 0;
    audioBufOut.frameLength = 0;
    ioBufferConfig.inputCfg.samplingRate = DEFAULT_SAMPLE_RATE;
    ioBufferConfig.inputCfg.channels = DEFAULT_NUM_CHANNEL;
    ioBufferConfig.inputCfg.format = DATA_FORMAT_F32;
    ioBufferConfig.inputCfg.channelLayout = DEFAULT_NUM_CHANNELLAYOUT;
    ioBufferConfig.outputCfg.samplingRate = DEFAULT_SAMPLE_RATE;
    ioBufferConfig.outputCfg.channels = DEFAULT_NUM_CHANNEL;
    ioBufferConfig.outputCfg.format = DATA_FORMAT_F32;
    ioBufferConfig.outputCfg.channelLayout = DEFAULT_NUM_CHANNELLAYOUT;
    headTracker_ = headTracker;
}
#else
AudioEffectChain::AudioEffectChain(std::string scene)
{
    sceneType = scene;
    effectMode = AUDIO_SUPPORTED_SCENE_MODES.find(EFFECT_DEFAULT)->second;
    audioBufIn.frameLength = 0;
    audioBufOut.frameLength = 0;
    ioBufferConfig.inputCfg.samplingRate = DEFAULT_SAMPLE_RATE;
    ioBufferConfig.inputCfg.channels = DEFAULT_NUM_CHANNEL;
    ioBufferConfig.inputCfg.format = DATA_FORMAT_F32;
    ioBufferConfig.inputCfg.channelLayout = DEFAULT_NUM_CHANNELLAYOUT;
    ioBufferConfig.outputCfg.samplingRate = DEFAULT_SAMPLE_RATE;
    ioBufferConfig.outputCfg.channels = DEFAULT_NUM_CHANNEL;
    ioBufferConfig.outputCfg.format = DATA_FORMAT_F32;
    ioBufferConfig.outputCfg.channelLayout = DEFAULT_NUM_CHANNELLAYOUT;
}
#endif

AudioEffectChain::~AudioEffectChain()
{
    std::lock_guard<std::mutex> lock(reloadMutex);
    ReleaseEffectChain();
}

void AudioEffectChain::Dump()
{
    std::lock_guard<std::mutex> lock(reloadMutex);
    for (AudioEffectHandle handle : standByEffectHandles) {
        AUDIO_INFO_LOG("standByEffectHandle for [%{public}s], handle address is %{public}p", sceneType.c_str(), handle);
    }
}

std::string AudioEffectChain::GetEffectMode()
{
    return effectMode;
}

void AudioEffectChain::SetEffectMode(std::string mode)
{
    effectMode = mode;
}

void AudioEffectChain::ReleaseEffectChain()
{
    for (uint32_t i = 0; i < standByEffectHandles.size() && i < libHandles.size(); ++i) {
        if (!libHandles[i]) {
            continue;
        }
        if (!standByEffectHandles[i]) {
            continue;
        }
        if (!libHandles[i]->releaseEffect) {
            continue;
        }
        libHandles[i]->releaseEffect(standByEffectHandles[i]);
    }
    standByEffectHandles.clear();
    libHandles.clear();
}

void AudioEffectChain::AddEffectHandle(AudioEffectHandle handle, AudioEffectLibrary *libHandle,
    AudioEffectScene currSceneType)
{
    int32_t ret;
    int32_t replyData = 0;
    AudioEffectTransInfo cmdInfo = {sizeof(AudioEffectConfig), &ioBufferConfig};
    AudioEffectTransInfo replyInfo = {sizeof(int32_t), &replyData};
    ret = (*handle)->command(handle, EFFECT_CMD_INIT, &cmdInfo, &replyInfo);
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with mode [%{public}s], either one of libs EFFECT_CMD_INIT fail",
        sceneType.c_str(), effectMode.c_str());
    ret = (*handle)->command(handle, EFFECT_CMD_ENABLE, &cmdInfo, &replyInfo);
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with mode [%{public}s], either one of libs EFFECT_CMD_ENABLE fail",
        sceneType.c_str(), effectMode.c_str());
    ret = (*handle)->command(handle, EFFECT_CMD_SET_CONFIG, &cmdInfo, &replyInfo);
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with mode [%{public}s], either one of libs EFFECT_CMD_SET_CONFIG fail",
        sceneType.c_str(), effectMode.c_str());
    // Set param
    AudioEffectParam *effectParam =
        new AudioEffectParam[sizeof(AudioEffectParam) + NUM_SET_EFFECT_PARAM * sizeof(int32_t)];
    effectParam->status = 0;
    effectParam->paramSize = sizeof(int32_t);
    effectParam->valueSize = 0;
    int32_t *data = &(effectParam->data[0]);
    *data++ = EFFECT_SET_PARAM;
    *data++ = static_cast<int32_t>(currSceneType);
    *data++ = GetKeyFromValue(AUDIO_SUPPORTED_SCENE_MODES, effectMode);
#ifdef WINDOW_MANAGER_ENABLE
    std::shared_ptr<AudioEffectRotation> audioEffectRotation = AudioEffectRotation::GetInstance();
    if (audioEffectRotation == nullptr) {
        *data++ = 0;
    } else {
        *data++ = audioEffectRotation->GetRotation();
    }
#else
    *data++ = 0;
#endif
    AUDIO_DEBUG_LOG("set ap integration rotation: %{public}u", *(data - 1));
    std::shared_ptr<AudioEffectVolume> audioEffectVolume = AudioEffectVolume::GetInstance();
    if (audioEffectVolume == nullptr) {
        *data++ = 0;
    } else {
        *data++ = audioEffectVolume->GetApVolume(sceneType);
    }
    AUDIO_DEBUG_LOG("set ap integration volume: %{public}u", *(data - 1));
    cmdInfo = {sizeof(AudioEffectParam) + sizeof(int32_t) * NUM_SET_EFFECT_PARAM, effectParam};
    ret = (*handle)->command(handle, EFFECT_CMD_SET_PARAM, &cmdInfo, &replyInfo);
    delete[] effectParam;
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with mode [%{public}s], either one of libs EFFECT_CMD_SET_PARAM fail",
        sceneType.c_str(), effectMode.c_str());
    standByEffectHandles.emplace_back(handle);
    libHandles.emplace_back(libHandle);
    latency_ += static_cast<uint32_t>(replyData);
}

int32_t AudioEffectChain::SetEffectParam(AudioEffectScene currSceneType)
{
    std::lock_guard<std::mutex> lock(reloadMutex);
    latency_ = 0;
    for (AudioEffectHandle handle : standByEffectHandles) {
        AudioEffectParam *effectParam =
            new AudioEffectParam[sizeof(AudioEffectParam) + NUM_SET_EFFECT_PARAM * sizeof(int32_t)];
        effectParam->status = 0;
        effectParam->paramSize = sizeof(int32_t);
        effectParam->valueSize = 0;
        int32_t *data = &(effectParam->data[0]);
        *data++ = EFFECT_SET_PARAM;
        *data++ = static_cast<int32_t>(currSceneType);
        *data++ = GetKeyFromValue(AUDIO_SUPPORTED_SCENE_MODES, effectMode);
#ifdef WINDOW_MANAGER_ENABLE
        std::shared_ptr<AudioEffectRotation> audioEffectRotation = AudioEffectRotation::GetInstance();
        if (audioEffectRotation == nullptr) {
            AUDIO_DEBUG_LOG("null audioEffectRotation");
            *data++ = 0;
        } else {
            *data++ = audioEffectRotation->GetRotation();
        }
#else
        *data++ = 0;
#endif
        AUDIO_DEBUG_LOG("set ap integration rotation: %{public}u", *(data - 1));
        std::shared_ptr<AudioEffectVolume> audioEffectVolume = AudioEffectVolume::GetInstance();
        if (audioEffectVolume == nullptr) {
            AUDIO_DEBUG_LOG("null audioEffectVolume");
            *data++ = 0;
        } else {
            *data++ = audioEffectVolume->GetApVolume(sceneType);
        }
        AUDIO_DEBUG_LOG("set ap integration volume: %{public}u", *(data - 1));
        int32_t replyData = 0;
        AudioEffectTransInfo cmdInfo = {sizeof(AudioEffectParam) + sizeof(int32_t) * NUM_SET_EFFECT_PARAM, effectParam};
        AudioEffectTransInfo replyInfo = {sizeof(int32_t), &replyData};
        int32_t ret = (*handle)->command(handle, EFFECT_CMD_SET_PARAM, &cmdInfo, &replyInfo);
        delete[] effectParam;
        CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "set rotation EFFECT_CMD_SET_PARAM fail");
        latency_ += replyData;
    }
    return SUCCESS;
}

void AudioEffectChain::ApplyEffectChain(float *bufIn, float *bufOut, uint32_t frameLen, AudioEffectProcInfo procInfo)
{
    if (IsEmptyEffectHandles()) {
        size_t totlen = frameLen * ioBufferConfig.outputCfg.channels * sizeof(float);
        CHECK_AND_RETURN_LOG(memcpy_s(bufOut, totlen, bufIn, totlen) == 0, "memcpy error in apply effect");
        return;
    }

#ifdef SENSOR_ENABLE
    int32_t replyData = 0;
    auto imuData = headTracker_->GetHeadPostureData();
    AudioEffectTransInfo cmdInfo = {sizeof(HeadPostureData), &imuData};
    AudioEffectTransInfo replyInfo = {sizeof(int32_t), &replyData};
#endif

    audioBufIn.frameLength = frameLen;
    audioBufOut.frameLength = frameLen;
    int32_t count = 0;
    {
        std::lock_guard<std::mutex> lock(reloadMutex);
        for (AudioEffectHandle handle : standByEffectHandles) {
#ifdef SENSOR_ENABLE
            if ((!procInfo.offloadEnabled) && procInfo.headTrackingEnabled) {
                (*handle)->command(handle, EFFECT_CMD_SET_IMU, &cmdInfo, &replyInfo);
            }
#endif
            if ((count & 1) == 0) {
                audioBufIn.raw = bufIn;
                audioBufOut.raw = bufOut;
            } else {
                audioBufOut.raw = bufIn;
                audioBufIn.raw = bufOut;
            }
            int32_t ret = (*handle)->process(handle, &audioBufIn, &audioBufOut);
            CHECK_AND_CONTINUE_LOG(ret == 0, "[%{public}s] with mode [%{public}s], either one of libs process fail",
                sceneType.c_str(), effectMode.c_str());
            count++;
        }
    }

    if ((count & 1) == 0) {
        size_t totlen = frameLen * ioBufferConfig.outputCfg.channels * sizeof(float);
        CHECK_AND_RETURN_LOG(memcpy_s(bufOut, totlen, bufIn, totlen), "memcpy error when last copy");
    }
}

void AudioEffectChain::SetIOBufferConfig(bool isInput, uint32_t samplingRate, uint32_t channels)
{
    if (isInput) {
        ioBufferConfig.inputCfg.samplingRate = samplingRate;
        ioBufferConfig.inputCfg.channels = channels;
    } else {
        ioBufferConfig.outputCfg.samplingRate = samplingRate;
        ioBufferConfig.outputCfg.channels = channels;
    }
}

bool AudioEffectChain::IsEmptyEffectHandles()
{
    std::lock_guard<std::mutex> lock(reloadMutex);
    return standByEffectHandles.size() == 0;
}

int32_t AudioEffectChain::UpdateMultichannelIoBufferConfig(const uint32_t &channels, const uint64_t &channelLayout)
{
    std::lock_guard<std::mutex> lock(reloadMutex);
    if (ioBufferConfig.inputCfg.channels == channels && ioBufferConfig.inputCfg.channelLayout == channelLayout) {
        return SUCCESS;
    }
    ioBufferConfig.inputCfg.channels = channels;
    ioBufferConfig.inputCfg.channelLayout = channelLayout;
    int32_t replyData = 0;
    AudioEffectTransInfo cmdInfo = {sizeof(AudioEffectConfig), &ioBufferConfig};
    AudioEffectTransInfo replyInfo = {sizeof(int32_t), &replyData};
    for (AudioEffectHandle handle : standByEffectHandles) {
        int32_t ret = (*handle)->command(handle, EFFECT_CMD_SET_CONFIG, &cmdInfo, &replyInfo);
        CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "Multichannel effect chain update EFFECT_CMD_SET_CONFIG fail");
    }
    return SUCCESS;
}

AudioEffectConfig AudioEffectChain::GetIoBufferConfig()
{
    return ioBufferConfig;
}

void AudioEffectChain::StoreOldEffectChainInfo(std::string &sceneMode, AudioEffectConfig &ioBufferConfig)
{
    sceneMode = GetEffectMode();
    ioBufferConfig = GetIoBufferConfig();
    return;
}

uint32_t AudioEffectChain::GetLatency()
{
    return latency_;
}

#ifdef SENSOR_ENABLE
void AudioEffectChain::SetHeadTrackingDisabled()
{
    if (IsEmptyEffectHandles()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(reloadMutex);
        for (AudioEffectHandle handle : standByEffectHandles) {
            int32_t replyData = 0;
            HeadPostureData imuDataDisabled = {1, 1.0, 0.0, 0.0, 0.0};
            AudioEffectTransInfo cmdInfo = {sizeof(HeadPostureData), &imuDataDisabled};
            AudioEffectTransInfo replyInfo = {sizeof(int32_t), &replyData};
            int32_t ret = (*handle)->command(handle, EFFECT_CMD_SET_IMU, &cmdInfo, &replyInfo);
            if (ret != 0) {
                AUDIO_WARNING_LOG("SetHeadTrackingDisabled failed");
            }
        }
    }
}
#endif

void AudioEffectChain::InitEffectChain()
{
    if (IsEmptyEffectHandles()) {
        return;
    }
    std::lock_guard<std::mutex> lock(reloadMutex);
    for (AudioEffectHandle handle : standByEffectHandles) {
        int32_t replyData = 0;
        AudioEffectTransInfo cmdInfo = {sizeof(int32_t), &replyData};
        AudioEffectTransInfo replyInfo = {sizeof(int32_t), &replyData};
        int32_t ret = (*handle)->command(handle, EFFECT_CMD_ENABLE, &cmdInfo, &replyInfo);
        CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with mode [%{public}s], either one of libs EFFECT_CMD_ENABLE fail",
            sceneType.c_str(), effectMode.c_str());
    }
}
} // namespace AudioStandard
} // namespace OHOS