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

#include "audio_capturer_state_change_listener_proxy.h"
#include "audio_system_manager.h"
#include "audio_log.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
AudioCapturerStateChangeListenerProxy::AudioCapturerStateChangeListenerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardCapturerStateChangeListener>(impl)
{
    AUDIO_DEBUG_LOG("AudioCapturerStateChangeListenerProxy:Instances create");
}

AudioCapturerStateChangeListenerProxy::~AudioCapturerStateChangeListenerProxy()
{
    AUDIO_DEBUG_LOG("~AudioCapturerStateChangeListenerProxy: Instance destroy");
}

void AudioCapturerStateChangeListenerProxy::WriteCapturerChangeInfo(MessageParcel &data,
    const unique_ptr<AudioCapturerChangeInfo> &capturerChangeInfo)
{
    AUDIO_DEBUG_LOG("AudioCapturerStateChangeListenerProxy WriteCapturerChangeInfo sessionId = %{public}d",
        capturerChangeInfo->sessionId);
    AUDIO_DEBUG_LOG("AudioCapturerStateChangeListenerProxy WriteCapturerChangeInfo capturerState = %{public}d",
        capturerChangeInfo->capturerState);
    AUDIO_DEBUG_LOG("AudioCapturerStateChangeListenerProxy WriteCapturerChangeInfo client id = %{public}d",
        capturerChangeInfo->clientUID);
    data.WriteInt32(capturerChangeInfo->sessionId);
    data.WriteInt32(capturerChangeInfo->capturerState);
    data.WriteInt32(capturerChangeInfo->clientUID);
    data.WriteInt32(capturerChangeInfo->capturerInfo.sourceType);
    data.WriteInt32(capturerChangeInfo->capturerInfo.capturerFlags);
}

void AudioCapturerStateChangeListenerProxy::OnCapturerStateChange(
    const vector<unique_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    AUDIO_DEBUG_LOG("AudioCapturerStateChangeListenerProxy OnCapturerStateChange entered");

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioCapturerStateChangeListener: WriteInterfaceToken failed");
        return;
    }

    size_t size = audioCapturerChangeInfos.size();
    data.WriteInt32(size);
    for (const unique_ptr<AudioCapturerChangeInfo> &capturerChangeInfo: audioCapturerChangeInfos) {
        WriteCapturerChangeInfo(data, capturerChangeInfo);
    }

    int error = Remote()->SendRequest(ON_CAPTURERSTATE_CHANGE, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioCapturerStateChangeListener failed, error: %{public}d", error);
    }

    return;
}

AudioCapturerStateChangeListenerCallback::AudioCapturerStateChangeListenerCallback(
    const sptr<IStandardCapturerStateChangeListener> &listener) : listener_(listener)
{
    AUDIO_DEBUG_LOG("AudioCapturerStateChangeListenerCallback: Instance create");
}

AudioCapturerStateChangeListenerCallback::~AudioCapturerStateChangeListenerCallback()
{
    AUDIO_DEBUG_LOG("AudioCapturerStateChangeListenerCallback: Instance destroy");
}

void AudioCapturerStateChangeListenerCallback::OnCapturerStateChange(
    const vector<unique_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos)
{
    AUDIO_DEBUG_LOG("AudioCapturerStateChangeListenerCallback OnCapturerStateChange entered");
    if (listener_ != nullptr) {
        listener_->OnCapturerStateChange(audioCapturerChangeInfos);
    }
}
} // namespace AudioStandard
} // namespace OHOS
