/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "audio_volume_key_event_callback_stub.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
AudioVolumeKeyEventCallbackStub::AudioVolumeKeyEventCallbackStub()
{
}

AudioVolumeKeyEventCallbackStub::~AudioVolumeKeyEventCallbackStub()
{
}

int AudioVolumeKeyEventCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    AUDIO_DEBUG_LOG("AudioVolumeKeyEventCallbackStub::OnRemoteRequest");
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        AUDIO_ERR_LOG("AudioVolumeKeyEventCallbackStub: ReadInterfaceToken failed");
        return -1;
    }
    switch (code) {
        case ON_VOLUME_KEY_EVENT: {
            VolumeEvent volumeEvent;
            volumeEvent.volumeType = static_cast<AudioStreamType>(data.ReadInt32());
            volumeEvent.volume = data.ReadInt32();
            volumeEvent.updateUi = data.ReadBool();
            volumeEvent.volumeGroupId = data.ReadInt32();
            volumeEvent.networkId = data.ReadString();
            OnVolumeKeyEvent(volumeEvent);
            reply.WriteInt32(0);
            break;
        }
        default: {
            reply.WriteInt32(-1);
            break;
        }
    }
    return 0;
}

void AudioVolumeKeyEventCallbackStub::OnVolumeKeyEvent(VolumeEvent volumeEvent)
{
    AUDIO_DEBUG_LOG("AudioVolumeKeyEventCallbackStub::OnVolumeKeyEvent");
    std::shared_ptr<VolumeKeyEventCallback> cb = callback_.lock();
    if (cb != nullptr) {
        AUDIO_DEBUG_LOG("AudioVolumeKeyEventCallbackStub::OnVolumeKeyEvent CALLBACK NOT NULL");
        cb->OnVolumeKeyEvent(volumeEvent);
    } else {
        AUDIO_DEBUG_LOG("AudioVolumeKeyEventCallbackStub::OnVolumeKeyEvent CALLBACK NULL");
    }
}

void AudioVolumeKeyEventCallbackStub::SetOnVolumeKeyEventCallback(
    const std::weak_ptr<VolumeKeyEventCallback> &callback)
{
    AUDIO_DEBUG_LOG("AudioVolumeKeyEventCallbackStub::SetOnVolumeKeyEventCallback");
    callback_ = callback;
}
} // namespace AudioStandard
} // namespace OHOS
