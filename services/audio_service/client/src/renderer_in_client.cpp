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
#ifndef FAST_AUDIO_STREAM_H
#define FAST_AUDIO_STREAM_H

#include "renderer_in_client.h"

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
#include "audio_manager_base.h"
#include "audio_log.h"
#include "audio_ring_cache.h"
#include "audio_channel_blend.h"
#include "audio_server_death_recipient.h"
#include "audio_stream_tracker.h"
#include "audio_system_manager.h"
#include "audio_utils.h"
#include "ipc_stream_listener_stub.h"
#include "volume_ramp.h"

namespace OHOS {
namespace AudioStandard {
namespace {
static const size_t MAX_CLIENT_WRITE_SIZE = 20 * 1024 * 1024; // 20M
static const int32_t CREATE_TIMEOUT_IN_SECOND = 5; // 5S
static const int32_t OPERATION_TIMEOUT_IN_MS = 500; // 500ms
}
class RendererStreamListener;
class RendererInClientInner : public RendererInClient, public IStreamListener,
    public std::enable_shared_from_this<RendererInClientInner> {
public:
    RendererInClientInner(AudioStreamType eStreamType, int32_t appUid);
    ~RendererInClientInner();

    // IStreamListener
    int32_t OnOperationHandled(Operation operation, int64_t result) override;

    // IAudioStream
    void SetClientID(int32_t clientPid, int32_t clientUid) override;

    void SetRendererInfo(const AudioRendererInfo &rendererInfo) override;
    void SetCapturerInfo(const AudioCapturerInfo &capturerInfo) override;
    int32_t SetAudioStreamInfo(const AudioStreamParams info,
        const std::shared_ptr<AudioClientTracker> &proxyObj) override;
    int32_t GetAudioStreamInfo(AudioStreamParams &info) override;
    bool CheckRecordingCreate(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid) override;
    bool CheckRecordingStateChange(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid,
        AudioPermissionState state) override;
    int32_t GetAudioSessionID(uint32_t &sessionID) override;
    State GetState() override;
    bool GetAudioTime(Timestamp &timestamp, Timestamp::Timestampbase base) override;
    int32_t GetBufferSize(size_t &bufferSize) override;
    int32_t GetFrameCount(uint32_t &frameCount) override;
    int32_t GetLatency(uint64_t &latency) override;
    int32_t SetAudioStreamType(AudioStreamType audioStreamType) override;
    int32_t SetVolume(float volume) override;
    float GetVolume() override;
    int32_t SetRenderRate(AudioRendererRate renderRate) override;
    AudioRendererRate GetRenderRate() override;
    int32_t SetStreamCallback(const std::shared_ptr<AudioStreamCallback> &callback) override;

    // callback mode api
    int32_t SetRenderMode(AudioRenderMode renderMode) override;
    AudioRenderMode GetRenderMode() override;
    int32_t SetRendererWriteCallback(const std::shared_ptr<AudioRendererWriteCallback> &callback) override;
    int32_t SetCaptureMode(AudioCaptureMode captureMode) override;
    AudioCaptureMode GetCaptureMode() override;
    int32_t SetCapturerReadCallback(const std::shared_ptr<AudioCapturerReadCallback> &callback) override;
    int32_t GetBufferDesc(BufferDesc &bufDesc) override;
    int32_t GetBufQueueState(BufferQueueState &bufState) override;
    int32_t Enqueue(const BufferDesc &bufDesc) override;
    int32_t Clear() override;

    int32_t SetLowPowerVolume(float volume) override;
    float GetLowPowerVolume() override;
    int32_t SetOffloadMode(int32_t state, bool isAppBack) override;
    int32_t UnsetOffloadMode() override;
    float GetSingleStreamVolume() override;
    AudioEffectMode GetAudioEffectMode() override;
    int32_t SetAudioEffectMode(AudioEffectMode effectMode) override;
    int64_t GetFramesWritten() override;
    int64_t GetFramesRead() override;

    void SetInnerCapturerState(bool isInnerCapturer) override;
    void SetWakeupCapturerState(bool isWakeupCapturer) override;
    void SetCapturerSource(int capturerSource) override;
    void SetPrivacyType(AudioPrivacyType privacyType) override;

    // Common APIs
    bool StartAudioStream(StateChangeCmdType cmdType = CMD_FROM_CLIENT) override;
    bool PauseAudioStream(StateChangeCmdType cmdType = CMD_FROM_CLIENT) override;
    bool StopAudioStream() override;
    bool ReleaseAudioStream(bool releaseRunner = true) override;
    bool FlushAudioStream() override;

    // Playback related APIs
    bool DrainAudioStream() override;
    int32_t Write(uint8_t *buffer, size_t buffer_size) override;

    // Recording related APIs
    int32_t Read(uint8_t &buffer, size_t userSize, bool isBlockingRead) override;

    uint32_t GetUnderflowCount() override;
    void SetRendererPositionCallback(int64_t markPosition, const std::shared_ptr<RendererPositionCallback> &callback)
        override;
    void UnsetRendererPositionCallback() override;
    void SetRendererPeriodPositionCallback(int64_t markPosition,
        const std::shared_ptr<RendererPeriodPositionCallback> &callback) override;
    void UnsetRendererPeriodPositionCallback() override;
    void SetCapturerPositionCallback(int64_t markPosition, const std::shared_ptr<CapturerPositionCallback> &callback)
        override;
    void UnsetCapturerPositionCallback() override;
    void SetCapturerPeriodPositionCallback(int64_t markPosition,
        const std::shared_ptr<CapturerPeriodPositionCallback> &callback) override;
    void UnsetCapturerPeriodPositionCallback() override;
    int32_t SetRendererSamplingRate(uint32_t sampleRate) override;
    uint32_t GetRendererSamplingRate() override;
    int32_t SetBufferSizeInMsec(int32_t bufferSizeInMsec) override;
    void SetApplicationCachePath(const std::string cachePath) override;
    int32_t SetChannelBlendMode(ChannelBlendMode blendMode) override;
    int32_t SetVolumeWithRamp(float volume, int32_t duration) override;

    void SetStreamTrackerState(bool trackerRegisteredState) override;
    void GetSwitchInfo(IAudioStream::SwitchInfo& info) override;

    IAudioStream::StreamClass GetStreamClass() override;

    static const sptr<IStandardAudioService> GetAudioServerProxy();
    static void AudioServerDied(pid_t pid);
private:
    void RegisterTracker(const std::shared_ptr<AudioClientTracker> &proxyObj);
    void UpdateTracker(const std::string &updateCase);

    int32_t DeinitIpcStream();

    int32_t InitIpcStream();

    const AudioProcessConfig ConstructConfig();

    int32_t InitSharedBuffer();
    int32_t InitCacheBuffer(size_t targetSize);

    int32_t DrainAudioCache();

    int32_t WritCacheData();
private:
    AudioStreamType eStreamType_;
    int32_t appUid_;
    uint32_t sessionId_;
    int32_t clientPid_ = -1;
    int32_t clientUid_ = -1;

    std::unique_ptr<AudioStreamTracker> audioStreamTracker_;

    AudioRendererInfo rendererInfo_;
    AudioCapturerInfo capturerInfo_; // not in use

    AudioPrivacyType privacyType_ = PRIVACY_TYPE_PUBLIC;
    bool streamTrackerRegistered_ = false;

    bool needSetThreadPriority_ = true;
    uint32_t writeThreadId_ = 0; // 0 is invalid

    AudioStreamParams streamParams_; // in plan: replace it with AudioRendererParams

    // for data process
    bool isBlendSet_ = false;
    AudioBlend audioBlend_;
    VolumeRamp volumeRamp_;

    // callbacks
    std::mutex streamCbMutex_;
    std::weak_ptr<AudioStreamCallback> streamCallback_;

    size_t cacheSizeInByte_ = 0;
    size_t clientSpanSizeInByte_ = 0;
    size_t sizePerFrameInByte_ = 4; // 16bit 2ch as default

    int32_t bufferSizeInMsec_ = 20; // 20ms
    std::string cachePath_;

    // callback mode
    AudioRenderMode renderMode_ = RENDER_MODE_NORMAL;

    std::atomic<State> state_ = INVALID;
    // using this lock when change status_
    std::mutex statusMutex_;
    // for status operation wait and notify
    std::mutex callServerMutex_;
    std::condition_variable callServerCV_;

    Operation notifiedOperation_ = MAX_OPERATION_CODE;
    int64_t notifiedResult_ = 0;

    // write data
    std::mutex writeDataMutex_;
    std::condition_variable writeDataCV_;

    float clientVolume_ = 1.0;

    uint64_t clientWrittenBytes_ = 0;
    uint32_t underrunCount_ = 0;
    // ipc stream related
    sptr<RendererStreamListener> listener_ = nullptr;
    sptr<IpcStream> ipcStream_ = nullptr;
    std::shared_ptr<OHAudioBuffer> clientBuffer_ = nullptr;

    // buffer handle
    std::unique_ptr<AudioRingCache> ringCache_ = nullptr;
    std::mutex writeMutex_; // used for prevent multi thread call write
};

// RendererStreamListener --> sptr | RendererInClientInner --> shared_ptr
class RendererStreamListener : public IpcStreamListenerStub {
public:
    RendererStreamListener(std::shared_ptr<RendererInClientInner> rendererInClinet);
    virtual ~RendererStreamListener() = default;

