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

#include "renderer_in_server.h"
#include <cinttypes>
#include "securec.h"
#include "audio_errors.h"
#include "audio_log.h"
#include "i_stream_manager.h"

namespace OHOS {
namespace AudioStandard {
namespace {
    static constexpr int32_t VOLUME_SHIFT_NUMBER = 16; // 1 >> 16 = 65536, max volume
}

RendererInServer::RendererInServer(AudioStreamParams params, AudioStreamType audioType)
{
    audioStreamParams_ = params;
    audioType_ = audioType;
    int32_t ret = IStreamManager::GetPlaybackManager().CreateRender(params, audioType, stream_);
    AUDIO_INFO_LOG("Construct rendererInServer result: %{public}d", ret);
    streamIndex_ = stream_->GetStreamIndex();
    ConfigServerBuffer();
}

inline uint32_t PcmFormatToBits(uint8_t format)
{
    switch (format) {
        case SAMPLE_U8:
            return 1; // 1 byte
        case SAMPLE_S16LE:
            return 2; // 2 byte
        case SAMPLE_S24LE:
            return 3; // 3 byte
        case SAMPLE_S32LE:
            return 4; // 4 byte
        case SAMPLE_F32LE:
            return 4; // 4 byte
        default:
            return 2; // 2 byte
    }
}

int32_t RendererInServer::ConfigServerBuffer()
{
    spanSizeInFrame_ = 20 * audioStreamParams_.samplingRate / 1000; // 20ms per span
    totalSizeInFrame_ = 4 * spanSizeInFrame_;

    if (audioServerBuffer_ != nullptr) {
        AUDIO_INFO_LOG("ConfigProcessBuffer: process buffer already configed!");
        return SUCCESS;
    }
    if (totalSizeInFrame_ == 0 || spanSizeInFrame_ == 0 || totalSizeInFrame_ % spanSizeInFrame_ != 0) {
        AUDIO_ERR_LOG("ConfigProcessBuffer: ERR_INVALID_PARAM");
        return ERR_INVALID_PARAM;
    }

    uint8_t channel = audioStreamParams_.channels;
    uint32_t formatbyte = PcmFormatToBits(audioStreamParams_.format);
    byteSizePerFrame_ = channel * formatbyte;
    AUDIO_INFO_LOG("ConfigProcessBuffer: totalSizeInFrame_: %{public}u, spanSizeInFrame_: %{public}u, byteSizePerFrame_:%{public}u, channel: %{public}u, formatbyte:%{public}u",
        totalSizeInFrame_, spanSizeInFrame_, byteSizePerFrame_, channel, formatbyte);
    
    // create OHAudioBuffer in server
    audioServerBuffer_ = OHAudioBuffer::CreateFormLocal(totalSizeInFrame_, spanSizeInFrame_, byteSizePerFrame_);
    CHECK_AND_RETURN_RET_LOG(audioServerBuffer_ != nullptr, ERR_OPERATION_FAILED, "Create oh audio buffer failed");

    // we need to clear data buffer to avoid dirty data.
    memset_s(audioServerBuffer_->GetDataBase(), audioServerBuffer_->GetDataSize(), 0, audioServerBuffer_->GetDataSize());
    int32_t ret = InitBufferStatus();
    AUDIO_DEBUG_LOG("Clear data buffer, ret:%{public}d", ret);

    isBufferConfiged_ = true;
    isInited_ = true;
    return SUCCESS;
}

int32_t RendererInServer::InitBufferStatus()
{
    if (audioServerBuffer_ == nullptr) {
        AUDIO_ERR_LOG("InitBufferStatus failed, null buffer.");
        return ERR_ILLEGAL_STATE;
    }

    uint32_t spanCount = audioServerBuffer_->GetSpanCount();
    AUDIO_INFO_LOG("InitBufferStatus: spanCount %{public}u", spanCount);
    for (uint32_t i = 0; i < spanCount; i++) {
        SpanInfo *spanInfo = audioServerBuffer_->GetSpanInfoByIndex(i);
        if (spanInfo == nullptr) {
            AUDIO_ERR_LOG("InitBufferStatus failed, null spaninfo");
            return ERR_ILLEGAL_STATE;
        }
        spanInfo->spanStatus = SPAN_READ_DONE;
        spanInfo->offsetInFrame = 0;

        spanInfo->readStartTime = 0;
        spanInfo->readDoneTime = 0;

        spanInfo->writeStartTime = 0;
        spanInfo->writeDoneTime = 0;

        spanInfo->volumeStart = 1 << VOLUME_SHIFT_NUMBER; // 65536 for initialize
        spanInfo->volumeEnd = 1 << VOLUME_SHIFT_NUMBER; // 65536 for initialize
        spanInfo->isMute = false;
    }
    return SUCCESS;
}

void RendererInServer::RegisterStatusCallback()
{
    AUDIO_INFO_LOG("RendererInServer::RegisterStatusCallback");
    stream_->RegisterStatusCallback(shared_from_this());
}

void RendererInServer::RegisterWriteCallback()
{
    AUDIO_INFO_LOG("RendererInServer::RegisterWriteCallback");
    stream_->RegisterWriteCallback(shared_from_this());
}

void RendererInServer::OnStatusUpdate(IOperation operation)
{
    AUDIO_INFO_LOG("RendererInServer::OnStatusUpdate operation: %{public}d", operation);
    operation_ = operation;
    std::lock_guard<std::mutex> lock(statusLock_);
    if (status_ == I_STATUS_RELEASED) {
        AUDIO_WARNING_LOG("Stream already released");
        return;
    }
    std::shared_ptr<RendererListener> callback = testCallback_.lock();
    switch (operation) {
        case OPERATION_UNDERRUN:
            AUDIO_INFO_LOG("Underrun: audioServerBuffer_->GetAvailableDataFrames(): %{public}d",audioServerBuffer_->GetAvailableDataFrames());
            if (audioServerBuffer_->GetAvailableDataFrames() == 4 * spanSizeInFrame_) {
                AUDIO_INFO_LOG("Buffer is empty");
                needStart = 0;

            } else {
                AUDIO_INFO_LOG("Buffer is not empty");
                WriteData();
            }
            callback->CancelBufferFromBlock();
            break;
        case OPERATION_STARTED:
            status_ = I_STATUS_STARTED;
            break;
        case OPERATION_PAUSED:
            status_ = I_STATUS_PAUSED;
            break;
        case OPERATION_STOPPED:
            status_ = I_STATUS_STOPPED;
            break;
        case OPERATION_FLUSHED:
            if (status_ == I_STATUS_FLUSHING_WHEN_STARTED) {
                status_ = I_STATUS_STARTED;
            } else if (status_ == I_STATUS_FLUSHING_WHEN_PAUSED) {
                status_ = I_STATUS_PAUSED;
            } else {
                AUDIO_WARNING_LOG("Invalid status before flusing");
            }
            break;
        case OPERATION_DRAINED:
            status_ = I_STATUS_DRAINED;
            afterDrain = true;
            if (callback != nullptr) {
                callback->OnDrainDone();
            }
            break;
        case OPERATION_RELEASED:
            status_ = I_STATUS_RELEASED;
            break;
        default:
            AUDIO_INFO_LOG("Invalid operation %{public}u", operation);
            status_ = I_STATUS_INVALID;
    }
}

BufferDesc RendererInServer::DequeueBuffer(size_t length)
{
    return stream_->DequeueBuffer(length);
}

void RendererInServer::WriteData()
{
    uint64_t currentReadFrame = audioServerBuffer_->GetCurReadFrame();
    uint64_t currentWriteFrame = audioServerBuffer_->GetCurWriteFrame();
    if (currentReadFrame + spanSizeInFrame_ > currentWriteFrame) {
        AUDIO_INFO_LOG("Underrun!!!");
        return;
    }
    BufferDesc bufferDesc = {nullptr, totalSizeInFrame_, totalSizeInFrame_};

    if (audioServerBuffer_->GetReadbuffer(currentReadFrame, bufferDesc) == 0) {
        AUDIO_INFO_LOG("Buffer length: %{public}zu", bufferDesc.bufLength);
        stream_->EnqueueBuffer(bufferDesc);
        uint64_t nextReadFrame = currentReadFrame + spanSizeInFrame_;
        AUDIO_INFO_LOG("RendererInServer::WriteData: CurrentReadFrame: %{public}" PRIu64 ", nextReadFrame:%{public}" PRIu64 "", currentReadFrame, nextReadFrame);
        audioServerBuffer_->SetCurReadFrame(nextReadFrame);
    }
}

void RendererInServer::WriteEmptyData()
{
    AUDIO_WARNING_LOG("Underrun, write empty data");
    BufferDesc bufferDesc = stream_->DequeueBuffer(totalSizeInFrame_);
    memset_s(bufferDesc.buffer, bufferDesc.bufLength, 0, bufferDesc.bufLength);
    stream_->EnqueueBuffer(bufferDesc);
    return;
}


int32_t RendererInServer::OnWriteData(size_t length)
{
    AUDIO_INFO_LOG("RendererInServer::OnWriteData");
    std::shared_ptr<RendererListener> callback = testCallback_.lock();
    if (callback == nullptr) {
        AUDIO_ERR_LOG("Callback from test is nullptr");
        return -1;
    }
    for (int32_t i = 0; i < length / totalSizeInFrame_; i++) {
        WriteData();

    }
    callback->CancelBufferFromBlock();
    return PA_ADAPTER_SUCCESS;
}

void RendererInServer::UpdateWriteIndex()
{
    AUDIO_INFO_LOG("UpdateWriteIndex: audioServerBuffer_->GetAvailableDataFrames(): %{public}d, needStart: %{public}d", audioServerBuffer_->GetAvailableDataFrames(), needStart);
    if (audioServerBuffer_->GetAvailableDataFrames() == spanSizeInFrame_ && needStart < 3) {
        AUDIO_WARNING_LOG("Start write data");
        WriteData();
        needStart++;
    }
    if (afterDrain == true) {
        afterDrain = false;
        AUDIO_WARNING_LOG("After drain, start write data");
        WriteData();
    }
}

int32_t RendererInServer::Start()
{
    needStart = 0;
    std::unique_lock<std::mutex> lock(statusLock_);
    if (status_ != I_STATUS_IDLE && status_ != I_STATUS_PAUSED && status_ != I_STATUS_STOPPED) {
        AUDIO_ERR_LOG("RendererInServer::Start failed, Illegal state: %{public}u", status_);
        return -1;
    }
    status_ = I_STATUS_STARTING;
    int ret = stream_->Start();
    if (ret < 0) {
        AUDIO_ERR_LOG("Start stream failed, reason: %{public}d", ret);
        IStreamManager::GetPlaybackManager().ReleaseRender(streamIndex_);
        status_ = I_STATUS_INVALID;
        return ret;
    }
    return PA_ADAPTER_SUCCESS;
}

int32_t RendererInServer::Pause()
{
    std::unique_lock<std::mutex> lock(statusLock_);
    if (status_ != I_STATUS_STARTED) {
        AUDIO_ERR_LOG("RendererInServer::Pause failed, Illegal state: %{public}u", status_);
        return -1;
    }
    status_ = I_STATUS_PAUSING;
    int ret = stream_->Pause();
    if (ret < 0) {
        AUDIO_ERR_LOG("Pause stream failed, reason: %{public}d", ret);
        IStreamManager::GetPlaybackManager().ReleaseRender(streamIndex_);
        status_ = I_STATUS_INVALID;
        return ret;
    }
    return PA_ADAPTER_SUCCESS;
}

int32_t RendererInServer::Flush()
{
    std::unique_lock<std::mutex> lock(statusLock_);
    if (status_ != I_STATUS_STARTED) {
        status_ = I_STATUS_FLUSHING_WHEN_STARTED;
    } else if (status_ != I_STATUS_PAUSED) {
        status_ = I_STATUS_FLUSHING_WHEN_PAUSED;
    } else {
        AUDIO_ERR_LOG("RendererInServer::Flush failed, Illegal state: %{public}u", status_);
        return -1;
    }

    // Flush buffer of audio server
    uint64_t writeFrame = audioServerBuffer_->GetCurWriteFrame();
    uint64_t readFrame = audioServerBuffer_->GetCurReadFrame();

    while (readFrame < writeFrame) {
        BufferDesc bufferDesc = {nullptr, 0, 0};
        int32_t readResult = audioServerBuffer_->GetReadbuffer(readFrame, bufferDesc);
        if (readResult != 0) {
            return -1;
        }
        memset_s(bufferDesc.buffer, bufferDesc.bufLength, 0, bufferDesc.bufLength);
        readFrame += spanSizeInFrame_;
        AUDIO_INFO_LOG("On flush, read frame: %{public}" PRIu64 ", nextReadFrame: %{public}u, writeFrame: %{public}" PRIu64 "", readFrame, spanSizeInFrame_, writeFrame);
        audioServerBuffer_->SetCurReadFrame(readFrame);
    }

    int ret = stream_->Flush();
    if (ret < 0) {
        AUDIO_ERR_LOG("Flush stream failed, reason: %{public}d", ret);
        IStreamManager::GetPlaybackManager().ReleaseRender(streamIndex_);
        status_ = I_STATUS_INVALID;
        return ret;
    }
    return PA_ADAPTER_SUCCESS;
}

int32_t RendererInServer::DrainAudioBuffer()
{
    return PA_ADAPTER_SUCCESS;
}

int32_t RendererInServer::SendOneFrame()
{
    OnWriteData(100000);
    return PA_ADAPTER_SUCCESS;
}

int32_t RendererInServer::GetInfo()
{
    IStreamManager::GetPlaybackManager().GetInfo();
    return 0;
}

int32_t RendererInServer::Drain()
{
    std::unique_lock<std::mutex> lock(statusLock_);
    if (status_ != I_STATUS_STARTED) {
        AUDIO_ERR_LOG("RendererInServer::Drain failed, Illegal state: %{public}u", status_);
        return -1;
    }
    status_ = I_STATUS_DRAINING;
    AUDIO_INFO_LOG("Start drain");
    DrainAudioBuffer();
    int ret = stream_->Drain();
    if (ret < 0) {
        AUDIO_ERR_LOG("Drain stream failed, reason: %{public}d", ret);
        IStreamManager::GetPlaybackManager().ReleaseRender(streamIndex_);
        status_ = I_STATUS_INVALID;
        return ret;
    }
    return PA_ADAPTER_SUCCESS;
}

int32_t RendererInServer::Stop()
{
    std::unique_lock<std::mutex> lock(statusLock_);
    if (status_ != I_STATUS_STARTED && status_ != I_STATUS_PAUSED) {
        AUDIO_ERR_LOG("RendererInServer::Stop failed, Illegal state: %{public}u", status_);
        return -1;
    }
    status_ = I_STATUS_STOPPING;
    int ret = stream_->Stop();
    if (ret < 0) {
        AUDIO_ERR_LOG("Stop stream failed, reason: %{public}d", ret);
        IStreamManager::GetPlaybackManager().ReleaseRender(streamIndex_);
        status_ = I_STATUS_INVALID;
        return ret;
    }
    return PA_ADAPTER_SUCCESS;
}

int32_t RendererInServer::Release()
{
    std::unique_lock<std::mutex> lock(statusLock_);
    status_ = I_STATUS_RELEASED;
    int32_t ret = IStreamManager::GetPlaybackManager().ReleaseRender(streamIndex_);
    
    stream_ = nullptr;
    if (ret < 0) {
        AUDIO_ERR_LOG("Release stream failed, reason: %{public}d", ret);
        status_ = I_STATUS_INVALID;
        return ret;
    }

    return PA_ADAPTER_SUCCESS;
}

void RendererInServer::RegisterTestCallback(const std::weak_ptr<RendererListener> &callback)
{
    testCallback_ = callback;
}

int32_t RendererInServer::WriteOneFrame()
{
    // if buffer ready, get buffer, else, return
    size_t minBufferSize = 0;
    if (stream_->GetMinimumBufferSize(minBufferSize) < 0) {
        AUDIO_ERR_LOG("Get min buffer size err");
        return -1;
    }
    BufferDesc bufferDesc = stream_->DequeueBuffer(minBufferSize);
    stream_->EnqueueBuffer(bufferDesc);
    return 0;
}

int32_t RendererInServer::AbortOneCallback()
{
    std::lock_guard<std::mutex> lock(statusLock_);
    stream_->AbortCallback(1);
    return 0;
}

int32_t RendererInServer::AbortAllCallback()
{
    std::lock_guard<std::mutex> lock(statusLock_);
    stream_->AbortCallback(5);
    return 0;
}

std::shared_ptr<OHAudioBuffer> RendererInServer::GetOHSharedBuffer()
{
    return audioServerBuffer_;
}
} // namespace AudioStandard
} // namespace OHOS
