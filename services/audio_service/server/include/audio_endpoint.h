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

#ifndef AUDIO_ENDPOINT_H
#define AUDIO_ENDPOINT_H

#include <sstream>
#include <memory>

#include "i_process_status_listener.h"

namespace OHOS {
namespace AudioStandard {
// When AudioEndpoint is offline, notify the owner.
class IAudioEndpointStatusListener {
public:
    enum HdiDeviceStatus : uint32_t {
        STATUS_ONLINE = 0,
        STATUS_OFFLINE,
        STATUS_INVALID,
    };

    /**
     * When AudioEndpoint changed status, we need to notify AudioProcessStream.
    */
    virtual int32_t OnEndpointStatusChange(HdiDeviceStatus status) = 0;
};

class AudioEndpoint : public IProcessStatusListener {
public:
    static constexpr int32_t MAX_LINKED_PROCESS = 10; // 10
    enum EndpointType : uint32_t {
        TYPE_MMAP = 0,
        TYPE_INVALID
    };

    enum EndpointStatus : uint32_t {
        INVALID = 0,
        UNLINKED, // no process linked
        IDEL,     // no running process
        STARTING, // calling start sink
        RUNNING,  // at least one process is running
        STOPPING, // calling stop sink
        STOPPED   // sink stoped
    };

    static std::shared_ptr<AudioEndpoint> GetInstance(EndpointType type, const DeviceInfo &deviceInfo);

    virtual std::string GetEndpointName() = 0;

    virtual EndpointStatus GetStatus() = 0;

    virtual void Release() = 0;

    virtual int32_t LinkProcessStream(IAudioProcessStream *processStream) = 0;
    virtual int32_t UnlinkProcessStream(IAudioProcessStream *processStream) = 0;

    virtual int32_t GetPreferBufferInfo(uint32_t &totalSizeInframe, uint32_t &spanSizeInframe) = 0;

    virtual void Dump(std::stringstream &dumpStringStream) = 0;

    virtual ~AudioEndpoint() = default;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_ENDPOINT_H
