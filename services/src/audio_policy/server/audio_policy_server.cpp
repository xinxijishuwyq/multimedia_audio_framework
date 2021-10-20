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

#include <memory>

#include "audio_errors.h"
#include "audio_policy_manager_listener_proxy.h"
#include "i_standard_audio_policy_manager_listener.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "system_ability_definition.h"
#include "audio_policy_server.h"

namespace OHOS {
namespace AudioStandard {
REGISTER_SYSTEM_ABILITY_BY_ID(AudioPolicyServer, AUDIO_POLICY_SERVICE_ID, true)

AudioPolicyServer::AudioPolicyServer(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate),
      mPolicyService(AudioPolicyService::GetAudioPolicyService())
{
}

void AudioPolicyServer::OnDump()
{
    return;
}

void AudioPolicyServer::OnStart()
{
    bool res = Publish(this);
    if (res) {
        MEDIA_DEBUG_LOG("AudioPolicyService OnStart res=%d", res);
    }

    mPolicyService.Init();
    return;
}

void AudioPolicyServer::OnStop()
{
    mPolicyService.Deinit();
    return;
}

int32_t AudioPolicyServer::SetStreamVolume(AudioStreamType streamType, float volume)
{
    return mPolicyService.SetStreamVolume(streamType, volume);
}

float AudioPolicyServer::GetStreamVolume(AudioStreamType streamType)
{
    return mPolicyService.GetStreamVolume(streamType);
}

int32_t AudioPolicyServer::SetStreamMute(AudioStreamType streamType, bool mute)
{
    return mPolicyService.SetStreamMute(streamType, mute);
}

bool AudioPolicyServer::GetStreamMute(AudioStreamType streamType)
{
    return mPolicyService.GetStreamMute(streamType);
}

bool AudioPolicyServer::IsStreamActive(AudioStreamType streamType)
{
    return mPolicyService.IsStreamActive(streamType);
}

int32_t AudioPolicyServer::SetDeviceActive(InternalDeviceType deviceType, bool active)
{
    return mPolicyService.SetDeviceActive(deviceType, active);
}

bool AudioPolicyServer::IsDeviceActive(InternalDeviceType deviceType)
{
    return mPolicyService.IsDeviceActive(deviceType);
}

int32_t AudioPolicyServer::SetRingerMode(AudioRingerMode ringMode)
{
    return mPolicyService.SetRingerMode(ringMode);
}

AudioRingerMode AudioPolicyServer::GetRingerMode()
{
    return mPolicyService.GetRingerMode();
}

int32_t AudioPolicyServer::SetAudioManagerCallback(const AudioStreamType streamType, const sptr<IRemoteObject> &object)
{
    std::lock_guard<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_PARAM, "AudioPolicyServer:set listener object is nullptr");

    sptr<IStandardAudioPolicyManagerListener> listener = iface_cast<IStandardAudioPolicyManagerListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, ERR_INVALID_PARAM, "AudioPolicyServer: listener obj cast failed");

    std::shared_ptr<AudioManagerCallback> callback = std::make_shared<AudioPolicyManagerListenerCallback>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_PARAM, "AudioPolicyServer: failed to  create cb obj");

    policyListenerCbsMap_[streamType] = callback;

    return SUCCESS;
}

int32_t AudioPolicyServer::UnsetAudioManagerCallback(const AudioStreamType streamType)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (policyListenerCbsMap_.find(streamType) != policyListenerCbsMap_.end()) {
        policyListenerCbsMap_.erase(streamType);
        MEDIA_ERR_LOG("AudioPolicyServer: UnsetAudioManagerCallback for streamType %{public}d done", streamType);
        return SUCCESS;
    } else {
        MEDIA_ERR_LOG("AudioPolicyServer: Cb does not exit for streamType %{public}d cannot unregister", streamType);
        return ERR_INVALID_OPERATION;
    }
}

