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

#include "audio_renderer_state_change_listener_stub.h"

#include "audio_errors.h"
#include "audio_system_manager.h"
#include "audio_log.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
AudioRendererStateChangeListenerStub::AudioRendererStateChangeListenerStub()
{
    AUDIO_DEBUG_LOG("AudioRendererStateChangeListenerStub Instance create");
}

AudioRendererStateChangeListenerStub::~AudioRendererStateChangeListenerStub()
{
    AUDIO_DEBUG_LOG("AudioRendererStateChangeListenerStub Instance destroy");
}

int AudioRendererStateChangeListenerStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        AUDIO_ERR_LOG("AudioRendererStateChangeListenerStub: ReadInterfaceToken failed");
        return -1;
    }

    switch (code) {
        case ON_RENDERERSTATE_CHANGE: {
            vector<unique_ptr<AudioRendererChangeInfo>> audioRendererChangeInfos;
            int32_t size = data.ReadInt32();
            while (size > 0) {
                unique_ptr<AudioRendererChangeInfo> rendererChangeInfo = make_unique<AudioRendererChangeInfo>();
                CHECK_AND_RETURN_RET_LOG(rendererChangeInfo != nullptr, ERR_MEMORY_ALLOC_FAILED, "No memory!!");
                rendererChangeInfo->Unmarshalling(data);
                audioRendererChangeInfos.push_back(move(rendererChangeInfo));
                size--;
            }
            OnRendererStateChange(audioRendererChangeInfos);
            return AUDIO_OK;
        }
        default: {
            AUDIO_ERR_LOG("default case, need check AudioListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void AudioRendererStateChangeListenerStub::OnRendererStateChange(
    const vector<unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    AUDIO_DEBUG_LOG("AudioRendererStateChangeListenerStub OnRendererStateChange");

    shared_ptr<AudioRendererStateChangeCallback> cb = callback_.lock();
    if (cb == nullptr) {
        AUDIO_ERR_LOG("AudioRendererStateChangeListenerStub: callback_ is nullptr");
        return;
    }

    cb->OnRendererStateChange(audioRendererChangeInfos);
    return;
}

void AudioRendererStateChangeListenerStub::SetCallback(const weak_ptr<AudioRendererStateChangeCallback> &callback)
{
    AUDIO_DEBUG_LOG("AudioRendererStateChangeListenerStub SetCallback");
    callback_ = callback;
}
} // namespace AudioStandard
} // namespace OHOS
