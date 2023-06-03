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

#include "audio_process_in_client.h"

#include <atomic>
#include <cinttypes>
#include <condition_variable>
#include <sstream>
#include <string>
#include <mutex>
#include <thread>

#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "audio_errors.h"
#include "audio_log.h"
#include "audio_system_manager.h"
#include "audio_utils.h"
#include "securec.h"

#include "audio_manager_base.h"
#include "audio_server_death_recipient.h"
#include "i_audio_process.h"
#include "linear_pos_time_model.h"

namespace OHOS {
namespace AudioStandard {
class AudioProcessInClientInner : public AudioProcessInClient {
public:
    explicit AudioProcessInClientInner(const sptr<IAudioProcess> &ipcProxy);
    ~AudioProcessInClientInner();

    int32_t SaveDataCallback(const std::shared_ptr<AudioDataCallback> &dataCallback) override;

    int32_t SaveUnderrunCallback(const std::shared_ptr<ClientUnderrunCallBack> &underrunCallback) override;

    int32_t GetBufferDesc(BufferDesc &bufDesc) const override;

    int32_t Enqueue(const BufferDesc &bufDesc) const override;

    int32_t SetVolume(int32_t vol) override;

    int32_t Start() override;

    int32_t Pause(bool isFlush) override;

    int32_t Resume() override;

    int32_t Stop() override;

    int32_t Release() override;

    bool Init(const AudioProcessConfig &config);

    static const sptr<IStandardAudioService> GetAudioServerProxy();
    static void AudioServerDied(pid_t pid);

private:
    bool InitAudioBuffer();

    bool PrepareCurrent(uint64_t curWritePos);
    void CallClientHandleCurrent();
    bool FinishHandleCurrent(uint64_t &curWritePos, int64_t &clientWriteCost);
    int32_t ReadFromProcessClient() const;
    int32_t RecordReSyncServicePos();
    int32_t RecordPrepareCurrent(uint64_t curReadPos);
    int32_t RecordFinishHandleCurrent(uint64_t &curReadPos, int64_t &clientReadCost);

    void UpdateHandleInfo();
    int64_t GetPredictNextHandleTime(uint64_t posInFrame);
    bool PrepareNext(uint64_t curHandPos, int64_t &wakeUpTime);

    std::string GetStatusInfo(StreamStatus status);
    bool KeepLoopRunning();
    void ProcessCallbackFuc();
    void RecordProcessCallbackFuc();

private:
    static constexpr int64_t ONE_MILLISECOND_DURATION = 1000000; // 1ms
    static constexpr int64_t MAX_WRITE_COST_DUTATION_NANO = 5000000; // 5ms
    static constexpr int64_t MAX_READ_COST_DUTATION_NANO = 5000000; // 5ms
    static constexpr int64_t RECORD_RESYNC_SLEEP_NANO = 2000000; // 2ms
    static constexpr int64_t RECORD_HANDLE_DELAY_NANO = 3000000; // 3ms
    enum ThreadStatus : uint32_t {
        WAITTING = 0,
        SLEEPING,
        INRUNNING,
        INVALID
    };
    AudioProcessConfig processConfig_;
    sptr<IAudioProcess> processProxy_ = nullptr;
    std::shared_ptr<OHAudioBuffer> audioBuffer_ = nullptr;

    uint32_t totalSizeInFrame_ = 0;
    uint32_t spanSizeInFrame_ = 0;
    uint32_t byteSizePerFrame_ = 0;
    size_t spanSizeInByte_ = 0;
    std::weak_ptr<AudioDataCallback> audioDataCallback_;
    std::weak_ptr<ClientUnderrunCallBack> underrunCallback_;

    std::unique_ptr<uint8_t[]> callbackBuffer_ = nullptr;

    std::mutex statusSwitchLock_;
    std::atomic<StreamStatus> *streamStatus_ = nullptr;
    bool isInited_ = false;
    bool needReSyncPosition_ = true;

    int32_t processVolume_ = PROCESS_VOLUME_MAX; // 0 ~ 65536
    LinearPosTimeModel handleTimeModel_;

