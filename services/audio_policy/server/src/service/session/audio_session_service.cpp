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

#include "audio_errors.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
static const std::unordered_map<AudioStreamType, AudioSessionType> SESSION_TYPE_MAP = {
    {STREAM_ALARM, AudioSessionType::SONIFICATION},
    {STREAM_RING, AudioSessionType::SONIFICATION},
    {STREAM_MUSIC, AudioSessionType::MEDIA},
    {STREAM_MOVIE, AudioSessionType::MEDIA},
    {STREAM_GAME, AudioSessionType::MEDIA},
    {STREAM_SPEECH, AudioSessionType::MEDIA},
    {STREAM_NAVIGATION, AudioSessionType::MEDIA},
    {STREAM_VOICE_CALL, AudioSessionType::CALL},
    {STREAM_VOICE_CALL_ASSISTANT, AudioSessionType::CALL},
    {STREAM_VOICE_COMMUNICATION, AudioSessionType::VOIP},
    {STREAM_SYSTEM, AudioSessionType::SYSTEM},
    {STREAM_SYSTEM_ENFORCED, AudioSessionType::SYSTEM},
    {STREAM_ACCESSIBILITY, AudioSessionType::SYSTEM},
    {STREAM_ULTRASONIC, AudioSessionType::SYSTEM},
    {STREAM_NOTIFICATION, AudioSessionType::NOTIFICATION},
    {STREAM_DTMF, AudioSessionType::DTMF},
    {STREAM_VOICE_ASSISTANT, AudioSessionType::VOICE_ASSISTANT},
};

AudioSessionService::AudioSessionService()
{
    Init();
}

AudioSessionService::~AudioSessionService()
{
    AUDIO_ERR_LOG("should not happen");
}

void AudioSessionService::Init()
{
    AUDIO_INFO_LOG("AudioSessionService::Init");

    sessionTimer_ = std::make_shared<AudioSessionTimer>();
    sessionTimer_->SetAudioSessionTimerCallback(shared_from_this());
}

bool AudioSessionService::IsSameTypeForAudioSession(const AudioStreamType &newType, const AudioStreamType &oldType)
{
    if (SESSION_TYPE_MAP.count(newType) == 0 || SESSION_TYPE_MAP.count(oldType) == 0) {
        AUDIO_WARNING_LOG("The stream type (new: %{public}d or old: %{public}d) is invalid!", newType, oldType);
        return false;
    }
    return SESSION_TYPE_MAP.at(newType) == SESSION_TYPE_MAP.at(oldType);
}

int32_t AudioSessionService::ActivateAudioSession(const int32_t callerPid, const AudioSessionStrategy &strategy)
{
    AUDIO_INFO_LOG("ActivateAudioSession: callerPid %{public}d, concurrencyMode %{public}d",
        callerPid, static_cast<int32_t>(strategy.concurrencyMode));
    std::lock_guard<std::mutex> lock(sessionServiceMutex_);
    if (sessionMap_.count(callerPid) != 0) {
        // The audio session of the callerPid is already created. The strategy can not be modified.
        AUDIO_ERR_LOG("The audio seesion of pid %{public}d has already been created!", callerPid);
        return ERR_ILLEGAL_STATE;
    }

    sessionMap_[callerPid] = std::make_shared<AudioSession>(callerPid, strategy, sessionTimer_);
    sessionMap_[callerPid]->Activate();

    sessionTimer_->StartTimer(callerPid);

    return SUCCESS;
}

int32_t AudioSessionService::DeactivateAudioSession(const int32_t callerPid)
{
    AUDIO_INFO_LOG("DeactivateAudioSession: callerPid %{public}d", callerPid);
    std::lock_guard<std::mutex> lock(sessionServiceMutex_);
    return DeactivateAudioSessionInternal(callerPid);
}

int32_t AudioSessionService::DeactivateAudioSessionInternal(const int32_t callerPid)
{
    AUDIO_INFO_LOG("DeactivateAudioSessionInternal: callerPid %{public}d", callerPid);
    if (sessionMap_.count(callerPid) == 0) {
        // The audio session of the callerPid is not existed or has been released.
        AUDIO_ERR_LOG("The audio seesion of pid %{public}d is not found!", callerPid);
        return ERR_ILLEGAL_STATE;
    }
    sessionMap_[callerPid]->Deactivate();
    sessionMap_.erase(callerPid);

    sessionTimer_->StopTimer(callerPid);

    return SUCCESS;
}

bool AudioSessionService::IsAudioSessionActive(const int32_t callerPid)
{
    std::lock_guard<std::mutex> lock(sessionServiceMutex_);
    if (sessionMap_.count(callerPid) == 0) {
        // The audio session of the callerPid is not existed or has been released.
        AUDIO_WARNING_LOG("The audio seesion of pid %{public}d is not found!", callerPid);
        return false;
    }
    AudioSessionState state = sessionMap_[callerPid]->GetSessionState();

    return state == AudioSessionState::SESSION_ACTIVE;
}

int32_t AudioSessionService::SetSessionTimeOutCallback(
    const std::shared_ptr<SessionTimeOutCallback> &timeOutCallback)
{
    AUDIO_INFO_LOG("SetSessionTimeOutCallback is nullptr!");
    std::lock_guard<std::mutex> lock(sessionServiceMutex_);
    if (timeOutCallback == nullptr) {
        AUDIO_ERR_LOG("timeOutCallback is nullptr!");
        return AUDIO_INVALID_PARAM;
    }
    timeOutCallback_ = timeOutCallback;
    return SUCCESS;
}

std::shared_ptr<AudioSession> AudioSessionService::GetAudioSessionByPid(const int32_t callerPid)
{
    AUDIO_INFO_LOG("GetAudioSessionByPid: callerPid %{public}d", callerPid);
    std::lock_guard<std::mutex> lock(sessionServiceMutex_);
    if (sessionMap_.count(callerPid) == 0) {
        AUDIO_ERR_LOG("The audio seesion of pid %{public}d is not found!", callerPid);
        return nullptr;
    }
    return sessionMap_[callerPid];
}

int32_t AudioSessionService::DeactivateSessionByInterruptService(const int32_t callerPid)
{
    return SUCCESS;
}

// Audio session timer callback
void AudioSessionService::OnAudioSessionTimeOut(const int32_t callerPid)
{
    AUDIO_INFO_LOG("OnAudioSessionTimeOut: callerPid %{public}d", callerPid);
    std::lock_guard<std::mutex> lock(sessionServiceMutex_);

    DeactivateAudioSessionInternal(callerPid);

    auto cb = timeOutCallback_.lock();
    if (cb == nullptr) {
        AUDIO_ERR_LOG("timeOutCallback_ is nullptr!");
        return;
    }
    cb->OnSessionTimeout(callerPid);
}
} // namespace AudioStandard
} // namespace OHOS
