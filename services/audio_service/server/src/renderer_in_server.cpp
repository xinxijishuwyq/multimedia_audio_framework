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
#undef LOG_TAG
#define LOG_TAG "RendererInServer"

#include "renderer_in_server.h"
#include <cinttypes>
#include "securec.h"
#include "audio_errors.h"
#include "audio_log.h"
#include "audio_utils.h"
#include "audio_service.h"
#include "futex_tool.h"
#include "i_stream_manager.h"
#ifdef RESSCHE_ENABLE
#include "res_type.h"
#include "res_sched_client.h"
#endif
#include "volume_tools.h"
#include "policy_handler.h"

namespace OHOS {
namespace AudioStandard {
namespace {
    static constexpr int32_t VOLUME_SHIFT_NUMBER = 16; // 1 >> 16 = 65536, max volume
    static const int64_t MOCK_LATENCY = 45000000; // 45000000 -> 45ms
    static const uint32_t UNDERRUN_LOG_LOOP_COUNT = 100;
    static const int32_t NO_FADING = 0;
    static const int32_t DO_FADINGOUT = 1;
    static const int32_t FADING_OUT_DONE = 2;
    static const int32_t BYTE_LEN_FOR_INVALID_SAMPLE_FORMAT = 0;
    static const int32_t BYTE_LEN_FOR_8BIT = 1;
    static const int32_t BYTE_LEN_FOR_16BIT = 2;
    static const int32_t BYTE_LEN_FOR_24BIT = 3;
    static const int32_t BYTE_LEN_FOR_32BIT = 4;
    static constexpr int32_t ONE_MINUTE = 60;
    const int32_t MEDIA_UID = 1013;
    const float AUDIO_VOLOMUE_EPSILON = 0.0001;
    static const int32_t UINT8_SILENCE_VALUE = 128;
}

RendererInServer::RendererInServer(AudioProcessConfig processConfig, std::weak_ptr<IStreamListener> streamListener)
    : processConfig_(processConfig)
{
    streamListener_ = streamListener;
    managerType_ = PLAYBACK;
    if (processConfig_.callerUid == MEDIA_UID) {
        isNeedFade_ = true;
        oldAppliedVolume_ = MIN_FLOAT_VOLUME;
    }
}

RendererInServer::~RendererInServer()
{
    if (status_ != I_STATUS_RELEASED && status_ != I_STATUS_IDLE) {
        Release();
    }
    DumpFileUtil::CloseDumpFile(&dumpC2S_);
}

int32_t RendererInServer::ConfigServerBuffer()
{
    if (audioServerBuffer_ != nullptr) {
        AUDIO_INFO_LOG("ConfigProcessBuffer: process buffer already configed!");
        return SUCCESS;
    }
    stream_->GetSpanSizePerFrame(spanSizeInFrame_);
    totalSizeInFrame_ = spanSizeInFrame_ * 4; // 4 frames
    stream_->GetByteSizePerFrame(byteSizePerFrame_);
    if (totalSizeInFrame_ == 0 || spanSizeInFrame_ == 0 || totalSizeInFrame_ % spanSizeInFrame_ != 0) {
        AUDIO_ERR_LOG("ConfigProcessBuffer: ERR_INVALID_PARAM");
        return ERR_INVALID_PARAM;
    }

    spanSizeInByte_ = spanSizeInFrame_ * byteSizePerFrame_;
    CHECK_AND_RETURN_RET_LOG(spanSizeInByte_ != 0, ERR_OPERATION_FAILED, "Config oh audio buffer failed");
    AUDIO_INFO_LOG("totalSizeInFrame_: %{public}zu, spanSizeInFrame_: %{public}zu, byteSizePerFrame_:%{public}zu "
        "spanSizeInByte_: %{public}zu,", totalSizeInFrame_, spanSizeInFrame_, byteSizePerFrame_, spanSizeInByte_);

    // create OHAudioBuffer in server
    audioServerBuffer_ = OHAudioBuffer::CreateFromLocal(totalSizeInFrame_, spanSizeInFrame_, byteSizePerFrame_);
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

int32_t RendererInServer::Init()
{
    if (IsHighResolution()) {
        Trace trace("current stream marked as high resolution");
        managerType_ = DIRECT_PLAYBACK;
        AUDIO_INFO_LOG("current stream marked as high resolution");
    }

    if (processConfig_.rendererInfo.rendererFlags == AUDIO_FLAG_VOIP_DIRECT) {
        AUDIO_INFO_LOG("current stream marked as VoIP direct stream");
        managerType_ = VOIP_PLAYBACK;
    }

    int32_t ret = IStreamManager::GetPlaybackManager(managerType_).CreateRender(processConfig_, stream_);
    if (ret != SUCCESS && (managerType_ == DIRECT_PLAYBACK || managerType_ == VOIP_PLAYBACK)) {
        Trace trace("high resolution create failed use normal replace");
        managerType_ = PLAYBACK;
        ret = IStreamManager::GetPlaybackManager(managerType_).CreateRender(processConfig_, stream_);
        AUDIO_DEBUG_LOG("high resolution create failed use normal replace");
    }
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS && stream_ != nullptr, ERR_OPERATION_FAILED,
        "Construct rendererInServer failed: %{public}d", ret);
    streamIndex_ = stream_->GetStreamIndex();
    traceTag_ = "RendererInServer::sessionid:" + std::to_string(streamIndex_);
    ret = ConfigServerBuffer();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED,
        "Construct rendererInServer failed: %{public}d", ret);
    stream_->RegisterStatusCallback(shared_from_this());
    stream_->RegisterWriteCallback(shared_from_this());

