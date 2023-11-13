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

#include "pa_renderer_stream_impl.h"
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

PaRendererStreamImpl::PaRendererStreamImpl(pa_stream *paStream, AudioStreamParams params, pa_threaded_mainloop *mainloop)
{
    mainloop_ = mainloop;
    paStream_ = paStream;
    params_ = params;

    pa_stream_set_moved_callback(paStream, PAStreamMovedCb, (void *)this); // used to notify sink/source moved
    pa_stream_set_write_callback(paStream, PAStreamWriteCb, (void *)this);
    pa_stream_set_underflow_callback(paStream, PAStreamUnderFlowCb, (void *)this);
    pa_stream_set_started_callback(paStream, PAStreamSetStartedCb, (void *)this);
}

int32_t PaRendererStreamImpl::Start()
{
    AUDIO_DEBUG_LOG("Enter PaRendererStreamImpl::Start");
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

int32_t PaRendererStreamImpl::Pause()
{
    AUDIO_INFO_LOG("Enter PaRendererStreamImpl::Pause");
    PAStreamCorkSuccessCb = PAStreamPauseSuccessCb;
    return CorkStream();
}

int32_t PaRendererStreamImpl::CorkStream()
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

int32_t PaRendererStreamImpl::Flush()
{
    AUDIO_INFO_LOG("Enter PaRendererStreamImpl::Flush");
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

int32_t PaRendererStreamImpl::Drain()
{
    AUDIO_INFO_LOG("Enter PaRendererStreamImpl::Drain");
    if (CheckReturnIfStreamInvalid(paStream_, PA_ADAPTER_PA_ERR) < 0) {
        return PA_ADAPTER_PA_ERR;
    }
    isDrain_ = true;

    pa_operation *operation = nullptr;
    pa_stream_state_t state = pa_stream_get_state(paStream_);
    if (state != PA_STREAM_READY) {
        AUDIO_ERR_LOG("Stream drain failed, state is not ready");
        return PA_ADAPTER_ERR;
    }
    streamDrainStatus_ = 0;
    operation = pa_stream_drain(paStream_, PAStreamDrainSuccessCb, (void *)this);
    pa_operation_unref(operation);
    return PA_ADAPTER_SUCCESS;
}

int32_t PaRendererStreamImpl::Stop()
{
    AUDIO_INFO_LOG("Enter PaRendererStreamImpl::Stop");
    state_ = STOPPING;

    if (CheckReturnIfStreamInvalid(paStream_, PA_ADAPTER_PA_ERR) < 0) {
        return PA_ADAPTER_PA_ERR;
    }

    streamDrainStatus_ = 0;
    isDrain_ = true;
    pa_operation *operation = pa_stream_drain(paStream_, PAStreamDrainInStopCb, (void *)this);
    if (operation == nullptr) {
        AUDIO_ERR_LOG("pa_stream_drain operation is null");
        return PA_ADAPTER_ERR;
    }

    pa_operation_unref(operation);
    return PA_ADAPTER_SUCCESS;
}

int32_t PaRendererStreamImpl::Release()
{
    state_ = RELEASED;
    if (paStream_) {
        pa_stream_set_state_callback(paStream_, nullptr, nullptr);
        pa_stream_set_write_callback(paStream_, nullptr, nullptr);
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

void PaRendererStreamImpl::RegisterStatusCallback(const std::weak_ptr<IStatusCallback> &callback)
{
    statusCallback_ = callback;
}

void PaRendererStreamImpl::RegisterWriteCallback(const std::weak_ptr<IWriteCallback> &callback)
{
    writeCallback_ = callback;
}

BufferDesc PaRendererStreamImpl::DequeueBuffer(size_t length)
{
    BufferDesc bufferDesc;
    bufferDesc.bufLength = length;
    pa_stream_begin_write(paStream_, reinterpret_cast<void **>(&bufferDesc.buffer), &bufferDesc.bufLength);
    return bufferDesc;
}

int32_t PaRendererStreamImpl::EnqueueBuffer(const BufferDesc &bufferDesc)
{
    pa_threaded_mainloop_lock(mainloop_);
    int32_t error = 0;
    error = pa_stream_write(paStream_, static_cast<void*>(bufferDesc.buffer), bufferDesc.bufLength, nullptr, 0LL, PA_SEEK_RELATIVE);
    if (error < 0) {
        AUDIO_ERR_LOG("Write stream failed");
        pa_stream_cancel_write(paStream_);
    }
    pa_threaded_mainloop_unlock(mainloop_);
    return PA_ADAPTER_SUCCESS;
}

void PaRendererStreamImpl::PAStreamWriteCb(pa_stream *stream, size_t length, void *userdata)
{
    AUDIO_INFO_LOG("PAStreamWriteCb, length: %{public}zu, pa_stream_writeable_size: %{public}zu", length, pa_stream_writable_size(stream));
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamWriteCb: userdata is null");
        return;
    }

    auto streamImpl = static_cast<PaRendererStreamImpl *>(userdata);
    if (streamImpl->abortFlag_ != 0) {
        AUDIO_ERR_LOG("PAStreamWriteCb: Abort pa stream write callback");
        streamImpl->abortFlag_--;
        return ;
    }
    std::shared_ptr<IWriteCallback> writeCallback = streamImpl->writeCallback_.lock();
    if (writeCallback != nullptr) {
        writeCallback->OnWriteData(length);
    } else {
        AUDIO_ERR_LOG("Write callback is nullptr");
    }
}


void PaRendererStreamImpl::PAStreamMovedCb(pa_stream *stream, void *userdata)
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

void PaRendererStreamImpl::PAStreamUnderFlowCb(pa_stream *stream, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamUnderFlowCb: userdata is null");
        return;
    }

    PaRendererStreamImpl *streamImpl = static_cast<PaRendererStreamImpl *>(userdata);
    streamImpl->underFlowCount_++;
    std::shared_ptr<IStatusCallback> statusCallback = streamImpl->statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_UNDERRUN);
    }
    AUDIO_WARNING_LOG("PaRendererStreamImpl underrun: %{public}d!", streamImpl->underFlowCount_);
}


