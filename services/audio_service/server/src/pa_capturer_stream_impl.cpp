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

#include "pa_capturer_stream_impl.h"
#include "audio_errors.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
static int32_t CheckReturnIfStreamInvalid(pa_stream *paStream, const int32_t retVal)
{
    do {
        if (!(paStream && PA_STREAM_IS_GOOD(pa_stream_get_state(paStream)))) {
            return retVal;
        }
    } while (false);
    return SUCCESS;
}

PaCapturerStreamImpl::PaCapturerStreamImpl(pa_stream *paStream, AudioProcessConfig processConfig,
    pa_threaded_mainloop *mainloop)
{
    mainloop_ = mainloop;
    paStream_ = paStream;
    processConfig_ = processConfig;

    pa_stream_set_moved_callback(paStream, PAStreamMovedCb, (void *)this); // used to notify sink/source moved
    pa_stream_set_read_callback(paStream, PAStreamReadCb, (void *)this);
    pa_stream_set_underflow_callback(paStream, PAStreamUnderFlowCb, (void *)this);
    pa_stream_set_started_callback(paStream, PAStreamSetStartedCb, (void *)this);

    InitParams();
}

inline uint32_t PcmFormatToBits(uint8_t format)
{
    switch (format) {
        case SAMPLE_U8:
            return 1; // 1 byte
        case SAMPLE_S16LE:
            return 2; // 2 byt
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

void PaCapturerStreamImpl::InitParams()
{
    if (CheckReturnIfStreamInvalid(paStream_, ERROR) < 0) {
        return;
    }

    // Get byte size per frame
    const pa_sample_spec *sampleSpec = pa_stream_get_sample_spec(paStream_);
    sampleSpec_->channels = sampleSpec->channels;
    sampleSpec_->format = sampleSpec->format;
    sampleSpec_->rate = sampleSpec->rate;
    if (sampleSpec->channels != processConfig_.streamInfo.channels) {
        AUDIO_WARNING_LOG("Unequal channels, in server: %{public}d, in client: %{public}d", sampleSpec->channels,
            processConfig_.streamInfo.channels);
    }
    if (static_cast<uint8_t>(sampleSpec->format) != processConfig_.streamInfo.format) {
        AUDIO_WARNING_LOG("Unequal format, in server: %{public}d, in client: %{public}d", sampleSpec->format,
            processConfig_.streamInfo.format);
    }
    uint32_t formatbyte = PcmFormatToBits(sampleSpec->format);
    byteSizePerFrame_ = sampleSpec->channels * formatbyte;

    // Get min buffer size in frame
    const pa_buffer_attr *bufferAttr = pa_stream_get_buffer_attr(paStream_);
    if (bufferAttr == nullptr) {
        AUDIO_ERR_LOG("pa_stream_get_buffer_attr returned nullptr");
        return;
    }
    minBufferSize_ = (size_t)bufferAttr->minreq;
    spanSizeInFrame_ = minBufferSize_ / byteSizePerFrame_;
}

int32_t PaCapturerStreamImpl::Start()
{
    AUDIO_DEBUG_LOG("Enter PaCapturerStreamImpl::Start");
    if (CheckReturnIfStreamInvalid(paStream_, ERROR) < 0) {
        return ERR_ILLEGAL_STATE;
    }
    pa_operation *operation = nullptr;
    pa_stream_state_t state = pa_stream_get_state(paStream_);
    if (state != PA_STREAM_READY) {
        return ERR_ILLEGAL_STATE;
    }

    streamCmdStatus_ = 0;
    operation = pa_stream_cork(paStream_, 0, PAStreamStartSuccessCb, (void *)this);
    pa_operation_unref(operation);
    return SUCCESS;
}

int32_t PaCapturerStreamImpl::Pause()
{
    AUDIO_INFO_LOG("Enter PaCapturerStreamImpl::Pause");
    PAStreamCorkSuccessCb = PAStreamPauseSuccessCb;
    return CorkStream();
}

int32_t PaCapturerStreamImpl::CorkStream()
{
    if (CheckReturnIfStreamInvalid(paStream_, ERROR) < 0) {
        return ERR_ILLEGAL_STATE;
    }
    pa_operation *operation = nullptr;

    pa_stream_state_t state = pa_stream_get_state(paStream_);
    if (state != PA_STREAM_READY) {
        AUDIO_ERR_LOG("Stream Stop Failed");
        return ERR_ILLEGAL_STATE;
    }
    operation = pa_stream_cork(paStream_, 1, PAStreamCorkSuccessCb, (void *)this);
    pa_operation_unref(operation);
    return SUCCESS;
}

int32_t PaCapturerStreamImpl::GetStreamFramesRead(uint64_t &framesRead)
{
    if (byteSizePerFrame_ == 0) {
        AUDIO_ERR_LOG("Error frame size");
        return ERR_OPERATION_FAILED;
    }
    framesRead = totalBytesRead_ / byteSizePerFrame_;
    return SUCCESS;
}

int32_t PaCapturerStreamImpl::GetCurrentTimeStamp(uint64_t &timeStamp)
{
    if (CheckReturnIfStreamInvalid(paStream_, ERROR) < 0) {
        return ERR_ILLEGAL_STATE;
    }
    pa_threaded_mainloop_lock(mainloop_);

    pa_operation *operation = pa_stream_update_timing_info(paStream_, NULL, NULL);
    if (operation != nullptr) {
        while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING) {
            pa_threaded_mainloop_wait(mainloop_);
        }
        pa_operation_unref(operation);
    } else {
        AUDIO_ERR_LOG("pa_stream_update_timing_info failed");
    }

    const pa_timing_info *info = pa_stream_get_timing_info(paStream_);
    if (info == nullptr) {
        AUDIO_ERR_LOG("pa_stream_get_timing_info failed");
        pa_threaded_mainloop_unlock(mainloop_);
        return ERR_OPERATION_FAILED;
    }

    if (pa_stream_get_time(paStream_, &timeStamp)) {
        AUDIO_ERR_LOG("GetCurrentTimeStamp failed for AUDIO_SERVICE_CLIENT_RECORD");
        pa_threaded_mainloop_unlock(mainloop_);
        return ERR_OPERATION_FAILED;
    }
    int32_t uid = static_cast<int32_t>(getuid());

    // 1013 is media_service's uid
    int32_t media_service = 1013;
    if (uid == media_service) {
        timeStamp = pa_bytes_to_usec(totalBytesRead_, sampleSpec_);
    }
    pa_threaded_mainloop_unlock(mainloop_);
    return SUCCESS;
}

int32_t PaCapturerStreamImpl::GetLatency(uint64_t &latency)
{
    // LYH in plan, 增加dataMutex的锁
    if (CheckReturnIfStreamInvalid(paStream_, ERROR) < 0) {
        return ERR_ILLEGAL_STATE;
    }
    pa_usec_t paLatency {0};
    pa_usec_t cacheLatency {0};
    int32_t negative {0};

    // Get PA latency
    pa_threaded_mainloop_lock(mainloop_);
    while (true) {
        pa_operation *operation = pa_stream_update_timing_info(paStream_, NULL, NULL);
        if (operation != nullptr) {
            pa_operation_unref(operation);
        } else {
            AUDIO_ERR_LOG("pa_stream_update_timing_info failed");
        }
        if (pa_stream_get_latency(paStream_, &paLatency, &negative) >= 0) {
            if (negative) {
                latency = 0;
                pa_threaded_mainloop_unlock(mainloop_);
                return ERR_OPERATION_FAILED;
            }
            break;
        }
        AUDIO_INFO_LOG("waiting for audio latency information");
        pa_threaded_mainloop_wait(mainloop_);
    }
    pa_threaded_mainloop_unlock(mainloop_);

    // In plan, 怎么计算cacheLatency
    // Get audio read cache latency
    cacheLatency = pa_bytes_to_usec(minBufferSize_, sampleSpec_);

    // Total latency will be sum of audio read cache latency + PA latency
    latency = paLatency + cacheLatency;
    AUDIO_DEBUG_LOG("total latency: %{public}" PRIu64 ", pa latency: %{public}" PRIu64, latency, paLatency);
    return SUCCESS;
}

int32_t PaCapturerStreamImpl::Flush()
{
    AUDIO_INFO_LOG("Enter PaCapturerStreamImpl::Flush");
    if (CheckReturnIfStreamInvalid(paStream_, ERROR) < 0) {
        return ERR_ILLEGAL_STATE;
    }

    pa_operation *operation = nullptr;
    pa_stream_state_t state = pa_stream_get_state(paStream_);
    if (state != PA_STREAM_READY) {
        AUDIO_ERR_LOG("Stream Flush Failed");
        return ERR_ILLEGAL_STATE;
    }
    streamFlushStatus_ = 0;
    operation = pa_stream_flush(paStream_, PAStreamFlushSuccessCb, (void *)this);
    if (operation == nullptr) {
        AUDIO_ERR_LOG("Stream Flush Operation Failed");
        return ERR_OPERATION_FAILED;
    }
    pa_operation_unref(operation);
    return SUCCESS;
}

int32_t PaCapturerStreamImpl::Stop()
{
    AUDIO_INFO_LOG("Enter PaCapturerStreamImpl::Stop");
    PAStreamCorkSuccessCb = PAStreamStopSuccessCb;
    return CorkStream();
}

int32_t PaCapturerStreamImpl::Release()
{
    state_ = RELEASED;
    if (paStream_) {
        pa_stream_set_state_callback(paStream_, nullptr, nullptr);
        pa_stream_set_read_callback(paStream_, nullptr, nullptr);
        pa_stream_set_read_callback(paStream_, nullptr, nullptr);
        pa_stream_set_latency_update_callback(paStream_, nullptr, nullptr);
        pa_stream_set_underflow_callback(paStream_, nullptr, nullptr);

        pa_stream_disconnect(paStream_);
        pa_stream_unref(paStream_);
        paStream_ = nullptr;
    }
    std::shared_ptr<IStatusCallback> statusCallback = statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_RELEASED);
    }
    return SUCCESS;
}

