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
#define LOG_TAG "AudioSpatializationStateChangeListenerProxy"

#include "audio_spatialization_state_change_listener_proxy.h"
#include "audio_spatialization_manager.h"
#include "audio_log.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
AudioSpatializationEnabledChangeListenerProxy::AudioSpatializationEnabledChangeListenerProxy(
    const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardSpatializationEnabledChangeListener>(impl)
{
    AUDIO_DEBUG_LOG("AudioSpatializationEnabledChangeListenerProxy:Instances create");
}

AudioSpatializationEnabledChangeListenerProxy::~AudioSpatializationEnabledChangeListenerProxy()
{
    AUDIO_DEBUG_LOG("~AudioSpatializationEnabledChangeListenerProxy: Instance destroy");
}

void AudioSpatializationEnabledChangeListenerProxy::OnSpatializationEnabledChange(const bool &enabled)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    AUDIO_DEBUG_LOG("AudioSpatializationEnabledChangeListenerProxy OnSpatializationEnabledChange entered");

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioSpatializationEnabledChangeListener: WriteInterfaceToken failed");
        return;
    }

    data.WriteBool(enabled);

    int32_t error = Remote()->SendRequest(ON_SPATIALIZATION_ENABLED_CHANGE, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioSpatializationEnabledChangeListener failed, error: %{public}d", error);
    }

    return;
}

AudioHeadTrackingEnabledChangeListenerProxy::AudioHeadTrackingEnabledChangeListenerProxy(
    const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardHeadTrackingEnabledChangeListener>(impl)
{
    AUDIO_DEBUG_LOG("AudioHeadTrackingEnabledChangeListenerProxy:Instances create");
}

AudioHeadTrackingEnabledChangeListenerProxy::~AudioHeadTrackingEnabledChangeListenerProxy()
{
    AUDIO_DEBUG_LOG("~AudioHeadTrackingEnabledChangeListenerProxy: Instance destroy");
}

void AudioHeadTrackingEnabledChangeListenerProxy::OnHeadTrackingEnabledChange(const bool &enabled)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    AUDIO_DEBUG_LOG("AudioHeadTrackingEnabledChangeListenerProxy OnHeadTrackingEnabledChange entered");

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioHeadTrackingEnabledChangeListener: WriteInterfaceToken failed");
        return;
    }

    data.WriteBool(enabled);

    int32_t error = Remote()->SendRequest(ON_HEAD_TRACKING_ENABLED_CHANGE, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioHeadTrackingEnabledChangeListener failed, error: %{public}d", error);
    }

    return;
}

AudioSpatializationStateChangeListenerProxy::AudioSpatializationStateChangeListenerProxy(
    const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardSpatializationStateChangeListener>(impl)
{
    AUDIO_DEBUG_LOG("AudioSpatializationStateChangeListenerProxy:Instances create");
}

AudioSpatializationStateChangeListenerProxy::~AudioSpatializationStateChangeListenerProxy()
{
    AUDIO_DEBUG_LOG("~AudioSpatializationStateChangeListenerProxy: Instance destroy");
}

void AudioSpatializationStateChangeListenerProxy::OnSpatializationStateChange(
    const AudioSpatializationState &spatializationState)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    AUDIO_DEBUG_LOG("AudioSpatializationStateChangeListenerProxy OnSpatializationStateChange entered");

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioSpatializationStateChangeListener: WriteInterfaceToken failed");
        return;
    }

    data.WriteBool(spatializationState.spatializationEnabled);
    data.WriteBool(spatializationState.headTrackingEnabled);

    int32_t error = Remote()->SendRequest(ON_SPATIALIZATION_STATE_CHANGE, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioSpatializationStateChangeListener failed, error: %{public}d", error);
    }

    return;
}

AudioSpatializationEnabledChangeListenerCallback::AudioSpatializationEnabledChangeListenerCallback(
    const sptr<IStandardSpatializationEnabledChangeListener> &listener, bool hasSystemPermission)
    : listener_(listener), hasSystemPermission_(hasSystemPermission)
{
    AUDIO_DEBUG_LOG("AudioSpatializationEnabledChangeListenerCallback: Instance create");
}

AudioSpatializationEnabledChangeListenerCallback::~AudioSpatializationEnabledChangeListenerCallback()
{
    AUDIO_DEBUG_LOG("AudioSpatializationEnabledChangeListenerCallback: Instance destroy");
}

void AudioSpatializationEnabledChangeListenerCallback::OnSpatializationEnabledChange(const bool &enabled)
{
    AUDIO_DEBUG_LOG("AudioSpatializationEnabledChangeListenerCallback OnSpatializationEnabledChange entered");
    if (listener_ != nullptr) {
        if (hasSystemPermission_) {
            listener_->OnSpatializationEnabledChange(enabled);
        } else {
            listener_->OnSpatializationEnabledChange(false);
        }
    }
}

AudioHeadTrackingEnabledChangeListenerCallback::AudioHeadTrackingEnabledChangeListenerCallback(
    const sptr<IStandardHeadTrackingEnabledChangeListener> &listener, bool hasSystemPermission)
    : listener_(listener), hasSystemPermission_(hasSystemPermission)
{
    AUDIO_DEBUG_LOG("AudioHeadTrackingEnabledChangeListenerCallback: Instance create");
}

AudioHeadTrackingEnabledChangeListenerCallback::~AudioHeadTrackingEnabledChangeListenerCallback()
{
    AUDIO_DEBUG_LOG("AudioHeadTrackingEnabledChangeListenerCallback: Instance destroy");
}

void AudioHeadTrackingEnabledChangeListenerCallback::OnHeadTrackingEnabledChange(const bool &enabled)
{
    AUDIO_DEBUG_LOG("AudioHeadTrackingEnabledChangeListenerCallback OnHeadTrackingEnabledChange entered");
    if (listener_ != nullptr) {
        if (hasSystemPermission_) {
            listener_->OnHeadTrackingEnabledChange(enabled);
        } else {
            listener_->OnHeadTrackingEnabledChange(false);
        }
    }
}

AudioSpatializationStateChangeListenerCallback::AudioSpatializationStateChangeListenerCallback(
    const sptr<IStandardSpatializationStateChangeListener> &listener)
    : listener_(listener)
{
    AUDIO_DEBUG_LOG("AudioSpatializationStateChangeListenerCallback: Instance create");
}

AudioSpatializationStateChangeListenerCallback::~AudioSpatializationStateChangeListenerCallback()
{
    AUDIO_DEBUG_LOG("AudioSpatializationStateChangeListenerCallback: Instance destroy");
}

void AudioSpatializationStateChangeListenerCallback::OnSpatializationStateChange(
    const AudioSpatializationState &spatializationState)
{
    AUDIO_DEBUG_LOG("AudioSpatializationStateChangeListenerCallback OnSpatializationStateChange entered");
    if (listener_ != nullptr) {
        listener_->OnSpatializationStateChange(spatializationState);
    }
}
} // namespace AudioStandard
} // namespace OHOS
