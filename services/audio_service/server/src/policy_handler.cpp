/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#define LOG_TAG "PolicyHandler"

#include "policy_handler.h"

#include <iomanip>
#include <thread>

#include "audio_errors.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
namespace {
const uint32_t FIRST_SESSIONID = 100000;
constexpr uint32_t MAX_VALID_SESSIONID = UINT32_MAX - FIRST_SESSIONID;
}

PolicyHandler& PolicyHandler::GetInstance()
{
    static PolicyHandler PolicyHandler;

    return PolicyHandler;
}

PolicyHandler::PolicyHandler()
{
    AUDIO_INFO_LOG("PolicyHandler()");
}

PolicyHandler::~PolicyHandler()
{
    volumeVector_ = nullptr;
    policyVolumeMap_ = nullptr;
    iPolicyProvider_ = nullptr;
    AUDIO_INFO_LOG("~PolicyHandler()");
}

void PolicyHandler::Dump(std::stringstream &dumpStringStream)
{
    AUDIO_INFO_LOG("PolicyHandler dump begin");
    if (iPolicyProvider_ == nullptr || policyVolumeMap_ == nullptr || volumeVector_ == nullptr) {
        dumpStringStream << "PolicyHandler is null..." << std::endl;
        AUDIO_INFO_LOG("nothing to dump");
        return;
    }
    dumpStringStream << std::endl;
    // dump active output device
    dumpStringStream << "active output device:[" << deviceType_ << "]" << std::endl;
    // dump volume
    int formatSize = 2;
    for (size_t i = 0; i < IPolicyProvider::GetVolumeVectorSize(); i++) {
        dumpStringStream << "streamtype[" << g_volumeIndexVector[i].first << "] ";
        dumpStringStream << "device[" << std::setw(formatSize) << g_volumeIndexVector[i].second << "]: ";
        dumpStringStream << "isMute[" << (volumeVector_[i].isMute ? "true" : "false") << "] ";
        dumpStringStream << "volFloat[" << volumeVector_[i].volumeFloat << "] ";
        dumpStringStream << "volint[" << volumeVector_[i].volumeInt << "] ";
        dumpStringStream << std::endl;
    }
}

bool PolicyHandler::ConfigPolicyProvider(const sptr<IPolicyProviderIpc> policyProvider)
{
    CHECK_AND_RETURN_RET_LOG(policyProvider != nullptr, false, "ConfigPolicyProvider failed with null provider.");
    if (iPolicyProvider_ == nullptr) {
        iPolicyProvider_ = policyProvider;
    } else {
        AUDIO_ERR_LOG("Provider is already configed!");
        return false;
    }
    bool ret = InitVolumeMap();
    AUDIO_INFO_LOG("ConfigPolicyProvider end and InitVolumeMap %{public}s", (ret ? "SUCCESS" : "FAILED"));
    return ret;
}

bool PolicyHandler::GetProcessDeviceInfo(const AudioProcessConfig &config, DeviceInfo &deviceInfo)
{
    // send the config to AudioPolicyServer and get the device info.
    CHECK_AND_RETURN_RET_LOG(iPolicyProvider_ != nullptr, false, "GetProcessDeviceInfo failed with null provider.");
    int32_t ret = iPolicyProvider_->GetProcessDeviceInfo(config, deviceInfo);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "GetProcessDeviceInfo failed:%{public}d", ret);
    return true;
}

bool PolicyHandler::InitVolumeMap()
{
    CHECK_AND_RETURN_RET_LOG(iPolicyProvider_ != nullptr, false, "InitVolumeMap failed with null provider.");
    iPolicyProvider_->InitSharedVolume(policyVolumeMap_);
    CHECK_AND_RETURN_RET_LOG((policyVolumeMap_ != nullptr && policyVolumeMap_->GetBase() != nullptr), false,
        "InitSharedVolume failed.");
    size_t mapSize = IPolicyProvider::GetVolumeVectorSize() * sizeof(Volume);
    CHECK_AND_RETURN_RET_LOG(policyVolumeMap_->GetSize() == mapSize, false,
        "InitSharedVolume get error size:%{public}zu, target:%{public}zu", policyVolumeMap_->GetSize(), mapSize);
    volumeVector_ = reinterpret_cast<Volume *>(policyVolumeMap_->GetBase());
    AUDIO_INFO_LOG("InitSharedVolume success.");
    return true;
}

