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
#include "audio_system_manager.h"
#include "media_log.h"
#include "audio_policy_manager_listener_stub.h"

namespace OHOS {
namespace AudioStandard {
AudioPolicyManagerListenerStub::AudioPolicyManagerListenerStub()
{
    MEDIA_DEBUG_LOG("AudioPolicyManagerLiternerStub Instance create");
}

AudioPolicyManagerListenerStub::~AudioPolicyManagerListenerStub()
{
    MEDIA_DEBUG_LOG("AudioPolicyManagerLiternerStub Instance destroy");
}

void AudioPolicyManagerListenerStub::ReadInterruptActionParams(MessageParcel &data, InterruptAction &interruptAction)
{
    interruptAction.actionType = static_cast<InterruptActionType>(data.ReadInt32());
    interruptAction.interruptType = static_cast<InterruptType>(data.ReadInt32());
    interruptAction.interruptHint = static_cast<InterruptHint>(data.ReadInt32());
}

int AudioPolicyManagerListenerStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    switch (code) {
        case ON_INTERRUPT: {
            InterruptAction interruptAction = {};
            ReadInterruptActionParams(data, interruptAction);
            // To be modified by enqueuing the interrupt action scheduler
            OnInterrupt(interruptAction);
            return MEDIA_OK;
        }
        default: {
            MEDIA_ERR_LOG("default case, need check AudioListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void AudioPolicyManagerListenerStub::OnInterrupt(const InterruptAction &interruptAction)
{
    std::shared_ptr<AudioManagerCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnInterrupt(interruptAction);
    } else {
        MEDIA_ERR_LOG("AudioPolicyManagerListenerStub: callback_ is nullptr");
    }
}

void AudioPolicyManagerListenerStub::SetCallback(const std::weak_ptr<AudioManagerCallback> &callback)
{
    callback_ = callback;
}
} // namespace AudioStandard
} // namespace OHOS
