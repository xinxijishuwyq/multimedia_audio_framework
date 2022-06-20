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

#include "audio_renderer_state_change_listener_proxy.h"
#include "audio_system_manager.h"
#include "audio_log.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
AudioRendererStateChangeListenerProxy::AudioRendererStateChangeListenerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardRendererStateChangeListener>(impl)
{
    AUDIO_DEBUG_LOG("AudioRendererStateChangeListenerProxy:Instances create");
}

AudioRendererStateChangeListenerProxy::~AudioRendererStateChangeListenerProxy()
{
    AUDIO_DEBUG_LOG("~AudioRendererStateChangeListenerProxy: Instance destroy");
}

void AudioRendererStateChangeListenerProxy::WriteRendererChangeInfo(MessageParcel &data,
    const unique_ptr<AudioRendererChangeInfo> &rendererChangeInfo)
{
    AUDIO_INFO_LOG("AudioRendererStateChangeListenerProxy WriteRendererChangeInfo sessionId = %{public}d",
        rendererChangeInfo->sessionId);
    AUDIO_INFO_LOG("AudioRendererStateChangeListenerProxy WriteRendererChangeInfo rendererState = %{public}d",
        rendererChangeInfo->rendererState);
    AUDIO_INFO_LOG("AudioRendererStateChangeListenerProxy WriteRendererChangeInfo client id = %{public}d",
        rendererChangeInfo->clientUID);
    AUDIO_INFO_LOG("AudioRendererStateChangeListenerProxy WriteRendererChangeInfo contenttype = %{public}d",
        rendererChangeInfo->rendererInfo.contentType);
    AUDIO_INFO_LOG("AudioRendererStateChangeListenerProxy WriteRendererChangeInfo streamusage = %{public}d",
        rendererChangeInfo->rendererInfo.streamUsage);
    AUDIO_INFO_LOG("AudioRendererStateChangeListenerProxy WriteRendererChangeInfo rendererflags = %{public}d",
        rendererChangeInfo->rendererInfo.rendererFlags);
    data.WriteInt32(rendererChangeInfo->sessionId);
    data.WriteInt32(rendererChangeInfo->rendererState);
    data.WriteInt32(rendererChangeInfo->clientUID);
    data.WriteInt32(rendererChangeInfo->rendererInfo.contentType);
    data.WriteInt32(rendererChangeInfo->rendererInfo.streamUsage);
    data.WriteInt32(rendererChangeInfo->rendererInfo.rendererFlags);
}

void AudioRendererStateChangeListenerProxy::OnRendererStateChange(
    const vector<unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    AUDIO_DEBUG_LOG("AudioRendererStateChangeListenerProxy OnRendererStateChange entered");

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioRendererStateChangeListener: WriteInterfaceToken failed");
        return;
    }

    size_t size = audioRendererChangeInfos.size();
    data.WriteInt32(size);
    for (const unique_ptr<AudioRendererChangeInfo> &rendererChangeInfo: audioRendererChangeInfos) {
        WriteRendererChangeInfo(data, rendererChangeInfo);
    }

    int error = Remote()->SendRequest(ON_RENDERERSTATE_CHANGE, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioRendererStateChangeListener failed, error: %{public}d", error);
    }

    return;
}

AudioRendererStateChangeListenerCallback::AudioRendererStateChangeListenerCallback(
    const sptr<IStandardRendererStateChangeListener> &listener) : listener_(listener)
{
        AUDIO_DEBUG_LOG("AudioRendererStateChangeListenerCallback: Instance create");
}

AudioRendererStateChangeListenerCallback::~AudioRendererStateChangeListenerCallback()
{
    AUDIO_DEBUG_LOG("AudioRendererStateChangeListenerCallback: Instance destroy");
}

void AudioRendererStateChangeListenerCallback::OnRendererStateChange(
    const vector<unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    AUDIO_DEBUG_LOG("AudioRendererStateChangeListenerCallback OnRendererStateChange entered");
    if (listener_ != nullptr) {
        listener_->OnRendererStateChange(audioRendererChangeInfos);
    }
}
} // namespace AudioStandard
} // namespace OHOS