    std::thread callbackLoop_; // thread for callback to client and write.
    bool isCallbackLoopEnd_ = false;
    std::atomic<ThreadStatus> threadStatus_ = INVALID;
    std::mutex loopThreadLock_;
    std::condition_variable threadStatusCV_;
#ifdef DUMP_CLIENT
    FILE *dcp_ = nullptr;
#endif
};

std::mutex g_audioServerProxyMutex;
sptr<IStandardAudioService> gAudioServerProxy = nullptr;

AudioProcessInClientInner::AudioProcessInClientInner(const sptr<IAudioProcess> &ipcProxy) : processProxy_(ipcProxy)
{
    AUDIO_INFO_LOG("AudioProcessInClient construct.");
}

const sptr<IStandardAudioService> AudioProcessInClientInner::GetAudioServerProxy()
{
    std::lock_guard<std::mutex> lock(g_audioServerProxyMutex);
    if (gAudioServerProxy == nullptr) {
        auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgr == nullptr) {
            AUDIO_ERR_LOG("GetAudioServerProxy: get sa manager failed");
            return nullptr;
        }
        sptr<IRemoteObject> object = samgr->GetSystemAbility(AUDIO_DISTRIBUTED_SERVICE_ID);
        if (object == nullptr) {
            AUDIO_ERR_LOG("GetAudioServerProxy: get audio service remote object failed");
            return nullptr;
        }
        gAudioServerProxy = iface_cast<IStandardAudioService>(object);
        if (gAudioServerProxy == nullptr) {
            AUDIO_ERR_LOG("GetAudioServerProxy: get audio service proxy failed");
            return nullptr;
        }

        // register death recipent to restore proxy
        sptr<AudioServerDeathRecipient> asDeathRecipient = new(std::nothrow) AudioServerDeathRecipient(getpid());
        if (asDeathRecipient != nullptr) {
            asDeathRecipient->SetNotifyCb(std::bind(&AudioProcessInClientInner::AudioServerDied,
                std::placeholders::_1));
            bool result = object->AddDeathRecipient(asDeathRecipient);
            if (!result) {
                AUDIO_ERR_LOG("GetAudioServerProxy: failed to add deathRecipient");
            }
        }
    }
    sptr<IStandardAudioService> gasp = gAudioServerProxy;
    return gasp;
}

/**
 * When AudioServer died, all stream in client should be notified. As they were proxy stream ,the stub stream
 * has been destoried in server.
*/
void AudioProcessInClientInner::AudioServerDied(pid_t pid)
{
    AUDIO_INFO_LOG("audio server died, will restore proxy in next call");
    std::lock_guard<std::mutex> lock(g_audioServerProxyMutex);
    gAudioServerProxy = nullptr;
}

std::shared_ptr<AudioProcessInClient> AudioProcessInClient::Create(const AudioProcessConfig &config)
{
    AUDIO_INFO_LOG("Create with config: render flag %{public}d, capturer flag %{public}d, isRemote %{public}d.",
        config.rendererInfo.rendererFlags, config.capturerInfo.capturerFlags, config.isRemote);
    sptr<IStandardAudioService> gasp = AudioProcessInClientInner::GetAudioServerProxy();
    CHECK_AND_RETURN_RET_LOG(gasp != nullptr, nullptr, "Create failed, can not get service.");
    sptr<IRemoteObject> ipcProxy = gasp->CreateAudioProcess(config);
    CHECK_AND_RETURN_RET_LOG(ipcProxy != nullptr, nullptr, "Create failed with null ipcProxy.");
    sptr<IAudioProcess> iProcessProxy = iface_cast<IAudioProcess>(ipcProxy);
    CHECK_AND_RETURN_RET_LOG(iProcessProxy != nullptr, nullptr, "Create failed when iface_cast.");
    std::shared_ptr<AudioProcessInClientInner> process = std::make_shared<AudioProcessInClientInner>(iProcessProxy);
    if (!process->Init(config)) {
        AUDIO_ERR_LOG("Init failed!");
        process = nullptr;
    }

    return process;
}

AudioProcessInClientInner::~AudioProcessInClientInner()
{
    AUDIO_INFO_LOG("AudioProcessInClient deconstruct.");
    if (callbackLoop_.joinable()) {
        AUDIO_INFO_LOG("AudioProcess join work thread start");
        if (threadStatus_ == WAITTING) {
            threadStatusCV_.notify_all();
        }
        isCallbackLoopEnd_ = true;
        callbackLoop_.join();
        AUDIO_INFO_LOG("AudioProcess join work thread end");
    }
    if (isInited_) {
        AudioProcessInClientInner::Release();
    }
#ifdef DUMP_CLIENT
    if (dcp_) {
        fclose(dcp_);
        dcp_ = nullptr;
    }
#endif
}

bool AudioProcessInClientInner::InitAudioBuffer()
{
    CHECK_AND_RETURN_RET_LOG(processProxy_ != nullptr, false, "Init failed with null ipcProxy.");
    int32_t ret = processProxy_->ResolveBuffer(audioBuffer_);
    if (ret != SUCCESS || audioBuffer_ == nullptr) {
        AUDIO_ERR_LOG("Init failed to call ResolveBuffer");
        return false;
    }
    streamStatus_ = audioBuffer_->GetStreamStatus();
    CHECK_AND_RETURN_RET_LOG(streamStatus_ != nullptr, false, "Init failed, access buffer failed.");

    audioBuffer_->GetSizeParameter(totalSizeInFrame_, spanSizeInFrame_, byteSizePerFrame_);
    spanSizeInByte_ = spanSizeInFrame_ * byteSizePerFrame_;
    AUDIO_INFO_LOG("Using totalSizeInFrame_ %{public}d spanSizeInFrame_ %{public}d byteSizePerFrame_ %{public}d "
        "spanSizeInByte_ %{public}zu", totalSizeInFrame_, spanSizeInFrame_, byteSizePerFrame_, spanSizeInByte_);

    callbackBuffer_ = std::make_unique<uint8_t[]>(spanSizeInByte_);
    CHECK_AND_RETURN_RET_LOG(callbackBuffer_ != nullptr, false, "Init callbackBuffer_ failed.");
    memset_s(callbackBuffer_.get(), spanSizeInByte_, 0, spanSizeInByte_);

    return true;
}

bool AudioProcessInClientInner::Init(const AudioProcessConfig &config)
{
    AUDIO_INFO_LOG("Call Init.");
    bool isBufferInited = InitAudioBuffer();
    CHECK_AND_RETURN_RET_LOG(isBufferInited, isBufferInited, "%{public}s init audio buffer fail.", __func__);
    processConfig_ = config;

    bool ret = handleTimeModel_.ConfigSampleRate(processConfig_.streamInfo.samplingRate);
    CHECK_AND_RETURN_RET_LOG(ret != false, false, "Init LinearPosTimeModel failed.");
    uint64_t handlePos = 0;
    int64_t handleTime = 0;
    audioBuffer_->GetHandleInfo(handlePos, handleTime);
    handleTimeModel_.ResetFrameStamp(handlePos, handleTime);

    streamStatus_->store(StreamStatus::STREAM_IDEL);
    if (config.audioMode == AUDIO_MODE_RECORD) {
        callbackLoop_ = std::thread(&AudioProcessInClientInner::RecordProcessCallbackFuc, this);
        pthread_setname_np(callbackLoop_.native_handle(), "AudioProcessRecordCb");
    } else {
        callbackLoop_ = std::thread(&AudioProcessInClientInner::ProcessCallbackFuc, this);
        pthread_setname_np(callbackLoop_.native_handle(), "AudioProcessCb");
    }

    int waitThreadStartTime = 5; // wait for thread start.
    while (threadStatus_.load() == INVALID) {
        AUDIO_INFO_LOG("%{public}s wait %{public}d ms for %{public}s started...", __func__, waitThreadStartTime,
            config.audioMode == AUDIO_MODE_RECORD ? "RecordProcessCallbackFuc" : "ProcessCallbackFuc");
        ClockTime::RelativeSleep(ONE_MILLISECOND_DURATION * waitThreadStartTime);
    }

#ifdef DUMP_CLIENT
    std::stringstream strStream;
    std::string dumpPatch;
    strStream << "/data/local/tmp/client-" << processConfig_.appInfo.appUid << ".pcm";
    strStream >> dumpPatch;
    AUDIO_INFO_LOG("Client dump using path: %{public}s with uid:%{public}d", dumpPatch.c_str(),
        processConfig_.appInfo.appUid);

    dcp_ = fopen(dumpPatch.c_str(), "a+");
    CHECK_AND_BREAK_LOG(dcp_ != nullptr, "Error opening pcm test file!");
#endif
    isInited_ = true;
    return true;
}

int32_t AudioProcessInClientInner::SaveDataCallback(const std::shared_ptr<AudioDataCallback> &dataCallback)
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");

