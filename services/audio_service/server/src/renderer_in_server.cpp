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

RendererInServer::RendererInServer(AudioProcessConfig processConfig, std::weak_ptr<IStreamListener> streamListener)
{
    processConfig_ = processConfig;
    streamListener_ = streamListener; // LYH wating for review

    int32_t ret = IStreamManager::GetPlaybackManager().CreateRender(processConfig_, stream_);
    AUDIO_INFO_LOG("Construct rendererInServer result: %{public}d", ret);
    streamIndex_ = stream_->GetStreamIndex();
    ConfigServerBuffer();
}

int32_t RendererInServer::ConfigServerBuffer()
{
    if (audioServerBuffer_ != nullptr) {
        AUDIO_INFO_LOG("ConfigProcessBuffer: process buffer already configed!");
        return SUCCESS;
    }
    stream_->GetSpanSizePerFrame(spanSizeInFrame_);
    stream_->GetMinimumBufferSize(totalSizeInFrame_);
    stream_->GetByteSizePerFrame(byteSizePerFrame_);
    if (totalSizeInFrame_ == 0 || spanSizeInFrame_ == 0 || totalSizeInFrame_ % spanSizeInFrame_ != 0) {
        AUDIO_ERR_LOG("ConfigProcessBuffer: ERR_INVALID_PARAM");
        return ERR_INVALID_PARAM;
    }

    stream_->GetByteSizePerFrame(byteSizePerFrame_);
    AUDIO_INFO_LOG("ConfigProcessBuffer: totalSizeInFrame_: %{public}u, spanSizeInFrame_: %{public}u,"
        "byteSizePerFrame_:%{public}u", totalSizeInFrame_, spanSizeInFrame_, byteSizePerFrame_);
    
    // create OHAudioBuffer in server
    audioServerBuffer_ = OHAudioBuffer::CreateFormLocal(totalSizeInFrame_, spanSizeInFrame_, byteSizePerFrame_);
    CHECK_AND_RETURN_RET_LOG(audioServerBuffer_ != nullptr, ERR_OPERATION_FAILED, "Create oh audio buffer failed");

    // we need to clear data buffer to avoid dirty data.
    memset_s(audioServerBuffer_->GetDataBase(), audioServerBuffer_->GetDataSize(), 0,
        audioServerBuffer_->GetDataSize());
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

void RendererInServer::Init()
{
    AUDIO_INFO_LOG("Init, register status and write callback");
    CHECK_AND_RETURN_LOG(stream_ != nullptr, "Renderer stream is nullptr");
    stream_->RegisterStatusCallback(shared_from_this());
    stream_->RegisterWriteCallback(shared_from_this());
}
void RendererInServer::OnStatusUpdate(IOperation operation)
{
    AUDIO_INFO_LOG("RendererInServer::OnStatusUpdate operation: %{public}d", operation);
    operation_ = operation;
    CHECK_AND_RETURN_LOG(operation != OPERATION_RELEASED, "Stream already released");
    std::lock_guard<std::mutex> lock(statusLock_);
    std::shared_ptr<IStreamListener> stateListener = streamListener_.lock();
    switch (operation) {
        case OPERATION_UNDERRUN:
            AUDIO_INFO_LOG("Underrun: audioServerBuffer_->GetAvailableDataFrames(): %{public}d",
                audioServerBuffer_->GetAvailableDataFrames());
            if (audioServerBuffer_->GetAvailableDataFrames() == 4 * spanSizeInFrame_) { //In plan, maxlength is 4
                AUDIO_INFO_LOG("Buffer is empty");
                needStart = 0;
            } else {
                AUDIO_INFO_LOG("Buffer is not empty");
                WriteData();
            }
            break;
        case OPERATION_STARTED:
            status_ = I_STATUS_STARTED;
            stateListener->OnOperationHandled(START_STREAM, 0);
            break;
        case OPERATION_PAUSED:
            status_ = I_STATUS_PAUSED;
            stateListener->OnOperationHandled(PAUSE_STREAM, 0);
            break;
        case OPERATION_STOPPED:
            stateListener->OnOperationHandled(STOP_STREAM, 0);
            status_ = I_STATUS_STOPPED;
            break;
        case OPERATION_FLUSHED:
            stateListener->OnOperationHandled(FLUSH_STREAM, 0);
            if (status_ == I_STATUS_FLUSHING_WHEN_STARTED) {
                status_ = I_STATUS_STARTED;
            } else if (status_ == I_STATUS_FLUSHING_WHEN_PAUSED) {
                status_ = I_STATUS_PAUSED;
            } else {
                AUDIO_WARNING_LOG("Invalid status before flusing");
            }
            break;
        case OPERATION_DRAINED:
            stateListener->OnOperationHandled(DRAIN_STREAM, 0);
            status_ = I_STATUS_DRAINED;
            afterDrain = true;
            break;
        case OPERATION_RELEASED:
            stateListener->OnOperationHandled(RELEASE_STREAM, 0);
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
        AUDIO_INFO_LOG("RendererInServer::WriteData: CurrentReadFrame: %{public}" PRIu64 ","
            "nextReadFrame:%{public}" PRIu64 "", currentReadFrame, nextReadFrame);
        audioServerBuffer_->SetCurReadFrame(nextReadFrame);
    }
    std::shared_ptr<IStreamListener> stateListener = streamListener_.lock();
    CHECK_AND_RETURN_LOG(stateListener != nullptr, "IStreamListener is nullptr");
    stateListener->OnOperationHandled(UPDATE_STREAM, currentReadFrame);
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
    for (int32_t i = 0; i < length / totalSizeInFrame_; i++) {
        WriteData();
    }
    return SUCCESS;
}

int32_t RendererInServer::UpdateWriteIndex()
{
    AUDIO_INFO_LOG("UpdateWriteIndex: audioServerBuffer_->GetAvailableDataFrames(): %{public}d, "
        "spanSizeInFrame_:%{public}d, needStart: %{public}d", audioServerBuffer_->GetAvailableDataFrames(),
        spanSizeInFrame_, needStart);
    if (audioServerBuffer_->GetAvailableDataFrames() == spanSizeInFrame_ && needStart < 3) { // 3 is maxlength - 1
        AUDIO_WARNING_LOG("Start write data");
        WriteData();
        needStart++;
    }
    if (afterDrain == true) {
        afterDrain = false;
        AUDIO_WARNING_LOG("After drain, start write data");
        WriteData();
    }
    return SUCCESS;
}

int32_t RendererInServer::ResolveBuffer(std::shared_ptr<OHAudioBuffer> &buffer)
{
    buffer = audioServerBuffer_;
    return SUCCESS;
}

int32_t RendererInServer::GetSessionId(uint32_t &sessionId)
{
    CHECK_AND_RETURN_RET_LOG(stream_ != nullptr, ERR_OPERATION_FAILED, "GetSessionId failed, stream_ is null");
    sessionId = streamIndex_;
    CHECK_AND_RETURN_RET_LOG(sessionId < INT32_MAX, ERR_OPERATION_FAILED, "GetSessionId failed, sessionId:%{public}d",
        sessionId);

    return SUCCESS;
}

int32_t RendererInServer::Start()
{
    needStart = 0;
    std::unique_lock<std::mutex> lock(statusLock_);
    if (status_ != I_STATUS_IDLE && status_ != I_STATUS_PAUSED && status_ != I_STATUS_STOPPED) {
        AUDIO_ERR_LOG("RendererInServer::Start failed, Illegal state: %{public}u", status_);
        return ERR_ILLEGAL_STATE;
    }
    status_ = I_STATUS_STARTING;
    int ret = stream_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Start stream failed, reason: %{public}d", ret);
    resetTime_ = true;
    return SUCCESS;
}

int32_t RendererInServer::Pause()
{
    std::unique_lock<std::mutex> lock(statusLock_);
    if (status_ != I_STATUS_STARTED) {
        AUDIO_ERR_LOG("RendererInServer::Pause failed, Illegal state: %{public}u", status_);
        return ERR_ILLEGAL_STATE;
    }
    status_ = I_STATUS_PAUSING;
    int ret = stream_->Pause();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Pause stream failed, reason: %{public}d", ret);
    return SUCCESS;
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
        return ERR_ILLEGAL_STATE;
    }

    // Flush buffer of audio server
    uint64_t writeFrame = audioServerBuffer_->GetCurWriteFrame();
    uint64_t readFrame = audioServerBuffer_->GetCurReadFrame();

    while (readFrame < writeFrame) {
        BufferDesc bufferDesc = {nullptr, 0, 0};
        int32_t readResult = audioServerBuffer_->GetReadbuffer(readFrame, bufferDesc);
        if (readResult != 0) {
            return ERR_OPERATION_FAILED;
        }
        memset_s(bufferDesc.buffer, bufferDesc.bufLength, 0, bufferDesc.bufLength);
        readFrame += spanSizeInFrame_;
        AUDIO_INFO_LOG("On flush, read frame: %{public}" PRIu64 ", nextReadFrame: %{public}u,"
            "writeFrame: %{public}" PRIu64 "", readFrame, spanSizeInFrame_, writeFrame);
        audioServerBuffer_->SetCurReadFrame(readFrame);
    }

    int ret = stream_->Flush();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Flush stream failed, reason: %{public}d", ret);
    return SUCCESS;
}

