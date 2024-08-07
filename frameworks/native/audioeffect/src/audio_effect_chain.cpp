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

#include "audio_effect_chain.h"
#include "audio_effect_chain_adapter.h"
#include "audio_effect.h"
#include "audio_errors.h"
#include "audio_log.h"
#include "audio_utils.h"
#include "securec.h"

namespace OHOS {
namespace AudioStandard {

const uint32_t NUM_SET_EFFECT_PARAM = 6;
const uint32_t DEFAULT_SAMPLE_RATE = 48000;
const uint32_t DEFAULT_NUM_CHANNEL = STEREO;
const uint64_t DEFAULT_NUM_CHANNELLAYOUT = CH_LAYOUT_STEREO;

template <typename T>
static void Swap(T &a, T &b)
{
    T temp = a;
    a = b;
    b = temp;
}

#ifdef SENSOR_ENABLE
AudioEffectChain::AudioEffectChain(std::string scene, std::shared_ptr<HeadTracker> headTracker)
{
    sceneType_ = scene;
    effectMode_ = AUDIO_SUPPORTED_SCENE_MODES.find(EFFECT_DEFAULT)->second;
    audioBufIn_.frameLength = 0;
    audioBufOut_.frameLength = 0;
    ioBufferConfig_.inputCfg.samplingRate = DEFAULT_SAMPLE_RATE;
    ioBufferConfig_.inputCfg.channels = DEFAULT_NUM_CHANNEL;
    ioBufferConfig_.inputCfg.format = DATA_FORMAT_F32;
    ioBufferConfig_.inputCfg.channelLayout = DEFAULT_NUM_CHANNELLAYOUT;
    ioBufferConfig_.outputCfg.samplingRate = DEFAULT_SAMPLE_RATE;
    ioBufferConfig_.outputCfg.channels = DEFAULT_NUM_CHANNEL;
    ioBufferConfig_.outputCfg.format = DATA_FORMAT_F32;
    ioBufferConfig_.outputCfg.channelLayout = DEFAULT_NUM_CHANNELLAYOUT;
    headTracker_ = headTracker;
    DumpFileUtil::OpenDumpFile(DUMP_SERVER_PARA,
        "dump_effect_in_" + scene + "_" + std::to_string(ioBufferConfig_.inputCfg.samplingRate) +
        "_" + std::to_string(ioBufferConfig_.inputCfg.channels) +
        "_float.pcm",
        &dumpFileInput_);
    DumpFileUtil::OpenDumpFile(DUMP_SERVER_PARA,
        "dump_effect_out_" + scene + "_" + std::to_string(ioBufferConfig_.outputCfg.samplingRate) +
        "_" + std::to_string(ioBufferConfig_.outputCfg.channels) +
        "_float.pcm",
        &dumpFileOutput_);
}
#else
AudioEffectChain::AudioEffectChain(std::string scene)
{
    sceneType_ = scene;
    effectMode_ = AUDIO_SUPPORTED_SCENE_MODES.find(EFFECT_DEFAULT)->second;
    audioBufIn_.frameLength = 0;
    audioBufOut_.frameLength = 0;
    ioBufferConfig_.inputCfg.samplingRate = DEFAULT_SAMPLE_RATE;
    ioBufferConfig_.inputCfg.channels = DEFAULT_NUM_CHANNEL;
    ioBufferConfig_.inputCfg.format = DATA_FORMAT_F32;
    ioBufferConfig_.inputCfg.channelLayout = DEFAULT_NUM_CHANNELLAYOUT;
    ioBufferConfig_.outputCfg.samplingRate = DEFAULT_SAMPLE_RATE;
    ioBufferConfig_.outputCfg.channels = DEFAULT_NUM_CHANNEL;
    ioBufferConfig_.outputCfg.format = DATA_FORMAT_F32;
    ioBufferConfig_.outputCfg.channelLayout = DEFAULT_NUM_CHANNELLAYOUT;
    DumpFileUtil::OpenDumpFile(DUMP_SERVER_PARA,
        "dump_effect_in_" + scene + "_" + std::to_string(ioBufferConfig_.inputCfg.samplingRate) +
        "_" + std::to_string(ioBufferConfig_.inputCfg.channels) +
        "_float.pcm",
        &dumpFileInput_);
    DumpFileUtil::OpenDumpFile(DUMP_SERVER_PARA,
        "dump_effect_out_" + scene + "_" + std::to_string(ioBufferConfig_.outputCfg.samplingRate) +
        "_" + std::to_string(ioBufferConfig_.outputCfg.channels) +
        "_float.pcm",
        &dumpFileOutput_);
}
#endif

AudioEffectChain::~AudioEffectChain()
{
    ReleaseEffectChain();
    DumpFileUtil::CloseDumpFile(&dumpFileInput_);
    DumpFileUtil::CloseDumpFile(&dumpFileOutput_);
}

void AudioEffectChain::Dump()
{
    std::lock_guard<std::mutex> lock(reloadMutex_);
    for (AudioEffectHandle handle : standByEffectHandles_) {
        AUDIO_INFO_LOG("standByEffectHandle for [%{public}s], handle address is %{public}p", sceneType_.c_str(),
            handle);
    }
}

std::string AudioEffectChain::GetEffectMode()
{
    return effectMode_;
}

void AudioEffectChain::SetEffectMode(const std::string &mode)
{
    effectMode_ = mode;
}

void AudioEffectChain::SetExtraSceneType(const std::string &extraSceneType)
{
    extraEffectChainType_ = static_cast<uint32_t>(std::stoi(extraSceneType));
}

void AudioEffectChain::SetEffectCurrSceneType(AudioEffectScene currSceneType)
{
    currSceneType_ = currSceneType;
}

void AudioEffectChain::ReleaseEffectChain()
{
    for (uint32_t i = 0; i < standByEffectHandles_.size() && i < libHandles_.size(); ++i) {
        if (!libHandles_[i]) {
            continue;
        }
        if (!standByEffectHandles_[i]) {
            continue;
        }
        if (!libHandles_[i]->releaseEffect) {
            continue;
        }
        libHandles_[i]->releaseEffect(standByEffectHandles_[i]);
    }
    standByEffectHandles_.clear();
    libHandles_.clear();
}

int32_t AudioEffectChain::SetEffectParamToHandle(AudioEffectHandle handle, int32_t &replyData)
{
    AudioEffectTransInfo cmdInfo = {sizeof(AudioEffectConfig), &ioBufferConfig_};
    AudioEffectTransInfo replyInfo = {sizeof(int32_t), &replyData};
    // Set param
    AudioEffectParam *effectParam =
        new AudioEffectParam[sizeof(AudioEffectParam) + NUM_SET_EFFECT_PARAM * sizeof(int32_t)];
    effectParam->status = 0;
    effectParam->paramSize = sizeof(int32_t);
    effectParam->valueSize = 0;
    int32_t *data = &(effectParam->data[0]);
    *data++ = EFFECT_SET_PARAM;
    *data++ = static_cast<int32_t>(currSceneType_);
    *data++ = GetKeyFromValue(AUDIO_SUPPORTED_SCENE_MODES, effectMode_);
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
        *data++ = audioEffectVolume->GetApVolume(sceneType_);
    }
    AUDIO_DEBUG_LOG("set ap integration volume: %{public}u", *(data - 1));
    *data++ = extraEffectChainType_;
    AUDIO_DEBUG_LOG("set scene type: %{public}d", extraEffectChainType_);
    cmdInfo = {sizeof(AudioEffectParam) + sizeof(int32_t) * NUM_SET_EFFECT_PARAM, effectParam};
    int32_t ret = (*handle)->command(handle, EFFECT_CMD_SET_PARAM, &cmdInfo, &replyInfo);
    delete[] effectParam;
    return ret;
}

void AudioEffectChain::AddEffectHandle(AudioEffectHandle handle, AudioEffectLibrary *libHandle,
    AudioEffectScene currSceneType)
{
    int32_t ret;
    int32_t replyData = 0;
    currSceneType_ = currSceneType;
    AudioEffectTransInfo cmdInfo = {sizeof(AudioEffectConfig), &ioBufferConfig_};
    AudioEffectTransInfo replyInfo = {sizeof(int32_t), &replyData};
    ret = (*handle)->command(handle, EFFECT_CMD_INIT, &cmdInfo, &replyInfo);
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with mode [%{public}s], %{public}s lib EFFECT_CMD_INIT fail",
        sceneType_.c_str(), effectMode_.c_str(), libHandle->name);
    ret = (*handle)->command(handle, EFFECT_CMD_ENABLE, &cmdInfo, &replyInfo);
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with mode [%{public}s], %{public}s lib EFFECT_CMD_ENABLE fail",
        sceneType_.c_str(), effectMode_.c_str(), libHandle->name);

    CHECK_AND_RETURN_LOG(SetEffectParamToHandle(handle, replyData) == 0,
        "[%{public}s] with mode [%{public}s], %{public}s lib EFFECT_CMD_SET_PARAM fail", sceneType_.c_str(),
        effectMode_.c_str(), libHandle->name);

    cmdInfo = {sizeof(AudioEffectConfig), &ioBufferConfig_};
    ret = (*handle)->command(handle, EFFECT_CMD_SET_CONFIG, &cmdInfo, &replyInfo);
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with mode [%{public}s], %{public}s lib EFFECT_CMD_SET_CONFIG fail",
        sceneType_.c_str(), effectMode_.c_str(), libHandle->name);

    ret = (*handle)->command(handle, EFFECT_CMD_GET_CONFIG, &cmdInfo, &cmdInfo);
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with mode [%{public}s], %{public}s lib EFFECT_CMD_GET_CONFIG fail",
        sceneType_.c_str(), effectMode_.c_str(), libHandle->name);
    Swap(ioBufferConfig_.inputCfg, ioBufferConfig_.outputCfg); // pass outputCfg to next algo as inputCfg

    standByEffectHandles_.emplace_back(handle);
    libHandles_.emplace_back(libHandle);
    latency_ += static_cast<uint32_t>(replyData);
}