    // IpcStreamListenerStub
    int32_t OnOperationHandled(Operation operation, int64_t result) override;
private:
    std::weak_ptr<RendererInClientInner> rendererInClinet_;
};

RendererStreamListener::RendererStreamListener(std::shared_ptr<RendererInClientInner> rendererInClinet)
{
    if (rendererInClinet == nullptr) {
        AUDIO_ERR_LOG("RendererStreamListener() find null rendererInClinet");
    }
    rendererInClinet_ = rendererInClinet;
}

int32_t RendererStreamListener::OnOperationHandled(Operation operation, int64_t result)
{
    std::shared_ptr<RendererInClientInner> render = rendererInClinet_.lock();
    if(render == nullptr) {
        AUDIO_WARNING_LOG("OnOperationHandled() find rendererInClinet is null, operation:%{public}d result:"
            "%{public}" PRId64".", operation, result);
        return ERR_ILLEGAL_STATE;
    }
    return render->OnOperationHandled(operation, result);
}

std::shared_ptr<RendererInClient> RendererInClient::GetInstance(AudioStreamType eStreamType, int32_t appUid)
{
    return std::make_shared<RendererInClientInner>(eStreamType, appUid);
}

RendererInClientInner::RendererInClientInner(AudioStreamType eStreamType, int32_t appUid)
    : eStreamType_(eStreamType), appUid_(appUid)
{
    AUDIO_INFO_LOG("RendererInClientInner() with appUid:%{public}d", appUid_);
    audioStreamTracker_ = std::make_unique<AudioStreamTracker>(AUDIO_MODE_PLAYBACK, appUid);
    state_ = NEW;
}

RendererInClientInner::~RendererInClientInner()
{
    AUDIO_INFO_LOG("~RendererInClientInner()");
}

int32_t RendererInClientInner::OnOperationHandled(Operation operation, int64_t result)
{
    // read/write operation may print many log, use debug.
    if (operation == UPDATE_STREAM) {
        AUDIO_DEBUG_LOG("OnOperationHandled() UPDATE_STREAM result:%{public}" PRId64".", result);
        // notify write if blocked
        writeDataCV_.notify_all();
        return SUCCESS;
    }
    if (operation == BUFFER_UNDERRUN) {
        underrunCount_++;
        // in plan: do more to reduce underrun
        writeDataCV_.notify_all();
        return SUCCESS;
    }
    AUDIO_INFO_LOG("OnOperationHandled() recv operation:%{public}d result:%{public}" PRId64".", operation, result);
    std::unique_lock<std::mutex> lock(callServerMutex_);
    notifiedOperation_ = operation;
    notifiedResult_ = result;
    callServerCV_.notify_all();
    return SUCCESS;
}

void RendererInClientInner::SetClientID(int32_t clientPid, int32_t clientUid)
{
    AUDIO_INFO_LOG("Set client PID: %{public}d, UID: %{public}d", clientPid, clientUid);
    clientPid_ = clientPid;
    clientUid_ = clientUid;
}

void RendererInClientInner::SetRendererInfo(const AudioRendererInfo &rendererInfo)
{
    rendererInfo_ = rendererInfo;
    AUDIO_INFO_LOG("SetRendererInfo with flag %{public}d", rendererInfo_.rendererFlags);
}

void RendererInClientInner::SetCapturerInfo(const AudioCapturerInfo &capturerInfo)
{
    AUDIO_WARNING_LOG("SetCapturerInfo is not supported");
    return;
}

void RendererInClientInner::RegisterTracker(const std::shared_ptr<AudioClientTracker> &proxyObj)
{
    if (audioStreamTracker_ && audioStreamTracker_.get() && !streamTrackerRegistered_) {
        // GetSessionID(sessionId_); // in plan:make sure sessionId_ is valid.
        AUDIO_INFO_LOG("AudioStream:Calling register tracker, sessionid = %{public}d", sessionId_);
        audioStreamTracker_->RegisterTracker(sessionId_, state_, rendererInfo_, capturerInfo_, proxyObj);
        streamTrackerRegistered_ = true;
    }
}

void RendererInClientInner::UpdateTracker(const std::string &updateCase)
{
    if (audioStreamTracker_ && audioStreamTracker_.get()) {
        AUDIO_DEBUG_LOG("Renderer:Calling Update tracker for %{public}s", updateCase.c_str());
        audioStreamTracker_->UpdateTracker(sessionId_, state_, rendererInfo_, capturerInfo_);
    }
}

int32_t RendererInClientInner::SetAudioStreamInfo(const AudioStreamParams info,
        const std::shared_ptr<AudioClientTracker> &proxyObj)
{
    AUDIO_INFO_LOG("AudioStreamInfo, Sampling rate: %{public}d, channels: %{public}d, format: %{public}d,"
        " stream type: %{public}d, encoding type: %{public}d", info.samplingRate, info.channels, info.format,
        eStreamType_, info.encoding);

    AudioXCollie guard("RendererInClientInner::SetAudioStreamInfo", CREATE_TIMEOUT_IN_SECOND);
    if (!IsFormatValid(info.format) || !IsSamplingRateValid(info.samplingRate) || !IsEncodingTypeValid(info.encoding)) {
        AUDIO_ERR_LOG("RendererInClient: Unsupported audio parameter");
        return ERR_NOT_SUPPORTED;
    }
    if (!IsPlaybackChannelRelatedInfoValid(info.channels, info.channelLayout)) {
        return ERR_NOT_SUPPORTED;
    }

    CHECK_AND_RETURN_RET_LOG(IAudioStream::GetByteSizePerFrame(info, sizePerFrameInByte_) == SUCCESS,
        ERROR_INVALID_PARAM, "GetByteSizePerFrame failed with invalid params");

    if (state_ != NEW) {
        AUDIO_INFO_LOG("RendererInClient: State is not new, release existing stream and recreate.");
        int32_t ret = DeinitIpcStream();
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "release existing stream failed.");
    }