void PaCapturerStreamImpl::RegisterStatusCallback(const std::weak_ptr<IStatusCallback> &callback)
{
    statusCallback_ = callback;
}

void PaCapturerStreamImpl::RegisterReadCallback(const std::weak_ptr<IReadCallback> &callback)
{
    readCallback_ = callback;
}

BufferDesc PaCapturerStreamImpl::DequeueBuffer(size_t length)
{
    BufferDesc bufferDesc;
    const void *tempBuffer = nullptr;
    pa_stream_peek(paStream_, &tempBuffer, &bufferDesc.bufLength);
    bufferDesc.buffer = static_cast<uint8_t *>(const_cast<void* >(tempBuffer));
    totalBytesRead_ += bufferDesc.bufLength;
    AUDIO_INFO_LOG("PaCapturerStreamImpl::DequeueBuffer length %{public}zu", bufferDesc.bufLength);
    return bufferDesc;
}

int32_t PaCapturerStreamImpl::EnqueueBuffer(const BufferDesc &bufferDesc)
{
    AUDIO_INFO_LOG("Capturer enqueue buffer");
    pa_stream_drop(paStream_);
    AUDIO_INFO_LOG("After enqueue capturere buffer, readable size is %{public}zu", pa_stream_readable_size(paStream_));
    return SUCCESS;
}