    if (dataCallback == nullptr) {
        AUDIO_ERR_LOG("%{public}s data callback is null.", __func__);
        return ERR_INVALID_PARAM;
    }
    audioDataCallback_ = dataCallback;
    AUDIO_INFO_LOG("%{public}s end.", __func__);
    return SUCCESS;
}

int32_t AudioProcessInClientInner::SaveUnderrunCallback(const std::shared_ptr<ClientUnderrunCallBack> &underrunCallback)
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");

    if (underrunCallback == nullptr) {
        AUDIO_ERR_LOG("%{public}s underrun callback is null.", __func__);
        return ERR_INVALID_PARAM;
    }
    underrunCallback_ = underrunCallback;
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    return SUCCESS;
}

int32_t AudioProcessInClientInner::ReadFromProcessClient() const
{
    CHECK_AND_RETURN_RET_LOG(audioBuffer_ != nullptr, ERR_INVALID_HANDLE,
        "%{public}s audio buffer is null.", __func__);
    uint64_t curReadPos = audioBuffer_->GetCurReadFrame();

    BufferDesc readbufDesc = {nullptr, 0, 0};
    int32_t ret = audioBuffer_->GetReadbuffer(curReadPos, readbufDesc);
    if (ret != SUCCESS || readbufDesc.buffer == nullptr || readbufDesc.bufLength != spanSizeInByte_ ||
        readbufDesc.dataLength != spanSizeInByte_) {
        AUDIO_ERR_LOG("%{public}s get client mmap read buffer failed, ret %{public}d.", __func__, ret);
        return ERR_OPERATION_FAILED;
    }
    ret = memcpy_s(static_cast<void *>(callbackBuffer_.get()), spanSizeInByte_,
        static_cast<void *>(readbufDesc.buffer), spanSizeInByte_);
    CHECK_AND_RETURN_RET_LOG(ret == EOK, ERR_OPERATION_FAILED, "%{public}s memcpy fail, ret %{public}d,"
        " spanSizeInByte %{public}zu.", __func__, ret, spanSizeInByte_);
#ifdef DUMP_CLIENT
    if (dcp_ != nullptr) {
        fwrite((void *)bufDesc.buffer, 1, spanSizeInByte_, dcp_);
    }
#endif

    ret = memset_s(readbufDesc.buffer, readbufDesc.bufLength, 0, readbufDesc.bufLength);
    CHECK_AND_BREAK_LOG(ret == EOK, "%{public}s reset buffer fail, ret %{public}d.", __func__, ret);
    return SUCCESS;
}

// the buffer will be used by client
int32_t AudioProcessInClientInner::GetBufferDesc(BufferDesc &bufDesc) const
{
    AUDIO_INFO_LOG("%{public}s enter, audio mode:%{public}d", __func__, processConfig_.audioMode);
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "%{public}s not inited!", __func__);

    if (processConfig_.audioMode == AUDIO_MODE_RECORD) {
        ReadFromProcessClient();
    }

    bufDesc.buffer = callbackBuffer_.get();
    bufDesc.dataLength = spanSizeInByte_;
    bufDesc.bufLength = spanSizeInByte_;
    return SUCCESS;
}