    streamParams_ = info; // keep it for later use
    int32_t initRet = InitIpcStream();
    CHECK_AND_RETURN_RET_LOG(initRet == SUCCESS, initRet, "Init stream failed: %{public}d", initRet);
    state_ = PREPARED;

    RegisterTracker(proxyObj);
    return SUCCESS;
}

std::mutex g_serverProxyMutex;
sptr<IStandardAudioService> gServerProxy_ = nullptr;
const sptr<IStandardAudioService> RendererInClientInner::GetAudioServerProxy()
{
    std::lock_guard<std::mutex> lock(g_serverProxyMutex);
    if (gServerProxy_ == nullptr) {
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
        gServerProxy_ = iface_cast<IStandardAudioService>(object);
        if (gServerProxy_ == nullptr) {
            AUDIO_ERR_LOG("GetAudioServerProxy: get audio service proxy failed");
            return nullptr;
        }

        // register death recipent to restore proxy
        sptr<AudioServerDeathRecipient> asDeathRecipient = new(std::nothrow) AudioServerDeathRecipient(getpid());
        if (asDeathRecipient != nullptr) {
            asDeathRecipient->SetNotifyCb(std::bind(&RendererInClientInner::AudioServerDied,
                std::placeholders::_1));
            bool result = object->AddDeathRecipient(asDeathRecipient);
            if (!result) {
                AUDIO_ERR_LOG("GetAudioServerProxy: failed to add deathRecipient");
            }
        }
    }
    sptr<IStandardAudioService> gasp = gServerProxy_;
    return gasp;
}

void RendererInClientInner::AudioServerDied(pid_t pid)
{
    AUDIO_INFO_LOG("audio server died clear proxy, will restore proxy in next call");
    std::lock_guard<std::mutex> lock(g_serverProxyMutex);
    gServerProxy_ = nullptr;
}

// call this without lock, we should be able to call deinit in any case.
int32_t RendererInClientInner::DeinitIpcStream()
{
    // in plan:
    ipcStream_ = nullptr;
    ringCache_->ResetBuffer();
    return SUCCESS;
}

const AudioProcessConfig RendererInClientInner::ConstructConfig()
{
    AudioProcessConfig config = {};
    // in plan: get token id
    config.appInfo.appPid = clientPid_;
    config.appInfo.appUid = clientUid_;

    config.streamInfo.channels = static_cast<AudioChannel>(streamParams_.channels);
    config.streamInfo.encoding = static_cast<AudioEncodingType>(streamParams_.encoding);
    config.streamInfo.format = static_cast<AudioSampleFormat>(streamParams_.format);
    config.streamInfo.samplingRate = static_cast<AudioSamplingRate>(streamParams_.samplingRate);

    config.audioMode = AUDIO_MODE_PLAYBACK;

    config.rendererInfo = rendererInfo_;
    if (rendererInfo_.rendererFlags != 0) {
        AUDIO_WARNING_LOG("ConstructConfig find renderer flag invalid:%{public}d", rendererInfo_.rendererFlags);
        rendererInfo_.rendererFlags = 0;
    }

    config.capturerInfo = {};

    config.streamType = eStreamType_;

    // in plan: add privacyType_

    return config;
}

int32_t RendererInClientInner::InitSharedBuffer()
{
    CHECK_AND_RETURN_RET_LOG(ipcStream_ != nullptr, ERR_OPERATION_FAILED, "InitSharedBuffer failed, null ipcStream_.");
    int32_t ret = ipcStream_->ResolveBuffer(clientBuffer_);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS && clientBuffer_ != nullptr, ret, "ResolveBuffer failed:%{public}d", ret);

