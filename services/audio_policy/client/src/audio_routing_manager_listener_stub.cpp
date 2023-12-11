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

#include "audio_routing_manager_listener_stub.h"

#include "audio_errors.h"
#include "audio_routing_manager.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
AudioRoutingManagerListenerStub::AudioRoutingManagerListenerStub()
{
    AUDIO_DEBUG_LOG("AudioRoutingManagerListenerStub Instance create");
}

AudioRoutingManagerListenerStub::~AudioRoutingManagerListenerStub()
{
    AUDIO_DEBUG_LOG("AudioRoutingManagerListenerStub Instance destroy");
}

int AudioRoutingManagerListenerStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        AUDIO_ERR_LOG("AudioRingerModeUpdateListenerStub: ReadInterfaceToken failed");
        return -1;
    }
    switch (code) {
        case ON_DISTRIBUTED_ROUTING_ROLE_CHANGE: {
            sptr<AudioDeviceDescriptor> descriptor = AudioDeviceDescriptor::Unmarshalling(data);
            CastType type = static_cast<CastType>(data.ReadInt32());
            OnDistributedRoutingRoleChange(descriptor, type);
            return AUDIO_OK;
        }
        default: {
            AUDIO_ERR_LOG("default case, need check AudioListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void AudioRoutingManagerListenerStub::OnDistributedRoutingRoleChange(const sptr<AudioDeviceDescriptor> descriptor,
    const CastType type)
{
    std::shared_ptr<AudioDistributedRoutingRoleCallback> audioDistributedRoutingRoleCallback =
        audioDistributedRoutingRoleCallback_.lock();

    if (audioDistributedRoutingRoleCallback == nullptr) {
        AUDIO_ERR_LOG("OnDistributedRoutingRoleChange: audioDistributedRoutingRoleCallback_ is nullptr");
        return;
    }

    audioDistributedRoutingRoleCallback->OnDistributedRoutingRoleChange(descriptor, type);
}

void AudioRoutingManagerListenerStub::SetDistributedRoutingRoleCallback(
    const std::weak_ptr<AudioDistributedRoutingRoleCallback> &callback)
{
    audioDistributedRoutingRoleCallback_ = callback;
}
} // namespace AudioStandard
} // namespace OHOS
