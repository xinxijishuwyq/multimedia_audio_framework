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

#include "audio_service.h"

#include "audio_errors.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
AudioService *AudioService::GetInstance()
{
    static AudioService AudioService;

    return &AudioService;
}

AudioService::AudioService()
{
    AUDIO_INFO_LOG("AudioService()");
}

AudioService::~AudioService()
{
    AUDIO_INFO_LOG("~AudioService()");
}

inline void DumpProcessConfig(const AudioProcessConfig &config)
{
    AUDIO_INFO_LOG("Dump AudioProcessConfig: sample rate:%{public}d", config.streamInfo.samplingRate);
}

sptr<AudioProcessInServer> AudioService::GetAudioProcess(const AudioProcessConfig &config)
{
    DumpProcessConfig(config);

    DeviceInfo deviceInfo = GetDeviceInfoForProcess(config);
    std::shared_ptr<AudioEndpoint> audioEndpoint = GetAudioEndpointForDevice(deviceInfo);
    CHECK_AND_RETURN_RET_LOG(audioEndpoint != nullptr, nullptr, "no endpoint found for the process");

    uint32_t totalSizeInframe = 0;
    uint32_t spanSizeInframe = 0;
    audioEndpoint->GetPreferBufferInfo(totalSizeInframe, spanSizeInframe);

    sptr<AudioProcessInServer> process = AudioProcessInServer::Create(config);
    CHECK_AND_RETURN_RET_LOG(process != nullptr, nullptr, "AudioProcessInServer create failed.");

    int32_t ret = process->ConfigProcessBuffer(totalSizeInframe, spanSizeInframe);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, nullptr, "ConfigProcessBuffer failed");

    ret = LinkProcessToEndpoint(process, audioEndpoint);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, nullptr, "LinkProcessToEndpoint failed");

    return process;
}

int32_t AudioService::LinkProcessToEndpoint(sptr<AudioProcessInServer> process,
    std::shared_ptr<AudioEndpoint> endpoint)
{
    int32_t ret = endpoint->LinkProcessStream(process);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "LinkProcessStream failed");

    ret = process->AddProcessStatusListener(endpoint);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "AddProcessStatusListener failed");

    return SUCCESS;
}

DeviceInfo AudioService::GetDeviceInfoForProcess(const AudioProcessConfig &config)
{
    // send the config to AudioPolicyServera and get the device info.
    DeviceInfo deviceInfo;
    deviceInfo.deviceId = 6; // 6 for test
    deviceInfo.networkId = LOCAL_NETWORK_ID;
    deviceInfo.deviceRole = OUTPUT_DEVICE;
    deviceInfo.deviceType = DEVICE_TYPE_SPEAKER;
    deviceInfo.audioStreamInfo = config.streamInfo;
    deviceInfo.deviceName = "mmap_device";
    return deviceInfo;
}

std::shared_ptr<AudioEndpoint> AudioService::GetAudioEndpointForDevice(DeviceInfo deviceInfo)
{
    if (endpointList_.find(deviceInfo.deviceId) != endpointList_.end()) {
        AUDIO_INFO_LOG("AudioService find endpoint already exist for deviceId:%{public}d", deviceInfo.deviceId);
        return endpointList_[deviceInfo.deviceId];
    }
    std::shared_ptr<AudioEndpoint> endpoint = AudioEndpoint::GetInstance(AudioEndpoint::EndpointType::TYPE_MMAP,
        deviceInfo.audioStreamInfo);
    if (endpoint == nullptr) {
        AUDIO_ERR_LOG("Find no endpoint for the process");
        return nullptr;
    }
    endpointList_[deviceInfo.deviceId] = endpoint;
    return endpoint;
}

void AudioService::Dump()
{
    AUDIO_INFO_LOG("AudioService dump begin");
}
} // namespace AudioStandard
} // namespace OHOS