    // eg: /data/data/.pulse_dir/100001_48000_2_1_server_in.pcm
    AudioStreamInfo tempInfo = processConfig_.streamInfo;
    std::string dumpName = std::to_string(streamIndex_) + "_" + std::to_string(tempInfo.samplingRate) + "_" +
        std::to_string(tempInfo.channels) + "_" + std::to_string(tempInfo.format) + "_server_in.pcm";
    DumpFileUtil::OpenDumpFile(DUMP_SERVER_PARA, dumpName, &dumpC2S_);

    return SUCCESS;
}

void RendererInServer::OnStatusUpdate(IOperation operation)
{
    AUDIO_DEBUG_LOG("RendererInServer::OnStatusUpdate operation: %{public}d", operation);
    operation_ = operation;
    CHECK_AND_RETURN_LOG(operation != OPERATION_RELEASED, "Stream already released");
    std::shared_ptr<IStreamListener> stateListener = streamListener_.lock();
    CHECK_AND_RETURN_LOG(stateListener != nullptr, "StreamListener is nullptr");
    switch (operation) {
        case OPERATION_STARTED:
            status_ = I_STATUS_STARTED;
            stateListener->OnOperationHandled(START_STREAM, 0);
            break;
        case OPERATION_PAUSED:
            status_ = I_STATUS_PAUSED;
            stateListener->OnOperationHandled(PAUSE_STREAM, 0);
            break;
        case OPERATION_STOPPED:
            status_ = I_STATUS_STOPPED;
            stateListener->OnOperationHandled(STOP_STREAM, 0);
            break;
        case OPERATION_FLUSHED:
            HandleOperationFlushed();
            stateListener->OnOperationHandled(FLUSH_STREAM, 0);
            break;
        case OPERATION_DRAINED:
            status_ = I_STATUS_STARTED;
            stateListener->OnOperationHandled(DRAIN_STREAM, 0);
            afterDrain = true;
            break;
        case OPERATION_RELEASED:
            stateListener->OnOperationHandled(RELEASE_STREAM, 0);
            status_ = I_STATUS_RELEASED;
            break;
        default:
            OnStatusUpdateSub(operation);
    }
}

void RendererInServer::OnStatusUpdateSub(IOperation operation)
{
    std::shared_ptr<IStreamListener> stateListener = streamListener_.lock();
    switch (operation) {
        case OPERATION_UNDERRUN:
            AUDIO_INFO_LOG("Underrun: audioServerBuffer_->GetAvailableDataFrames(): %{public}d",
                audioServerBuffer_->GetAvailableDataFrames());
            // In plan, maxlength is 4
            if (audioServerBuffer_->GetAvailableDataFrames() == static_cast<int32_t>(4 * spanSizeInFrame_)) {
                AUDIO_INFO_LOG("Buffer is empty");
                needForceWrite_ = 0;
            } else {
                AUDIO_INFO_LOG("Buffer is not empty");
                WriteData();
            }
            break;
        case OPERATION_UNDERFLOW:
            underrunCount_++;
            audioServerBuffer_->SetUnderrunCount(underrunCount_);
            break;
        case OPERATION_SET_OFFLOAD_ENABLE:
        case OPERATION_UNSET_OFFLOAD_ENABLE:
            stateListener->OnOperationHandled(SET_OFFLOAD_ENABLE, operation == OPERATION_SET_OFFLOAD_ENABLE ? 1 : 0);
            break;
        default:
            AUDIO_INFO_LOG("Invalid operation %{public}u", operation);
            status_ = I_STATUS_INVALID;
    }
}

void RendererInServer::HandleOperationFlushed()
{
    switch (status_) {
        case I_STATUS_FLUSHING_WHEN_STARTED:
            status_ = I_STATUS_STARTED;
            break;
        case I_STATUS_FLUSHING_WHEN_PAUSED:
            status_ = I_STATUS_PAUSED;
            break;
        case I_STATUS_FLUSHING_WHEN_STOPPED:
            status_ = I_STATUS_STOPPED;
            break;
        default:
            AUDIO_WARNING_LOG("Invalid status before flusing");
    }
}

BufferDesc RendererInServer::DequeueBuffer(size_t length)
{
    return stream_->DequeueBuffer(length);
}

void RendererInServer::DoFadingOutFor8Bit(BufferDesc& bufferDesc, size_t byteLen)
{
    if (byteLen == 0) {
        return;
    }
    uint8_t *data = (uint8_t *)bufferDesc.buffer;
    size_t length = bufferDesc.bufLength / byteLen;
    if (length == 0) {
        return;
    }
    uint32_t numChannels = processConfig_.streamInfo.channels;
    for (size_t i = 0; i < length / numChannels; i++) {
        for (uint32_t j = 0; j < numChannels; j++) {
            float fadeoutRatio = (float)(length - (i * numChannels + j)) / (length);
            data[i * numChannels + j] =
                (data[i * numChannels + j] - UINT8_SILENCE_VALUE) * fadeoutRatio + UINT8_SILENCE_VALUE;
        }
    }
}

void RendererInServer::DoFadingOutFor16Bit(BufferDesc& bufferDesc, size_t byteLen)
{
    if (byteLen == 0) {
        return;
    }
    int16_t *data = (int16_t *)bufferDesc.buffer;
    size_t length = bufferDesc.bufLength / byteLen;
    if (length == 0) {
        return;
    }
    uint32_t numChannels = processConfig_.streamInfo.channels;
    size_t lastPos = 0;
    for (size_t i = 0; i < length / numChannels; i++) {
        for (uint32_t j = 0; j < numChannels; j++) {
            float fadeoutRatio = (float)(length - (i * numChannels + j)) / (length);
            lastPos = i * numChannels + j;
            data[lastPos] *= fadeoutRatio;
        }
    }
    while (lastPos < length) {
        data[lastPos++] = 0;
    }
}

void RendererInServer::DoFadingOutFor24Bit(BufferDesc& bufferDesc, size_t byteLen)
{
    int8_t *data = (int8_t *)bufferDesc.buffer;
    size_t length = bufferDesc.bufLength;
    if (length == 0) {
        return;
    }

    uint32_t numChannels = processConfig_.streamInfo.channels;
    size_t step = byteLen * numChannels;
    size_t lastPos = 0;
    for (size_t i = 0; i < length;) {
        if ((i + step) < length) {
            float fadeoutRatio = (float)(length - i) / (length);
            for (size_t j = 0; j < step; j++) {
                data[i + j] *= fadeoutRatio;
            }
            lastPos = i + step;
        }
        i += step;
    }
    while (lastPos < length) {
        data[lastPos++] = 0;
    }
}

void RendererInServer::DoFadingOutFor32Bit(BufferDesc& bufferDesc, size_t byteLen)
{
    if (byteLen == 0) {
        return;
    }
    int32_t *data = (int32_t *)bufferDesc.buffer;
    size_t length = bufferDesc.bufLength / byteLen;
    if (length == 0) {
        return;
    }
    uint32_t numChannels = processConfig_.streamInfo.channels;
    size_t lastPos = 0;
    for (size_t i = 0; i < length / numChannels; i++) {
        for (uint32_t j = 0; j < numChannels; j++) {
            float fadeoutRatio = (float)(length - (i * numChannels + j)) / (length);
            lastPos = i * numChannels + j;
            data[lastPos] *= fadeoutRatio;
        }
    }
    while (lastPos < length) {
        data[lastPos++] = 0;
    }
}

void RendererInServer::DoFadingOut(BufferDesc& bufferDesc)
{
    std::lock_guard<std::mutex> lock(fadeoutLock_);
    size_t byteLen;

    switch (processConfig_.streamInfo.format) {
        case SAMPLE_U8:
            byteLen = BYTE_LEN_FOR_8BIT;
            break;
        case SAMPLE_S16:
            byteLen = BYTE_LEN_FOR_16BIT;
            break;
        case SAMPLE_S24:
            byteLen = BYTE_LEN_FOR_24BIT;
            break;
        case SAMPLE_S32:
            byteLen = BYTE_LEN_FOR_32BIT;
            break;
        default:
            byteLen = BYTE_LEN_FOR_INVALID_SAMPLE_FORMAT;
            break;
    }

    if (fadeoutFlag_ == DO_FADINGOUT) {
        switch (byteLen) {
            case BYTE_LEN_FOR_8BIT:
                DoFadingOutFor8Bit(bufferDesc, byteLen);
                break;
            case BYTE_LEN_FOR_16BIT:
                DoFadingOutFor16Bit(bufferDesc, byteLen);
                break;
            case BYTE_LEN_FOR_24BIT:
                DoFadingOutFor24Bit(bufferDesc, byteLen);
                break;
            case BYTE_LEN_FOR_32BIT:
                DoFadingOutFor32Bit(bufferDesc, byteLen);
                break;
            default:
                break;
        }
    }
}

void RendererInServer::CheckFadingOutDone(int32_t fadeoutFlag_, BufferDesc& bufferDesc)
{
    std::lock_guard<std::mutex> lock(fadeoutLock_);
    if (fadeoutFlag_ == FADING_OUT_DONE && processConfig_.streamInfo.format == SAMPLE_U8) {
        memset_s(bufferDesc.buffer, bufferDesc.bufLength, 0x80, bufferDesc.bufLength);
        return;
    }
    if (fadeoutFlag_ == FADING_OUT_DONE) {
        memset_s(bufferDesc.buffer, bufferDesc.bufLength, 0, bufferDesc.bufLength);
    }
}

void RendererInServer::WriteMuteDataSysEvent(uint8_t *buffer, size_t bufferSize)
{
    if (silentModeAndMixWithOthers_) {
        return;
    }
    if (buffer[0] == 0) {
        if (startMuteTime_ == 0) {
            startMuteTime_ = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        }
        std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if ((currentTime - startMuteTime_ >= ONE_MINUTE) && silentState_ == 1) { // 1 means unsilent
            silentState_ = 0; // 0 means silent
            AUDIO_WARNING_LOG("write invalid data for some time in server");
            ReportDataToResSched(true);
        }
    } else if (buffer[0] != 0) {
        if (startMuteTime_ != 0) {
            startMuteTime_ = 0;
        }
        if (silentState_ == 0) { // 0 means silent
            AUDIO_WARNING_LOG("begin write valid data in server");
            silentState_ = 1; // 1 means unsilent
            ReportDataToResSched(false);
        }
    }
}

void RendererInServer::ReportDataToResSched(bool isSilent)
{
    #ifdef RESSCHE_ENABLE
    std::unordered_map<std::string, std::string> payload;
    payload["uid"] = std::to_string(processConfig_.appInfo.appUid);
    payload["sessionId"] = std::to_string(streamIndex_);
    payload["isSilent"] = std::to_string(isSilent);
    uint32_t type = ResourceSchedule::ResType::RES_TYPE_AUDIO_RENDERER_SILENT_PLAYBACK;
    ResourceSchedule::ResSchedClient::GetInstance().ReportData(type, 0, payload);
    #endif
}

void RendererInServer::VolumeHandle(BufferDesc &desc)
{
    // volume process in server
    if (audioServerBuffer_ == nullptr) {
        AUDIO_WARNING_LOG("buffer in not inited");
        return;
    }
    float applyVolume = audioServerBuffer_->GetStreamVolume();
    float duckVolume_ = audioServerBuffer_->GetDuckFactor();
    if (!IsVolumeSame(MAX_FLOAT_VOLUME, lowPowerVolume_, AUDIO_VOLOMUE_EPSILON)) {
        applyVolume *= lowPowerVolume_;
    }
    if (!IsVolumeSame(MAX_FLOAT_VOLUME, duckVolume_, AUDIO_VOLOMUE_EPSILON)) {
        applyVolume *= duckVolume_;
    }
    //in plan: put system volume handle here
    if (!IsVolumeSame(MAX_FLOAT_VOLUME, applyVolume, AUDIO_VOLOMUE_EPSILON) ||
        !IsVolumeSame(oldAppliedVolume_, applyVolume, AUDIO_VOLOMUE_EPSILON)) {
        Trace traceVol("RendererInServer::VolumeTools::Process " + std::to_string(oldAppliedVolume_) + "~" +
            std::to_string(applyVolume));
        AudioChannel channel = processConfig_.streamInfo.channels;
        ChannelVolumes mapVols = VolumeTools::GetChannelVolumes(channel, oldAppliedVolume_, applyVolume);
        int32_t volRet = VolumeTools::Process(desc, processConfig_.streamInfo.format, mapVols);
        oldAppliedVolume_ = applyVolume;
        if (volRet != SUCCESS) {
            AUDIO_WARNING_LOG("VolumeTools::Process error: %{public}d", volRet);
        }
    }
}

int32_t RendererInServer::WriteData()
{
    uint64_t currentReadFrame = audioServerBuffer_->GetCurReadFrame();
    uint64_t currentWriteFrame = audioServerBuffer_->GetCurWriteFrame();
    Trace trace1(traceTag_ + " WriteData"); // RendererInServer::sessionid:100001 WriteData
    if (currentReadFrame + spanSizeInFrame_ > currentWriteFrame) {
        if (underRunLogFlag_ == 0) {
            AUDIO_INFO_LOG("near underrun");
        } else if (underRunLogFlag_ == UNDERRUN_LOG_LOOP_COUNT) {
            underRunLogFlag_ = 0;
        }
        underRunLogFlag_++;
        Trace trace2("RendererInServer::Underrun");
        std::shared_ptr<IStreamListener> stateListener = streamListener_.lock();
        CHECK_AND_RETURN_RET_LOG(stateListener != nullptr, ERR_OPERATION_FAILED, "IStreamListener is nullptr");
        FutexTool::FutexWake(audioServerBuffer_->GetFutex());
        return ERR_OPERATION_FAILED;
    }

    BufferDesc bufferDesc = {nullptr, 0, 0}; // will be changed in GetReadbuffer

    if (audioServerBuffer_->GetReadbuffer(currentReadFrame, bufferDesc) == SUCCESS) {
        VolumeHandle(bufferDesc);
        if (processConfig_.streamType != STREAM_ULTRASONIC) {
            DoFadingOut(bufferDesc);
            CheckFadingOutDone(fadeoutFlag_, bufferDesc);
        }
        Trace::CountVolume(traceTag_, *bufferDesc.buffer);
        stream_->EnqueueBuffer(bufferDesc);
        DumpFileUtil::WriteDumpFile(dumpC2S_, static_cast<void *>(bufferDesc.buffer), bufferDesc.bufLength);

        OtherStreamEnqueue(bufferDesc);

        WriteMuteDataSysEvent(bufferDesc.buffer, bufferDesc.bufLength);
        memset_s(bufferDesc.buffer, bufferDesc.bufLength, 0, bufferDesc.bufLength); // clear is needed for reuse.
        // Client may write the buffer immediately after SetCurReadFrame, so put memset_s before it!
        uint64_t nextReadFrame = currentReadFrame + spanSizeInFrame_;
        audioServerBuffer_->SetCurReadFrame(nextReadFrame);
        if (fadeoutFlag_ == DO_FADINGOUT) {
            std::lock_guard<std::mutex> lock(fadeoutLock_);
            fadeoutFlag_ = FADING_OUT_DONE;
            AUDIO_INFO_LOG("fadeoutFlag_ = FADING_OUT_DONE");
        }
    }
    std::shared_ptr<IStreamListener> stateListener = streamListener_.lock();
    CHECK_AND_RETURN_RET_LOG(stateListener != nullptr, SUCCESS, "IStreamListener is nullptr");
    FutexTool::FutexWake(audioServerBuffer_->GetFutex());
    return SUCCESS;
}

void RendererInServer::OtherStreamEnqueue(const BufferDesc &bufferDesc)
{
    // for inner capture
    if (isInnerCapEnabled_) {
        Trace traceDup("RendererInServer::WriteData DupSteam write");
        std::lock_guard<std::mutex> lock(dupMutex_);
        if (dupStream_ != nullptr) {
            dupStream_->EnqueueBuffer(bufferDesc); // what if enqueue fail?
        }
    }
    // for dual tone
    if (isDualToneEnabled_) {
        Trace traceDup("RendererInServer::WriteData DualToneSteam write");
        std::lock_guard<std::mutex> lock(dualToneMutex_);
        if (dualToneStream_ != nullptr) {
            dualToneStream_->EnqueueBuffer(bufferDesc); // what if enqueue fail?
        }
    }
}

void RendererInServer::WriteEmptyData()
{
    Trace trace("RendererInServer::WriteEmptyData");
    AUDIO_WARNING_LOG("Underrun, write empty data");
    BufferDesc bufferDesc = stream_->DequeueBuffer(spanSizeInByte_);
    memset_s(bufferDesc.buffer, bufferDesc.bufLength, 0, bufferDesc.bufLength);
    stream_->EnqueueBuffer(bufferDesc);
    return;
}

int32_t RendererInServer::OnWriteData(size_t length)
{
    Trace trace("RendererInServer::OnWriteData length " + std::to_string(length));
    bool mayNeedForceWrite = false;
    if (writeLock_.try_lock()) {
        // length unit is bytes, using spanSizeInByte_
        for (size_t i = 0; i < length / spanSizeInByte_; i++) {
            mayNeedForceWrite = WriteData() != SUCCESS || mayNeedForceWrite;
        }
        writeLock_.unlock();
    } else {
        mayNeedForceWrite = true;
    }

    size_t maxEmptyCount = 1;
    size_t writableSize = stream_->GetWritableSize();
    if (mayNeedForceWrite && writableSize >= spanSizeInByte_ * maxEmptyCount) {
        AUDIO_DEBUG_LOG("Server need force write to recycle callback");
        needForceWrite_ =
            writableSize / spanSizeInByte_ > 3 ? 0 : 3 - writableSize / spanSizeInByte_; // 3 is maxlength - 1
    }

    uint64_t currentReadFrame = audioServerBuffer_->GetCurReadFrame();
    audioServerBuffer_->SetHandleInfo(currentReadFrame, ClockTime::GetCurNano() + MOCK_LATENCY);
    return SUCCESS;
}

// Call WriteData will hold mainloop lock in EnqueueBuffer, we should not lock a mutex in WriteData while OnWriteData is
// called with mainloop locking.
int32_t RendererInServer::UpdateWriteIndex()
{
    Trace trace("RendererInServer::UpdateWriteIndex");
    if (managerType_ != PLAYBACK) {
        IStreamManager::GetPlaybackManager(managerType_).TriggerStartIfNecessary();
    }
    if (needForceWrite_ < 3 && stream_->GetWritableSize() >= spanSizeInByte_) { // 3 is maxlength - 1
        if (writeLock_.try_lock()) {
            AUDIO_DEBUG_LOG("Start force write data");
            WriteData();
            needForceWrite_++;
            writeLock_.unlock();
        }
    }

    if (afterDrain == true) {
        afterDrain = false;
        AUDIO_DEBUG_LOG("After drain, start write data");
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
    AUDIO_INFO_LOG("Start.");
    needForceWrite_ = 0;
    std::unique_lock<std::mutex> lock(statusLock_);
    if (status_ != I_STATUS_IDLE && status_ != I_STATUS_PAUSED && status_ != I_STATUS_STOPPED) {
        AUDIO_ERR_LOG("RendererInServer::Start failed, Illegal state: %{public}u", status_);
        return ERR_ILLEGAL_STATE;
    }
    status_ = I_STATUS_STARTING;
    {
        std::lock_guard<std::mutex> lock(fadeoutLock_);
        AUDIO_INFO_LOG("fadeoutFlag_ = NO_FADING");
        fadeoutFlag_ = NO_FADING;
    }
    int ret = IStreamManager::GetPlaybackManager(managerType_).StartRender(streamIndex_);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Start stream failed, reason: %{public}d", ret);

    uint64_t currentReadFrame = audioServerBuffer_->GetCurReadFrame();
    int64_t tempTime = ClockTime::GetCurNano() + MOCK_LATENCY;
    audioServerBuffer_->SetHandleInfo(currentReadFrame, tempTime);
    AUDIO_INFO_LOG("Server update position %{public}" PRIu64" time%{public} " PRId64".", currentReadFrame, tempTime);
    resetTime_ = true;

    if (isInnerCapEnabled_) {
        std::lock_guard<std::mutex> lock(dupMutex_);
        if (dupStream_ != nullptr) {
            dupStream_->Start();
        }
    }

    if (isDualToneEnabled_) {
        std::lock_guard<std::mutex> lock(dualToneMutex_);
        if (dualToneStream_ != nullptr) {
            dualToneStream_->Start();
        }
    }
    return SUCCESS;
}

int32_t RendererInServer::Pause()
{
    AUDIO_INFO_LOG("Pause.");
    std::unique_lock<std::mutex> lock(statusLock_);
    if (status_ != I_STATUS_STARTED) {
        AUDIO_ERR_LOG("RendererInServer::Pause failed, Illegal state: %{public}u", status_);
        return ERR_ILLEGAL_STATE;
    }
    status_ = I_STATUS_PAUSING;
    int ret = IStreamManager::GetPlaybackManager(managerType_).PauseRender(streamIndex_);
    if (isInnerCapEnabled_) {
        std::lock_guard<std::mutex> lock(dupMutex_);
        if (dupStream_ != nullptr) {
            dupStream_->Pause();
        }
    }
    if (isDualToneEnabled_) {
        std::lock_guard<std::mutex> lock(dualToneMutex_);
        if (dualToneStream_ != nullptr) {
            dualToneStream_->Pause();
        }
    }
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Pause stream failed, reason: %{public}d", ret);

    return SUCCESS;
}

int32_t RendererInServer::Flush()
{
    AUDIO_INFO_LOG("Flush.");
    Trace trace(traceTag_ + " Flush");
    std::unique_lock<std::mutex> lock(statusLock_);
    if (status_ == I_STATUS_STARTED) {
        status_ = I_STATUS_FLUSHING_WHEN_STARTED;
    } else if (status_ == I_STATUS_PAUSED) {
        status_ = I_STATUS_FLUSHING_WHEN_PAUSED;
    } else if (status_ == I_STATUS_STOPPED) {
        status_ = I_STATUS_FLUSHING_WHEN_STOPPED;
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
        AUDIO_INFO_LOG("On flush, read frame: %{public}" PRIu64 ", nextReadFrame: %{public}zu,"
            "writeFrame: %{public}" PRIu64 "", readFrame, spanSizeInFrame_, writeFrame);
        audioServerBuffer_->SetCurReadFrame(readFrame);
    }

    int ret = stream_->Flush();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Flush stream failed, reason: %{public}d", ret);
    if (isInnerCapEnabled_) {
        std::lock_guard<std::mutex> lock(dupMutex_);
        if (dupStream_ != nullptr) {
            dupStream_->Flush();
        }
    }
    if (isDualToneEnabled_) {
        std::lock_guard<std::mutex> lock(dualToneMutex_);
        if (dualToneStream_ != nullptr) {
            dualToneStream_->Flush();
        }
    }
    return SUCCESS;
}

int32_t RendererInServer::DrainAudioBuffer()
{
    return SUCCESS;
}

int32_t RendererInServer::Drain(bool stopFlag)
{
    {
        std::unique_lock<std::mutex> lock(statusLock_);
        if (status_ != I_STATUS_STARTED) {
            AUDIO_ERR_LOG("RendererInServer::Drain failed, Illegal state: %{public}u", status_);
            return ERR_ILLEGAL_STATE;
        }
        status_ = I_STATUS_DRAINING;
    }
    AUDIO_INFO_LOG("Start drain. stopFlag:%{public}d", stopFlag);
    if (stopFlag) {
        std::lock_guard<std::mutex> lock(fadeoutLock_);
        AUDIO_INFO_LOG("fadeoutFlag_ = DO_FADINGOUT");
        fadeoutFlag_ = DO_FADINGOUT;
    }
    DrainAudioBuffer();
    int ret = stream_->Drain();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Drain stream failed, reason: %{public}d", ret);
    if (isInnerCapEnabled_) {
        std::lock_guard<std::mutex> lock(dupMutex_);
        if (dupStream_ != nullptr) {
            dupStream_->Drain();
        }
    }
    if (isDualToneEnabled_) {
        std::lock_guard<std::mutex> lock(dualToneMutex_);
        if (dualToneStream_ != nullptr) {
            dualToneStream_->Drain();
        }
    }
    return SUCCESS;
}

int32_t RendererInServer::Stop()
{
    {
        std::unique_lock<std::mutex> lock(statusLock_);
        if (status_ != I_STATUS_STARTED && status_ != I_STATUS_PAUSED && status_ != I_STATUS_DRAINING &&
            status_ != I_STATUS_STARTING) {
            AUDIO_ERR_LOG("RendererInServer::Stop failed, Illegal state: %{public}u", status_);
            return ERR_ILLEGAL_STATE;
        }
        status_ = I_STATUS_STOPPING;
    }
    {
        std::lock_guard<std::mutex> lock(fadeoutLock_);
        AUDIO_INFO_LOG("fadeoutFlag_ = NO_FADING");
        fadeoutFlag_ = NO_FADING;
    }
    int ret = IStreamManager::GetPlaybackManager(managerType_).StopRender(streamIndex_);
    if (isInnerCapEnabled_) {
        std::lock_guard<std::mutex> lock(dupMutex_);
        if (dupStream_ != nullptr) {
            dupStream_->Stop();
        }
    }
    if (isDualToneEnabled_) {
        std::lock_guard<std::mutex> lock(dualToneMutex_);
        if (dualToneStream_ != nullptr) {
            dualToneStream_->Stop();
        }
    }
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Stop stream failed, reason: %{public}d", ret);
    return SUCCESS;
}

int32_t RendererInServer::Release()
{
    AudioService::GetInstance()->RemoveRenderer(streamIndex_);
    {
        std::unique_lock<std::mutex> lock(statusLock_);
        if (status_ == I_STATUS_RELEASED) {
            AUDIO_INFO_LOG("Already released");
            return SUCCESS;
        }
    }
    int32_t ret = IStreamManager::GetPlaybackManager(managerType_).ReleaseRender(streamIndex_);
    if (ret < 0) {
        AUDIO_ERR_LOG("Release stream failed, reason: %{public}d", ret);
        status_ = I_STATUS_INVALID;
        return ret;
    }
    status_ = I_STATUS_RELEASED;

    if (isInnerCapEnabled_) {
        DisableInnerCap();
    }

    return SUCCESS;
}

int32_t RendererInServer::GetAudioTime(uint64_t &framePos, uint64_t &timestamp)
{
    if (status_ == I_STATUS_STOPPED) {
        AUDIO_WARNING_LOG("Current status is stopped");
        return ERR_ILLEGAL_STATE;
    }
    stream_->GetStreamFramesWritten(framePos);
    stream_->GetCurrentTimeStamp(timestamp);
    if (resetTime_) {
        resetTime_ = false;
        resetTimestamp_ = timestamp;
    }
    return SUCCESS;
}

int32_t RendererInServer::GetAudioPosition(uint64_t &framePos, uint64_t &timestamp)
{
    if (status_ == I_STATUS_STOPPED) {
        AUDIO_WARNING_LOG("Current status is stopped");
        return ERR_ILLEGAL_STATE;
    }
    stream_->GetCurrentPosition(framePos, timestamp);
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
    if (volume < MIN_FLOAT_VOLUME || volume > MAX_FLOAT_VOLUME) {
        AUDIO_ERR_LOG("invalid volume:%{public}f", volume);
        return ERR_INVALID_PARAM;
    }
    lowPowerVolume_ = volume;
    return SUCCESS;
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

int32_t RendererInServer::EnableInnerCap()
{
    // in plan
    if (isInnerCapEnabled_) {
        AUDIO_INFO_LOG("InnerCap is already enabled");
        return SUCCESS;
    }
    int32_t ret = InitDupStream();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "Init dup stream failed");
    return SUCCESS;
}

int32_t RendererInServer::DisableInnerCap()
{
    std::lock_guard<std::mutex> lock(dupMutex_);
    if (!isInnerCapEnabled_) {
        AUDIO_WARNING_LOG("InnerCap is already disabled.");
        return ERR_INVALID_OPERATION;
    }
    isInnerCapEnabled_ = false;
    AUDIO_INFO_LOG("Disable dup renderer %{public}u with status: %{public}d", streamIndex_, status_);
    // in plan: call stop?
    IStreamManager::GetDupPlaybackManager().ReleaseRender(dupStreamIndex_);
    dupStream_ = nullptr;

    return ERROR;
}

int32_t RendererInServer::InitDupStream()
{
    std::lock_guard<std::mutex> lock(dupMutex_);
    int32_t ret = IStreamManager::GetDupPlaybackManager().CreateRender(processConfig_, dupStream_);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS && dupStream_ != nullptr, ERR_OPERATION_FAILED, "Failed: %{public}d", ret);
    dupStreamIndex_ = dupStream_->GetStreamIndex();

    dupStreamCallback_ = std::make_shared<StreamCallbacks>(dupStreamIndex_);
    dupStream_->RegisterStatusCallback(dupStreamCallback_);
    dupStream_->RegisterWriteCallback(dupStreamCallback_);

    AUDIO_INFO_LOG("Dup Renderer %{public}u with status: %{public}d", streamIndex_, status_);

    isInnerCapEnabled_ = true;

    if (status_ == I_STATUS_STARTED) {
        AUDIO_INFO_LOG("Renderer %{public}u is already running, let's start the dup stream", streamIndex_);
        dupStream_->Start();
    }
    return SUCCESS;
}

int32_t RendererInServer::EnableDualTone()
{
    if (isDualToneEnabled_) {
        AUDIO_INFO_LOG("DualTone is already enabled");
        return SUCCESS;
    }
    int32_t ret = InitDualToneStream();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "Init dual tone stream failed");
    return SUCCESS;
}

