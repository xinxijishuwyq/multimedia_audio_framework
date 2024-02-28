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
#include "policy_handler.h"

#include "securec.h"

#include "audio_errors.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
namespace {
    static constexpr int32_t VOLUME_SHIFT_NUMBER = 16; // 1 >> 16 = 65536, max volume
}

sptr<AudioProcessInServer> AudioProcessInServer::Create(const AudioProcessConfig &processConfig,
    ProcessReleaseCallback *releaseCallback)
{
    sptr<AudioProcessInServer> process = new(std::nothrow) AudioProcessInServer(processConfig, releaseCallback);

    return process;
}

AudioProcessInServer::AudioProcessInServer(const AudioProcessConfig &processConfig,
    ProcessReleaseCallback *releaseCallback) : processConfig_(processConfig), releaseCallback_(releaseCallback)
{
    sessionId_ = PolicyHandler::GetInstance().GenerateSessionId(processConfig_.appInfo.appUid);
}

AudioProcessInServer::~AudioProcessInServer()
{
    AUDIO_INFO_LOG("~AudioProcessInServer()");
}

int32_t AudioProcessInServer::GetSessionId(uint32_t &sessionId)
{
    sessionId = sessionId_;
    return SUCCESS;
}

int32_t AudioProcessInServer::ResolveBuffer(std::shared_ptr<OHAudioBuffer> &buffer)
{
    AUDIO_INFO_LOG("ResolveBuffer start");
    CHECK_AND_RETURN_RET_LOG(isBufferConfiged_, ERR_ILLEGAL_STATE,
        "ResolveBuffer failed, buffer is not configed.");

    if (processBuffer_ == nullptr) {
        AUDIO_ERR_LOG("ResolveBuffer failed, buffer is nullptr.");
    }
    buffer = processBuffer_;
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, ERR_ILLEGAL_STATE, "ResolveBuffer failed, processBuffer_ is null.");

    return SUCCESS;
}

int32_t AudioProcessInServer::RequestHandleInfo(bool isAsync)
{
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");
    CHECK_AND_RETURN_RET_LOG(processBuffer_ != nullptr, ERR_ILLEGAL_STATE, "buffer not inited!");

    for (size_t i = 0; i < listenerList_.size(); i++) {
        listenerList_[i]->OnUpdateHandleInfo(this);
    }
    return SUCCESS;
}

int32_t AudioProcessInServer::Start()
{
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");

    std::lock_guard<std::mutex> lock(statusLock_);
    CHECK_AND_RETURN_RET_LOG(streamStatus_->load() == STREAM_STARTING,
        ERR_ILLEGAL_STATE, "Start failed, invalid status.");

    for (size_t i = 0; i < listenerList_.size(); i++) {
        listenerList_[i]->OnStart(this);
    }

    AUDIO_INFO_LOG("Start in server success!");
    return SUCCESS;
}

int32_t AudioProcessInServer::Pause(bool isFlush)
{
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");

    (void)isFlush;
    std::lock_guard<std::mutex> lock(statusLock_);
    CHECK_AND_RETURN_RET_LOG(streamStatus_->load() == STREAM_PAUSING,
        ERR_ILLEGAL_STATE, "Pause failed, invalid status.");

    for (size_t i = 0; i < listenerList_.size(); i++) {
        listenerList_[i]->OnPause(this);
    }

    AUDIO_INFO_LOG("Pause in server success!");
    return SUCCESS;
}

int32_t AudioProcessInServer::Resume()
{
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");
    std::lock_guard<std::mutex> lock(statusLock_);
    CHECK_AND_RETURN_RET_LOG(streamStatus_->load() == STREAM_STARTING,
        ERR_ILLEGAL_STATE, "Resume failed, invalid status.");

    for (size_t i = 0; i < listenerList_.size(); i++) {
        listenerList_[i]->OnStart(this);
    }

    AUDIO_INFO_LOG("Resume in server success!");
    return SUCCESS;
}