int32_t AudioProcessInClientInner::Enqueue(const BufferDesc &bufDesc) const
{
    Trace trace("AudioProcessInClient::Enqueue");
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "%{public}s not inited!", __func__);

    if (bufDesc.buffer == nullptr || bufDesc.bufLength != spanSizeInByte_ || bufDesc.dataLength != spanSizeInByte_) {
        AUDIO_ERR_LOG("%{public}s bufDesc error, bufLen %{public}zu, dataLen %{public}zu, spanSize %{public}zu.",
            __func__, bufDesc.bufLength, bufDesc.dataLength, spanSizeInByte_);
        return ERR_INVALID_PARAM;
    }
    // check if this buffer is form us.
    CHECK_AND_BREAK_LOG(bufDesc.buffer == callbackBuffer_.get(),
        "%{public}s the buffer is not created by client.", __func__);

    AUDIO_INFO_LOG("%{public}s bufDesc check end, audio mode:%{public}d", __func__, processConfig_.audioMode);
    if (processConfig_.audioMode == AUDIO_MODE_PLAYBACK) {
        BufferDesc curWriteBuffer = {nullptr, 0, 0};
        uint64_t curWritePos = audioBuffer_->GetCurWriteFrame();
        int32_t ret = audioBuffer_->GetWriteBuffer(curWritePos, curWriteBuffer);
        if (ret != SUCCESS || curWriteBuffer.buffer == nullptr || curWriteBuffer.bufLength != spanSizeInByte_ ||
            curWriteBuffer.dataLength != spanSizeInByte_) {
            AUDIO_ERR_LOG("%{public}s get write buffer fail, ret:%{public}d", __func__, ret);
            return ERR_OPERATION_FAILED;
        }

        ret = memcpy_s(static_cast<void *>(curWriteBuffer.buffer), spanSizeInByte_, static_cast<void *>(bufDesc.buffer),
            spanSizeInByte_);
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "Copy data failed!");
#ifdef DUMP_CLIENT
        if (dcp_ != nullptr) {
            fwrite(static_cast<void *>(bufDesc.buffer), 1, spanSizeInByte_, dcp_);
        }
#endif
    }

    CHECK_AND_BREAK_LOG(memset_s(callbackBuffer_.get(), spanSizeInByte_, 0, spanSizeInByte_) == EOK,
        "%{public}s reset callback buffer fail.", __func__);

    return SUCCESS;
}

int32_t AudioProcessInClientInner::SetVolume(int32_t vol)
{
    AUDIO_INFO_LOG("SetVolume to %{public}d", vol);
    Trace trace("AudioProcessInClient::SetVolume");
    if (vol < 0 || vol > PROCESS_VOLUME_MAX) {
        AUDIO_ERR_LOG("SetVolume failed, invalid volume:%{public}d", vol);
        return ERR_INVALID_PARAM;
    }
    processVolume_ = vol;
    return SUCCESS;
}

int32_t AudioProcessInClientInner::Start()
{
    Trace traceWithLog("AudioProcessInClient::Start", true);
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");

    std::lock_guard<std::mutex> lock(statusSwitchLock_);
    if (streamStatus_->load() == StreamStatus::STREAM_RUNNING) {
        AUDIO_INFO_LOG("Start find already started");
        return SUCCESS;
    }

    StreamStatus targetStatus = StreamStatus::STREAM_IDEL;
    if (!streamStatus_->compare_exchange_strong(targetStatus, StreamStatus::STREAM_STARTING)) {
        AUDIO_ERR_LOG("Start failed, invalid status : %{public}s", GetStatusInfo(targetStatus).c_str());
        return ERR_ILLEGAL_STATE;
    }

    if (processProxy_->Start() != SUCCESS) {
        streamStatus_->store(StreamStatus::STREAM_IDEL);
        AUDIO_ERR_LOG("Start failed to call process proxy, reset status to IDEL.");
        threadStatusCV_.notify_all();
        return ERR_OPERATION_FAILED;
    }
    UpdateHandleInfo();
    streamStatus_->store(StreamStatus::STREAM_RUNNING);
    threadStatusCV_.notify_all();
    return SUCCESS;
}

int32_t AudioProcessInClientInner::Pause(bool isFlush)
{
    Trace traceWithLog("AudioProcessInClient::Pause", true);
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");

    std::lock_guard<std::mutex> lock(statusSwitchLock_);
    if (streamStatus_->load() == StreamStatus::STREAM_PAUSED) {
        AUDIO_INFO_LOG("Pause find already paused");
        return SUCCESS;
    }
    StreamStatus targetStatus = StreamStatus::STREAM_RUNNING;
    if (!streamStatus_->compare_exchange_strong(targetStatus, StreamStatus::STREAM_PAUSING)) {
        AUDIO_ERR_LOG("Pause failed, invalid status : %{public}s", GetStatusInfo(targetStatus).c_str());
        return ERR_ILLEGAL_STATE;
    }

    if (processProxy_->Pause(isFlush) != SUCCESS) {
        streamStatus_->store(StreamStatus::STREAM_RUNNING);
        AUDIO_ERR_LOG("Pause failed to call process proxy, reset status to RUNNING");
        threadStatusCV_.notify_all(); // avoid thread blocking with status PAUSING
        return ERR_OPERATION_FAILED;
    }
    streamStatus_->store(StreamStatus::STREAM_PAUSED);

    return SUCCESS;
}