int32_t RendererInServer::DisableDualTone()
{
    std::lock_guard<std::mutex> lock(dualToneMutex_);
    if (!isDualToneEnabled_) {
        AUDIO_WARNING_LOG("DualTone is already disabled.");
        return ERR_INVALID_OPERATION;
    }
    isDualToneEnabled_ = false;
    AUDIO_INFO_LOG("Disable dual tone renderer %{public}u with status: %{public}d", dualToneStreamIndex_, status_);
    IStreamManager::GetDupPlaybackManager().ReleaseRender(dualToneStreamIndex_);
    dupStream_ = nullptr;

    return ERROR;
}

int32_t RendererInServer::InitDualToneStream()
{
    std::lock_guard<std::mutex> lock(dualToneMutex_);

    int32_t ret = IStreamManager::GetDualPlaybackManager().CreateRender(processConfig_, dualToneStream_);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS && dualToneStream_ != nullptr,
        ERR_OPERATION_FAILED, "Failed: %{public}d", ret);
    dualToneStreamIndex_ = dualToneStream_->GetStreamIndex();

    dualToneStreamCallback_ = std::make_shared<StreamCallbacks>(dualToneStreamIndex_);
    dualToneStream_->RegisterStatusCallback(dualToneStreamCallback_);
    dualToneStream_->RegisterWriteCallback(dualToneStreamCallback_);

    isDualToneEnabled_ = true;

    if (status_ == I_STATUS_STARTED) {
        AUDIO_INFO_LOG("Renderer %{public}u is already running, let's start the dual stream", dualToneStreamIndex_);
        dualToneStream_->Start();
    }
    return SUCCESS;
}

