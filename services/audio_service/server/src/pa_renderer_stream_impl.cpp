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
const uint32_t DOUBLE_VALUE = 2;


static int32_t CheckReturnIfStreamInvalid(pa_stream *paStream, const int32_t retVal)
{
    do {
        if (!(paStream && PA_STREAM_IS_GOOD(pa_stream_get_state(paStream)))) {
            return retVal;
        }
    } while (false);
    return SUCCESS;
}

PaRendererStreamImpl::PaRendererStreamImpl(pa_stream *paStream, AudioProcessConfig processConfig,
    pa_threaded_mainloop *mainloop)
{
    mainloop_ = mainloop;
    paStream_ = paStream;
    processConfig_ = processConfig;

    pa_stream_set_moved_callback(paStream, PAStreamMovedCb, (void *)this); // used to notify sink/source moved
    pa_stream_set_write_callback(paStream, PAStreamWriteCb, (void *)this);
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

void PaRendererStreamImpl::InitParams()
{
    if (CheckReturnIfStreamInvalid(paStream_, ERR_ILLEGAL_STATE) < 0) {
        return;
    }

    // Get byte size per frame
    const pa_sample_spec *sampleSpec = pa_stream_get_sample_spec(paStream_);
    AUDIO_INFO_LOG("sampleSpec: channels: %{public}u, formats: %{public}d, rate: %{public}d", sampleSpec->channels,
        sampleSpec->format, sampleSpec->rate);

    if (sampleSpec->channels != processConfig_.streamInfo.channels) {
        AUDIO_WARNING_LOG("Unequal channels, in server: %{public}d, in client: %{public}d", sampleSpec->channels,
            processConfig_.streamInfo.channels);
    }
    if (static_cast<uint8_t>(sampleSpec->format) != processConfig_.streamInfo.format) { // In plan
        AUDIO_WARNING_LOG("Unequal format, in server: %{public}d, in client: %{public}d", sampleSpec->format,
            processConfig_.streamInfo.format);
    }
    byteSizePerFrame_ = pa_frame_size(sampleSpec);

    // Get min buffer size in frame
    const pa_buffer_attr *bufferAttr = pa_stream_get_buffer_attr(paStream_);
    if (bufferAttr == nullptr) {
        AUDIO_ERR_LOG("pa_stream_get_buffer_attr returned nullptr");
        return;
    }
    minBufferSize_ = (size_t)bufferAttr->minreq;
    spanSizeInFrame_ = minBufferSize_ / byteSizePerFrame_;

    // In plan: Get data from xml
    effectSceneName_ = GetEffectSceneName(processConfig_.streamType);
}

int32_t PaRendererStreamImpl::Start()
{
    AUDIO_DEBUG_LOG("Enter PaRendererStreamImpl::Start");
    if (CheckReturnIfStreamInvalid(paStream_, ERR_ILLEGAL_STATE) < 0) {
        return ERR_ILLEGAL_STATE;
    }
    pa_operation *operation = nullptr;

    pa_stream_state_t state = pa_stream_get_state(paStream_);
    if (state != PA_STREAM_READY) {
        return ERR_OPERATION_FAILED;
    }

    streamCmdStatus_ = 0;
    operation = pa_stream_cork(paStream_, 0, PAStreamStartSuccessCb, (void *)this);
    pa_operation_unref(operation);
    return SUCCESS;
}

int32_t PaRendererStreamImpl::Pause()
{
    AUDIO_INFO_LOG("Enter PaRendererStreamImpl::Pause");
    PAStreamCorkSuccessCb = PAStreamPauseSuccessCb;
    return CorkStream();
}

int32_t PaRendererStreamImpl::CorkStream()
{
    if (CheckReturnIfStreamInvalid(paStream_, ERR_ILLEGAL_STATE) < 0) {
        return ERR_ILLEGAL_STATE;
    }

    pa_operation *operation = nullptr;
    pa_stream_state_t state = pa_stream_get_state(paStream_);
    if (state != PA_STREAM_READY) {
        AUDIO_ERR_LOG("Stream Stop Failed");
        return ERR_OPERATION_FAILED;
    }
    operation = pa_stream_cork(paStream_, 1, PAStreamCorkSuccessCb, (void *)this);
    pa_operation_unref(operation);
    return SUCCESS;
}

int32_t PaRendererStreamImpl::Flush()
{
    AUDIO_INFO_LOG("Enter PaRendererStreamImpl::Flush");
    if (CheckReturnIfStreamInvalid(paStream_, ERR_ILLEGAL_STATE) < 0) {
        return ERR_ILLEGAL_STATE;
    }

    pa_operation *operation = nullptr;
    pa_stream_state_t state = pa_stream_get_state(paStream_);
    if (state != PA_STREAM_READY) {
        AUDIO_ERR_LOG("Stream Flush Failed");
        return ERR_OPERATION_FAILED;
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

int32_t PaRendererStreamImpl::Drain()
{
    AUDIO_INFO_LOG("Enter PaRendererStreamImpl::Drain");
    if (CheckReturnIfStreamInvalid(paStream_, ERR_ILLEGAL_STATE) < 0) {
        return ERR_ILLEGAL_STATE;
    }
    isDrain_ = true;

    pa_operation *operation = nullptr;
    pa_stream_state_t state = pa_stream_get_state(paStream_);
    if (state != PA_STREAM_READY) {
        AUDIO_ERR_LOG("Stream drain failed, state is not ready");
        return ERR_OPERATION_FAILED;
    }
    streamDrainStatus_ = 0;
    operation = pa_stream_drain(paStream_, PAStreamDrainSuccessCb, (void *)this);
    pa_operation_unref(operation);
    return SUCCESS;
}

int32_t PaRendererStreamImpl::Stop()
{
    AUDIO_INFO_LOG("Enter PaRendererStreamImpl::Stop");
    state_ = STOPPING;

    if (CheckReturnIfStreamInvalid(paStream_, ERR_ILLEGAL_STATE) < 0) {
        return ERR_ILLEGAL_STATE;
    }

    streamDrainStatus_ = 0;
    isDrain_ = true;
    pa_operation *operation = pa_stream_drain(paStream_, PAStreamDrainInStopCb, (void *)this);
    if (operation == nullptr) {
        AUDIO_ERR_LOG("pa_stream_drain operation is null");
        return ERR_OPERATION_FAILED;
    }

    pa_operation_unref(operation);
    return SUCCESS;
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
    return SUCCESS;
}

int32_t PaRendererStreamImpl::GetStreamFramesWritten(uint64_t &framesWritten)
{
    CHECK_AND_RETURN_RET_LOG(byteSizePerFrame_ != 0, ERR_ILLEGAL_STATE, "Error frame size");
    framesWritten = totalBytesWritten_ / byteSizePerFrame_;
    return SUCCESS;
}

int32_t PaRendererStreamImpl::GetCurrentTimeStamp(uint64_t &timeStamp)
{
    if (CheckReturnIfStreamInvalid(paStream_, ERR_ILLEGAL_STATE) < 0) {
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

    const pa_sample_spec *sampleSpec = pa_stream_get_sample_spec(paStream_);
    timeStamp = pa_bytes_to_usec(info->write_index, sampleSpec);
    pa_threaded_mainloop_unlock(mainloop_);
    return SUCCESS;
}

int32_t PaRendererStreamImpl::GetLatency(uint64_t &latency)
{
    // LYH in plan, 增加dataMutex的锁
    if (CheckReturnIfStreamInvalid(paStream_, ERR_ILLEGAL_STATE) < 0) {
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
    cacheLatency = 0;
    // In plan: Total latency will be sum of audio write cache latency plus PA latency
    uint64_t fwLatency = paLatency + cacheLatency;
    uint64_t sinkLatency = sinkLatencyInMsec_ * PA_USEC_PER_MSEC;
    if (fwLatency > sinkLatency) {
        latency = fwLatency - sinkLatency;
    } else {
        latency = fwLatency;
    }
    AUDIO_DEBUG_LOG("total latency: %{public}" PRIu64 ", pa latency: %{public}"
        PRIu64 ", cache latency: %{public}" PRIu64, latency, paLatency, cacheLatency);

    return SUCCESS;
}

int32_t PaRendererStreamImpl::SetRate(int32_t rate)
{
    AUDIO_INFO_LOG("SetRate in");
    if (CheckReturnIfStreamInvalid(paStream_, ERR_ILLEGAL_STATE) < 0) {
        return ERR_ILLEGAL_STATE;
    }
    uint32_t currentRate = processConfig_.streamInfo.samplingRate;
    switch (rate) {
        case RENDER_RATE_NORMAL:
            break;
        case RENDER_RATE_DOUBLE:
            currentRate *= DOUBLE_VALUE;
            break;
        case RENDER_RATE_HALF:
            currentRate /= DOUBLE_VALUE;
            break;
        default:
            return ERR_INVALID_PARAM;
    }
    renderRate_ = rate;

    pa_threaded_mainloop_lock(mainloop_);
    pa_operation *operation = pa_stream_update_sample_rate(paStream_, currentRate, nullptr, nullptr);
    if (operation != nullptr) {
        pa_operation_unref(operation);
    } else {
        AUDIO_ERR_LOG("SetRate: operation is nullptr");
    }
    pa_threaded_mainloop_unlock(mainloop_);
    return SUCCESS;
}

int32_t PaRendererStreamImpl::SetLowPowerVolume(float powerVolume)
{
    AUDIO_INFO_LOG("SetLowPowerVolume: %{public}f", powerVolume);

    if (CheckReturnIfStreamInvalid(paStream_, ERR_ILLEGAL_STATE) < 0) {
        return ERR_ILLEGAL_STATE;
    }

    /* Validate and return INVALID_PARAMS error */
    if ((powerVolume < MIN_STREAM_VOLUME_LEVEL) || (powerVolume > MAX_STREAM_VOLUME_LEVEL)) {
        AUDIO_ERR_LOG("Invalid Power Volume Set!");
        return -1;
    }

    pa_threaded_mainloop_lock(mainloop_);

    powerVolumeFactor_ = powerVolume;
    pa_proplist *propList = pa_proplist_new();
    if (propList == nullptr) {
        AUDIO_ERR_LOG("pa_proplist_new failed");
        pa_threaded_mainloop_unlock(mainloop_);
        return ERR_OPERATION_FAILED;
    }

    pa_proplist_sets(propList, "stream.powerVolumeFactor", std::to_string(powerVolumeFactor_).c_str());
    pa_operation *updatePropOperation = pa_stream_proplist_update(paStream_, PA_UPDATE_REPLACE, propList,
        nullptr, nullptr);
    pa_proplist_free(propList);
    pa_operation_unref(updatePropOperation);

    // In plan: Call reset volume
    pa_threaded_mainloop_unlock(mainloop_);

    return SUCCESS;
}

int32_t PaRendererStreamImpl::GetLowPowerVolume(float &powerVolume)
{
    powerVolume = powerVolumeFactor_;
    return SUCCESS;
}

int32_t PaRendererStreamImpl::SetAudioEffectMode(int32_t effectMode)
{
    AUDIO_INFO_LOG("SetAudioEffectMode: %{public}d", effectMode);
    if (CheckReturnIfStreamInvalid(paStream_, ERR_ILLEGAL_STATE) < 0) {
        return ERR_ILLEGAL_STATE;
    }
    pa_threaded_mainloop_lock(mainloop_);

    effectMode_ = effectMode;
    const std::string effectModeName = GetEffectModeName(effectMode_);

    pa_proplist *propList = pa_proplist_new();
    if (propList == nullptr) {
        AUDIO_ERR_LOG("pa_proplist_new failed");
        pa_threaded_mainloop_unlock(mainloop_);
        return ERR_OPERATION_FAILED;
    }

    pa_proplist_sets(propList, "scene.mode", effectModeName.c_str());
    pa_operation *updatePropOperation = pa_stream_proplist_update(paStream_, PA_UPDATE_REPLACE, propList,
        nullptr, nullptr);
    pa_proplist_free(propList);
    pa_operation_unref(updatePropOperation);

    pa_threaded_mainloop_unlock(mainloop_);

    return SUCCESS;
}

const std::string PaRendererStreamImpl::GetEffectModeName(int32_t effectMode)
{
    std::string name;
    switch (effectMode) {
        case 0: // AudioEffectMode::EFFECT_NONE
            name = "EFFECT_NONE";
            break;
        default:
            name = "EFFECT_DEFAULT";
    }

    const std::string modeName = name;
    return modeName;
}

int32_t PaRendererStreamImpl::GetAudioEffectMode(int32_t &effectMode)
{
    effectMode = effectMode_;
    return SUCCESS;
}

int32_t PaRendererStreamImpl::SetPrivacyType(int32_t privacyType)
{
    AUDIO_DEBUG_LOG("SetInnerCapturerState: %{public}d", privacyType);
    privacyType_ = privacyType;
    return SUCCESS;
}

int32_t PaRendererStreamImpl::GetPrivacyType(int32_t &privacyType)
{
    privacyType_ = privacyType;
    return SUCCESS;
}


void PaRendererStreamImpl::RegisterStatusCallback(const std::weak_ptr<IStatusCallback> &callback)
{
    AUDIO_DEBUG_LOG("RegisterStatusCallback in");
    statusCallback_ = callback;
}

void PaRendererStreamImpl::RegisterWriteCallback(const std::weak_ptr<IWriteCallback> &callback)
{
    AUDIO_DEBUG_LOG("RegisterWriteCallback in");
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
    error = pa_stream_write(paStream_, static_cast<void*>(bufferDesc.buffer), bufferDesc.bufLength, nullptr,
        0LL, PA_SEEK_RELATIVE);
    if (error < 0) {
        AUDIO_ERR_LOG("Write stream failed");
        pa_stream_cancel_write(paStream_);
    }
    totalBytesWritten_ += bufferDesc.bufLength;
    pa_threaded_mainloop_unlock(mainloop_);
    return SUCCESS;
}

void PaRendererStreamImpl::PAStreamWriteCb(pa_stream *stream, size_t length, void *userdata)
{
    AUDIO_INFO_LOG("PAStreamWriteCb, length: %{public}zu, pa_stream_writeable_size: %{public}zu",
        length, pa_stream_writable_size(stream));
    CHECK_AND_RETURN_LOG(userdata, "PAStreamWriteCb: userdata is null");

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
    CHECK_AND_RETURN_LOG(userdata, "PAStreamMovedCb: userdata is null");

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
    CHECK_AND_RETURN_LOG(userdata, "PAStreamUnderFlowCb: userdata is null");

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
    CHECK_AND_RETURN_LOG(userdata, "PAStreamSetStartedCb: userdata is null");
    AUDIO_WARNING_LOG("PAStreamSetStartedCb");
}

void PaRendererStreamImpl::PAStreamStartSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    AUDIO_INFO_LOG("PAStreamStartSuccessCb in");
    CHECK_AND_RETURN_LOG(userdata, "PAStreamStartSuccessCb: userdata is null");

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
    CHECK_AND_RETURN_LOG(userdata, "PAStreamPauseSuccessCb: userdata is null");

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
    CHECK_AND_RETURN_LOG(userdata, "PAStreamFlushSuccessCb: userdata is null");

    PaRendererStreamImpl *streamImpl = static_cast<PaRendererStreamImpl *>(userdata);
    std::shared_ptr<IStatusCallback> statusCallback = streamImpl->statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_FLUSHED);
    }
    streamImpl->streamFlushStatus_ = success;
}

void PaRendererStreamImpl::PAStreamDrainSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    CHECK_AND_RETURN_LOG(userdata, "PAStreamDrainSuccessCb: userdata is null");

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
    CHECK_AND_RETURN_LOG(userdata, "PAStreamDrainInStopCb: userdata is null");

    PaRendererStreamImpl *streamImpl = static_cast<PaRendererStreamImpl *>(userdata);
    pa_operation *operation = pa_stream_cork(streamImpl->paStream_, 1,
        PaRendererStreamImpl::PAStreamAsyncStopSuccessCb, userdata);

    pa_operation_unref(operation);
    streamImpl->streamDrainStatus_ = success;
}

void PaRendererStreamImpl::PAStreamAsyncStopSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    AUDIO_DEBUG_LOG("PAStreamAsyncStopSuccessCb in");
    CHECK_AND_RETURN_LOG(userdata, "PAStreamAsyncStopSuccessCb: userdata is null");

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
    minBufferSize = minBufferSize_;
    return SUCCESS;
}

