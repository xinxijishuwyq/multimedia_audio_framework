/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef AUDIO_ROUTING_MANAGER_LISTENER_STUB_H
#define AUDIO_ROUTING_MANAGER_LISTENER_STUB_H

#include "audio_routing_manager.h"
#include "i_standard_audio_routing_manager_listener.h"

namespace OHOS {
namespace AudioStandard {
class AudioRoutingManagerListenerStub : public IRemoteStub<IStandardAudioRoutingManagerListener> {
public:
    AudioRoutingManagerListenerStub();
    virtual ~AudioRoutingManagerListenerStub();

    int OnRemoteRequest(uint32_t code, MessageParcel &data,
                                MessageParcel &reply, MessageOption &option) override;
    void OnMicStateUpdated(const MicStateChangeEvent &micStateChangeEvent) override;
    void SetMicStateChangeCallback(const std::weak_ptr<AudioManagerMicStateChangeCallback> &callback);
private:
    std::weak_ptr<AudioManagerMicStateChangeCallback> micStateChangeCallback_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_RINGERMODE_UPDATE_LISTENER_STUB_H
