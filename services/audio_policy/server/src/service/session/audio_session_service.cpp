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
#define LOG_TAG "AudioSessionService"

#include "audio_session_service.h"

#include "audio_log.h"
#include "audio_errors.h"

namespace OHOS {
namespace AudioStandard {

AudioSessionService::AudioSessionService()
{
}

AudioSessionService::~AudioSessionService()
{
    AUDIO_ERR_LOG("should not happen");
}

void AudioSessionService::Init(sptr<AudioPolicyServer> server)
{
    AUDIO_INFO_LOG("AudioSessionService::Init");

    policyServer_ = server;

    sessionTimer_ = std::make_shared<AudioSessionTimer>();
    sessionTimer_->SetAudioSessionTimerCallback(shared_from_this());
}

int32_t AudioSessionService::ActivateAudioSession(const int32_t &callerPid, const AudioSessionStrategy &strategy)
{
    AUDIO_INFO_LOG("ActivateAudioSession: callerPid %{public}d, concurrencyMode %{public}d",
        callerPid, static_cast<int32_t>(strategy.concurrencyMode));
    std::lock_guard<std::mutex> lock(sessionServiceMutex_);
    if (sessionMap_.count(callerPid) != 0) {
        // The audio session of the callerPid is already created. The strategy can not be modified.
        AUDIO_ERR_LOG("The audio seesion of pid %{public}d has already been created!", callerPid);
        return ERR_ILLEGAL_STATE;
    }

    sessionMap_[callerPid] = std::make_shared<AudioSession>(callerPid, strategy);
    sessionMap_[callerPid]->Activate();

    return SUCCESS;
}

int32_t AudioSessionService::DeactivateAudioSession(const int32_t &callerPid)
{
    AUDIO_INFO_LOG("DeactivateAudioSession: callerPid %{public}d", callerPid);
    std::lock_guard<std::mutex> lock(sessionServiceMutex_);
    return DeactivateAudioSessionInternal(callerPid);
}

int32_t AudioSessionService::DeactivateAudioSessionInternal(const int32_t &callerPid)
{
    AUDIO_INFO_LOG("DeactivateAudioSessionInternal: callerPid %{public}d", callerPid);
    if (sessionMap_.count(callerPid) == 0) {
        // The audio session of the callerPid is not existed or has been released.
        AUDIO_ERR_LOG("The audio seesion of pid %{public}d is not found!", callerPid);
        return ERR_ILLEGAL_STATE;
    }
    sessionMap_[callerPid]->Deactivate();
    sessionMap_.erase(callerPid);

    return SUCCESS;
}

bool AudioSessionService::IsAudioSessionActive(const int32_t &callerPid)
{
    std::lock_guard<std::mutex> lock(sessionServiceMutex_);
    if (sessionMap_.count(callerPid) == 0) {
        // The audio session of the callerPid is not existed or has been released.
        AUDIO_WARNING_LOG("The audio seesion of pid %{public}d is not found!", callerPid);
        return false;
    }
    SessionState state = sessionMap_[callerPid]->GetSessionState();

    return state == SessionState::SESSION_ACTIVE;
}

void AudioSessionService::SendAudioSessionDeactiveEvent(
    const std::pair<int32_t, AudioSessionDeactiveEvent> &sessionDeactivePair)
{
    AUDIO_INFO_LOG("AudioSessionService::SendAudioSessionDeactiveEvent");
    if (policyServerHandler_ != nullptr) {
        AUDIO_INFO_LOG("AudioSessionService::policyServerHandler_ is not null. Send event!");
        policyServerHandler_->SendAudioSessionDeactiveCallback(sessionDeactivePair);
    }
}

// Audio session timer callback
void AudioSessionService::OnAudioSessionTimeOut(const int32_t &callerPid)
{
    AUDIO_INFO_LOG("OnAudioSessionTimeOut: callerPid %{public}d", callerPid);
    std::lock_guard<std::mutex> lock(sessionServiceMutex_);
    AudioSessionDeactiveEvent deactiveEvent;
    deactiveEvent.deactiveReason = AudioSessionDeactiveReason::TIMEOUT;
    std::pair<int32_t, AudioSessionDeactiveEvent> sessionDeactivePair = {callerPid, deactiveEvent};
    SendAudioSessionDeactiveEvent(sessionDeactivePair);
}
} // namespace AudioStandard
} // namespace OHOS