void PaRendererStreamImpl::GetByteSizePerFrame(size_t &byteSizePerFrame) const
{
    byteSizePerFrame = byteSizePerFrame_;
}

void PaRendererStreamImpl::GetSpanSizePerFrame(size_t &spanSizeInFrame) const
{
    spanSizeInFrame = spanSizeInFrame_;
}

void PaRendererStreamImpl::SetStreamIndex(uint32_t index)
{
    AUDIO_INFO_LOG("Using index/sessionId %{public}d", index);
    streamIndex_ = index;
}

uint32_t PaRendererStreamImpl::GetStreamIndex()
{
    return streamIndex_;
}

const std::string PaRendererStreamImpl::GetEffectSceneName(AudioStreamType audioType)
{
    std::string name;
    switch (audioType) {
        case STREAM_MUSIC:
            name = "SCENE_MUSIC";
            break;
        case STREAM_GAME:
            name = "SCENE_GAME";
            break;
        case STREAM_MOVIE:
            name = "SCENE_MOVIE";
            break;
        case STREAM_SPEECH:
        case STREAM_VOICE_CALL:
        case STREAM_VOICE_ASSISTANT:
            name = "SCENE_SPEECH";
            break;
        case STREAM_RING:
        case STREAM_ALARM:
        case STREAM_NOTIFICATION:
        case STREAM_SYSTEM:
        case STREAM_DTMF:
        case STREAM_SYSTEM_ENFORCED:
            name = "SCENE_RING";
            break;
        default:
            name = "SCENE_OTHERS";
    }

    const std::string sceneName = name;
    return sceneName;
}

void PaRendererStreamImpl::AbortCallback(int32_t abortTimes)
{
    abortFlag_ += abortTimes;
}
} // namespace AudioStandard
} // namespace OHOS