int32_t RendererInServer::DrainAudioBuffer()
{
    return SUCCESS;
}

int32_t RendererInServer::SendOneFrame()
{
    OnWriteData(100000);
    return SUCCESS;
}

int32_t RendererInServer::GetInfo()
{
    IStreamManager::GetPlaybackManager().GetInfo();
    return SUCCESS;
}

int32_t RendererInServer::Drain()
{
    {
        std::unique_lock<std::mutex> lock(statusLock_);
        if (status_ != I_STATUS_STARTED) {
            AUDIO_ERR_LOG("RendererInServer::Drain failed, Illegal state: %{public}u", status_);
            return ERR_ILLEGAL_STATE;
        }
        status_ = I_STATUS_DRAINING;
    }
    AUDIO_INFO_LOG("Start drain");
    DrainAudioBuffer();
    int ret = stream_->Drain();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Drain stream failed, reason: %{public}d", ret);
    return SUCCESS;
}

int32_t RendererInServer::Stop()
{
    {
        std::unique_lock<std::mutex> lock(statusLock_);
        if (status_ != I_STATUS_STARTED && status_ != I_STATUS_PAUSED) {
            AUDIO_ERR_LOG("RendererInServer::Stop failed, Illegal state: %{public}u", status_);
            return ERR_ILLEGAL_STATE;
        }
        status_ = I_STATUS_STOPPING;
    }
    int ret = stream_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Stop stream failed, reason: %{public}d", ret);
    return SUCCESS;
}

