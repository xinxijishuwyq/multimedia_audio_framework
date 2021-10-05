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

#include "audio_errors.h"
#include "audio_policy_manager.h"
#include "audio_policy_proxy.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace AudioStandard {
static sptr<IAudioPolicy> g_sProxy = nullptr;

void AudioPolicyManager::Init()
{
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        MEDIA_ERR_LOG("AudioPolicyManager::init failed");
        return;
    }

    sptr<IRemoteObject> object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);
    if (object == nullptr) {
        MEDIA_DEBUG_LOG("AudioPolicyManager::object is NULL.");
    }

    g_sProxy = iface_cast<IAudioPolicy>(object);
    if (g_sProxy == nullptr) {
        MEDIA_DEBUG_LOG("AudioPolicyManager::init g_sProxy is NULL.");
    } else {
        MEDIA_DEBUG_LOG("AudioPolicyManager::init g_sProxy is assigned.");
    }
}

int32_t AudioPolicyManager::SetStreamVolume(AudioStreamType streamType, float volume)
{
    return g_sProxy->SetStreamVolume(streamType, volume);
}

int32_t AudioPolicyManager::SetRingerMode(AudioRingerMode ringMode)
{
    return g_sProxy->SetRingerMode(ringMode);
}

AudioRingerMode AudioPolicyManager::GetRingerMode()
{
    return g_sProxy->GetRingerMode();
}

float AudioPolicyManager::GetStreamVolume(AudioStreamType streamType)
{
    return g_sProxy->GetStreamVolume(streamType);
}

int32_t AudioPolicyManager::SetStreamMute(AudioStreamType streamType, bool mute)
{
    return g_sProxy->SetStreamMute(streamType, mute);
}

bool AudioPolicyManager::GetStreamMute(AudioStreamType streamType)
{
    return g_sProxy->GetStreamMute(streamType);
}

bool AudioPolicyManager::IsStreamActive(AudioStreamType streamType)
{
    return g_sProxy->IsStreamActive(streamType);
}

int32_t AudioPolicyManager::SetDeviceActive(InternalDeviceType deviceType, bool active)
{
    return g_sProxy->SetDeviceActive(deviceType, active);
}

bool AudioPolicyManager::IsDeviceActive(InternalDeviceType deviceType)
{
    return g_sProxy->IsDeviceActive(deviceType);
}

int32_t AudioPolicyManager::SetAudioManagerCallback(const AudioStreamType streamType,
                                                    const std::shared_ptr<AudioManagerCallback> &callback)
{
    callback_ = callback; // used by client to trigger callback like onerror

    listenerStub_ = new(std::nothrow) AudioPolicyManagerListenerStub();
    if (listenerStub_ == nullptr || g_sProxy == nullptr) {
        MEDIA_ERR_LOG("AudioPolicyManager: object null");
        return ERROR;
    }
    listenerStub_->SetCallback(callback);

    sptr<IRemoteObject> object = listenerStub_->AsObject();
    if (object == nullptr) {
        MEDIA_ERR_LOG("AudioPolicyManager: listenerStub->AsObject is nullptr..");
        return ERROR;
    }

    return g_sProxy->SetAudioManagerCallback(streamType, object);
}

int32_t AudioPolicyManager::UnsetAudioManagerCallback(const AudioStreamType streamType)
{
    return g_sProxy->UnsetAudioManagerCallback(streamType);
}

int32_t AudioPolicyManager::ActivateAudioInterrupt(const AudioInterrupt &audioInterrupt)
{
    return g_sProxy->ActivateAudioInterrupt(audioInterrupt);
}

int32_t AudioPolicyManager::DeactivateAudioInterrupt(const AudioInterrupt &audioInterrupt)
{
    return g_sProxy->DeactivateAudioInterrupt(audioInterrupt);
}
} // namespace AudioStandard
} // namespace OHOS