int32_t PaCapturerStreamImpl::DropBuffer()
{
    AUDIO_INFO_LOG("Capturer DropBuffer");
    pa_stream_drop(paStream_);
    AUDIO_INFO_LOG("After capturere DropBuffer, readable size is %{public}zu", pa_stream_readable_size(paStream_));
    return SUCCESS;
}

void PaCapturerStreamImpl::PAStreamReadCb(pa_stream *stream, size_t length, void *userdata)
{
    AUDIO_INFO_LOG("PAStreamReadCb, length: %{public}zu, pa_stream_writeable_size: %{public}zu",
        length, pa_stream_writable_size(stream));
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamReadCb: userdata is null");
        return;
    }
    auto streamImpl = static_cast<PaCapturerStreamImpl *>(userdata);
    if (streamImpl->abortFlag_ != 0) {
        AUDIO_ERR_LOG("PAStreamWriteCb: Abort pa stream read callback");
        streamImpl->abortFlag_--;
        return ;
    }
    std::shared_ptr<IReadCallback> readCallback = streamImpl->readCallback_.lock();
    if (readCallback != nullptr) {
        readCallback->OnReadData(length);
    } else {
        AUDIO_ERR_LOG("Read callback is nullptr");
    }
}

void PaCapturerStreamImpl::PAStreamMovedCb(pa_stream *stream, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamMovedCb: userdata is null");
        return;
    }

    // get stream informations.
    uint32_t deviceIndex = pa_stream_get_device_index(stream); // pa_context_get_sink_info_by_index

    // Return 1 if the sink or source this stream is connected to has been suspended.
    // This will return 0 if not, and a negative value on error.
    int res = pa_stream_is_suspended(stream);
    AUDIO_DEBUG_LOG("PAstream moved to index:[%{public}d] suspended:[%{public}d]",
        deviceIndex, res);
}

