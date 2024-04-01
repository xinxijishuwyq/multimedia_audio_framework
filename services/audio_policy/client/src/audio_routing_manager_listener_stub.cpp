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
#define LOG_TAG "AudioRoutingManagerListenerStub"

#include "audio_routing_manager_listener_stub.h"

#include "audio_errors.h"
#include "audio_routing_manager.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
AudioRoutingManagerListenerStub::AudioRoutingManagerListenerStub()
{
}

AudioRoutingManagerListenerStub::~AudioRoutingManagerListenerStub()
{
}

int AudioRoutingManagerListenerStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    CHECK_AND_RETURN_RET_LOG(data.ReadInterfaceToken() == GetDescriptor(),
        -1, "AudioRingerModeUpdateListenerStub: ReadInterfaceToken failed");
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

    CHECK_AND_RETURN_LOG(audioDistributedRoutingRoleCallback != nullptr,
        "OnDistributedRoutingRoleChange: audioDistributedRoutingRoleCallback_ is nullptr");

    audioDistributedRoutingRoleCallback->OnDistributedRoutingRoleChange(descriptor, type);
}

void AudioRoutingManagerListenerStub::SetDistributedRoutingRoleCallback(
    const std::weak_ptr<AudioDistributedRoutingRoleCallback> &callback)
{
    audioDistributedRoutingRoleCallback_ = callback;
}
} // namespace AudioStandard
} // namespace OHOS