int32_t AudioEffectChain::UpdateEffectParam()
{
    std::lock_guard<std::mutex> lock(reloadMutex_);
    latency_ = 0;
    for (AudioEffectHandle handle : standByEffectHandles_) {
        int32_t replyData;
        int32_t ret = SetEffectParamToHandle(handle, replyData);
        CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "set EFFECT_CMD_SET_PARAM fail");
        AUDIO_DEBUG_LOG("Set Effect Param Scene Type: %{public}d Success", currSceneType_);
        latency_ += static_cast<uint32_t>(replyData);
    }
    return SUCCESS;
}

void AudioEffectChain::ApplyEffectChain(float *bufIn, float *bufOut, uint32_t frameLen, AudioEffectProcInfo procInfo)
{
    size_t inTotlen = frameLen * ioBufferConfig_.inputCfg.channels * sizeof(float);
    size_t outTotlen = frameLen * ioBufferConfig_.outputCfg.channels * sizeof(float);

    // dump pcm for bufIn
    DumpFileUtil::WriteDumpFile(dumpFileInput_, static_cast<void *>(bufIn), inTotlen);

    if (IsEmptyEffectHandles()) {
        CHECK_AND_RETURN_LOG(memcpy_s(bufOut, outTotlen, bufIn, outTotlen) == 0, "memcpy error in apply effect");
        DumpFileUtil::WriteDumpFile(dumpFileOutput_, static_cast<void *>(bufOut), outTotlen);
        return;
    }

#ifdef SENSOR_ENABLE
    int32_t replyData = 0;
    auto imuData = headTracker_->GetHeadPostureData();
    AudioEffectTransInfo cmdInfo = {sizeof(HeadPostureData), &imuData};
    AudioEffectTransInfo replyInfo = {sizeof(int32_t), &replyData};
#endif

    audioBufIn_.frameLength = frameLen;
    audioBufOut_.frameLength = frameLen;
    uint32_t count = 0;
    std::lock_guard<std::mutex> lock(reloadMutex_);
    for (AudioEffectHandle handle : standByEffectHandles_) {
#ifdef SENSOR_ENABLE
        if ((!procInfo.btOffloadEnabled) && procInfo.headTrackingEnabled) {
            (*handle)->command(handle, EFFECT_CMD_SET_IMU, &cmdInfo, &replyInfo);
        }
#endif
        if ((count & 1) == 0) {
            audioBufIn_.raw = bufIn;
            audioBufOut_.raw = bufOut;
        } else {
            audioBufOut_.raw = bufIn;
            audioBufIn_.raw = bufOut;
        }
        int32_t ret = (*handle)->process(handle, &audioBufIn_, &audioBufOut_);
        CHECK_AND_CONTINUE_LOG(ret == 0, "[%{public}s] with mode [%{public}s], either one of libs process fail",
            sceneType_.c_str(), effectMode_.c_str());
        count++;
    }
    if ((count & 1) == 0) {
        CHECK_AND_RETURN_LOG(memcpy_s(bufOut, outTotlen, bufIn, outTotlen) == 0, "memcpy error when last copy");
    }

    // dump pcm for bufOut
    DumpFileUtil::WriteDumpFile(dumpFileOutput_, static_cast<void *>(bufOut), outTotlen);
}

