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
#define LOG_TAG "AudioSession"

#include "audio_session.h"

#include "audio_log.h"
#include "audio_errors.h"

namespace OHOS {
namespace AudioStandard {
AudioSession::AudioSession(const int32_t &callerPid, const AudioSessionStrategy &strategy)
{
    AUDIO_INFO_LOG("AudioSession()");
    callerPid_ = callerPid;
    strategy_ = strategy;
    state_ = SessionState::SESSION_NEW;
}

AudioSession::~AudioSession()
{
    AUDIO_ERR_LOG("~AudioSession()");
}

int32_t AudioSession::Activate()
{
    AUDIO_INFO_LOG("Activate: pid %{public}d", callerPid_);
    state_ = SessionState::SESSION_ACTIVE;
    return SUCCESS;
}

int32_t AudioSession::Deactivate()
{
    AUDIO_INFO_LOG("Deactivate: pid %{public}d", callerPid_);
    state_ = SessionState::SESSION_DEACTIVE;
    return SUCCESS;
}

SessionState AudioSession::GetSessionState()
{
    AUDIO_INFO_LOG("GetSessionState: pid %{public}d, state %{public}d", callerPid_, state_);
    return state_;
}

int32_t AudioSession::AddAudioInterrpt(const std::pair<AudioInterrupt, AudioFocuState> &interruptPair)
{
    uint32_t streamId = interruptPair.first.sessionId;
    AUDIO_INFO_LOG("AddAudioInterrpt: streamId %{public}u", streamId);
    if (interruptMap_.count(streamId) != 0) {
        AUDIO_WARNING_LOG("The streamId has been added. The old interrupt will be coverd.");
    }
    interruptMap_[streamId] = interruptPair;
    return SUCCESS;
}

int32_t AudioSession::RemoveAudioInterrpt(const std::pair<AudioInterrupt, AudioFocuState> &interruptPair)
{
    uint32_t streamId = interruptPair.first.sessionId;
    AUDIO_INFO_LOG("RemoveAudioInterrpt: streamId %{public}u", streamId);
    if (interruptMap_.count(streamId) == 0) {
        AUDIO_WARNING_LOG("The streamId has been removed.");
        return SUCCESS;
    }
    interruptMap_.erase(streamId);
    return SUCCESS;
}
} // namespace AudioStandard
} // namespace OHOS
