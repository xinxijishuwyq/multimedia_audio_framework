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

#include "audio_process_stub.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
bool AudioProcessStub::CheckInterfaceToken(MessageParcel &data)
{
    static auto locaDescriptor = IAudioProcess::GetDescriptor();
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (remoteDescriptor != remoteDescriptor) {
        AUDIO_ERR_LOG("CheckInterFfaceToken failed.");
        return false;
    }
    return true;
}

int AudioProcessStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (!CheckInterfaceToken(data)) {
        return AUDIO_ERR;
    }
    if (code >= IAudioProcessMsg::PROCESS_MAX_MSG) {
        AUDIO_WARNING_LOG("OnRemoteRequest unsupported request code:%{public}d.", code);
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    return (this->*funcList_[code])(data, reply);
}

int32_t AudioProcessStub::HandleResolveBuffer(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    std::shared_ptr<OHAudioBuffer> buffer;
    int32_t ret = ResolveBuffer(buffer);
    reply.WriteInt32(ret);
    if (ret == AUDIO_OK && buffer != nullptr) {
        OHAudioBuffer::WriteToParcel(buffer, reply);
    } else {
        AUDIO_ERR_LOG("error: ResolveBuffer failed.");
        return AUDIO_INVALID_PARAM;
    }

    return AUDIO_OK;
}

int32_t AudioProcessStub::HandleStart(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Start());
    return AUDIO_OK;
}

int32_t AudioProcessStub::HandleResume(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Resume());
    return AUDIO_OK;
}

int32_t AudioProcessStub::HandlePause(MessageParcel &data, MessageParcel &reply)
{
    bool isFlush = data.ReadBool();
    reply.WriteInt32(Pause(isFlush));
    return AUDIO_OK;
}

int32_t AudioProcessStub::HandleStop(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Stop());
    return AUDIO_OK;
}

int32_t AudioProcessStub::HandleRequestHandleInfo(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(RequestHandleInfo());
    return AUDIO_OK;
}

int32_t AudioProcessStub::HandleRelease(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Release());
    return AUDIO_OK;
}
} // namespace AudioStandard
} // namespace OHOS