StreamCallbacks::StreamCallbacks(uint32_t streamIndex) : streamIndex_(streamIndex)
{
    AUDIO_INFO_LOG("DupStream %{public}u create StreamCallbacks", streamIndex_);
}

void StreamCallbacks::OnStatusUpdate(IOperation operation)
{
    AUDIO_INFO_LOG("DupStream %{public}u recv operation: %{public}d", streamIndex_, operation);
}

int32_t StreamCallbacks::OnWriteData(size_t length)
{
    Trace trace("DupStream::OnWriteData length " + std::to_string(length));
    return SUCCESS;
}

int32_t RendererInServer::SetOffloadMode(int32_t state, bool isAppBack)
{
    int32_t ret = stream_->SetOffloadMode(state, isAppBack);
    if (isInnerCapEnabled_) {
        std::lock_guard<std::mutex> lock(dupMutex_);
        if (dupStream_ != nullptr) {
            dupStream_->UpdateMaxLength(350); // 350 for cover offload
        }
    }
    if (isDualToneEnabled_) {
        std::lock_guard<std::mutex> lock(dualToneMutex_);
        if (dualToneStream_ != nullptr) {
            dualToneStream_->UpdateMaxLength(350); // 350 for cover offload
        }
    }
    return ret;
}

int32_t RendererInServer::UnsetOffloadMode()
{
    int32_t ret = stream_->UnsetOffloadMode();
    if (isInnerCapEnabled_) {
        std::lock_guard<std::mutex> lock(dupMutex_);
        if (dupStream_ != nullptr) {
            dupStream_->UpdateMaxLength(20); // 20 for unset offload
        }
    }
    if (isDualToneEnabled_) {
        std::lock_guard<std::mutex> lock(dualToneMutex_);
        if (dualToneStream_ != nullptr) {
            dualToneStream_->UpdateMaxLength(20); // 20 for cover offload
        }
    }
    return ret;
}