    uint32_t totalSizeInFrame = 0;
    uint32_t spanSizeInFrame = 0;
    uint32_t byteSizePerFrame = 0;
    ret = clientBuffer_->GetSizeParameter(totalSizeInFrame, spanSizeInFrame, byteSizePerFrame);

    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS && byteSizePerFrame == sizePerFrameInByte_, ret, "ResolveBuffer failed"
        ":%{public}d", ret);

    clientSpanSizeInByte_ = spanSizeInFrame * byteSizePerFrame;

    AUDIO_INFO_LOG("InitSharedBuffer totalSizeInFrame_[%{public}d] spanSizeInFrame_[%{public}d] sizePerFrameInByte_["
        "%{public}d] clientSpanSizeInByte_[%{public}zu]", totalSizeInFrame, spanSizeInFrame, sizePerFrameInByte_,
        clientSpanSizeInByte_);

    return SUCCESS;
}

// InitCacheBuffer should be able to modify the cache size between clientSpanSizeInByte_ and 4 * clientSpanSizeInByte_
int32_t RendererInClientInner::InitCacheBuffer(size_t targetSize)
{
    CHECK_AND_RETURN_RET_LOG(clientSpanSizeInByte_ != 0, ERR_OPERATION_FAILED, "clientSpanSizeInByte_ invalid");

    AUDIO_INFO_LOG("InitCacheBuffer old size:%{public}zu, new size:%{public}zu", cacheSizeInByte_, targetSize);
    cacheSizeInByte_ = targetSize;

    if (ringCache_ == nullptr) {
        ringCache_ = AudioRingCache::Create(cacheSizeInByte_);
    } else {
        // in plan
        OptResult result = ringCache_->ReConfig(cacheSizeInByte_, false); // false --> clear buffer
        if (result.ret != OPERATION_SUCCESS) {
            AUDIO_ERR_LOG("ReConfig AudioRingCache to size %{public}zu failed:ret%{public}d", result.ret, targetSize);
            return ERR_OPERATION_FAILED;
        }
    }

    return SUCCESS;
}

int32_t RendererInClientInner::InitIpcStream()
{
    AudioProcessConfig config = ConstructConfig();

    sptr<IStandardAudioService> gasp = RendererInClientInner::GetAudioServerProxy();
    CHECK_AND_RETURN_RET_LOG(gasp != nullptr, ERR_OPERATION_FAILED, "Create failed, can not get service.");
    sptr<IRemoteObject> ipcProxy = gasp->CreateAudioProcess(config); // in plan: add ret
    CHECK_AND_RETURN_RET_LOG(ipcProxy != nullptr, ERR_OPERATION_FAILED, "InitIpcStream failed with null ipcProxy.");
    ipcStream_ = iface_cast<IpcStream>(ipcProxy);
    CHECK_AND_RETURN_RET_LOG(ipcStream_ != nullptr, ERR_OPERATION_FAILED, "InitIpcStream failed when iface_cast.");

    // in plan: old listener_ is destoried here, will server receive dieth notify?
    listener_ = sptr<RendererStreamListener>::MakeSptr(shared_from_this());
    int32_t ret = ipcStream_->RegisterStreamListener(listener_->AsObject());
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "RegisterStreamListener failed:%{public}d", ret);

    ret = InitSharedBuffer();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "InitSharedBuffer failed:%{public}d", ret);

    ret = InitCacheBuffer(clientSpanSizeInByte_);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "InitCacheBuffer failed:%{public}d", ret);

    ret = ipcStream_->GetAudioSessionID(sessionId_);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "GetAudioSessionID failed:%{public}d", ret);

    return SUCCESS;
}

int32_t RendererInClientInner::GetAudioStreamInfo(AudioStreamParams &info)
{
    info = streamParams_;
    return SUCCESS;
}

bool RendererInClientInner::CheckRecordingCreate(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid)
{
    AUDIO_WARNING_LOG("CheckRecordingCreate is not supported");
    return false;
}

bool RendererInClientInner::CheckRecordingStateChange(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid,
        AudioPermissionState state)
{
    AUDIO_WARNING_LOG("CheckRecordingCreate is not supported");
    return false;
}

