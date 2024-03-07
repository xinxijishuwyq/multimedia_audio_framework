/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#define LOG_TAG "AudioSpatializationStateChangeListenerStub"

#include "audio_spatialization_state_change_listener_stub.h"

#include "audio_errors.h"
#include "audio_spatialization_manager.h"
#include "audio_log.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
AudioSpatializationEnabledChangeListenerStub::AudioSpatializationEnabledChangeListenerStub()
{
    AUDIO_DEBUG_LOG("AudioSpatializationEnabledChangeListenerStub Instance create");
}

AudioSpatializationEnabledChangeListenerStub::~AudioSpatializationEnabledChangeListenerStub()
{
    AUDIO_DEBUG_LOG("AudioSpatializationEnabledChangeListenerStub Instance destroy");
}

int AudioSpatializationEnabledChangeListenerStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        AUDIO_ERR_LOG("AudioSpatializationEnabledChangeListenerStub: ReadInterfaceToken failed");
        return -1;
    }

    switch (code) {
        case ON_SPATIALIZATION_ENABLED_CHANGE: {
            bool enabled = data.ReadBool();
            OnSpatializationEnabledChange(enabled);
            return AUDIO_OK;
        }
        default: {
            AUDIO_ERR_LOG("default case, need check AudioListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void AudioSpatializationEnabledChangeListenerStub::OnSpatializationEnabledChange(const bool &enabled)
{
    AUDIO_DEBUG_LOG("AudioSpatializationEnabledChangeListenerStub OnSpatializationEnabledChange");

    shared_ptr<AudioSpatializationEnabledChangeCallback> cb = callback_.lock();
    if (cb == nullptr) {
        AUDIO_ERR_LOG("AudioSpatializationEnabledChangeListenerStub: callback_ is nullptr");
        return;
    }

    cb->OnSpatializationEnabledChange(enabled);
    return;
}

void AudioSpatializationEnabledChangeListenerStub::SetCallback(
    const weak_ptr<AudioSpatializationEnabledChangeCallback> &callback)
{
    AUDIO_DEBUG_LOG("AudioSpatializationEnabledChangeListenerStub SetCallback");
    callback_ = callback;
}

AudioHeadTrackingEnabledChangeListenerStub::AudioHeadTrackingEnabledChangeListenerStub()
{
    AUDIO_DEBUG_LOG("AudioHeadTrackingEnabledChangeListenerStub Instance create");
}

AudioHeadTrackingEnabledChangeListenerStub::~AudioHeadTrackingEnabledChangeListenerStub()
{
    AUDIO_DEBUG_LOG("AudioHeadTrackingEnabledChangeListenerStub Instance destroy");
}

int AudioHeadTrackingEnabledChangeListenerStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        AUDIO_ERR_LOG("AudioHeadTrackingEnabledChangeListenerStub: ReadInterfaceToken failed");
        return -1;
    }

    switch (code) {
        case ON_HEAD_TRACKING_ENABLED_CHANGE: {
            bool enabled = data.ReadBool();
            OnHeadTrackingEnabledChange(enabled);
            return AUDIO_OK;
        }
        default: {
            AUDIO_ERR_LOG("default case, need check AudioListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void AudioHeadTrackingEnabledChangeListenerStub::OnHeadTrackingEnabledChange(const bool &enabled)
{
    AUDIO_DEBUG_LOG("AudioHeadTrackingEnabledChangeListenerStub OnHeadTrackingEnabledChange");

    shared_ptr<AudioHeadTrackingEnabledChangeCallback> cb = callback_.lock();
    if (cb == nullptr) {
        AUDIO_ERR_LOG("AudioHeadTrackingEnabledChangeListenerStub: callback_ is nullptr");
        return;
    }

    cb->OnHeadTrackingEnabledChange(enabled);
    return;
}

void AudioHeadTrackingEnabledChangeListenerStub::SetCallback(
    const weak_ptr<AudioHeadTrackingEnabledChangeCallback> &callback)
{
    AUDIO_DEBUG_LOG("AudioHeadTrackingEnabledChangeListenerStub SetCallback");
    callback_ = callback;
}

AudioSpatializationStateChangeListenerStub::AudioSpatializationStateChangeListenerStub()
{
    AUDIO_DEBUG_LOG("AudioSpatializationStateChangeListenerStub Instance create");
}

AudioSpatializationStateChangeListenerStub::~AudioSpatializationStateChangeListenerStub()
{
    AUDIO_DEBUG_LOG("AudioSpatializationStateChangeListenerStub Instance destroy");
}

int AudioSpatializationStateChangeListenerStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        AUDIO_ERR_LOG("AudioSpatializationStateChangeListenerStub: ReadInterfaceToken failed");
        return -1;
    }

    switch (code) {
        case ON_SPATIALIZATION_STATE_CHANGE: {
            AudioSpatializationState spatializationState;
            spatializationState.spatializationEnabled = data.ReadBool();
            spatializationState.headTrackingEnabled = data.ReadBool();
            OnSpatializationStateChange(spatializationState);
            return AUDIO_OK;
        }
        default: {
            AUDIO_ERR_LOG("default case, need check AudioListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void AudioSpatializationStateChangeListenerStub::OnSpatializationStateChange(
    const AudioSpatializationState &spatializationState)
{
    AUDIO_DEBUG_LOG("AudioSpatializationStateChangeListenerStub OnSpatializationStateChange");

    shared_ptr<AudioSpatializationStateChangeCallback> cb = callback_.lock();
    if (cb == nullptr) {
        AUDIO_ERR_LOG("AudioSpatializationStateChangeListenerStub: callback_ is nullptr");
        return;
    }

    cb->OnSpatializationStateChange(spatializationState);
    return;
}

void AudioSpatializationStateChangeListenerStub::SetCallback(
    const weak_ptr<AudioSpatializationStateChangeCallback> &callback)
{
    AUDIO_DEBUG_LOG("AudioSpatializationStateChangeListenerStub SetCallback");
    callback_ = callback;
}
} // namespace AudioStandard
} // namespace OHOS