int32_t AudioProcessInClientInner::Resume()
{
    Trace traceWithLog("AudioProcessInClient::Resume", true);
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");
    std::lock_guard<std::mutex> lock(statusSwitchLock_);

    if (streamStatus_->load() == StreamStatus::STREAM_RUNNING) {
        AUDIO_INFO_LOG("Resume find already running");
        return SUCCESS;
    }

    StreamStatus targetStatus = StreamStatus::STREAM_PAUSED;
    if (!streamStatus_->compare_exchange_strong(targetStatus, StreamStatus::STREAM_STARTING)) {
        AUDIO_ERR_LOG("Resume failed, invalid status : %{public}s", GetStatusInfo(targetStatus).c_str());
        return ERR_ILLEGAL_STATE;
    }

    if (processProxy_->Resume() != SUCCESS) {
        streamStatus_->store(StreamStatus::STREAM_PAUSED);
        AUDIO_ERR_LOG("Resume failed to call process proxy, reset status to PAUSED.");
        threadStatusCV_.notify_all();
        return ERR_OPERATION_FAILED;
    }
    UpdateHandleInfo();
    streamStatus_->store(StreamStatus::STREAM_RUNNING);
    threadStatusCV_.notify_all();

    return SUCCESS;
}

int32_t AudioProcessInClientInner::Stop()
{
    Trace traceWithLog("AudioProcessInClient::Stop", true);
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");
    std::lock_guard<std::mutex> lock(statusSwitchLock_);
    if (streamStatus_->load() == StreamStatus::STREAM_STOPPED) {
        AUDIO_INFO_LOG("Stop find already stopped");
        return SUCCESS;
    }

    StreamStatus oldStatus = streamStatus_->load();
    if (oldStatus == STREAM_IDEL || oldStatus == STREAM_RELEASED || oldStatus == STREAM_INVALID) {
        AUDIO_ERR_LOG("Stop failed, invalid status : %{public}s", GetStatusInfo(oldStatus).c_str());
        return ERR_ILLEGAL_STATE;
    }

    streamStatus_->store(StreamStatus::STREAM_STOPPING);
    if (processProxy_->Stop() != SUCCESS) {
        streamStatus_->store(oldStatus);
        AUDIO_ERR_LOG("Stop failed in server, reset status to %{public}s", GetStatusInfo(oldStatus).c_str());
        threadStatusCV_.notify_all(); // avoid thread blocking with status RUNNING
        return ERR_OPERATION_FAILED;
    }
    isCallbackLoopEnd_ = true;
    threadStatusCV_.notify_all();

    streamStatus_->store(StreamStatus::STREAM_STOPPED);
    AUDIO_INFO_LOG("Success stop form %{public}s", GetStatusInfo(oldStatus).c_str());
    return SUCCESS;
}

int32_t AudioProcessInClientInner::Release()
{
    Trace traceWithLog("AudioProcessInClient::Release", true);
    CHECK_AND_RETURN_RET_LOG(isInited_, ERR_ILLEGAL_STATE, "not inited!");

    // need any check?
    Stop();
    std::lock_guard<std::mutex> lock(statusSwitchLock_);
    if (streamStatus_->load() != StreamStatus::STREAM_STOPPED) {
        AUDIO_ERR_LOG("Process is not stopped");
        return ERR_ILLEGAL_STATE;
    }

    if (processProxy_->Release() != SUCCESS) {
        AUDIO_ERR_LOG("Release may failed in server");
        threadStatusCV_.notify_all(); // avoid thread blocking with status RUNNING
        return ERR_OPERATION_FAILED;
    }

    streamStatus_->store(StreamStatus::STREAM_RELEASED);
    AUDIO_INFO_LOG("Success Released");
    isInited_ = false;
    return SUCCESS;
}

// client should call GetBufferDesc and Enqueue in OnHandleData
void AudioProcessInClientInner::CallClientHandleCurrent()
{
    Trace trace("AudioProcessInClient::CallClientHandleCurrent");
    std::shared_ptr<AudioDataCallback> cb = audioDataCallback_.lock();
    if (cb == nullptr) {
        AUDIO_ERR_LOG("%{public}s audio data callback is null.", __func__);
        return;
    }

    cb->OnHandleData(spanSizeInByte_);
}

void AudioProcessInClientInner::UpdateHandleInfo()
{
    Trace traceSync("AudioProcessInClient::UpdateHandleInfo");
    uint64_t serverHandlePos = 0;
    int64_t serverHandleTime = 0;
    int32_t ret = processProxy_->RequestHandleInfo();
    CHECK_AND_RETURN_LOG(ret == SUCCESS, "RequestHandleInfo failed ret:%{public}d", ret);
    audioBuffer_->GetHandleInfo(serverHandlePos, serverHandleTime);

    handleTimeModel_.UpdataFrameStamp(serverHandlePos, serverHandleTime);
}

int64_t AudioProcessInClientInner::GetPredictNextHandleTime(uint64_t posInFrame)
{
    Trace trace("AudioProcessInClient::GetPredictNextRead");
    uint64_t handleSpanCnt = posInFrame / spanSizeInFrame_;
    uint32_t startPeriodCnt = 20; // sync each time when start
    uint32_t oneBigPeriodCnt = 40; // 200ms
    if (handleSpanCnt < startPeriodCnt || handleSpanCnt % oneBigPeriodCnt == 0) {
        UpdateHandleInfo();
    }

    int64_t nextHandleTime = handleTimeModel_.GetTimeOfPos(posInFrame);

    return nextHandleTime;
}

