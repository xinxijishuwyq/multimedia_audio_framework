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
#undef LOG_TAG
#define LOG_TAG "AudioRoutingManagerListenerProxy"

#include "audio_routing_manager_listener_proxy.h"
#include "audio_routing_manager.h"
#include "audio_system_manager.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
AudioRoutingManagerListenerProxy::AudioRoutingManagerListenerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardAudioRoutingManagerListener>(impl)
{
    AUDIO_DEBUG_LOG("Instances create");
}

AudioRoutingManagerListenerProxy::~AudioRoutingManagerListenerProxy()
{
    AUDIO_DEBUG_LOG("~AudioRoutingManagerListenerProxy: Instance destroy");
}

void AudioRoutingManagerListenerProxy::OnDistributedRoutingRoleChange(const sptr<AudioDeviceDescriptor> desciptor,
    const CastType type)
{
    AUDIO_DEBUG_LOG("AudioRoutingManagerListenerProxy: OnDistributedRoutingRoleChange as listener proxy");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    CHECK_AND_RETURN_LOG(data.WriteInterfaceToken(GetDescriptor()),
        "OnDistributedRoutingRoleChange: WriteInterfaceToken failed");

    desciptor->Marshalling(data);
    data.WriteInt32(type);

    int error = Remote()->SendRequest(ON_DISTRIBUTED_ROUTING_ROLE_CHANGE, data, reply, option);
    CHECK_AND_RETURN_LOG(error == ERR_NONE, "OnDistributedRoutingRoleChangefailed, error: %{public}d", error);
}
} // namespace AudioStandard
} // namespace OHOS
