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

#ifndef IPC_STREAM_H
#define IPC_STREAM_H

#include <memory>

#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"

#include "audio_info.h"
#include "audio_process_config.h"
#include "oh_audio_buffer.h"

namespace OHOS {
namespace AudioStandard {
class IpcStream : public IRemoteBroker {
public:
    virtual ~IpcStream() = default;

    virtual int32_t RegisterStreamListener(sptr<IRemoteObject> object) = 0;

    virtual int32_t ResolveBuffer(std::shared_ptr<OHAudioBuffer> &buffer) = 0;

    virtual int32_t UpdatePosition() = 0;

    virtual int32_t Start() = 0;

    virtual int32_t Pause() = 0;

    virtual int32_t Stop() = 0;

    virtual int32_t Release() = 0;

    virtual int32_t Flush() = 0;

    virtual int32_t Drain() = 0;

    virtual int32_t GetAudioTime(uint64_t &framePos, int64_t &timeStamp) = 0;

    virtual int32_t GetLatency(uint64_t &latency) = 0;

    virtual int32_t SetRate(int32_t rate) = 0; // SetRenderRate

    virtual int32_t GetRate(int32_t &rate) = 0; // SetRenderRate

    virtual int32_t SetLowPowerVolume(float volume) = 0; // renderer only

    virtual int32_t GetLowPowerVolume(float &volume) = 0; // renderer only

    virtual int32_t SetAudioEffectMode(int32_t effectMode) = 0; // renderer only

    virtual int32_t GetAudioEffectMode(int32_t &effectMode) = 0; // renderer only

    virtual int32_t SetPrivacyType(int32_t privacyType) = 0; // renderer only

    virtual int32_t GetPrivacyType(int32_t &privacyType) = 0; // renderer only

    // IPC code.
    enum IpcStreamMsg : uint32_t {
        ON_REGISTER_STREAM_LISTENER = 0,
        ON_RESOLVE_BUFFER,
        ON_UPDATE_POSITION,
        ON_START,
        ON_PAUSE,
        ON_STOP,
        ON_RELEASE,
        ON_FLUSH,
        ON_DRAIN,
        OH_GET_AUDIO_TIME,
        ON_GET_LATENCY,
        ON_SET_RATE,
        ON_GET_RATE,
        ON_SET_LOWPOWER_VOLUME,
        ON_GET_LOWPOWER_VOLUME,
        ON_SET_EFFECT_MODE,
        ON_GET_EFFECT_MODE,
        ON_SET_PRIVACY_TYPE,
        ON_GET_PRIVACY_TYPE,
        IPC_STREAM_MAX_MSG
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IpcStream");
};

class IpcStreamListener : public IRemoteBroker {
public:
    virtual ~IpcStreamListener() = default;

    virtual int32_t OnOperationHandled(int32_t operation, int64_t result) = 0;

    // IPC code.
    enum IpcStreamListenerMsg : uint32_t {
        ON_OPERATION_HANDLED = 0,
        IPC_STREAM_LISTENER_MAX_MSG
    };
    DECLARE_INTERFACE_DESCRIPTOR(u"IpcStreamListener");
};
} // namespace AudioStandard
} // namespace OHOS
#endif // IPC_STREAM_H