int32_t RendererInServer::GetOffloadApproximatelyCacheTime(uint64_t &timestamp, uint64_t &paWriteIndex,
    uint64_t &cacheTimeDsp, uint64_t &cacheTimePa)
{
    return stream_->GetOffloadApproximatelyCacheTime(timestamp, paWriteIndex, cacheTimeDsp, cacheTimePa);
}

int32_t RendererInServer::OffloadSetVolume(float volume)
{
    if (volume < MIN_FLOAT_VOLUME || volume > MAX_FLOAT_VOLUME) {
        AUDIO_ERR_LOG("invalid volume:%{public}f", volume);
        return ERR_INVALID_PARAM;
    }

    AudioVolumeType volumeType = PolicyHandler::GetInstance().GetVolumeTypeFromStreamType(processConfig_.streamType);
    DeviceType deviceType = PolicyHandler::GetInstance().GetActiveOutPutDevice();
    Volume vol = {false, 0.0f, 0};
    PolicyHandler::GetInstance().GetSharedVolume(volumeType, deviceType, vol);
    float systemVol = vol.isMute ? 0.0f : vol.volumeFloat;
    if (PolicyHandler::GetInstance().IsAbsVolumeSupported() &&
        PolicyHandler::GetInstance().GetActiveOutPutDevice() == DEVICE_TYPE_BLUETOOTH_A2DP) {
        systemVol = 1.0f; // 1.0f for a2dp abs volume
    }
    AUDIO_INFO_LOG("sessionId %{public}u set volume:%{public}f [volumeType:%{public}d deviceType:%{public}d systemVol:"
        "%{public}f]", streamIndex_, volume, volumeType, deviceType, systemVol);
    return stream_->OffloadSetVolume(systemVol * volume);
}

