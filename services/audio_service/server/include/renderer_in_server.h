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

#ifndef RENDERER_IN_SERVER_H
#define RENDERER_IN_SERVER_H

#include <mutex>
#include "i_stream.h"
#include "i_renderer_stream.h"
#include "oh_audio_buffer.h"

namespace OHOS {
namespace AudioStandard {
class RendererListener {
public:
    virtual void OnWriteEvent(BufferDesc& bufferesc) = 0;
    virtual void CancelBufferFromBlock() = 0;
    virtual void CallForImmediatelyWrite() = 0;
    virtual void OnDrainDone() = 0;
};

class RendererInServer : public IStatusCallback, public IWriteCallback,
    public std::enable_shared_from_this<RendererInServer> {
public:
    RendererInServer(AudioStreamParams params, AudioStreamType audioType);
    virtual ~RendererInServer() {};
    void OnStatusUpdate(IOperation operation) override;
    int32_t OnWriteData(size_t length) override;
    
    int32_t Start();
    int32_t Pause();
    int32_t Flush();
    int32_t Drain();
    int32_t Stop();
    int32_t Release();

    void RegisterStatusCallback();
    void RegisterWriteCallback();
    void RegisterTestCallback(const std::weak_ptr<RendererListener> &callback);
    int32_t ConfigServerBuffer();
    int32_t InitBufferStatus();
    void UpdateWriteIndex();
    BufferDesc DequeueBuffer(size_t length);
    void WriteData();
    void WriteEmptyData();
    int32_t DrainAudioBuffer();
    int32_t SendOneFrame();
    int32_t GetInfo();
    int32_t WriteOneFrame();
    int32_t AbortOneCallback();
    int32_t AbortAllCallback();
    std::shared_ptr<OHAudioBuffer> GetOHSharedBuffer();

private:
    std::mutex statusLock_;
    std::condition_variable statusCv_;
    std::shared_ptr<IRendererStream> stream_ = nullptr;
    uint32_t streamIndex_ = -1;
    IOperation operation_ = OPERATION_INVALID;
    IStatus status_ = I_STATUS_IDLE;

    std::weak_ptr<RendererListener> testCallback_;
    AudioStreamParams audioStreamParams_;
    AudioStreamType audioType_;
    uint32_t totalSizeInFrame_ = 0;
    uint32_t spanSizeInFrame_ = 0;
    uint32_t byteSizePerFrame_ = 0;
    bool isBufferConfiged_  = false;
    std::atomic<bool> isInited_ = false;
    std::shared_ptr<OHAudioBuffer> audioServerBuffer_ = nullptr;
    int32_t needStart = 0;
    bool afterDrain = false;
    std::mutex updateIndexLock_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // RENDERER_IN_SERVER_H
