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

#include "audio_policy_manager_listener_proxy.h"
#include "audio_system_manager.h"
#include "media_log.h"

namespace OHOS {
namespace AudioStandard {
AudioPolicyManagerListenerProxy::AudioPolicyManagerListenerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardAudioPolicyManagerListener>(impl)
{
    MEDIA_DEBUG_LOG("Instances create");
}

AudioPolicyManagerListenerProxy::~AudioPolicyManagerListenerProxy()
{
    MEDIA_DEBUG_LOG("~AudioPolicyManagerListenerProxy: Instance destroy");
}

void AudioPolicyManagerListenerProxy::WriteInterruptActionParams(MessageParcel &data,
                                                                 const InterruptAction &interruptAction)
{
    data.WriteInt32(static_cast<int32_t>(interruptAction.actionType));
    data.WriteInt32(static_cast<int32_t>(interruptAction.interruptType));
    data.WriteInt32(static_cast<int32_t>(interruptAction.interruptHint));
}

void AudioPolicyManagerListenerProxy::OnInterrupt(const InterruptAction &interruptAction)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    WriteInterruptActionParams(data, interruptAction);
    int error = Remote()->SendRequest(ON_INTERRUPT, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("OnInterrupt failed, error: %{public}d", error);
    }
}

AudioPolicyManagerListenerCallback::AudioPolicyManagerListenerCallback(
    const sptr<IStandardAudioPolicyManagerListener> &listener) : listener_(listener)
{
        MEDIA_DEBUG_LOG("AudioPolicyManagerListenerCallback: Instance create");
}

AudioPolicyManagerListenerCallback::~AudioPolicyManagerListenerCallback()
{
    MEDIA_DEBUG_LOG("AudioPolicyManagerListenerCallback: Instance destory");
}

void AudioPolicyManagerListenerCallback::OnInterrupt(const InterruptAction &interruptAction)
{
    if (listener_ != nullptr) {
        listener_->OnInterrupt(interruptAction);
    }
}
} // namespace AudioStandard
} // namespace OHOS