bool AudioEffectChain::IsEmptyEffectHandles()
{
    std::lock_guard<std::mutex> lock(reloadMutex_);
    return standByEffectHandles_.size() == 0;
}

int32_t AudioEffectChain::UpdateMultichannelIoBufferConfig(const uint32_t &channels, const uint64_t &channelLayout)
{
    if (ioBufferConfig_.inputCfg.channels == channels && ioBufferConfig_.inputCfg.channelLayout == channelLayout) {
        return SUCCESS;
    }
    ioBufferConfig_.inputCfg.channels = channels;
    ioBufferConfig_.inputCfg.channelLayout = channelLayout;
    if (IsEmptyEffectHandles()) {
        return SUCCESS;
    }
    std::lock_guard<std::mutex> lock(reloadMutex_);
    int32_t replyData = 0;
    AudioEffectTransInfo cmdInfo = {sizeof(AudioEffectConfig), &ioBufferConfig_};
    AudioEffectTransInfo replyInfo = {sizeof(int32_t), &replyData};
    AudioEffectHandle preHandle = nullptr;
    ioBufferConfig_.outputCfg.channels = 0;
    ioBufferConfig_.outputCfg.channelLayout = 0;
    for (AudioEffectHandle handle : standByEffectHandles_) {
        if (preHandle != nullptr) {
            int32_t ret = (*preHandle)->command(preHandle, EFFECT_CMD_SET_CONFIG, &cmdInfo, &replyInfo);
            CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "Multichannel effect chain update EFFECT_CMD_SET_CONFIG fail");

            ret = (*preHandle)->command(preHandle, EFFECT_CMD_GET_CONFIG, &cmdInfo, &cmdInfo);
            CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "Multichannel effect chain update EFFECT_CMD_GET_CONFIG fail");
            Swap(ioBufferConfig_.inputCfg, ioBufferConfig_.outputCfg); // pass outputCfg to next algo as inputCfg
        }
        preHandle = handle;
    }
    ioBufferConfig_.outputCfg.channels = DEFAULT_NUM_CHANNEL;
    ioBufferConfig_.outputCfg.channelLayout = DEFAULT_NUM_CHANNELLAYOUT;
    int32_t ret = (*preHandle)->command(preHandle, EFFECT_CMD_SET_CONFIG, &cmdInfo, &replyInfo);
    CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "last effect update EFFECT_CMD_SET_CONFIG fail");
    // recover bufferconfig
    ioBufferConfig_.inputCfg.channels = channels;
    ioBufferConfig_.inputCfg.channelLayout = channelLayout;
    return SUCCESS;
}