void PaCapturerStreamImpl::PAStreamUnderFlowCb(pa_stream *stream, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamUnderFlowCb: userdata is null");
        return;
    }

    PaCapturerStreamImpl *streamImpl = static_cast<PaCapturerStreamImpl *>(userdata);
    streamImpl->underFlowCount_++;

    std::shared_ptr<IStatusCallback> statusCallback = streamImpl->statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_UNDERRUN);
    }
    AUDIO_WARNING_LOG("PaCapturerStreamImpl underrun: %{public}d!", streamImpl->underFlowCount_);
}


void PaCapturerStreamImpl::PAStreamSetStartedCb(pa_stream *stream, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamSetStartedCb: userdata is null");
        return;
    }

    AUDIO_WARNING_LOG("PAStreamSetStartedCb");
}

void PaCapturerStreamImpl::PAStreamStartSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamStartSuccessCb: userdata is null");
        return;
    }

    PaCapturerStreamImpl *streamImpl = static_cast<PaCapturerStreamImpl *>(userdata);
    streamImpl->state_ = RUNNING;
    std::shared_ptr<IStatusCallback> statusCallback = streamImpl->statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_STARTED);
    }
    streamImpl->streamCmdStatus_ = success;
}

void PaCapturerStreamImpl::PAStreamPauseSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamPauseSuccessCb: userdata is null");
        return;
    }

    PaCapturerStreamImpl *streamImpl = static_cast<PaCapturerStreamImpl *>(userdata);
    streamImpl->state_ = PAUSED;
    std::shared_ptr<IStatusCallback> statusCallback = streamImpl->statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_PAUSED);
    }
    streamImpl->streamCmdStatus_ = success;
}

void PaCapturerStreamImpl::PAStreamFlushSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamFlushSuccessCb: userdata is null");
        return;
    }
    PaCapturerStreamImpl *streamImpl = static_cast<PaCapturerStreamImpl *>(userdata);
    std::shared_ptr<IStatusCallback> statusCallback = streamImpl->statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_FLUSHED);
    }
    streamImpl->streamFlushStatus_ = success;
}

void PaCapturerStreamImpl::PAStreamStopSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamAsyncStopSuccessCb: userdata is null");
        return;
    }

    PaCapturerStreamImpl *streamImpl = static_cast<PaCapturerStreamImpl *>(userdata);
    std::shared_ptr<IStatusCallback> statusCallback = streamImpl->statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_STOPPED);
    }
}

int32_t PaCapturerStreamImpl::GetMinimumBufferSize(size_t &minBufferSize) const
{
    minBufferSize = minBufferSize_;
    return SUCCESS;
}

void PaCapturerStreamImpl::GetByteSizePerFrame(size_t &byteSizePerFrame) const
{
    byteSizePerFrame = byteSizePerFrame_;
}

void PaCapturerStreamImpl::GetSpanSizePerFrame(size_t &spanSizeInFrame) const
{
    spanSizeInFrame = spanSizeInFrame_;
}

uint32_t PaCapturerStreamImpl::GetStreamIndex()
{
    return pa_stream_get_index(paStream_);
}

void PaCapturerStreamImpl::AbortCallback(int32_t abortTimes)
{
    abortFlag_ += abortTimes;
}
} // namespace AudioStandard
} // namespace OHOS
