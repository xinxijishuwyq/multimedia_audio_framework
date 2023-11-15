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

#ifndef CAPTURER_IN_SERVER_H
#define CAPTURER_IN_SERVER_H

#include <mutex>
#include "i_capturer_stream.h"
#include "i_stream_listener.h"
#include "oh_audio_buffer.h"

namespace OHOS {
namespace AudioStandard {
class CapturerListener {
public:
    virtual void OnReadEvent() = 0;
    virtual void ReceivedBuffer() = 0;
};
class CapturerInServer : public IStatusCallback, public IReadCallback,
    public std::enable_shared_from_this<CapturerInServer> {
public:
    // LYH in plan: add IStreamListener
    CapturerInServer(AudioStreamParams params, AudioStreamType audioType);
    virtual ~CapturerInServer() {};
    void OnStatusUpdate(IOperation operation) override;
    int32_t OnReadData(size_t length) override;

    int32_t GetSessionId(uint32_t &sessionId);
    int32_t Start();
    int32_t Pause();
    int32_t Flush();
    int32_t Stop();
    int32_t Release();

    void RegisterStatusCallback();
    void RegisterReadCallback();
    void RegisterTestCallback(const std::weak_ptr<CapturerListener> &callback);

    int32_t ConfigServerBuffer();
    int32_t InitBufferStatus();
    void UpdateReadIndex();
    BufferDesc DequeueBuffer(size_t length);
    void ReadData(size_t length);
    int32_t DrainAudioBuffer();
    int32_t ReadOneFrame();
    int32_t AbortOneCallback();
    int32_t AbortAllCallback();
    std::shared_ptr<OHAudioBuffer> GetOHSharedBuffer();

private:
    std::mutex statusLock_;
    std::condition_variable statusCv_;
    std::shared_ptr<ICapturerStream> stream_ = nullptr;
    uint32_t streamIndex_ = -1;
    IOperation operation_ = OPERATION_INVALID;
    IStatus status_ = I_STATUS_IDLE;

    // LYH in plan: IStreamListener
    std::weak_ptr<IStreamListener> streamListener_;
    std::weak_ptr<CapturerListener> testCallback_;
    AudioStreamParams audioStreamParams_;
    AudioStreamType audioType_;
    uint32_t totalSizeInFrame_ = 0;
    uint32_t spanSizeInFrame_ = 0;
    uint32_t byteSizePerFrame_ = 0;
    bool isBufferConfiged_  = false;
    std::atomic<bool> isInited_ = false;
    std::shared_ptr<OHAudioBuffer> audioServerBuffer_ = nullptr;
    int32_t needStart = 0;
    int32_t underflowCount = 0;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // CAPTURER_IN_SERVER_H
