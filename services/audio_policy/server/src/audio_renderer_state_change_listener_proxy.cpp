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
#include "audio_group_handle.h"

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

void AudioRendererStateChangeListenerProxy::OnRendererStateChange(
    const vector<unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    AUDIO_DEBUG_LOG("AudioRendererStateChangeListenerProxy OnRendererStateChange entered");

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioRendererStateChangeListener: WriteInterfaceToken failed");
        return;
    }

    size_t size = audioRendererChangeInfos.size();
    data.WriteInt32(size);
    for (const unique_ptr<AudioRendererChangeInfo> &rendererChangeInfo: audioRendererChangeInfos) {
        if (!rendererChangeInfo) {
            AUDIO_ERR_LOG("Renderer change info null, something wrong!!");
            continue;
        }
        rendererChangeInfo->Marshalling(data);
    }

    int error = Remote()->SendRequest(ON_RENDERERSTATE_CHANGE, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioRendererStateChangeListener failed, error: %{public}d", error);
    }

    return;
}

AudioRendererStateChangeListenerCallback::AudioRendererStateChangeListenerCallback(
    const sptr<IStandardRendererStateChangeListener> &listener, bool hasBTPermission, bool hasSystemPermission)
    : listener_(listener), hasBTPermission_(hasBTPermission), hasSystemPermission_(hasSystemPermission)
{
    AUDIO_DEBUG_LOG("AudioRendererStateChangeListenerCallback: Instance create");
}

AudioRendererStateChangeListenerCallback::~AudioRendererStateChangeListenerCallback()
{
    AUDIO_DEBUG_LOG("AudioRendererStateChangeListenerCallback: Instance destroy");
}

void AudioRendererStateChangeListenerCallback::UpdateDeviceInfo(
    const vector<unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    if (!hasBTPermission_) {
        size_t rendererChangeInfoLength = audioRendererChangeInfos.size();
        for (size_t i = 0; i < rendererChangeInfoLength; i++) {
            if ((audioRendererChangeInfos[i]->outputDeviceInfo.deviceType == DEVICE_TYPE_BLUETOOTH_A2DP)
                || (audioRendererChangeInfos[i]->outputDeviceInfo.deviceType == DEVICE_TYPE_BLUETOOTH_SCO)) {
                audioRendererChangeInfos[i]->outputDeviceInfo.deviceName = "";
                audioRendererChangeInfos[i]->outputDeviceInfo.macAddress = "";
            }
        }
    }

    if (!hasSystemPermission_) {
        size_t rendererChangeInfoLength = audioRendererChangeInfos.size();
        for (size_t i = 0; i < rendererChangeInfoLength; i++) {
            audioRendererChangeInfos[i]->clientUID = 0;
            audioRendererChangeInfos[i]->rendererState = RENDERER_INVALID;
            audioRendererChangeInfos[i]->outputDeviceInfo.networkId = "";
            audioRendererChangeInfos[i]->outputDeviceInfo.interruptGroupId = GROUP_ID_NONE;
            audioRendererChangeInfos[i]->outputDeviceInfo.volumeGroupId = GROUP_ID_NONE;
        }
    }
}

void AudioRendererStateChangeListenerCallback::OnRendererStateChange(
    const vector<unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    AUDIO_DEBUG_LOG("AudioRendererStateChangeListenerCallback OnRendererStateChange entered");
    if (listener_ != nullptr) {
        UpdateDeviceInfo(audioRendererChangeInfos);
        listener_->OnRendererStateChange(audioRendererChangeInfos);
    }
}
} // namespace AudioStandard
} // namespace OHOS