void PaRendererStreamImpl::PAStreamSetStartedCb(pa_stream *stream, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamSetStartedCb: userdata is null");
        return;
    }
    AUDIO_WARNING_LOG("PAStreamSetStartedCb");
}

void PaRendererStreamImpl::PAStreamStartSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamStartSuccessCb: userdata is null");
        return;
    }

    PaRendererStreamImpl *streamImpl = static_cast<PaRendererStreamImpl *>(userdata);
    streamImpl->state_ = RUNNING;
    std::shared_ptr<IStatusCallback> statusCallback = streamImpl->statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_STARTED);
    }
    streamImpl->streamCmdStatus_ = success;
}

void PaRendererStreamImpl::PAStreamPauseSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamPauseSuccessCb: userdata is null");
        return;
    }

    PaRendererStreamImpl *streamImpl = static_cast<PaRendererStreamImpl *>(userdata);
    streamImpl->state_ = PAUSED;
    std::shared_ptr<IStatusCallback> statusCallback = streamImpl->statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_PAUSED);
    }
    streamImpl->streamCmdStatus_ = success;
}

void PaRendererStreamImpl::PAStreamFlushSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamFlushSuccessCb: userdata is null");
        return;
    }
    PaRendererStreamImpl *streamImpl = static_cast<PaRendererStreamImpl *>(userdata);
    std::shared_ptr<IStatusCallback> statusCallback = streamImpl->statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_FLUSHED);
    }
    streamImpl->streamFlushStatus_ = success;
}

void PaRendererStreamImpl::PAStreamDrainSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamDrainSuccessCb: userdata is null");
        return;
    }

    PaRendererStreamImpl *streamImpl = static_cast<PaRendererStreamImpl *>(userdata);
    std::shared_ptr<IStatusCallback> statusCallback = streamImpl->statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_DRAINED);
    }
    streamImpl->streamDrainStatus_ = success;
    streamImpl->isDrain_ = false;
}

void PaRendererStreamImpl::PAStreamDrainInStopCb(pa_stream *stream, int32_t success, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamDrainInStopCb: userdata is null");
        return;
    }
    PaRendererStreamImpl *streamImpl = static_cast<PaRendererStreamImpl *>(userdata);

    pa_operation *operation = pa_stream_cork(streamImpl->paStream_, 1,
        PaRendererStreamImpl::PAStreamAsyncStopSuccessCb, userdata);

    pa_operation_unref(operation);
    streamImpl->streamDrainStatus_ = success;
}

void PaRendererStreamImpl::PAStreamAsyncStopSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    AUDIO_DEBUG_LOG("PAStreamAsyncStopSuccessCb in");
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamAsyncStopSuccessCb: userdata is null");
        return;
    }

    PaRendererStreamImpl *streamImpl = static_cast<PaRendererStreamImpl *>(userdata);
    streamImpl->state_ = STOPPED;
    std::shared_ptr<IStatusCallback> statusCallback = streamImpl->statusCallback_.lock();

    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_STOPPED);
    }
    streamImpl->streamCmdStatus_ = success;
    streamImpl->isDrain_ = false;
}

int32_t PaRendererStreamImpl::GetMinimumBufferSize(size_t &minBufferSize) const
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

uint32_t PaRendererStreamImpl::GetStreamIndex()
{
    return pa_stream_get_index(paStream_);
}

void PaRendererStreamImpl::AbortCallback(int32_t abortTimes)
{
    abortFlag_ += abortTimes;
}
} // namespace AudioStandard
} // namespace OHOS