int32_t RendererInClientInner::GetAudioSessionID(uint32_t &sessionID)
{
    sessionID = sessionId_;
    return SUCCESS;
}

State RendererInClientInner::GetState()
{
    return state_;
}

bool RendererInClientInner::GetAudioTime(Timestamp &timestamp, Timestamp::Timestampbase base)
{
    // in plan:

    return false;
}

int32_t RendererInClientInner::GetBufferSize(size_t &bufferSize)
{
    bufferSize = clientSpanSizeInByte_;
    AUDIO_INFO_LOG("GetBufferSize size:%{public}zu", bufferSize);
    return SUCCESS;
}

int32_t RendererInClientInner::GetFrameCount(uint32_t &frameCount)
{
    if (sizePerFrameInByte_ != 0) {
        frameCount = clientSpanSizeInByte_ / sizePerFrameInByte_;
        AUDIO_INFO_LOG("GetFrameCount count:%{public}d", frameCount);
        return SUCCESS;
    }
    AUDIO_ERR_LOG("GetFrameCount failed clientSpanSizeInByte_ is 0");
    return ERR_ILLEGAL_STATE;
}

int32_t RendererInClientInner::GetLatency(uint64_t &latency)
{
    // in plan:
    latency = 150000000; // for debug
    return ERROR;
}

int32_t RendererInClientInner::SetAudioStreamType(AudioStreamType audioStreamType)
{
    // in plan
    AUDIO_ERR_LOG("SetAudioStreamType is not supported");
    return ERROR;
}

int32_t RendererInClientInner::SetVolume(float volume)
{
    Trace trace("RendererInClientInner::SetVolume:" + std::to_string(volume));
    if (volume < 0.0 || volume > 1.0) {
        AUDIO_ERR_LOG("SetVolume with invalid volume %{public}f", volume);
        return ERR_INVALID_PARAM;
    }
    clientVolume_ = volume;
    return SUCCESS;
}

float RendererInClientInner::GetVolume()
{
    Trace trace("RendererInClientInner::GetVolume:" + std::to_string(clientVolume_));
    return clientVolume_;
}

int32_t RendererInClientInner::SetRenderRate(AudioRendererRate renderRate)
{
    // in plan
    return ERROR;
}

AudioRendererRate RendererInClientInner::GetRenderRate()
{
    // in plan
    return RENDER_RATE_NORMAL;
}

int32_t RendererInClientInner::SetStreamCallback(const std::shared_ptr<AudioStreamCallback> &callback)
{
    if (callback == nullptr) {
        AUDIO_ERR_LOG("SetStreamCallback failed. callback == nullptr");
        return ERR_INVALID_PARAM;
    }

    std::unique_lock<std::mutex> lock(streamCbMutex_);
    streamCallback_ = callback;

    if (state_ != PREPARED) {
        return SUCCESS;
    }
    std::shared_ptr<AudioStreamCallback> streamCb = streamCallback_.lock();
    if (streamCb != nullptr) {
        lock.unlock();
        streamCb->OnStateChange(PREPARED); // in plan: put is in eventrunner for async call
    }
    return SUCCESS;
}


int32_t RendererInClientInner::SetRenderMode(AudioRenderMode renderMode)
{
    // in plan
    return ERROR;
}

AudioRenderMode RendererInClientInner::GetRenderMode()
{
    // in plan
    return RENDER_MODE_NORMAL;
}

int32_t RendererInClientInner::SetRendererWriteCallback(const std::shared_ptr<AudioRendererWriteCallback> &callback)
{
    // in plan
    return ERROR;
}

int32_t RendererInClientInner::SetCaptureMode(AudioCaptureMode captureMode)
{
    AUDIO_ERR_LOG("SetCaptureMode is not supported");
    return ERROR;
}

AudioCaptureMode RendererInClientInner::GetCaptureMode()
{
    AUDIO_ERR_LOG("GetCaptureMode is not supported");
    return CAPTURE_MODE_NORMAL; // not supported
}

int32_t RendererInClientInner::SetCapturerReadCallback(const std::shared_ptr<AudioCapturerReadCallback> &callback)
{
    AUDIO_ERR_LOG("SetCapturerReadCallback is not supported");
    return ERROR;
}

int32_t RendererInClientInner::GetBufferDesc(BufferDesc &bufDesc)
{
    // in plan
    return ERROR;
}

int32_t RendererInClientInner::GetBufQueueState(BufferQueueState &bufState)
{
    // in plan
    return ERROR;
}

int32_t RendererInClientInner::Enqueue(const BufferDesc &bufDesc)
{
    // in plan
    return ERROR;
}

int32_t RendererInClientInner::Clear()
{
    // in plan
    return ERROR;
}


int32_t RendererInClientInner::SetLowPowerVolume(float volume)
{
    // in plan
    return ERROR;
}

float RendererInClientInner::GetLowPowerVolume()
{
    // in plan
    return 0.0;
}

int32_t RendererInClientInner::SetOffloadMode(int32_t state, bool isAppBack)
{
    // in plan
    return ERROR;
}

int32_t RendererInClientInner::UnsetOffloadMode()
{
    // in plan
    return ERROR;
}

float RendererInClientInner::GetSingleStreamVolume()
{
    // in plan
    return 0.0;
}

AudioEffectMode RendererInClientInner::GetAudioEffectMode()
{
    // in plan
    return EFFECT_DEFAULT;
}

int32_t RendererInClientInner::SetAudioEffectMode(AudioEffectMode effectMode)
{
    // in plan
    return SUCCESS;
}

int64_t RendererInClientInner::GetFramesWritten()
{
    // in plan
    return -1;
}

int64_t RendererInClientInner::GetFramesRead()
{
    // in plan
    return -1;
}


void RendererInClientInner::SetInnerCapturerState(bool isInnerCapturer)
{
    AUDIO_ERR_LOG("SetInnerCapturerState is not supported");
    return;
}

