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
#define LOG_TAG "AudioRendererWriteCallbackStub"

#include "audio_renderer_write_callback_stub.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
AudioRendererWriteCallbackStub::AudioRendererWriteCallbackStub()
{
}

AudioRendererWriteCallbackStub::~AudioRendererWriteCallbackStub()
{
}

int AudioRendererWriteCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    CHECK_AND_RETURN_RET_LOG(data.ReadInterfaceToken() == GetDescriptor(), -1, "ReadInterfaceToken failed");
    switch (code) {
        case ON_WRITE_DATA: {
            size_t length = static_cast<size_t>(data.ReadInt64());
            OnWriteData(length);
            reply.WriteInt32(AUDIO_OK);
            break;
        }
        default: {
            reply.WriteInt32(AUDIO_INVALID_PARAM);
            break;
        }
    }
    return AUDIO_OK;
}

void AudioRendererWriteCallbackStub::OnWriteData(size_t length)
{
    std::shared_ptr<AudioRendererWriteCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnWriteData(length);
    }
}

void AudioRendererWriteCallbackStub::SetOnRendererWriteCallback(
    const std::weak_ptr<AudioRendererWriteCallback> &callback)
{
    callback_ = callback;
}
} // namespace AudioStandard
} // namespace OHOS