bool AudioProcessInClientInner::PrepareNext(uint64_t curHandPos, int64_t &wakeUpTime)
{
    Trace trace("AudioProcessInClient::PrepareNext");
    int64_t handleModifyTime = 0;
    if (processConfig_.audioMode == AUDIO_MODE_RECORD) {
        handleModifyTime = RECORD_HANDLE_DELAY_NANO;
    } else {
        handleModifyTime = -ONE_MILLISECOND_DURATION;
    }

    int64_t nextServerHandleTime = GetPredictNextHandleTime(curHandPos) + handleModifyTime;
    AUDIO_DEBUG_LOG("%{public}s mode %{public}d, curHandPos %{public}" PRIu64", nextServerHandleTime "
        "%{public}" PRIu64".", __func__, processConfig_.audioMode, curHandPos, nextServerHandleTime);
    if (nextServerHandleTime < ClockTime::GetCurNano()) {
        wakeUpTime = ClockTime::GetCurNano() + ONE_MILLISECOND_DURATION; // make sure less than duration
    } else {
        wakeUpTime = nextServerHandleTime;
    }
    AUDIO_INFO_LOG("%{public}s end, audioMode %{public}d, curReadPos %{public}" PRIu64", wakeUpTime "
        "%{public}" PRIu64".", __func__, processConfig_.audioMode, curHandPos, wakeUpTime);
    return true;
}

std::string AudioProcessInClientInner::GetStatusInfo(StreamStatus status)
{
    switch (status) {
        case STREAM_IDEL:
            return "STREAM_IDEL";
        case STREAM_STARTING:
            return "STREAM_STARTING";
        case STREAM_RUNNING:
            return "STREAM_RUNNING";
        case STREAM_PAUSING:
            return "STREAM_PAUSING";
        case STREAM_PAUSED:
            return "STREAM_PAUSED";
        case STREAM_STOPPING:
            return "STREAM_STOPPING";
        case STREAM_STOPPED:
            return "STREAM_STOPPED";
        case STREAM_RELEASED:
            return "STREAM_RELEASED";
        case STREAM_INVALID:
            return "STREAM_INVALID";
        default:
            break;
    }
    return "NO_SUCH_STATUS";
}

bool AudioProcessInClientInner::KeepLoopRunning()
{
    StreamStatus targetStatus = STREAM_INVALID;

    switch (streamStatus_->load()) {
        case STREAM_RUNNING:
            return true;
        case STREAM_STARTING:
            targetStatus = STREAM_RUNNING;
            break;
        case STREAM_IDEL:
            targetStatus = STREAM_STARTING;
            break;
        case STREAM_PAUSING:
            targetStatus = STREAM_PAUSED;
            break;
        case STREAM_PAUSED:
            targetStatus = STREAM_STARTING;
            break;
        case STREAM_STOPPING:
            targetStatus = STREAM_STOPPED;
            break;
        case STREAM_STOPPED:
            targetStatus = STREAM_RELEASED;
            break;
        default:
            break;
    }

    Trace trace("AudioProcessInClient::InWaitStatus");
    std::unique_lock<std::mutex> lock(loopThreadLock_);
    AUDIO_INFO_LOG("Process status is %{public}s now, wait for %{public}s...",
        GetStatusInfo(streamStatus_->load()).c_str(), GetStatusInfo(targetStatus).c_str());
    threadStatus_ = WAITTING;
    threadStatusCV_.wait(lock);
    AUDIO_INFO_LOG("Process wait end. Cur is %{public}s now, target is %{public}s...",
        GetStatusInfo(streamStatus_->load()).c_str(), GetStatusInfo(targetStatus).c_str());

    return false;
}

void AudioProcessInClientInner::RecordProcessCallbackFuc()
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    AudioSystemManager::GetInstance()->RequestThreadPriority(gettid());
    uint64_t curReadPos = 0;
    int64_t wakeUpTime = ClockTime::GetCurNano();
    int64_t clientReadCost = 0;

    while (!isCallbackLoopEnd_ && audioBuffer_ != nullptr) {
        if (!KeepLoopRunning()) {
            continue;
        }
        threadStatus_ = INRUNNING;
        AUDIO_INFO_LOG("%{public}s thread running.", __func__);
        if (needReSyncPosition_ && RecordReSyncServicePos() == SUCCESS) {
            wakeUpTime = ClockTime::GetCurNano();
            needReSyncPosition_ = false;
            continue;
        }
        int64_t curTime = ClockTime::GetCurNano();
        if (curTime - wakeUpTime > ONE_MILLISECOND_DURATION) {
            AUDIO_WARNING_LOG("%{public}s wake up too late.", __func__);
            wakeUpTime = curTime;
        }

        curReadPos = audioBuffer_->GetCurReadFrame();
        if (RecordPrepareCurrent(curReadPos) != SUCCESS) {
            AUDIO_ERR_LOG("%{public}s prepare current fail.", __func__);
            continue;
        }
        CallClientHandleCurrent();
        if (RecordFinishHandleCurrent(curReadPos, clientReadCost) != SUCCESS) {
            AUDIO_ERR_LOG("%{public}s finish handle current fail.", __func__);
            continue;
        }

        if (!PrepareNext(curReadPos, wakeUpTime)) {
            AUDIO_ERR_LOG("%{public}s prepare next loop in process fail.", __func__);
            break;
        }

        threadStatus_ = SLEEPING;
        curTime = ClockTime::GetCurNano();
        if (wakeUpTime > curTime && wakeUpTime - curTime < MAX_READ_COST_DUTATION_NANO + clientReadCost) {
            ClockTime::AbsoluteSleep(wakeUpTime);
        } else {
            Trace trace("RecordBigWakeUpTime");
            AUDIO_WARNING_LOG("%{public}s wakeUpTime is too late...", __func__);
            ClockTime::RelativeSleep(MAX_READ_COST_DUTATION_NANO);
        }
    }
    AUDIO_INFO_LOG("%{public}s end.", __func__);
}

