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

#ifndef AUDIO_PROCESS_IN_CLIENT_H
#define AUDIO_PROCESS_IN_CLIENT_H

#include <map>
#include <memory>

#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {
class AudioDataCallback {
public:
    virtual ~AudioDataCallback() = default;

    /**
     * Called when request handle data.
     *
     * @param length Indicates requested buffer length.
     */
    virtual void OnHandleData(size_t length) = 0;
};

class ClientUnderrunCallBack {
    virtual ~ClientUnderrunCallBack() = default;

    /**
     * Callback function when underrun occurs.
     *
     * @param posInFrames Indicates the postion when client handle underrun in frames.
     */
    virtual void OnUnderrun(size_t posInFrames) = 0;
};

class AudioProcessInClient {
public:
    static constexpr int32_t PROCESS_VOLUME_MAX = 1 << 16; // 0 ~ 65536
    static std::shared_ptr<AudioProcessInClient> Create(const AudioProcessConfig &config);

    virtual ~AudioProcessInClient() = default;

    virtual int32_t SaveDataCallback(const std::shared_ptr<AudioDataCallback> &dataCallback) = 0;

    virtual int32_t SaveUnderrunCallback(const std::shared_ptr<ClientUnderrunCallBack> &underrunCallback) = 0;

    virtual int32_t GetBufferDesc(BufferDesc &bufDesc) const = 0;

    virtual int32_t Enqueue(const BufferDesc &bufDesc) const = 0;

    virtual int32_t SetVolume(int32_t vol) = 0;

    virtual int32_t Start() = 0;

    virtual int32_t Pause(bool isFlush = false) = 0;

    virtual int32_t Resume() = 0;

    virtual int32_t Stop() = 0;

    virtual int32_t Release() = 0;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_PROCESS_IN_CLIENT_H