int32_t AudioProcessInServer::Stop()
{
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");

    std::lock_guard<std::mutex> lock(statusLock_);
    CHECK_AND_RETURN_RET_LOG(streamStatus_->load() == STREAM_STOPPING,
        ERR_ILLEGAL_STATE, "Stop failed, invalid status.");

    for (size_t i = 0; i < listenerList_.size(); i++) {
        listenerList_[i]->OnPause(this); // notify endpoint?
    }

    AUDIO_INFO_LOG("Stop in server success!");
    return SUCCESS;
}

int32_t AudioProcessInServer::Release()
{
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited or already released");

    isInited_ = false;
    std::lock_guard<std::mutex> lock(statusLock_);
    CHECK_AND_RETURN_RET_LOG(releaseCallback_ != nullptr, ERR_OPERATION_FAILED, "Failed: no service to notify.");

    int32_t ret = releaseCallback_->OnProcessRelease(this);
    AUDIO_INFO_LOG("notify service release result: %{public}d", ret);
    return SUCCESS;
}

ProcessDeathRecipient::ProcessDeathRecipient(AudioProcessInServer *processInServer,
    ProcessReleaseCallback *processHolder)
{
    processInServer_ = processInServer;
    processHolder_ = processHolder;
}

void ProcessDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    CHECK_AND_RETURN_LOG(processHolder_ != nullptr, "processHolder_ is null.");
    int32_t ret = processHolder_->OnProcessRelease(processInServer_);
    AUDIO_INFO_LOG("OnRemoteDied, call release ret: %{public}d", ret);
}

int32_t AudioProcessInServer::RegisterProcessCb(sptr<IRemoteObject> object)
{
    sptr<IProcessCb> processCb = iface_cast<IProcessCb>(object);
    CHECK_AND_RETURN_RET_LOG(processCb != nullptr, ERR_INVALID_PARAM, "RegisterProcessCb obj cast failed");
    bool result = object->AddDeathRecipient(new ProcessDeathRecipient(this, releaseCallback_));
    CHECK_AND_RETURN_RET_LOG(result, ERR_OPERATION_FAILED, "AddDeathRecipient failed.");

    return SUCCESS;
}

int AudioProcessInServer::Dump(int fd, const std::vector<std::u16string> &args)
{
    return SUCCESS;
}

void AudioProcessInServer::Dump(std::stringstream &dumpStringStream)
{
    dumpStringStream << std::endl << "uid:" << processConfig_.appInfo.appUid;
    dumpStringStream << " pid:" << processConfig_.appInfo.appPid << std::endl;
    dumpStringStream << " process info:" << std::endl;
    dumpStringStream << " stream info:" << std::endl;
    dumpStringStream << "   samplingRate:" << processConfig_.streamInfo.samplingRate << std::endl;
    dumpStringStream << "   channels:" << processConfig_.streamInfo.channels << std::endl;
    dumpStringStream << "   format:" << processConfig_.streamInfo.format << std::endl;
    dumpStringStream << "   encoding:" << processConfig_.streamInfo.encoding << std::endl;
    if (streamStatus_ != nullptr) {
        dumpStringStream << "Status:" << streamStatus_->load() << std::endl;
    }
    dumpStringStream << std::endl;
}

std::shared_ptr<OHAudioBuffer> AudioProcessInServer::GetStreamBuffer()
{
    CHECK_AND_RETURN_RET_LOG(isBufferConfiged_ && processBuffer_ != nullptr,
        nullptr, "GetStreamBuffer failed:process buffer not config.");
    return processBuffer_;
}

AudioStreamInfo AudioProcessInServer::GetStreamInfo()
{
    return processConfig_.streamInfo;
}