void RendererInClientInner::SetWakeupCapturerState(bool isWakeupCapturer)
{
    AUDIO_ERR_LOG("SetWakeupCapturerState is not supported");
    return;
}

void RendererInClientInner::SetCapturerSource(int capturerSource)
{
    AUDIO_ERR_LOG("SetCapturerSource is not supported");
    return;
}

void RendererInClientInner::SetPrivacyType(AudioPrivacyType privacyType)
{
    privacyType_ = privacyType;
    // should we update it after create?
}

bool RendererInClientInner::StartAudioStream(StateChangeCmdType cmdType)
{
    Trace trace("RendererInClientInner::StartAudioStream " + std::to_string(sessionId_));
    std::unique_lock<std::mutex> statusLock(statusMutex_);
    if (state_ != PREPARED && state_ != STOPPED && state_ != PAUSED) {
        AUDIO_ERR_LOG("StartAudioStream  failed Illegal state:%{public}d", state_.load());
        return false;
    }

    CHECK_AND_RETURN_RET_LOG(ipcStream_ != nullptr, false, "ipcStream is not inited!");
    int32_t ret = ipcStream_->Start();
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("StartAudioStream call server failed:%{public}u", ret);
        return false;
    }
    std::unique_lock<std::mutex> waitLock(callServerMutex_);
    bool stopWaiting = callServerCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return notifiedOperation_ == START_STREAM; // will be false when got notified.
    });

    if (notifiedOperation_ != START_STREAM || notifiedResult_ != SUCCESS) {
        AUDIO_ERR_LOG("StartAudioStream failed: %{public}s Operation:%{public}d result:%{public}" PRId64".",
            (!stopWaiting ? "timeout" : "no timeout"), notifiedOperation_, notifiedResult_);
        return false;
    }
    waitLock.unlock();

    if (renderMode_ == RENDER_MODE_CALLBACK) {
        // in plan
        // start the callback-write thread
    }
    state_ = RUNNING;
    statusLock.unlock();
    // in plan: use send event to clent with cmdType | call OnStateChange | call HiSysEventWrite
    (void)cmdType;

    AUDIO_INFO_LOG("StartAudioStream SUCCESS, sessionId: %{public}d, uid: %{public}d", sessionId_, clientUid_);
    UpdateTracker("RUNNING");
    return true;
}

bool RendererInClientInner::PauseAudioStream(StateChangeCmdType cmdType)
{
    Trace trace("RendererInClientInner::PauseAudioStream " + std::to_string(sessionId_));
    std::unique_lock<std::mutex> statusLock(statusMutex_);
    if (state_ != RUNNING) {
        AUDIO_ERR_LOG("PauseAudioStream State is not RUNNING. Illegal state:%{public}u", state_.load());
        return false;
    }

    if (renderMode_ == RENDER_MODE_CALLBACK) {
        // in plan
        // stop the callback-write thread
    }

    CHECK_AND_RETURN_RET_LOG(ipcStream_ != nullptr, false, "ipcStream is not inited!");
    int32_t ret = ipcStream_->Pause();
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("PauseAudioStream call server failed:%{public}u", ret);
        return false;
    }
    std::unique_lock<std::mutex> waitLock(callServerMutex_);
    bool stopWaiting = callServerCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return notifiedOperation_ == PAUSE_STREAM; // will be false when got notified.
    });

    if (notifiedOperation_ != PAUSE_STREAM || notifiedResult_ != SUCCESS) {
        AUDIO_ERR_LOG("PauseAudioStream failed: %{public}s Operation:%{public}d result:%{public}" PRId64".",
            (!stopWaiting ? "timeout" : "no timeout"), notifiedOperation_, notifiedResult_);
        return false;
    }
    waitLock.unlock();

    state_ = PAUSED;
    statusLock.unlock();

    // in plan: use send event to clent with cmdType | call OnStateChange | call HiSysEventWrite
    (void)cmdType;

    AUDIO_INFO_LOG("PauseAudioStream SUCCESS, sessionId: %{public}d, uid: %{public}d", sessionId_, clientUid_);
    UpdateTracker("PAUSED");
    return true;
}

bool RendererInClientInner::StopAudioStream()
{
    Trace trace("RendererInClientInner::StopAudioStream " + std::to_string(sessionId_));
    AUDIO_INFO_LOG("StopAudioStream begin for sessionId %{public}d uid: %{public}d", sessionId_, clientUid_);
    std::unique_lock<std::mutex> statusLock(statusMutex_);
    if ((state_ != RUNNING) && (state_ != PAUSED)) {
        AUDIO_ERR_LOG("StopAudioStream failed. Illegal state:%{public}u", state_.load());
        return false;
    }

    if (renderMode_ == RENDER_MODE_CALLBACK) {
        // in plan
        // stop the callback-write thread
    }

    CHECK_AND_RETURN_RET_LOG(ipcStream_ != nullptr, false, "ipcStream is not inited!");
    int32_t ret = ipcStream_->Stop();
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("StopAudioStream call server failed:%{public}u", ret);
        return false;
    }
    std::unique_lock<std::mutex> waitLock(callServerMutex_);
    bool stopWaiting = callServerCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return notifiedOperation_ == STOP_STREAM; // will be false when got notified.
    });

    if (notifiedOperation_ != STOP_STREAM || notifiedResult_ != SUCCESS) {
        AUDIO_ERR_LOG("StopAudioStream failed: %{public}s Operation:%{public}d result:%{public}" PRId64".",
            (!stopWaiting ? "timeout" : "no timeout"), notifiedOperation_, notifiedResult_);
        return false;
    }
    waitLock.unlock();

    state_ = STOPPED;
    statusLock.unlock();

    // in plan: use send event to clent with cmdType | call OnStateChange | call HiSysEventWrite

    AUDIO_INFO_LOG("StopAudioStream SUCCESS, sessionId: %{public}d, uid: %{public}d", sessionId_, clientUid_);
    UpdateTracker("STOPPED");
    return true;
}

