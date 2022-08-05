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

#include "audio_manager_listener_stub.h"
#include "audio_log.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
AudioManagerListenerStub::AudioManagerListenerStub()
{
}
AudioManagerListenerStub::~AudioManagerListenerStub()
{
}
int AudioManagerListenerStub::OnRemoteRequest(
    uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    AUDIO_DEBUG_LOG("OnRemoteRequest, cmd = %{public}u", code);
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        AUDIO_ERR_LOG("AudioManagerStub: ReadInterfaceToken failed");
        return -1;
    }

    switch (code) {
        case ON_PARAMETER_CHANGED: {
            AUDIO_DEBUG_LOG("ON_PARAMETER_CHANGED AudioManagerStub");
            string networkId = data.ReadString();
            AudioParamKey key = static_cast<AudioParamKey>(data.ReadInt32());
            string condition = data.ReadString();
            string value = data.ReadString();
            OnAudioParameterChange(networkId, key, condition, value);
            return AUDIO_OK;
        }
        default: {
            AUDIO_ERR_LOG("default case, need check AudioManagerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void AudioManagerListenerStub::SetParameterCallback(const std::weak_ptr<AudioParameterCallback>& callback)
{
    callback_ = callback;
}

void AudioManagerListenerStub::OnAudioParameterChange(const std::string networkId, const AudioParamKey key,
    const std::string& condition, const std::string& value)
{
    std::shared_ptr<AudioParameterCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnAudioParameterChange(networkId, key, condition, value);
    } else {
        AUDIO_ERR_LOG("AudioRingerModeUpdateListenerStub: callback_ is nullptr");
    }
}
} // namespace AudioStandard
} // namespace OHOS