AudioStreamType AudioProcessInServer::GetAudioStreamType()
{
    return processConfig_.streamType;
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
    CHECK_AND_RETURN_RET_LOG(processBuffer_ != nullptr, ERR_ILLEGAL_STATE,
        "InitBufferStatus failed, null buffer.");

    uint32_t spanCount = processBuffer_->GetSpanCount();
    for (uint32_t i = 0; i < spanCount; i++) {
        SpanInfo *spanInfo = processBuffer_->GetSpanInfoByIndex(i);
        CHECK_AND_RETURN_RET_LOG(spanInfo != nullptr, ERR_ILLEGAL_STATE,
            "InitBufferStatus failed, null spaninfo");
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

int32_t AudioProcessInServer::ConfigProcessBuffer(uint32_t &totalSizeInframe,
    uint32_t &spanSizeInframe, const std::shared_ptr<OHAudioBuffer> &buffer)
{
    if (processBuffer_ != nullptr) {
        AUDIO_INFO_LOG("ConfigProcessBuffer: process buffer already configed!");
        return SUCCESS;
    }
    // check
    CHECK_AND_RETURN_RET_LOG(totalSizeInframe != 0 && spanSizeInframe != 0 && totalSizeInframe % spanSizeInframe == 0,
        ERR_INVALID_PARAM, "ConfigProcessBuffer failed: ERR_INVALID_PARAM");
    totalSizeInframe_ = totalSizeInframe;
    spanSizeInframe_ = spanSizeInframe;

    uint32_t channel = processConfig_.streamInfo.channels;
    uint32_t formatbyte = PcmFormatToBits(processConfig_.streamInfo.format);
    byteSizePerFrame_ = channel * formatbyte;

    if (buffer == nullptr) {
        // create OHAudioBuffer in server.
        processBuffer_ = OHAudioBuffer::CreateFromLocal(totalSizeInframe_, spanSizeInframe_, byteSizePerFrame_);
        CHECK_AND_RETURN_RET_LOG(processBuffer_ != nullptr, ERR_OPERATION_FAILED, "Create process buffer failed.");

        CHECK_AND_RETURN_RET_LOG(processBuffer_->GetBufferHolder() == AudioBufferHolder::AUDIO_SERVER_SHARED,
            ERR_ILLEGAL_STATE, "CreateFormLocal in server failed.");
        AUDIO_INFO_LOG("Config: totalSizeInframe:%{public}d spanSizeInframe:%{public}d byteSizePerFrame:%{public}d",
            totalSizeInframe_, spanSizeInframe_, byteSizePerFrame_);

        // we need to clear data buffer to avoid dirty data.
        memset_s(processBuffer_->GetDataBase(), processBuffer_->GetDataSize(), 0, processBuffer_->GetDataSize());
        int32_t ret = InitBufferStatus();
        AUDIO_DEBUG_LOG("clear data buffer, ret:%{public}d", ret);
    } else {
        processBuffer_ = buffer;
        AUDIO_INFO_LOG("ConfigBuffer in server separate, base: %{public}d", *processBuffer_->GetDataBase());
    }

    streamStatus_ = processBuffer_->GetStreamStatus();
    CHECK_AND_RETURN_RET_LOG(streamStatus_ != nullptr, ERR_OPERATION_FAILED, "Create process buffer failed.");
    isBufferConfiged_ = true;
    isInited_ = true;
    return SUCCESS;
}

int32_t AudioProcessInServer::AddProcessStatusListener(std::shared_ptr<IProcessStatusListener> listener)
{
    std::lock_guard<std::mutex> lock(listenerListLock_);
    listenerList_.push_back(listener);
    return SUCCESS;
}

int32_t AudioProcessInServer::RemoveProcessStatusListener(std::shared_ptr<IProcessStatusListener> listener)
{
    std::lock_guard<std::mutex> lock(listenerListLock_);
    std::vector<std::shared_ptr<IProcessStatusListener>>::iterator it = listenerList_.begin();
    bool isFind = false;
    while (it != listenerList_.end()) {
        if (*it == listener) {
            listenerList_.erase(it);
            isFind = true;
            break;
        } else {
            it++;
        }
    }

    AUDIO_INFO_LOG("%{public}s the endpoint.", (isFind ? "find and remove" : "not find"));
    return SUCCESS;
}
} // namespace AudioStandard
} // namespace OHOS