bool RendererInClientInner::ReleaseAudioStream(bool releaseRunner)
{
    Trace trace("RendererInClientInner::ReleaseAudioStream " + std::to_string(sessionId_));
    // lock_guard<mutex> lock(statusMutex_) // no lock, call release in any case, include blocked case.
    return false;
}

bool RendererInClientInner::FlushAudioStream()
{
    Trace trace("RendererInClientInner::FlushAudioStream " + std::to_string(sessionId_));
    std::unique_lock<std::mutex> statusLock(statusMutex_);
    if ((state_ != RUNNING) && (state_ != PAUSED) && (state_ != STOPPED)) {
        AUDIO_ERR_LOG("FlushAudioStream failed. Illegal state:%{public}u", state_.load());
        return false;
    }
    CHECK_AND_RETURN_RET_LOG(ipcStream_ != nullptr, false, "ipcStream is not inited!");
    int32_t ret = ipcStream_->Flush();
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("FlushAudioStream call server failed:%{public}u", ret);
        return false;
    }
    std::unique_lock<std::mutex> waitLock(callServerMutex_);
    bool stopWaiting = callServerCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return notifiedOperation_ == FLUSH_STREAM; // will be false when got notified.
    });

    if (notifiedOperation_ != FLUSH_STREAM || notifiedResult_ != SUCCESS) {
        AUDIO_ERR_LOG("FlushAudioStream failed: %{public}s Operation:%{public}d result:%{public}" PRId64".",
            (!stopWaiting ? "timeout" : "no timeout"), notifiedOperation_, notifiedResult_);
        return false;
    }
    waitLock.unlock();
    AUDIO_INFO_LOG("Flush stream SUCCESS, sessionId: %{public}d", sessionId_);
    return true;
}

int32_t RendererInClientInner::DrainAudioCache()
{
    // in plan: send all data in ringCache_ to server even if GetReadableSize() < clientSpanSizeInByte_. GetReadableSize() should be 0
    return SUCCESS;
}

bool RendererInClientInner::DrainAudioStream()
{
    Trace trace("RendererInClientInner::DrainAudioStream " + std::to_string(sessionId_));
    std::unique_lock<std::mutex> statusLock(statusMutex_);
    if (state_ != RUNNING) {
        AUDIO_ERR_LOG("DrainAudioStream failed. Illegal state:%{public}u", state_.load());
        return false;
    }

    CHECK_AND_RETURN_RET_LOG(DrainAudioCache() == SUCCESS, false, "Drain cache failed");

    CHECK_AND_RETURN_RET_LOG(ipcStream_ != nullptr, false, "ipcStream is not inited!");
    int32_t ret = ipcStream_->Drain();
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("DrainAudioStream call server failed:%{public}u", ret);
        return false;
    }
    std::unique_lock<std::mutex> waitLock(callServerMutex_);
    bool stopWaiting = callServerCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return notifiedOperation_ == DRAIN_STREAM; // will be false when got notified.
    });

    if (notifiedOperation_ != DRAIN_STREAM || notifiedResult_ != SUCCESS) {
        AUDIO_ERR_LOG("DrainAudioStream failed: %{public}s Operation:%{public}d result:%{public}" PRId64".",
            (!stopWaiting ? "timeout" : "no timeout"), notifiedOperation_, notifiedResult_);
        return false;
    }
    waitLock.unlock();
    AUDIO_INFO_LOG("Drain stream SUCCESS, sessionId: %{public}d", sessionId_);
    return true;
}

int32_t RendererInClientInner::Write(uint8_t *buffer, size_t buffer_size)
{
    Trace trace("RendererInClient::Write " + std::to_string(buffer_size));
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr && buffer_size > 0 && buffer_size < MAX_CLIENT_WRITE_SIZE,
        ERR_INVALID_PARAM, "Write with invalid params, size is %{public}zu", buffer_size);

    std::lock_guard<std::mutex> lock(writeMutex_);

    // if first call, call set thread priority. if thread tid change recall set thread priority
    if (needSetThreadPriority_ || writeThreadId_ != gettid()) {
        AudioSystemManager::GetInstance()->RequestThreadPriority(gettid());
        writeThreadId_ = gettid();
        needSetThreadPriority_ = false;
    }

    std::unique_lock<std::mutex> statusLock(statusMutex_); // status check
    CHECK_AND_RETURN_RET_LOG(state_ == RUNNING, ERR_ILLEGAL_STATE, "Write: Illegal state:%{public}u", state_.load());
    statusLock.unlock();

    // hold lock
    if (isBlendSet_) {
        audioBlend_.Process(buffer, buffer_size);
    }

    size_t targetSize = buffer_size;
    size_t offset = 0;
    while (targetSize > sizePerFrameInByte_) {
        // 1. write data into ring cache
        OptResult result = ringCache_->GetWritableSize();
        if (result.ret != OPERATION_SUCCESS) {
            AUDIO_ERR_LOG("RingCache write status invalid size is:%{public}u", result.size);
            break;
        }
        size_t writableSize = result.size;
        Trace::Count("RendererInClient::CacheBuffer->writableSize", writableSize);

        size_t writeSize = std::min(writableSize, targetSize);
        BufferWrap bufferWrap = {buffer + offset, writeSize};

        if (writeSize > 0) {
            result = ringCache_->Enqueue(bufferWrap);
            if (result.ret != OPERATION_SUCCESS) {
                // in plan: recall enqueue in some cases
                AUDIO_ERR_LOG("RingCache Enqueue failed ret:%{public}d size:%{public}zu", result.ret, result.size);
                break;
            }
            offset += writeSize;
            targetSize -= writeSize;
            clientWrittenBytes_ += writeSize;
        }

        // 2. copy data from cache to OHAudioBuffer
        result = ringCache_->GetReadableSize();
        if (result.ret != OPERATION_SUCCESS) {
            AUDIO_ERR_LOG("RingCache read status invalid size is:%{public}u", result.size);
            break;
        }
        size_t readableSize = result.size;
        Trace::Count("RendererInClient::CacheBuffer->readableSize", readableSize);

        if (readableSize < clientSpanSizeInByte_) {
            continue;
        }
        // if readable size is enough, we will call write data to server
        int32_t ret = WritCacheData();
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERROR, "WritCacheData failed %{public}d", ret);
    }

    return buffer_size - targetSize;
}

