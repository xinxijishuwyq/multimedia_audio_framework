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
        case ON_MIC_STATE_UPDATED: {
            MicStateChangeEvent micStateChangeEvent = {};

            micStateChangeEvent.mute = data.ReadBool();
            OnMicStateUpdated(micStateChangeEvent);

            return AUDIO_OK;
        }
        case ON_ACTIVE_OUTPUT_DEVICE_UPDATED: {
            std::vector<sptr<AudioDeviceDescriptor>> deviceInfo;
            int32_t size = data.ReadInt32();
            AUDIO_INFO_LOG("Entered %{public}s ,desc size:%{public}d", __func__, size);
            for (int32_t i = 0; i < size; i++) {
                deviceInfo.push_back(AudioDeviceDescriptor::Unmarshalling(data));
            }
            OnPreferredOutputDeviceUpdated(deviceInfo);

            return AUDIO_OK;
        }
        default: {
            AUDIO_ERR_LOG("default case, need check AudioListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void AudioRoutingManagerListenerStub::OnMicStateUpdated(const MicStateChangeEvent &micStateChangeEvent)
{
    AUDIO_DEBUG_LOG("AudioPolicyManagerLiternerStub OnMicStateChange start");
    std::shared_ptr<AudioManagerMicStateChangeCallback> micStateChangedCallback = micStateChangeCallback_.lock();

    if (micStateChangedCallback == nullptr) {
        AUDIO_ERR_LOG("OnMicStateUpdated: micStateChangeCallback_ or micStateChangeEvent is nullptr");
        return;
    }

    micStateChangedCallback->OnMicStateUpdated(micStateChangeEvent);
}

void AudioRoutingManagerListenerStub::SetMicStateChangeCallback(
    const std::weak_ptr<AudioManagerMicStateChangeCallback> &cb)
{
    micStateChangeCallback_ = cb;
}

void AudioRoutingManagerListenerStub::OnPreferredOutputDeviceUpdated(
    const std::vector<sptr<AudioDeviceDescriptor>> &desc)
{
    std::shared_ptr<AudioPreferredOutputDeviceChangeCallback> activeOutputDeviceChangeCallback =
        activeOutputDeviceChangeCallback_.lock();

    if (activeOutputDeviceChangeCallback == nullptr) {
        AUDIO_ERR_LOG("OnPreferredOutputDeviceUpdated: activeOutputDeviceChangeCallback_ is nullptr");
        return;
    }

    activeOutputDeviceChangeCallback->OnPreferredOutputDeviceUpdated(desc);
}

void AudioRoutingManagerListenerStub::SetPreferredOutputDeviceChangeCallback(
    const std::weak_ptr<AudioPreferredOutputDeviceChangeCallback> &cb)
{
    activeOutputDeviceChangeCallback_ = cb;
}
} // namespace AudioStandard
} // namespace OHOS
