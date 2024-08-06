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

#ifndef ST_AUDIO_SESSION_SERVICE_H
#define ST_AUDIO_SESSION_SERVICE_H

#include <mutex>

#include "audio_interrupt_service.h"
#include "audio_policy_server_handler.h"
#include "audio_policy_server.h"
#include "audio_session.h"
#include "audio_session_timer.h"

namespace OHOS {
namespace AudioStandard {

class AudioSessionService : public AudioSessionTimerCallback, public std::enable_shared_from_this<AudioSessionService> {
public:
    AudioSessionService();
    virtual ~AudioSessionService();

    // Audio session manager interfaces
    int32_t ActivateAudioSession(const int32_t &callerPid, const AudioSessionStrategy &strategy);
    int32_t DeactivateAudioSession(const int32_t &callerPid);
    bool IsAudioSessionActive(const int32_t &callerPid);

    // Audio session timer callback
    void OnAudioSessionTimeOut(const int32_t &callerPid) override;

    // other public interfaces
    void Init(sptr<AudioPolicyServer> server);

private:
    int32_t DeactivateAudioSessionInternal(const int32_t &callerPid);
    void SendAudioSessionDeactiveEvent(const std::pair<int32_t, AudioSessionDeactiveEvent> &sessionDeactivePair);

    std::mutex sessionServiceMutex_;

    sptr<AudioPolicyServer> policyServer_;
    std::shared_ptr<AudioPolicyServerHandler> policyServerHandler_;
    std::shared_ptr<AudioSessionTimer> sessionTimer_;

    std::unordered_map<int32_t, std::shared_ptr<AudioSession>> sessionMap_;
};
} // namespace AudioStandard
} // namespace OHOS

#endif // ST_AUDIO_SESSION_SERVICE_H