int32_t RendererInServer::UpdateSpatializationState(bool spatializationEnabled, bool headTrackingEnabled)
{
    return stream_->UpdateSpatializationState(spatializationEnabled, headTrackingEnabled);
}

int32_t RendererInServer::GetStreamManagerType() const noexcept
{
    return managerType_;
}

bool RendererInServer::IsHighResolution() const noexcept
{
    if ((processConfig_.deviceType == DEVICE_TYPE_WIRED_HEADSET ||
        processConfig_.deviceType == DEVICE_TYPE_USB_HEADSET) &&
        processConfig_.streamType == STREAM_MUSIC && processConfig_.streamInfo.samplingRate >= SAMPLE_RATE_48000 &&
        processConfig_.streamInfo.format >= SAMPLE_S24LE &&
        processConfig_.rendererInfo.pipeType == PIPE_TYPE_DIRECT_MUSIC) {
        if (IStreamManager::GetPlaybackManager(DIRECT_PLAYBACK).GetStreamCount() <= 0) {
            return true;
        }
    }
    Trace trace("RendererInServer::IsHighResolution false");
    return false;
}

int32_t RendererInServer::SetSilentModeAndMixWithOthers(bool on)
{
    silentModeAndMixWithOthers_ = on;
    return SUCCESS;
}

int32_t RendererInServer::SetClientVolume()
{
    if (audioServerBuffer_ == nullptr) {
        AUDIO_WARNING_LOG("buffer in not inited");
        return ERROR;
    }
    float clientVolume = audioServerBuffer_->GetStreamVolume();
    int32_t ret = stream_->SetClientVolume(clientVolume);
    return ret;
}
} // namespace AudioStandard
} // namespace OHOS