int32_t RendererInServer::Release()
{
    {
        std::unique_lock<std::mutex> lock(statusLock_);
        status_ = I_STATUS_RELEASED;
    }
    int32_t ret = IStreamManager::GetPlaybackManager().ReleaseRender(streamIndex_);
    
    stream_ = nullptr;
    if (ret < 0) {
        AUDIO_ERR_LOG("Release stream failed, reason: %{public}d", ret);
        status_ = I_STATUS_INVALID;
        return ret;
    }

    return SUCCESS;
}

int32_t RendererInServer::GetAudioTime(uint64_t &framePos, uint64_t &timeStamp)
{
    if (status_ == I_STATUS_STOPPED) {
        AUDIO_WARNING_LOG("Current status is stopped");
        return ERR_ILLEGAL_STATE;
    }
    stream_->GetStreamFramesWritten(framePos);
    stream_->GetCurrentTimeStamp(timeStamp);
    if (resetTime_) {
        resetTime_ = false;
        resetTimestamp_ = timeStamp;
    }
    return SUCCESS;
}

int32_t RendererInServer::GetLatency(uint64_t &latency)
{
    return stream_->GetLatency(latency);
}

int32_t RendererInServer::SetRate(int32_t rate)
{
    return stream_->SetRate(rate);
}

int32_t RendererInServer::SetLowPowerVolume(float volume)
{
    return stream_->SetLowPowerVolume(volume);
}

int32_t RendererInServer::GetLowPowerVolume(float &volume)
{
    return stream_->GetLowPowerVolume(volume);
}

int32_t RendererInServer::SetAudioEffectMode(int32_t effectMode)
{
    return stream_->SetAudioEffectMode(effectMode);
}

int32_t RendererInServer::GetAudioEffectMode(int32_t &effectMode)
{
    return stream_->GetAudioEffectMode(effectMode);
}

int32_t RendererInServer::SetPrivacyType(int32_t privacyType)
{
    return stream_->SetPrivacyType(privacyType);
}

int32_t RendererInServer::GetPrivacyType(int32_t &privacyType)
{
    return stream_->GetPrivacyType(privacyType);
}

int32_t RendererInServer::WriteOneFrame()
{
    // if buffer ready, get buffer, else, return
    size_t minBufferSize = 0;
    if (stream_->GetMinimumBufferSize(minBufferSize) < 0) {
        AUDIO_ERR_LOG("Get min buffer size err");
        return ERR_OPERATION_FAILED;
    }
    BufferDesc bufferDesc = stream_->DequeueBuffer(minBufferSize);
    stream_->EnqueueBuffer(bufferDesc);
    return SUCCESS;
}

int32_t RendererInServer::AbortOneCallback()
{
    std::lock_guard<std::mutex> lock(statusLock_);
    stream_->AbortCallback(1);
    return SUCCESS;
}

int32_t RendererInServer::AbortAllCallback()
{
    std::lock_guard<std::mutex> lock(statusLock_);
    stream_->AbortCallback(5);
    return SUCCESS;
}

std::shared_ptr<OHAudioBuffer> RendererInServer::GetOHSharedBuffer()
{
    return audioServerBuffer_;
}
} // namespace AudioStandard
} // namespace OHOS