AudioVolumeType PolicyHandler::GetVolumeTypeFromStreamType(AudioStreamType streamType)
{
    switch (streamType) {
        case STREAM_VOICE_CALL:
        case STREAM_VOICE_MESSAGE:
        case STREAM_VOICE_COMMUNICATION:
            return STREAM_VOICE_CALL;
        case STREAM_RING:
        case STREAM_SYSTEM:
        case STREAM_NOTIFICATION:
        case STREAM_SYSTEM_ENFORCED:
        case STREAM_DTMF:
        case STREAM_VOICE_RING:
            return STREAM_RING;
        case STREAM_MUSIC:
        case STREAM_MEDIA:
        case STREAM_MOVIE:
        case STREAM_GAME:
        case STREAM_SPEECH:
        case STREAM_NAVIGATION:
            return STREAM_MUSIC;
        case STREAM_VOICE_ASSISTANT:
            return STREAM_VOICE_ASSISTANT;
        case STREAM_ALARM:
            return STREAM_ALARM;
        case STREAM_ACCESSIBILITY:
            return STREAM_ACCESSIBILITY;
        case STREAM_ULTRASONIC:
            return STREAM_ULTRASONIC;
        case STREAM_ALL:
            return STREAM_ALL;
        default:
            return STREAM_MUSIC;
    }
}

bool PolicyHandler::GetSharedVolume(AudioVolumeType streamType, DeviceType deviceType, Volume &vol)
{
    CHECK_AND_RETURN_RET_LOG((iPolicyProvider_ != nullptr && volumeVector_ != nullptr), false,
        "GetSharedVolume failed not configed");
    size_t index = 0;
    if (!IPolicyProvider::GetVolumeIndex(streamType, GetVolumeGroupForDevice(deviceType), index) ||
        index >= IPolicyProvider::GetVolumeVectorSize()) {
        return false;
    }
    vol.isMute = volumeVector_[index].isMute;
    vol.volumeFloat = volumeVector_[index].volumeFloat;
    vol.volumeInt = volumeVector_[index].volumeInt;
    return true;
}

void PolicyHandler::SetActiveOutputDevice(DeviceType deviceType)
{
    AUDIO_INFO_LOG("SetActiveOutputDevice to device[%{public}d].", deviceType);
    deviceType_ = deviceType;
}

std::atomic<uint32_t> g_sessionId = {FIRST_SESSIONID}; // begin at 100000

uint32_t PolicyHandler::GenerateSessionId(int32_t uid)
{
    uint32_t sessionId = g_sessionId++;
    AUDIO_INFO_LOG("uid:%{public}d sessionId:%{public}d", uid, sessionId);
    if (g_sessionId > MAX_VALID_SESSIONID) {
        AUDIO_WARNING_LOG("sessionId is too large, reset it!");
        g_sessionId = FIRST_SESSIONID;
    }
    return sessionId;
}

DeviceType PolicyHandler::GetActiveOutPutDevice()
{
    return deviceType_;
}

int32_t PolicyHandler::SetWakeUpAudioCapturerFromAudioServer(const AudioProcessConfig &config)
{
    CHECK_AND_RETURN_RET_LOG(iPolicyProvider_ != nullptr, ERROR, "iPolicyProvider_ is nullptr");
    int32_t ret = iPolicyProvider_->SetWakeUpAudioCapturerFromAudioServer(config);
    return ret;
}

int32_t PolicyHandler::NotifyCapturerAdded(AudioCapturerInfo capturerInfo, AudioStreamInfo streamInfo,
    uint32_t sessionId)
{
    CHECK_AND_RETURN_RET_LOG(iPolicyProvider_ != nullptr, ERROR, "iPolicyProvider_ is nullptr");
    return iPolicyProvider_->NotifyCapturerAdded(capturerInfo, streamInfo, sessionId);
}

int32_t PolicyHandler::NotifyWakeUpCapturerRemoved()
{
    CHECK_AND_RETURN_RET_LOG(iPolicyProvider_ != nullptr, ERROR, "iPolicyProvider_ is nullptr");
    return iPolicyProvider_->NotifyWakeUpCapturerRemoved();
}

bool PolicyHandler::IsAbsVolumeSupported()
{
    CHECK_AND_RETURN_RET_LOG(iPolicyProvider_ != nullptr, ERROR, "iPolicyProvider_ is nullptr");
    return iPolicyProvider_->IsAbsVolumeSupported();
}

bool PolicyHandler::GetHighResolutionExist()
{
    return isHighResolutionExist_;
}

void PolicyHandler::SetHighResolutionExist(bool isHighResExist)
{
    isHighResolutionExist_ = isHighResExist;
}
} // namespace AudioStandard
} // namespace OHOS