int32_t AudioProcessInClientInner::RecordReSyncServicePos()
{
    CHECK_AND_RETURN_RET_LOG(processProxy_ != nullptr && audioBuffer_ != nullptr, ERR_INVALID_HANDLE,
        "%{public}s process proxy or audio buffer is null.", __func__);
    uint64_t serverHandlePos = 0;
    int64_t serverHandleTime = 0;
    int32_t tryTimes = 3;
    int32_t ret = 0;
    while (tryTimes > 0) {
        ret = processProxy_->RequestHandleInfo();
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "%{public}s request handle info fail, ret %{public}d.",
            __func__, ret);

        CHECK_AND_RETURN_RET_LOG(audioBuffer_->GetHandleInfo(serverHandlePos, serverHandleTime), ERR_OPERATION_FAILED,
            "%{public}s get handle info fail.", __func__);
        if (serverHandlePos > 0) {
            break;
        }
        ClockTime::RelativeSleep(MAX_READ_COST_DUTATION_NANO);
        tryTimes--;
    }
    AUDIO_INFO_LOG("%{public}s get handle info OK, tryTimes %{public}d, serverHandlePos %{public}" PRIu64", "
        "serverHandleTime %{public}" PRId64".", __func__, tryTimes, serverHandlePos, serverHandleTime);
    ClockTime::AbsoluteSleep(serverHandleTime + RECORD_HANDLE_DELAY_NANO);

    ret = audioBuffer_->SetCurReadFrame(serverHandlePos);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "%{public}s set curReadPos fail, ret %{public}d.", __func__, ret);
    return SUCCESS;
}

int32_t AudioProcessInClientInner::RecordPrepareCurrent(uint64_t curReadPos)
{
    CHECK_AND_RETURN_RET_LOG(audioBuffer_ != nullptr, ERR_INVALID_HANDLE,
        "%{public}s audio buffer is null.", __func__);
    SpanInfo *curReadSpan = audioBuffer_->GetSpanInfo(curReadPos);
    CHECK_AND_RETURN_RET_LOG(curReadSpan != nullptr, ERR_INVALID_HANDLE,
        "%{public}s get read span info of process client fail.", __func__);

    int tryCount = 10;
    SpanStatus targetStatus = SpanStatus::SPAN_WRITE_DONE;
    while (!curReadSpan->spanStatus.compare_exchange_strong(targetStatus, SpanStatus::SPAN_READING)
        && tryCount > 0) {
        AUDIO_INFO_LOG("%{public}s unready, curReadSpan %{public}" PRIu64", curSpanStatus %{public}d, wait 2ms.",
            __func__, curReadPos, curReadSpan->spanStatus.load());
        targetStatus = SpanStatus::SPAN_WRITE_DONE;
        tryCount--;
        ClockTime::RelativeSleep(RECORD_RESYNC_SLEEP_NANO);
    }
    CHECK_AND_RETURN_RET_LOG(tryCount > 0, ERR_INVALID_READ,
        "%{public}s wait too long, curReadSpan %{public}" PRIu64".", __func__, curReadPos);

    curReadSpan->readStartTime = ClockTime::GetCurNano();
    return SUCCESS;
}

int32_t AudioProcessInClientInner::RecordFinishHandleCurrent(uint64_t &curReadPos, int64_t &clientReadCost)
{
    CHECK_AND_RETURN_RET_LOG(audioBuffer_ != nullptr, ERR_INVALID_HANDLE,
        "%{public}s audio buffer is null.", __func__);
    SpanInfo *curReadSpan = audioBuffer_->GetSpanInfo(curReadPos);
    CHECK_AND_RETURN_RET_LOG(curReadSpan != nullptr, ERR_INVALID_HANDLE,
        "%{public}s get read span info of process client fail.", __func__);

    SpanStatus targetStatus = SpanStatus::SPAN_READING;
    if (!curReadSpan->spanStatus.compare_exchange_strong(targetStatus, SpanStatus::SPAN_READ_DONE)) {
        AUDIO_INFO_LOG("%{public}s status error, curReadSpan %{public}" PRIu64", curSpanStatus %{public}d.",
            __func__, curReadPos, curReadSpan->spanStatus.load());
        return ERR_INVALID_OPERATION;
    }
    curReadSpan->readDoneTime = ClockTime::GetCurNano();

    clientReadCost = curReadSpan->readDoneTime - curReadSpan->readStartTime;
    if (clientReadCost > MAX_READ_COST_DUTATION_NANO) {
        AUDIO_WARNING_LOG("Client write cost too long...");
    }

    uint64_t nextWritePos = curReadPos + spanSizeInFrame_;
    int32_t ret = audioBuffer_->SetCurReadFrame(nextWritePos);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "%{public}s set next hand frame %{public}" PRIu64" fail, "
        "ret %{public}d.", __func__, nextWritePos, ret);
    curReadPos = nextWritePos;

    return SUCCESS;
}

