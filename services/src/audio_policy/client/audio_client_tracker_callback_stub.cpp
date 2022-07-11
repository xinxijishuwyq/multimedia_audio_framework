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

#include "audio_client_tracker_callback_stub.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
AudioClientTrackerCallbackStub::AudioClientTrackerCallbackStub()
{
}

AudioClientTrackerCallbackStub::~AudioClientTrackerCallbackStub()
{
}

int AudioClientTrackerCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    AUDIO_DEBUG_LOG("AudioClientTrackerCallbackStub::OnRemoteRequest");
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        AUDIO_ERR_LOG("AudioClientTrackerCallbackStub: ReadInterfaceToken failed");
        return -1;
    }

    switch (code) {
        case PAUSEDORRECOVERYSTREAM: {
            StreamSetStateEventInternal sreamSetStateEventInternal = {};
            sreamSetStateEventInternal.streamSetState= static_cast<StreamSetState>(data.ReadInt32());
            sreamSetStateEventInternal.audioStreamType = static_cast<AudioStreamType>(data.ReadInt32());
            PausedOrRecoveryStreamImpl(sreamSetStateEventInternal);
            return AUDIO_OK;
        }
        default: {
            AUDIO_ERR_LOG("default case, need check AudioListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }

    return 0;
}

void AudioClientTrackerCallbackStub::SetClientTrackerCallback(
    const std::weak_ptr<AudioClientTracker> &callback)
{
    AUDIO_DEBUG_LOG("AudioClientTrackerCallbackStub::SetClientTrackerCallback");
    callback_ = callback;
}

void AudioClientTrackerCallbackStub::PausedOrRecoveryStreamImpl(
    const StreamSetStateEventInternal &streamSetStateEventInternal)
{
    AUDIO_DEBUG_LOG("AudioClientTrackerCallbackStub PausedOrRecoveryStreamImpl start");
    std::shared_ptr<AudioClientTracker> cb = callback_.lock();
    if (cb != nullptr) {
        cb->PausedOrRecoveryStreamImpl(streamSetStateEventInternal);
    } else {
        AUDIO_ERR_LOG("AudioClientTrackerCallbackStub: callback_ is nullptr");
    }
}
} // namespace AudioStandard
} // namespace OHOS