int32_t AudioPolicyServer::ActivateAudioInterrupt(const AudioInterrupt &audioInterrupt)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_DEBUG_LOG("AudioPolicyServer: ActivateAudioInterrupt");
    MEDIA_DEBUG_LOG("AudioPolicyServer: audioInterrupt.streamUsage: %{public}d", audioInterrupt.streamUsage);
    MEDIA_DEBUG_LOG("AudioPolicyServer: audioInterrupt.contentType: %{public}d", audioInterrupt.contentType);
    MEDIA_DEBUG_LOG("AudioPolicyServer: audioInterrupt.streamType: %{public}d", audioInterrupt.streamType);
    MEDIA_DEBUG_LOG("AudioPolicyServer: audioInterrupt.sessionID: %{public}d", audioInterrupt.sessionID);

    if (policyListenerCbsMap_.find(audioInterrupt.streamType) == policyListenerCbsMap_.end()) {
        MEDIA_ERR_LOG("AudioPolicyServer: callback not registered for stream: %{public}d", audioInterrupt.streamType);
        return ERR_INVALID_OPERATION;
    }

    bool isPriorityStreamActive(false);
    if (curActiveInterruptsMap_.find(STREAM_VOICE_ASSISTANT) != curActiveInterruptsMap_.end()) {
        isPriorityStreamActive = true;
    }

    if (isPriorityStreamActive) {
        MEDIA_DEBUG_LOG("Priority Stream: %{public}d is active, Cannot activate stream %{public}d",
                        STREAM_VOICE_ASSISTANT, audioInterrupt.streamType);
        return ERR_INVALID_OPERATION;
    }
    curActiveInterruptsMap_[audioInterrupt.streamType] = audioInterrupt;

    InterruptAction activated {TYPE_ACTIVATED, INTERRUPT_TYPE_BEGIN, INTERRUPT_HINT_NONE};
    InterruptAction interrupted {TYPE_INTERRUPTED, INTERRUPT_TYPE_BEGIN, INTERRUPT_HINT_PAUSE};
    for (auto it = policyListenerCbsMap_.begin(); it != policyListenerCbsMap_.end(); ++it) {
        std::shared_ptr<AudioManagerCallback> policyListenerCb = it->second;
        if (policyListenerCb == nullptr) {
            MEDIA_ERR_LOG("policyListenerCbsMap_: nullptr for streamType : %{public}d", it->first);
            continue;
        }

        MEDIA_DEBUG_LOG("policyListenerCbsMap_ :streamType =  %{public}d", it->first);
        MEDIA_DEBUG_LOG("audioInterrupt.streamType = %{public}d", audioInterrupt.streamType);
        if (it->first == audioInterrupt.streamType) {
            policyListenerCb->OnInterrupt(activated);
        } else {
            policyListenerCb->OnInterrupt(interrupted);
        }
    }

    return SUCCESS;
}

int32_t AudioPolicyServer::DeactivateAudioInterrupt(const AudioInterrupt &audioInterrupt)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_DEBUG_LOG("AudioPolicyServer: DeactivateAudioInterrupt");
    MEDIA_DEBUG_LOG("AudioPolicyServer: audioInterrupt.streamUsage: %{public}d", audioInterrupt.streamUsage);
    MEDIA_DEBUG_LOG("AudioPolicyServer: audioInterrupt.contentType: %{public}d", audioInterrupt.contentType);
    MEDIA_DEBUG_LOG("AudioPolicyServer: audioInterrupt.streamType: %{public}d", audioInterrupt.streamType);
    MEDIA_DEBUG_LOG("AudioPolicyServer: audioInterrupt.sessionID: %{public}d", audioInterrupt.sessionID);

    if (curActiveInterruptsMap_.find(audioInterrupt.streamType) == curActiveInterruptsMap_.end()) {
        MEDIA_DEBUG_LOG("AudioPolicyServer: Stream : %{public}d is not active. Cannot deactivate",
                        audioInterrupt.streamType);
        return ERR_INVALID_OPERATION;
    }

    if (auto count = curActiveInterruptsMap_.erase(audioInterrupt.streamType)) {
        MEDIA_DEBUG_LOG("AudioPolicyServer: erase curActiveInterruptsMap_:streamType =  %{public}d success",
                        audioInterrupt.streamType);
    }

    InterruptAction deActivated {TYPE_DEACTIVATED, INTERRUPT_TYPE_END, INTERRUPT_HINT_NONE};
    InterruptAction interrupted {TYPE_INTERRUPTED, INTERRUPT_TYPE_END, INTERRUPT_HINT_RESUME};
    for (auto it = policyListenerCbsMap_.begin(); it != policyListenerCbsMap_.end(); ++it) {
        std::shared_ptr<AudioManagerCallback> policyListenerCb = it->second;
        if (policyListenerCb == nullptr) {
            MEDIA_ERR_LOG("AudioPolicyServer: policyListenerCbsMap_: nullptr for streamType : %{public}d", it->first);
            continue;
        }

        MEDIA_DEBUG_LOG("policyListenerCbsMap_ :streamType =  %{public}d", it->first);
        MEDIA_DEBUG_LOG("audioInterrupt.streamType = %{public}d", audioInterrupt.streamType);
        if (it->first == audioInterrupt.streamType) {
            policyListenerCb->OnInterrupt(deActivated);
        } else {
            policyListenerCb->OnInterrupt(interrupted);
        }
    }

    return SUCCESS;
}
} // namespace AudioStandard
} // namespace OHOS