int32_t RendererInClientInner::WritCacheData()
{
    int32_t sizeInFrame = clientBuffer_->GetAvailableDataFrames();
    CHECK_AND_RETURN_RET_LOG(sizeInFrame >= 0, ERROR, "GetAvailableDataFrames invalid, %{public}d", sizeInFrame);
    if (sizeInFrame * sizePerFrameInByte_ < clientSpanSizeInByte_) {
        // wait for server read some data
        std::unique_lock<std::mutex> lock(writeDataMutex_);
        std::cv_status stat = writeDataCV_.wait_for(lock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS));
        CHECK_AND_RETURN_RET_LOG(stat == std::cv_status::no_timeout, ERROR, "write data time out");
    }
    BufferDesc desc = {};
    int32_t ret = clientBuffer_->GetWriteBuffer(clientBuffer_->GetCurWriteFrame(), desc);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERROR, "GetWriteBuffer failed %{public}d", ret);
    OptResult result = ringCache_->Dequeue({desc.buffer, desc.bufLength});
    CHECK_AND_RETURN_RET_LOG(result.ret == OPERATION_SUCCESS, ERROR, "ringCache Dequeue failed %{public}d",
                             result.ret);
    ipcStream_->UpdatePosition(); // notiify server update position
    return SUCCESS;
}

int32_t RendererInClientInner::Read(uint8_t &buffer, size_t userSize, bool isBlockingRead)
{
    AUDIO_ERR_LOG("Read is not supported");
    return ERROR;
}


uint32_t RendererInClientInner::GetUnderflowCount()
{
    // in plan
    return 0;
}

void RendererInClientInner::SetRendererPositionCallback(int64_t markPosition, const std::shared_ptr<RendererPositionCallback> &callback)
       
{
    // in plan
}

void RendererInClientInner::UnsetRendererPositionCallback()
{
    // in plan
}

void RendererInClientInner::SetRendererPeriodPositionCallback(int64_t markPosition,
        const std::shared_ptr<RendererPeriodPositionCallback> &callback)
{
    // in plan
}

void RendererInClientInner::UnsetRendererPeriodPositionCallback()
{
    // in plan
}

void RendererInClientInner::SetCapturerPositionCallback(int64_t markPosition, const std::shared_ptr<CapturerPositionCallback> &callback)
       
{
    AUDIO_ERR_LOG("SetCapturerPositionCallback is not supported");
    return;
}

void RendererInClientInner::UnsetCapturerPositionCallback()
{
    AUDIO_ERR_LOG("SetCapturerPositionCallback is not supported");
    return;
}

void RendererInClientInner::SetCapturerPeriodPositionCallback(int64_t markPosition,
        const std::shared_ptr<CapturerPeriodPositionCallback> &callback)
{
    AUDIO_ERR_LOG("SetCapturerPositionCallback is not supported");
    return;
}

void RendererInClientInner::UnsetCapturerPeriodPositionCallback()
{
    AUDIO_ERR_LOG("SetCapturerPositionCallback is not supported");
    return;
}

int32_t RendererInClientInner::SetRendererSamplingRate(uint32_t sampleRate)
{
    AUDIO_ERR_LOG("SetRendererSamplingRate to %{publid}d is not supported", sampleRate);
    return ERROR;
}

uint32_t RendererInClientInner::GetRendererSamplingRate()
{
    return streamParams_.samplingRate;
}

int32_t RendererInClientInner::SetBufferSizeInMsec(int32_t bufferSizeInMsec)
{
    bufferSizeInMsec_ = bufferSizeInMsec;
    AUDIO_INFO_LOG("SetBufferSizeInMsec to %{publid}d", bufferSizeInMsec_);
    return SUCCESS;
}

void RendererInClientInner::SetApplicationCachePath(const std::string cachePath)
{
    cachePath_ = cachePath;
    AUDIO_INFO_LOG("SetApplicationCachePath to %{publid}s", cachePath_.c_str());
}

int32_t RendererInClientInner::SetChannelBlendMode(ChannelBlendMode blendMode)
{
    if ((state_ != PREPARED) && (state_ != NEW)) {
        AUDIO_ERR_LOG("SetChannelBlendMode in invalid status:%{public}d", state_.load());
        return ERR_ILLEGAL_STATE;
    }
    isBlendSet_ = true;
    audioBlend_.SetParams(blendMode, streamParams_.format, streamParams_.channels);
    return SUCCESS;
}

int32_t RendererInClientInner::SetVolumeWithRamp(float volume, int32_t duration)
{
    // in plan
    return ERROR;
}

void RendererInClientInner::SetStreamTrackerState(bool trackerRegisteredState)
{
    streamTrackerRegistered_ = trackerRegisteredState;
}

void RendererInClientInner::GetSwitchInfo(IAudioStream::SwitchInfo& info)
{
    // in plan
}

IAudioStream::StreamClass RendererInClientInner::GetStreamClass()
{
    return PA_STREAM;
}
} // namespace AudioStandard
} // namespace OHOS
#endif // FAST_AUDIO_STREAM_H
