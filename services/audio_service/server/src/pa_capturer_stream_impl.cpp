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
    return 0;
}

PaCapturerStreamImpl::PaCapturerStreamImpl(pa_stream *paStream, AudioStreamParams params, pa_threaded_mainloop *mainloop)
{
    mainloop_ = mainloop;
    paStream_ = paStream;
    params_ = params;

    pa_stream_set_moved_callback(paStream, PAStreamMovedCb, (void *)this); // used to notify sink/source moved
    pa_stream_set_read_callback(paStream, PAStreamReadCb, (void *)this);
    pa_stream_set_underflow_callback(paStream, PAStreamUnderFlowCb, (void *)this);
    pa_stream_set_started_callback(paStream, PAStreamSetStartedCb, (void *)this);
}

int32_t PaCapturerStreamImpl::Start()
{
    AUDIO_DEBUG_LOG("Enter PaCapturerStreamImpl::Start");
    if (CheckReturnIfStreamInvalid(paStream_, PA_ADAPTER_PA_ERR) < 0) {
        return PA_ADAPTER_PA_ERR;
    }
    pa_operation *operation = nullptr;
    pa_stream_state_t state = pa_stream_get_state(paStream_);
    if (state != PA_STREAM_READY) {
        return PA_ADAPTER_START_STREAM_ERR;
    }

    streamCmdStatus_ = 0;
    operation = pa_stream_cork(paStream_, 0, PAStreamStartSuccessCb, (void *)this);
    pa_operation_unref(operation);
    return PA_ADAPTER_SUCCESS;
}

int32_t PaCapturerStreamImpl::Pause()
{
    AUDIO_INFO_LOG("Enter PaCapturerStreamImpl::Pause");
    PAStreamCorkSuccessCb = PAStreamPauseSuccessCb;
    return CorkStream();
}

int32_t PaCapturerStreamImpl::CorkStream()
{
    if (CheckReturnIfStreamInvalid(paStream_, PA_ADAPTER_PA_ERR) < 0) {
        return PA_ADAPTER_PA_ERR;
    }
    pa_operation *operation = nullptr;

    pa_stream_state_t state = pa_stream_get_state(paStream_);
    if (state != PA_STREAM_READY) {
        AUDIO_ERR_LOG("Stream Stop Failed");
        return PA_ADAPTER_ERR;
    }
    operation = pa_stream_cork(paStream_, 1, PAStreamCorkSuccessCb, (void *)this);
    pa_operation_unref(operation);
    return PA_ADAPTER_SUCCESS;
}

int32_t PaCapturerStreamImpl::Flush()
{
    AUDIO_INFO_LOG("Enter PaCapturerStreamImpl::Flush");
    if (CheckReturnIfStreamInvalid(paStream_, PA_ADAPTER_PA_ERR) < 0) {
        return PA_ADAPTER_PA_ERR;
    }

    pa_operation *operation = nullptr;
    pa_stream_state_t state = pa_stream_get_state(paStream_);
    if (state != PA_STREAM_READY) {
        AUDIO_ERR_LOG("Stream Flush Failed");
        return PA_ADAPTER_ERR;
    }
    streamFlushStatus_ = 0;
    operation = pa_stream_flush(paStream_, PAStreamFlushSuccessCb, (void *)this);
    if (operation == nullptr) {
        AUDIO_ERR_LOG("Stream Flush Operation Failed");
        return PA_ADAPTER_ERR;
    }
    pa_operation_unref(operation);
    return PA_ADAPTER_SUCCESS;
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
    return PA_ADAPTER_SUCCESS;
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
    AUDIO_INFO_LOG("PaCapturerStreamImpl::DequeueBuffer length %{public}zu", bufferDesc.bufLength);
    return bufferDesc;
}

int32_t PaCapturerStreamImpl::EnqueueBuffer(const BufferDesc &bufferDesc)
{
    AUDIO_INFO_LOG("Capturer enqueue buffer");
    pa_stream_drop(paStream_);
    AUDIO_INFO_LOG("After enqueue capturere buffer, readable size is %{public}zu", pa_stream_readable_size(paStream_));
    return PA_ADAPTER_SUCCESS;
}

int32_t PaCapturerStreamImpl::DropBuffer()
{
    AUDIO_INFO_LOG("Capturer DropBuffer");
    pa_stream_drop(paStream_);
    AUDIO_INFO_LOG("After capturere DropBuffer, readable size is %{public}zu", pa_stream_readable_size(paStream_));
    return PA_ADAPTER_SUCCESS;
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
    if (CheckReturnIfStreamInvalid(paStream_, PA_ADAPTER_PA_ERR) < 0) {
        return PA_ADAPTER_PA_ERR;
    }

    const pa_buffer_attr *bufferAttr = pa_stream_get_buffer_attr(paStream_);
    if (bufferAttr == nullptr) {
        AUDIO_ERR_LOG("pa_stream_get_buffer_attr returned nullptr");
        return PA_ADAPTER_ERR;
    }
    minBufferSize = (size_t)bufferAttr->minreq;
    return PA_ADAPTER_SUCCESS;
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
