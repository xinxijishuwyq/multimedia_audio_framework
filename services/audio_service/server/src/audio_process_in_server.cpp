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

#include "audio_process_in_server.h"

#include "securec.h"

#include "audio_errors.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
namespace {
    static constexpr int32_t VOLUME_SHIFT_NUMBER = 16; // 1 >> 16 = 65536, max volume
}
sptr<AudioProcessInServer> AudioProcessInServer::Create(const AudioProcessConfig &processConfig)
{
    sptr<AudioProcessInServer> process = new(std::nothrow) AudioProcessInServer(processConfig);

    return process;
}

AudioProcessInServer::AudioProcessInServer(const AudioProcessConfig &processConfig) : processConfig_(processConfig)
{
    AUDIO_INFO_LOG("AudioProcessInServer()");
}

AudioProcessInServer::~AudioProcessInServer()
{
    AUDIO_INFO_LOG("~AudioProcessInServer()");
}

int32_t AudioProcessInServer::ResolveBuffer(std::shared_ptr<OHAudioBuffer> &buffer)
{
    if (!isBufferConfiged_) {
        AUDIO_ERR_LOG("ResolveBuffer failed, buffer is not configed.");
        return ERR_ILLEGAL_STATE;
    }

    buffer = processBuffer_;
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, ERR_ILLEGAL_STATE, "ResolveBuffer failed, processBuffer_ is null.");

    return SUCCESS;
}

int32_t AudioProcessInServer::Start()
{
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");
    if (streamStatus_->load() != STREAM_STARTING) {
        AUDIO_ERR_LOG("Start failed, invalid status.");
        return ERR_ILLEGAL_STATE;
    }

    for (size_t i = 0; i < listenerList_.size(); i++) {
        listenerList_[i]->OnStart(this);
    }

    streamStatus_->store(STREAM_RUNNING);
    AUDIO_INFO_LOG("Start in server success!");
    return SUCCESS;
}

int32_t AudioProcessInServer::RequestHandleInfo()
{
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");
    CHECK_AND_RETURN_RET_LOG(processBuffer_ != nullptr, ERR_ILLEGAL_STATE, "buffer not inited!");

    for (size_t i = 0; i < listenerList_.size(); i++) {
        listenerList_[i]->OnUpdateHandleInfo(this);
    }
    return SUCCESS;
}

int32_t AudioProcessInServer::Pause(bool isFlush)
{
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");
    // todo
    (void)isFlush;
    return SUCCESS;
}

int32_t AudioProcessInServer::Resume()
{
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");
    // todo
    return SUCCESS;
}

int32_t AudioProcessInServer::Stop()
{
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");
    // todo
    return SUCCESS;
}

int32_t AudioProcessInServer::Release()
{
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");

    isInited_ = false;
    // todo
    return SUCCESS;
}

std::shared_ptr<OHAudioBuffer> AudioProcessInServer::GetStreamBuffer()
{
    if (!isBufferConfiged_ || processBuffer_ == nullptr) {
        AUDIO_ERR_LOG("GetStreamBuffer failed:process buffer not config.");
        return nullptr;
    }
    return processBuffer_;
}

AudioStreamInfo AudioProcessInServer::GetStreamInfo()
{
    return processConfig_.streamInfo;
}

inline uint32_t PcmFormatToBits(AudioSampleFormat format)
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

int32_t AudioProcessInServer::InitBufferStatus()
{
    if (processBuffer_ == nullptr) {
        AUDIO_ERR_LOG("InitBufferStatus failed, null buffer.");
        return ERR_ILLEGAL_STATE;
    }

    uint32_t spanCount = processBuffer_->GetSpanCount();
    for (uint32_t i = 0; i < spanCount; i++) {
        SpanInfo *spanInfo = processBuffer_->GetSpanInfoByIndex(i);
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

int32_t AudioProcessInServer::ConfigProcessBuffer(uint32_t &totalSizeInframe, uint32_t &spanSizeInframe)
{
    if (processBuffer_ != nullptr) {
        AUDIO_INFO_LOG("ConfigProcessBuffer: process buffer already configed!");
        return SUCCESS;
    }
    // check
    if (totalSizeInframe == 0 || spanSizeInframe == 0 || totalSizeInframe % spanSizeInframe != 0) {
        AUDIO_ERR_LOG("ConfigProcessBuffer failed: ERR_INVALID_PARAM");
        return ERR_INVALID_PARAM;
    }
    totalSizeInframe_ = totalSizeInframe;
    spanSizeInframe_ = spanSizeInframe;

    uint32_t channel = processConfig_.streamInfo.channels;
    uint32_t formatbyte = PcmFormatToBits(processConfig_.streamInfo.format);
    byteSizePerFrame_ = channel * formatbyte;

    // create OHAudioBuffer in server.
    processBuffer_ = OHAudioBuffer::CreateFormLocal(totalSizeInframe_, spanSizeInframe_, byteSizePerFrame_);
    CHECK_AND_RETURN_RET_LOG(processBuffer_ != nullptr, ERR_OPERATION_FAILED, "Create process buffer failed.");

    if (processBuffer_->GetBufferHolder() != AudioBufferHolder::AUDIO_SERVER_SHARED) {
        AUDIO_ERR_LOG("CreateFormLocal in server failed.");
        return ERR_ILLEGAL_STATE;
    }
    AUDIO_INFO_LOG("Config: totalSizeInframe:%{public}d spanSizeInframe:%{public}d byteSizePerFrame:%{public}d",
        totalSizeInframe_, spanSizeInframe_, byteSizePerFrame_);

    // we need to clear data buffer to avoid dirty data.
    memset_s(processBuffer_->GetDataBase(), processBuffer_->GetDataSize(), 0, processBuffer_->GetDataSize());
    int32_t ret = InitBufferStatus();
    AUDIO_INFO_LOG("clear data buffer, ret:%{public}d", ret);

    streamStatus_ = processBuffer_->GetStreamStatus();
    CHECK_AND_RETURN_RET_LOG(streamStatus_ != nullptr, ERR_OPERATION_FAILED, "Create process buffer failed.");
    isBufferConfiged_ = true;
    isInited_ = true;
    return SUCCESS;
}

int32_t AudioProcessInServer::AddProcessStatusListener(std::shared_ptr<IProcessStatusListener> listener)
{
    listenerList_.push_back(listener);
    return SUCCESS;
}

int32_t AudioProcessInServer::RemoveProcessStatusListener(std::shared_ptr<IProcessStatusListener> listener)
{
    std::vector<std::shared_ptr<IProcessStatusListener>>::iterator it = listenerList_.begin();
    while (it != listenerList_.end()) {
        if (*it == listener) {
            listenerList_.erase(it);
            break;
        } else {
            it++;
        }
    }

    return SUCCESS;
}
} // namespace AudioStandard
} // namespace OHOS