bool AudioProcessInClientInner::PrepareCurrent(uint64_t curWritePos)
{
    SpanInfo *tempSpan = audioBuffer_->GetSpanInfo(curWritePos);
    if (tempSpan == nullptr) {
        AUDIO_ERR_LOG("GetSpanInfo failed!");
        return false;
    }

    int tryCount = 10; // try 10 * 2 = 20ms
    SpanStatus targetStatus = SpanStatus::SPAN_READ_DONE;
    while (!tempSpan->spanStatus.compare_exchange_strong(targetStatus, SpanStatus::SPAN_WRITTING) && tryCount-- > 0) {
        AUDIO_WARNING_LOG("current span %{public}" PRIu64" is not ready, status is %{public}d, wait 2ms.", curWritePos,
            targetStatus);
        targetStatus = SpanStatus::SPAN_READ_DONE;
        ClockTime::RelativeSleep(ONE_MILLISECOND_DURATION + ONE_MILLISECOND_DURATION);
    }
    if (tryCount <= 0) {
        AUDIO_ERR_LOG("wait on current span  %{public}" PRIu64" too long...", curWritePos);
        return false;
    }
    tempSpan->writeStartTime = ClockTime::GetCurNano();
    return true;
}

bool AudioProcessInClientInner::FinishHandleCurrent(uint64_t &curWritePos, int64_t &clientWriteCost)
{
    SpanInfo *tempSpan = audioBuffer_->GetSpanInfo(curWritePos);
    if (tempSpan == nullptr) {
        AUDIO_ERR_LOG("GetSpanInfo failed!");
        return false;
    }

    int32_t ret = ERROR;
    // mark status write-done and then server can read
    SpanStatus targetStatus = SpanStatus::SPAN_WRITTING;
    if (tempSpan->spanStatus.load() == targetStatus) {
        uint64_t nextWritePos = curWritePos + spanSizeInFrame_;
        ret = audioBuffer_->SetCurWriteFrame(nextWritePos); // move ahead before writedone
        curWritePos = nextWritePos;
        tempSpan->spanStatus.store(SpanStatus::SPAN_WRITE_DONE);
    }
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("SetCurWriteFrame %{public}" PRIu64" failed, ret:%{public}d", curWritePos, ret);
        return false;
    }
    tempSpan->writeDoneTime = ClockTime::GetCurNano();
    tempSpan->volumeStart = processVolume_;
    tempSpan->volumeEnd = processVolume_;
    clientWriteCost = tempSpan->writeDoneTime - tempSpan->writeStartTime;
    if (clientWriteCost > MAX_WRITE_COST_DUTATION_NANO) {
        AUDIO_WARNING_LOG("Client write cost too long...");
        // todo
        // handle write time out: send underrun msg to client, reset time model with latest server handle time.
    }

    return true;
}

void AudioProcessInClientInner::ProcessCallbackFuc()
{
    AUDIO_INFO_LOG("AudioProcessInClient Callback loop start.");
    AudioSystemManager::GetInstance()->RequestThreadPriority(gettid());

    uint64_t curWritePos = 0;
    int64_t curTime = 0;
    int64_t wakeUpTime = ClockTime::GetCurNano();
    int64_t clientWriteCost = 0;

    while (!isCallbackLoopEnd_) {
        if (!KeepLoopRunning()) {
            continue;
        }
        threadStatus_ = INRUNNING;
        Trace traceLoop("AudioProcessInClient::InRunning");
        curTime = ClockTime::GetCurNano();
        if (curTime - wakeUpTime > ONE_MILLISECOND_DURATION) {
            AUDIO_WARNING_LOG("Wake up too late...");
            wakeUpTime = curTime;
        }
        curWritePos = audioBuffer_->GetCurWriteFrame();
        if (!PrepareCurrent(curWritePos)) {
            AUDIO_ERR_LOG("PrepareCurrent failed!");
            continue;
        }
        // call client write
        CallClientHandleCurrent();

        // client write done, check if time out
        if (!FinishHandleCurrent(curWritePos, clientWriteCost)) {
            AUDIO_ERR_LOG("FinishHandleCurrent failed!");
            continue;
        }

        // prepare next sleep
        if (!PrepareNext(curWritePos, wakeUpTime)) {
            AUDIO_ERR_LOG("PrepareNextLoop in process failed!");
            break;
        }

        traceLoop.End();
        // start safe sleep
        threadStatus_ = SLEEPING;
        curTime = ClockTime::GetCurNano();
        if (wakeUpTime > curTime && wakeUpTime - curTime < MAX_WRITE_COST_DUTATION_NANO + clientWriteCost) {
            ClockTime::AbsoluteSleep(wakeUpTime);
        } else {
            Trace trace("BigWakeUpTime");
            AUDIO_WARNING_LOG("wakeUpTime is too late...");
            ClockTime::RelativeSleep(MAX_WRITE_COST_DUTATION_NANO);
        }
    }
    AUDIO_INFO_LOG("AudioProcessInClient Callback loop end.");
}
} // namespace AudioStandard
} // namespace OHOS
