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

#include "iservice_registry.h"
#include "audio_manager_proxy.h"
#include "audio_svc_manager.h"
#include "media_log.h"
#include "system_ability_definition.h"

namespace OHOS {
static sptr<IStandardAudioService> g_sProxy = nullptr;

AudioSvcManager::AudioSvcManager()
{
    MEDIA_DEBUG_LOG("AudioSvcManager start");
    init();
}

AudioSvcManager::~AudioSvcManager()
{
    MEDIA_DEBUG_LOG("AudioSvcManager::~AudioSvcManager");
}

AudioSvcManager* AudioSvcManager::GetInstance()
{
    static AudioSvcManager audioManager;
    return &audioManager;
}

void AudioSvcManager::init()
{
    MEDIA_DEBUG_LOG("AudioSvcManager::init start");
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        MEDIA_ERR_LOG("AudioSvcManager::init failed");
        return;
    }
    sptr<IRemoteObject> object = samgr->GetSystemAbility(AUDIO_DISTRIBUTED_SERVICE_ID);
    if (object == nullptr) {
        MEDIA_DEBUG_LOG("AudioSvcManager::object is NULL.");
    }
    g_sProxy = iface_cast<IStandardAudioService>(object);
    if (g_sProxy == nullptr) {
        MEDIA_DEBUG_LOG("AudioSvcManager::init g_sProxy is NULL.");
    } else {
        MEDIA_DEBUG_LOG("AudioSvcManager::init g_sProxy is assigned.");
    }
    MEDIA_ERR_LOG("AudioSvcManager::init end");
}

void AudioSvcManager::SetVolume(AudioSvcManager::AudioVolumeType volumeType, int32_t volume)
{
    MEDIA_DEBUG_LOG("AudioSvcManager::SetVolume Client");
    g_sProxy->SetVolume(volumeType, volume);
}

int AudioSvcManager::GetVolume(AudioSvcManager::AudioVolumeType volumeType)
{
    MEDIA_DEBUG_LOG("AudioSvcManager::GetVolume Client");
    return g_sProxy->GetVolume(volumeType);
}

int AudioSvcManager::GetMaxVolume(AudioSvcManager::AudioVolumeType volumeType)
{
    MEDIA_DEBUG_LOG("AudioSvcManager::GetMaxVolume Client");
    return g_sProxy->GetMaxVolume(volumeType);
}

int AudioSvcManager::GetMinVolume(AudioSvcManager::AudioVolumeType volumeType)
{
    MEDIA_DEBUG_LOG("AudioSvcManager::GetMinVolume Client");
    return g_sProxy->GetMinVolume(volumeType);
}

std::vector<sptr<AudioDeviceDescriptor>> AudioSvcManager::GetDevices(AudioDeviceDescriptor::DeviceFlag deviceFlag)
{
    MEDIA_DEBUG_LOG("AudioSvcManager::GetDevices Client");
    return g_sProxy->GetDevices(deviceFlag);
}
} // namespace OHOS
