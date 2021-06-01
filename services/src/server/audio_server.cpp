/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "audio_server.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "system_ability_definition.h"

using namespace std;

namespace OHOS {
    std::unordered_map<int, int> AudioServer::AudioStreamVolumeMap = {
        {AudioSvcManager::AudioVolumeType::STREAM_VOICE_CALL, MAX_VOLUME},
        {AudioSvcManager::AudioVolumeType::STREAM_SYSTEM, MAX_VOLUME},
        {AudioSvcManager::AudioVolumeType::STREAM_RING, MAX_VOLUME},
        {AudioSvcManager::AudioVolumeType::STREAM_MUSIC, MAX_VOLUME},
        {AudioSvcManager::AudioVolumeType::STREAM_ALARM, MAX_VOLUME},
        {AudioSvcManager::AudioVolumeType::STREAM_NOTIFICATION, MAX_VOLUME},
        {AudioSvcManager::AudioVolumeType::STREAM_BLUETOOTH_SCO, MAX_VOLUME},
        {AudioSvcManager::AudioVolumeType::STREAM_DTMF, MAX_VOLUME},
        {AudioSvcManager::AudioVolumeType::STREAM_TTS, MAX_VOLUME},
        {AudioSvcManager::AudioVolumeType::STREAM_ACCESSIBILITY, MAX_VOLUME}
        };

REGISTER_SYSTEM_ABILITY_BY_ID(AudioServer, AUDIO_DISTRIBUTED_SERVICE_ID, true)

AudioServer::AudioServer(int32_t systemAbilityId, bool runOnCreate)
        : SystemAbility(systemAbilityId, runOnCreate)
{}

void AudioServer::OnDump()
{}

void AudioServer::OnStart()
{
    MEDIA_DEBUG_LOG("AudioService OnStart");
    bool res = Publish(this);
    if (res) {
        MEDIA_DEBUG_LOG("AudioService OnStart res=%{public}d", res);
    }
}

void AudioServer::OnStop()
{
    MEDIA_DEBUG_LOG("AudioService OnStop");
}


void AudioServer::SetVolume(AudioSvcManager::AudioVolumeType volumeType, int32_t volume)
{
    MEDIA_DEBUG_LOG("set volume server");
    AudioServer::AudioStreamVolumeMap[volumeType] = volume;
}

int32_t AudioServer::GetVolume(AudioSvcManager::AudioVolumeType volumeType)
{
    MEDIA_DEBUG_LOG("GetVolume server volumeType=%{public}d", volumeType);
    int volume = AudioServer::AudioStreamVolumeMap[volumeType];
    MEDIA_DEBUG_LOG("GetVolume server volume=%{public}d", volume);
    return volume;
}

int32_t AudioServer::GetMaxVolume(AudioSvcManager::AudioVolumeType volumeType)
{
    MEDIA_DEBUG_LOG("GetMaxVolume server");
    return MAX_VOLUME;
}

int32_t AudioServer::GetMinVolume(AudioSvcManager::AudioVolumeType volumeType)
{
    MEDIA_DEBUG_LOG("GetMinVolume server");
    return MIN_VOLUME;
}

std::vector<sptr<AudioDeviceDescriptor>> AudioServer::GetDevices(AudioDeviceDescriptor::DeviceFlag deviceFlag)
{
    MEDIA_DEBUG_LOG("GetDevices server");
    audioDeviceDescriptor_.clear();
    AudioDeviceDescriptor *audioDescriptor = new(std::nothrow) AudioDeviceDescriptor();
    if (audioDescriptor == nullptr) {
        MEDIA_ERR_LOG("new AudioDeviceDescriptor fail");
        return audioDeviceDescriptor_;
    }
    audioDescriptor->deviceType_ = AudioDeviceDescriptor::DeviceType::MIC;
    audioDescriptor->deviceRole_ = AudioDeviceDescriptor::DeviceRole::INPUT_DEVICE;
    audioDeviceDescriptor_.push_back(audioDescriptor);
    return audioDeviceDescriptor_;
}
} // namespace OHOS
