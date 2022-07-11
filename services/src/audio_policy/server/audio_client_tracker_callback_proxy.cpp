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
#include "audio_log.h"
#include "audio_client_tracker_callback_proxy.h"

namespace OHOS {
namespace AudioStandard {
AudioClientTrackerCallbackProxy::AudioClientTrackerCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardClientTracker>(impl) { }

void AudioClientTrackerCallbackProxy::PausedOrRecoveryStreamImpl(
    const StreamSetStateEventInternal &streamSetStateEventInternal)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioClientTrackerCallbackProxy: WriteInterfaceToken failed");
        return;
    }

    data.WriteInt32(static_cast<int32_t>(streamSetStateEventInternal.streamSetState));
    data.WriteInt32(static_cast<int32_t>(streamSetStateEventInternal.audioStreamType));

    int error = Remote()->SendRequest(PAUSEDORRECOVERYSTREAM, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("PAUSEDORRECOVERYSTREAM failed, error: %{public}d", error);
    }
}


ClientTrackerCallbackListener::ClientTrackerCallbackListener(const sptr<IStandardClientTracker> &listener)
    : listener_(listener)
{
    AUDIO_DEBUG_LOG("ClientTrackerCallbackListener");
}

ClientTrackerCallbackListener::~ClientTrackerCallbackListener()
{
    AUDIO_DEBUG_LOG("ClientTrackerCallbackListener destructor");
}

void ClientTrackerCallbackListener::PausedOrRecoveryStreamImpl(
    const StreamSetStateEventInternal &streamSetStateEventInternal)
{
    if (listener_ != nullptr) {
        listener_->PausedOrRecoveryStreamImpl(streamSetStateEventInternal);
    }
}
} // namespace AudioStandard
} // namespace OHOS