void AudioEffectChain::ResetIoBufferConfig()
{
    ioBufferConfig_.inputCfg.channels = DEFAULT_NUM_CHANNEL;
    ioBufferConfig_.inputCfg.channelLayout = DEFAULT_NUM_CHANNELLAYOUT;
    ioBufferConfig_.outputCfg.channels = DEFAULT_NUM_CHANNEL;
    ioBufferConfig_.outputCfg.channelLayout = DEFAULT_NUM_CHANNELLAYOUT;
}

AudioEffectConfig AudioEffectChain::GetIoBufferConfig()
{
    return ioBufferConfig_;
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

    std::lock_guard<std::mutex> lock(reloadMutex_);
    for (AudioEffectHandle handle : standByEffectHandles_) {
        int32_t replyData = 0;
        HeadPostureData imuDataDisabled = {1, 1.0, 0.0, 0.0, 0.0};
        AudioEffectTransInfo cmdInfo = {sizeof(HeadPostureData), &imuDataDisabled};
        AudioEffectTransInfo replyInfo = {sizeof(int32_t), &replyData};
        int32_t ret = (*handle)->command(handle, EFFECT_CMD_SET_IMU, &cmdInfo, &replyInfo);
        if (ret != SUCCESS) {
            AUDIO_WARNING_LOG("SetHeadTrackingDisabled failed");
        }
    }
}
#endif

void AudioEffectChain::InitEffectChain()
{
    if (IsEmptyEffectHandles()) {
        return;
    }
    std::lock_guard<std::mutex> lock(reloadMutex_);
    for (AudioEffectHandle handle : standByEffectHandles_) {
        int32_t replyData = 0;
        AudioEffectTransInfo cmdInfo = {sizeof(int32_t), &replyData};
        AudioEffectTransInfo replyInfo = {sizeof(int32_t), &replyData};
        int32_t ret = (*handle)->command(handle, EFFECT_CMD_ENABLE, &cmdInfo, &replyInfo);
        CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with mode [%{public}s], either one of libs EFFECT_CMD_ENABLE fail",
            sceneType_.c_str(), effectMode_.c_str());
    }
}
} // namespace AudioStandard
} // namespace OHOS
