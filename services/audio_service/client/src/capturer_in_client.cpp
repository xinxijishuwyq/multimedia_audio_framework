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

#include "capturer_in_client.h"

#include <atomic>
#include <cinttypes>
#include <condition_variable>
#include <sstream>
#include <string>
#include <mutex>
#include <thread>

#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "ipc_stream.h"
#include "audio_log.h"
#include "audio_errors.h"

#include "audio_manager_base.h"
#include "audio_ring_cache.h"
#include "audio_utils.h"
#include "audio_policy_manager.h"
#include "audio_server_death_recipient.h"
#include "audio_stream_tracker.h"
#include "audio_system_manager.h"
#include "ipc_stream_listener_stub.h"

namespace OHOS {
namespace AudioStandard {
namespace {
static const int32_t CREATE_TIMEOUT_IN_SECOND = 5; // 5S
static const int32_t OPERATION_TIMEOUT_IN_MS = 500; // 500ms
}
class CapturerStreamListener;
class CapturerInClientInner : public CapturerInClient, public IStreamListener,
    public std::enable_shared_from_this<CapturerInClientInner> {
public:
    CapturerInClientInner(AudioStreamType eStreamType, int32_t appUid);
    ~CapturerInClientInner();

    // IStreamListener
    int32_t OnOperationHandled(Operation operation, int64_t result) override;

    void SetClientID(int32_t clientPid, int32_t clientUid) override;

    void SetRendererInfo(const AudioRendererInfo &rendererInfo) override;
    void SetCapturerInfo(const AudioCapturerInfo &capturerInfo) override;
    int32_t GetAudioStreamInfo(AudioStreamParams &info) override;
    int32_t SetAudioStreamInfo(const AudioStreamParams info,
        const std::shared_ptr<AudioClientTracker> &proxyObj) override;
    bool CheckRecordingCreate(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid, SourceType sourceType =
        SOURCE_TYPE_MIC) override;
    bool CheckRecordingStateChange(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid,
        AudioPermissionState state) override;
    State GetState() override;
    int32_t GetAudioSessionID(uint32_t &sessionID) override;
    bool GetAudioTime(Timestamp &timestamp, Timestamp::Timestampbase base) override;
    int32_t GetBufferSize(size_t &bufferSize) override;
    int32_t GetFrameCount(uint32_t &frameCount) override;
    int32_t GetLatency(uint64_t &latency) override;
    int32_t SetAudioStreamType(AudioStreamType audioStreamType) override;
    float GetVolume() override;
    int32_t SetVolume(float volume) override;
    int32_t SetRenderRate(AudioRendererRate renderRate) override;
    AudioRendererRate GetRenderRate() override;
    int32_t SetStreamCallback(const std::shared_ptr<AudioStreamCallback> &callback) override;

    // callback mode api
    AudioRenderMode GetRenderMode() override;
    int32_t SetRenderMode(AudioRenderMode renderMode) override;
    int32_t SetRendererWriteCallback(const std::shared_ptr<AudioRendererWriteCallback> &callback) override;
    int32_t SetCaptureMode(AudioCaptureMode captureMode) override;
    AudioCaptureMode GetCaptureMode() override;
    int32_t SetCapturerReadCallback(const std::shared_ptr<AudioCapturerReadCallback> &callback) override;
    int32_t GetBufferDesc(BufferDesc &bufDesc) override;
    int32_t Clear() override;
    int32_t GetBufQueueState(BufferQueueState &bufState) override;
    int32_t Enqueue(const BufferDesc &bufDesc) override;

    int32_t SetLowPowerVolume(float volume) override;
    float GetLowPowerVolume() override;
    int32_t UnsetOffloadMode() override;
    int32_t SetOffloadMode(int32_t state, bool isAppBack) override;
    float GetSingleStreamVolume() override;
    AudioEffectMode GetAudioEffectMode() override;
    int32_t SetAudioEffectMode(AudioEffectMode effectMode) override;
    int64_t GetFramesRead() override;
    int64_t GetFramesWritten() override;

    void SetInnerCapturerState(bool isInnerCapturer) override;
    void SetWakeupCapturerState(bool isWakeupCapturer) override;
    void SetPrivacyType(AudioPrivacyType privacyType) override;
    void SetCapturerSource(int capturerSource) override;

    // Common APIs
    bool StartAudioStream(StateChangeCmdType cmdType = CMD_FROM_CLIENT) override;
    bool PauseAudioStream(StateChangeCmdType cmdType = CMD_FROM_CLIENT) override;
    bool StopAudioStream() override;
    bool FlushAudioStream() override;
    bool ReleaseAudioStream(bool releaseRunner = true) override;

    // Playback related APIs
    bool DrainAudioStream() override;
    int32_t Write(uint8_t *buffer, size_t buffer_size) override;
    int32_t Write(uint8_t *pcmBuffer, size_t pcmSize, uint8_t *metaBuffer, size_t metaSize) override;
    void SetPreferredFrameSize(int32_t frameSize) override;

    // Recording related APIs
    int32_t Read(uint8_t &buffer, size_t userSize, bool isBlockingRead) override;

    uint32_t GetUnderflowCount() override;
    void SetRendererPositionCallback(int64_t markPosition, const std::shared_ptr<RendererPositionCallback> &callback)
        override;
    void UnsetRendererPositionCallback() override;
    void UnsetRendererPeriodPositionCallback() override;
    void SetRendererPeriodPositionCallback(int64_t markPosition,
        const std::shared_ptr<RendererPeriodPositionCallback> &callback) override;
    void SetCapturerPositionCallback(int64_t markPosition, const std::shared_ptr<CapturerPositionCallback> &callback)
        override;
    void UnsetCapturerPositionCallback() override;
    void SetCapturerPeriodPositionCallback(int64_t markPosition,
        const std::shared_ptr<CapturerPeriodPositionCallback> &callback) override;
    void UnsetCapturerPeriodPositionCallback() override;
    uint32_t GetRendererSamplingRate() override;
    int32_t SetRendererSamplingRate(uint32_t sampleRate) override;
    int32_t SetBufferSizeInMsec(int32_t bufferSizeInMsec) override;
    void SetApplicationCachePath(const std::string cachePath) override;
    int32_t SetChannelBlendMode(ChannelBlendMode blendMode) override;
    int32_t SetVolumeWithRamp(float volume, int32_t duration) override;

    void SetStreamTrackerState(bool trackerRegisteredState) override;
    void GetSwitchInfo(IAudioStream::SwitchInfo& info) override;

    IAudioStream::StreamClass GetStreamClass() override;

    static void AudioServerDied(pid_t pid);
    static const sptr<IStandardAudioService> GetAudioServerProxy();
private:
    void RegisterTracker(const std::shared_ptr<AudioClientTracker> &proxyObj);
    void UpdateTracker(const std::string &updateCase);

    int32_t DeinitIpcStream();

    int32_t InitIpcStream();

    const AudioProcessConfig ConstructConfig();

    int32_t InitCacheBuffer(size_t targetSize);
    int32_t InitSharedBuffer();
private:
    AudioStreamType eStreamType_;
    int32_t appUid_;
    uint32_t sessionId_;
    int32_t clientUid_ = -1;
    int32_t clientPid_ = -1;

    std::unique_ptr<AudioStreamTracker> audioStreamTracker_;
    bool streamTrackerRegistered_ = false;

    AudioRendererInfo rendererInfo_; // not in use
    AudioCapturerInfo capturerInfo_;

    std::string cachePath_;

    bool isInnerCapturer_ = false;
    bool isWakeupCapturer_ = false;

    AudioStreamParams streamParams_; // in plan: replace it with AudioCapturerParams

    size_t cacheSizeInByte_ = 0;
    size_t clientSpanSizeInByte_ = 0;
    size_t sizePerFrameInByte_ = 4; // 16bit 2ch as default

    // using this lock when change status_
    std::mutex statusMutex_;
    std::atomic<State> state_ = INVALID;
    // for status operation wait and notify
    std::mutex callServerMutex_;
    std::condition_variable callServerCV_;

    Operation notifiedOperation_ = MAX_OPERATION_CODE;
    int64_t notifiedResult_ = 0;

    // read data
    std::mutex readDataMutex_;
    std::condition_variable readDataCV_;

    // ipc stream related
    sptr<CapturerStreamListener> listener_ = nullptr;
    sptr<IpcStream> ipcStream_ = nullptr;
    std::shared_ptr<OHAudioBuffer> clientBuffer_ = nullptr;

    // buffer handle
    std::unique_ptr<AudioRingCache> ringCache_ = nullptr;
    std::mutex readMutex_; // used for prevent multi thread call read
};

// CapturerStreamListener --> sptr | CapturerInClientInner --> shared_ptr
class CapturerStreamListener : public IpcStreamListenerStub {
public:
    CapturerStreamListener(std::shared_ptr<CapturerInClientInner> capturerInClinet);
    virtual ~CapturerStreamListener() = default;

    // IpcStreamListenerStub
    int32_t OnOperationHandled(Operation operation, int64_t result) override;
private:
    std::weak_ptr<CapturerInClientInner> capturerInClinet_;
};

CapturerStreamListener::CapturerStreamListener(std::shared_ptr<CapturerInClientInner> capturerInClinet)
{
    if (capturerInClinet == nullptr) {
        AUDIO_ERR_LOG("CapturerStreamListener() find null rendererInClinet");
    }
    capturerInClinet_ = capturerInClinet;
}

int32_t CapturerStreamListener::OnOperationHandled(Operation operation, int64_t result)
{
    std::shared_ptr<CapturerInClientInner> capturer = capturerInClinet_.lock();
    if (capturer == nullptr) {
        AUDIO_WARNING_LOG("OnOperationHandled() find capturerInClinet_ is null, operation:%{public}d result:"
            "%{public}" PRId64".", operation, result);
        return ERR_ILLEGAL_STATE;
    }
    return capturer->OnOperationHandled(operation, result);
}

std::shared_ptr<CapturerInClient> CapturerInClient::GetInstance(AudioStreamType eStreamType, int32_t appUid)
{
    return std::make_shared<CapturerInClientInner>(eStreamType, appUid);
}

CapturerInClientInner::CapturerInClientInner(AudioStreamType eStreamType, int32_t appUid) : eStreamType_(eStreamType),
    appUid_(appUid)
{
    AUDIO_INFO_LOG("CapturerInClientInner() with StreamType:%{public}d appUid:%{public}d ", eStreamType_, appUid_);
    // AUDIO_MODE_RECORD
}

CapturerInClientInner::~CapturerInClientInner()
{
    AUDIO_INFO_LOG("~CapturerInClientInner()");
}

int32_t CapturerInClientInner::OnOperationHandled(Operation operation, int64_t result)
{
    // read/write operation may print many log, use debug.
    if (operation == UPDATE_STREAM) {
        AUDIO_DEBUG_LOG("OnOperationHandled() UPDATE_STREAM result:%{public}" PRId64".", result);
        // notify write if blocked
        readDataCV_.notify_all();
        return SUCCESS;
    }

    AUDIO_INFO_LOG("OnOperationHandled() recv operation:%{public}d result:%{public}" PRId64".", operation, result);
    std::unique_lock<std::mutex> lock(callServerMutex_);
    notifiedOperation_ = operation;
    notifiedResult_ = result;
    callServerCV_.notify_all();
    return SUCCESS;
}

void CapturerInClientInner::SetClientID(int32_t clientPid, int32_t clientUid)
{
    AUDIO_INFO_LOG("Capturer set client PID: %{public}d, UID: %{public}d", clientPid, clientUid);
    clientPid_ = clientPid;
    clientUid_ = clientUid;
    return;
}

void CapturerInClientInner::SetRendererInfo(const AudioRendererInfo &rendererInfo)
{
    AUDIO_WARNING_LOG("SetRendererInfo is not supported");
    return;
}

void CapturerInClientInner::SetCapturerInfo(const AudioCapturerInfo &capturerInfo)
{
    capturerInfo_ = capturerInfo;
    AUDIO_INFO_LOG("SetCapturerInfo with SourceType %{public}d flag %{public}d", capturerInfo_.sourceType,
        capturerInfo_.capturerFlags);
    return;
}

void CapturerInClientInner::RegisterTracker(const std::shared_ptr<AudioClientTracker> &proxyObj)
{
    if (audioStreamTracker_ && audioStreamTracker_.get() && !streamTrackerRegistered_) {
        // make sure sessionId_ is set before.
        AUDIO_INFO_LOG("AudioStream:Calling register tracker, sessionid = %{public}d", sessionId_);
        AudioRegisterTrackerInfo registerTrackerInfo;

        registerTrackerInfo.sessionId = sessionId_;
        registerTrackerInfo.clientPid = clientPid_;
        registerTrackerInfo.state = state_;
        registerTrackerInfo.rendererInfo = rendererInfo_;
        registerTrackerInfo.capturerInfo = capturerInfo_;

        audioStreamTracker_->RegisterTracker(registerTrackerInfo, proxyObj);
        streamTrackerRegistered_ = true;
    }
}

void CapturerInClientInner::UpdateTracker(const std::string &updateCase)
{
    if (audioStreamTracker_ && audioStreamTracker_.get()) {
        AUDIO_DEBUG_LOG("Capturer:Calling Update tracker for %{public}s", updateCase.c_str());
        audioStreamTracker_->UpdateTracker(sessionId_, state_, clientPid_, rendererInfo_, capturerInfo_);
    }
}

int32_t CapturerInClientInner::SetAudioStreamInfo(const AudioStreamParams info,
        const std::shared_ptr<AudioClientTracker> &proxyObj)
{
    AUDIO_INFO_LOG("AudioStreamInfo, Sampling rate: %{public}d, channels: %{public}d, format: %{public}d, stream type:"
        " %{public}d, encoding type: %{public}d", info.samplingRate, info.channels, info.format, eStreamType_,
        info.encoding);
    AudioXCollie guard("CapturerInClientInner::SetAudioStreamInfo", CREATE_TIMEOUT_IN_SECOND);
    if (!IsFormatValid(info.format) || !IsCapturerChannelValid(info.channels) || !IsEncodingTypeValid(info.encoding) ||
        !IsSamplingRateValid(info.samplingRate)) {
        AUDIO_ERR_LOG("CapturerInClient: Unsupported audio parameter");
        return ERR_NOT_SUPPORTED;
    }

    CHECK_AND_RETURN_RET_LOG(IAudioStream::GetByteSizePerFrame(info, sizePerFrameInByte_) == SUCCESS,
        ERROR_INVALID_PARAM, "GetByteSizePerFrame failed with invalid params");

    if (state_ != NEW) {
        AUDIO_INFO_LOG("CapturerInClient: State is not new, release existing stream and recreate.");
        int32_t ret = DeinitIpcStream();
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "release existing stream failed.");
    }

    streamParams_ = info; // keep it for later use
    int32_t initRet = InitIpcStream();
    CHECK_AND_RETURN_RET_LOG(initRet == SUCCESS, initRet, "Init stream failed: %{public}d", initRet);
    state_ = PREPARED;

    RegisterTracker(proxyObj);
    return ERROR;
}

std::mutex g_serverMutex;
sptr<IStandardAudioService> g_ServerProxy = nullptr;
const sptr<IStandardAudioService> CapturerInClientInner::GetAudioServerProxy()
{
    std::lock_guard<std::mutex> lock(g_serverMutex);
    if (g_ServerProxy == nullptr) {
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
        g_ServerProxy = iface_cast<IStandardAudioService>(object);
        if (g_ServerProxy == nullptr) {
            AUDIO_ERR_LOG("GetAudioServerProxy: get audio service proxy failed");
            return nullptr;
        }

        // register death recipent to restore proxy
        sptr<AudioServerDeathRecipient> asDeathRecipient = new(std::nothrow) AudioServerDeathRecipient(getpid());
        if (asDeathRecipient != nullptr) {
            asDeathRecipient->SetNotifyCb(std::bind(&CapturerInClientInner::AudioServerDied,
                std::placeholders::_1));
            bool result = object->AddDeathRecipient(asDeathRecipient);
            if (!result) {
                AUDIO_ERR_LOG("GetAudioServerProxy: failed to add deathRecipient");
            }
        }
    }
    sptr<IStandardAudioService> gasp = g_ServerProxy;
    return gasp;
}

void CapturerInClientInner::AudioServerDied(pid_t pid)
{
    AUDIO_INFO_LOG("audio server died clear proxy, will restore proxy in next call");
    std::lock_guard<std::mutex> lock(g_serverMutex);
    g_ServerProxy = nullptr;
}

// call this without lock, we should be able to call deinit in any case.
int32_t CapturerInClientInner::DeinitIpcStream()
{
    // in plan:
    ipcStream_ = nullptr;
    ringCache_->ResetBuffer();
    return SUCCESS;
}

const AudioProcessConfig CapturerInClientInner::ConstructConfig()
{
    AudioProcessConfig config = {};
    // in plan: get token id
    config.appInfo.appPid = clientPid_;
    config.appInfo.appUid = clientUid_;

    config.streamInfo.channels = static_cast<AudioChannel>(streamParams_.channels);
    config.streamInfo.encoding = static_cast<AudioEncodingType>(streamParams_.encoding);
    config.streamInfo.format = static_cast<AudioSampleFormat>(streamParams_.format);
    config.streamInfo.samplingRate = static_cast<AudioSamplingRate>(streamParams_.samplingRate);

    config.audioMode = AUDIO_MODE_RECORD;

    config.capturerInfo = capturerInfo_;
    if (capturerInfo_.capturerFlags != 0) {
        AUDIO_WARNING_LOG("ConstructConfig find Capturer flag invalid:%{public}d", capturerInfo_.capturerFlags);
        capturerInfo_.capturerFlags = 0;
    }

    config.rendererInfo = {};

    config.streamType = eStreamType_;

    config.isInnerCapturer = isInnerCapturer_;
    config.isWakeupCapturer = isWakeupCapturer_;

    return config;
}

int32_t CapturerInClientInner::InitSharedBuffer()
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
int32_t CapturerInClientInner::InitCacheBuffer(size_t targetSize)
{
    CHECK_AND_RETURN_RET_LOG(clientSpanSizeInByte_ != 0, ERR_OPERATION_FAILED, "clientSpanSizeInByte_ invalid");

    AUDIO_INFO_LOG("InitCacheBuffer old size:%{public}zu, new size:%{public}zu", cacheSizeInByte_, targetSize);
    cacheSizeInByte_ = targetSize;

    if (ringCache_ == nullptr) {
        ringCache_ = AudioRingCache::Create(cacheSizeInByte_);
    } else {
        OptResult result = ringCache_->ReConfig(cacheSizeInByte_, false); // false --> clear buffer
        if (result.ret != OPERATION_SUCCESS) {
            AUDIO_ERR_LOG("ReConfig AudioRingCache to size %{public}zu failed:ret%{public}d", result.ret, targetSize);
            return ERR_OPERATION_FAILED;
        }
    }

    return SUCCESS;
}

int32_t CapturerInClientInner::InitIpcStream()
{
    AudioProcessConfig config = ConstructConfig();

    sptr<IStandardAudioService> gasp = CapturerInClientInner::GetAudioServerProxy();
    CHECK_AND_RETURN_RET_LOG(gasp != nullptr, ERR_OPERATION_FAILED, "Create failed, can not get service.");
    sptr<IRemoteObject> ipcProxy = gasp->CreateAudioProcess(config); // in plan: add ret
    CHECK_AND_RETURN_RET_LOG(ipcProxy != nullptr, ERR_OPERATION_FAILED, "InitIpcStream failed with null ipcProxy.");
    ipcStream_ = iface_cast<IpcStream>(ipcProxy);
    CHECK_AND_RETURN_RET_LOG(ipcStream_ != nullptr, ERR_OPERATION_FAILED, "InitIpcStream failed when iface_cast.");

    // in plan: old listener_ is destoried here, will server receive dieth notify?
    listener_ = sptr<CapturerStreamListener>::MakeSptr(shared_from_this());
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

int32_t CapturerInClientInner::GetAudioStreamInfo(AudioStreamParams &info)
{
    info = streamParams_;
    return SUCCESS;
}

bool CapturerInClientInner::CheckRecordingCreate(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid,
    SourceType sourceType)
{
    return AudioPolicyManager::GetInstance().CheckRecordingCreate(appTokenId, appFullTokenId, appUid, sourceType);
}

bool CapturerInClientInner::CheckRecordingStateChange(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid,
    AudioPermissionState state)
{
    return AudioPolicyManager::GetInstance().CheckRecordingStateChange(appTokenId, appFullTokenId, appUid, state);
}

int32_t CapturerInClientInner::GetAudioSessionID(uint32_t &sessionID)
{
    sessionID = sessionId_;
    return SUCCESS;
}

State CapturerInClientInner::GetState()
{
    // in plan
    return INVALID;
}

bool CapturerInClientInner::GetAudioTime(Timestamp &timestamp, Timestamp::Timestampbase base)
{
    // in plan
    return false;
}

int32_t CapturerInClientInner::GetBufferSize(size_t &bufferSize)
{
    // in plan
    return ERROR;
}

int32_t CapturerInClientInner::GetFrameCount(uint32_t &frameCount)
{
    // in plan
    return ERROR;
}

int32_t CapturerInClientInner::GetLatency(uint64_t &latency)
{
    // in plan
    return ERROR;
}

int32_t CapturerInClientInner::SetAudioStreamType(AudioStreamType audioStreamType)
{
    // in plan
    return ERROR;
}

int32_t CapturerInClientInner::SetVolume(float volume)
{
    // in plan
    return ERROR;
}

float CapturerInClientInner::GetVolume()
{
    // in plan
    return 0.0;
}

int32_t CapturerInClientInner::SetRenderRate(AudioRendererRate renderRate)
{
    // in plan
    return ERROR;
}

AudioRendererRate CapturerInClientInner::GetRenderRate()
{
    // in plan
    return RENDER_RATE_NORMAL; // not supported
}

int32_t CapturerInClientInner::SetStreamCallback(const std::shared_ptr<AudioStreamCallback> &callback)
{
    // in plan
    return ERROR;
}


int32_t CapturerInClientInner::SetRenderMode(AudioRenderMode renderMode)
{
    // in plan
    return ERROR;
}

AudioRenderMode CapturerInClientInner::GetRenderMode()
{
    // in plan
    return RENDER_MODE_NORMAL; // not supported
}

int32_t CapturerInClientInner::SetRendererWriteCallback(const std::shared_ptr<AudioRendererWriteCallback> &callback)
{
    // in plan
    return ERROR;
}

int32_t CapturerInClientInner::SetCaptureMode(AudioCaptureMode captureMode)
{
    // in plan
    return ERROR;
}

AudioCaptureMode CapturerInClientInner::GetCaptureMode()
{
    // in plan
    return CAPTURE_MODE_NORMAL;
}

int32_t CapturerInClientInner::SetCapturerReadCallback(const std::shared_ptr<AudioCapturerReadCallback> &callback)
{
    // in plan
    return ERROR;
}

int32_t CapturerInClientInner::GetBufferDesc(BufferDesc &bufDesc)
{
    // in plan
    return ERROR;
}

int32_t CapturerInClientInner::GetBufQueueState(BufferQueueState &bufState)
{
    // in plan
    return ERROR;
}

int32_t CapturerInClientInner::Enqueue(const BufferDesc &bufDesc)
{
    // in plan
    return ERROR;
}

int32_t CapturerInClientInner::Clear()
{
    // in plan
    return ERROR;
}


int32_t CapturerInClientInner::SetLowPowerVolume(float volume)
{
    // in plan
    return ERROR;
}

float CapturerInClientInner::GetLowPowerVolume()
{
    // in plan
    return 0.0;
}

int32_t CapturerInClientInner::SetOffloadMode(int32_t state, bool isAppBack)
{
    // in plan
    return ERROR;
}

int32_t CapturerInClientInner::UnsetOffloadMode()
{
    // in plan
    return ERROR;
}

float CapturerInClientInner::GetSingleStreamVolume()
{
    // in plan
    return 0.0;
}

AudioEffectMode CapturerInClientInner::GetAudioEffectMode()
{
    // in plan
    return EFFECT_NONE;
}

int32_t CapturerInClientInner::SetAudioEffectMode(AudioEffectMode effectMode)
{
    // in plan
    return ERROR;
}

int64_t CapturerInClientInner::GetFramesWritten()
{
    // in plan
    return -1;
}

int64_t CapturerInClientInner::GetFramesRead()
{
    // in plan
    return -1;
}


void CapturerInClientInner::SetInnerCapturerState(bool isInnerCapturer)
{
    isInnerCapturer_ = isInnerCapturer;
    AUDIO_INFO_LOG("SetInnerCapturerState %{public}s", (isInnerCapturer_ ? "true" : "false"));
    return;
}

void CapturerInClientInner::SetWakeupCapturerState(bool isWakeupCapturer)
{
    isWakeupCapturer_ = isWakeupCapturer;
    AUDIO_INFO_LOG("SetWakeupCapturerState %{public}s", (isWakeupCapturer_ ? "true" : "false"));
    return;
}

void CapturerInClientInner::SetCapturerSource(int capturerSource)
{
    // capturerSource is kept in capturerInfo_, no need to be set again.
    (void)capturerSource;
    return;
}

void CapturerInClientInner::SetPrivacyType(AudioPrivacyType privacyType)
{
    // in plan
    return;
}

bool CapturerInClientInner::StartAudioStream(StateChangeCmdType cmdType)
{
    // in plan
    return false;
}

bool CapturerInClientInner::PauseAudioStream(StateChangeCmdType cmdType)
{
    // in plan
    return false;
}

bool CapturerInClientInner::StopAudioStream()
{
    // in plan
    return false;
}

bool CapturerInClientInner::ReleaseAudioStream(bool releaseRunner)
{
    // in plan
    return false;
}

bool CapturerInClientInner::FlushAudioStream()
{
    // in plan
    return false;
}

bool CapturerInClientInner::DrainAudioStream()
{
    // in plan
    return false;
}

void CapturerInClientInner::SetPreferredFrameSize(int32_t frameSize)
{
    AUDIO_WARNING_LOG("Not Supported Yet");
}

int32_t CapturerInClientInner::Write(uint8_t *pcmBuffer, size_t pcmBufferSize, uint8_t *metaBuffer, size_t metaBufferSize)
{
     AUDIO_ERR_LOG("Write is not supported");
    return ERR_INVALID_OPERATION;
}

int32_t CapturerInClientInner::Write(uint8_t *buffer, size_t buffer_size)
{
    // in plan
    return ERROR;
}

int32_t CapturerInClientInner::Read(uint8_t &buffer, size_t userSize, bool isBlockingRead)
{
    // in plan
    std::unique_lock<std::mutex> lock(readDataMutex_);
    std::cv_status stat = readDataCV_.wait_for(lock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS));
    CHECK_AND_RETURN_RET_LOG(stat == std::cv_status::no_timeout, ERROR, "write data time out");
    return ERROR;
}

uint32_t CapturerInClientInner::GetUnderflowCount()
{
    // in plan
    return 0;
}

void CapturerInClientInner::SetRendererPositionCallback(int64_t markPosition, const
    std::shared_ptr<RendererPositionCallback> &callback)
       
{
    // in plan
}

void CapturerInClientInner::UnsetRendererPositionCallback()
{
    // in plan
}

void CapturerInClientInner::SetRendererPeriodPositionCallback(int64_t markPosition,
        const std::shared_ptr<RendererPeriodPositionCallback> &callback)
{
    // in plan
}

void CapturerInClientInner::UnsetRendererPeriodPositionCallback()
{
    // in plan
}

void CapturerInClientInner::SetCapturerPositionCallback(int64_t markPosition, const
    std::shared_ptr<CapturerPositionCallback> &callback)
       
{
    // in plan
}

void CapturerInClientInner::UnsetCapturerPositionCallback()
{
    // in plan
}

void CapturerInClientInner::SetCapturerPeriodPositionCallback(int64_t markPosition, const
    std::shared_ptr<CapturerPeriodPositionCallback> &callback)
{
    // in plan
}

void CapturerInClientInner::UnsetCapturerPeriodPositionCallback()
{
    // in plan
}

int32_t CapturerInClientInner::SetRendererSamplingRate(uint32_t sampleRate)
{
    // in plan
    return ERROR;
}

uint32_t CapturerInClientInner::GetRendererSamplingRate()
{
    // in plan
    return 0; // not supported
}

int32_t CapturerInClientInner::SetBufferSizeInMsec(int32_t bufferSizeInMsec)
{
    // in plan
    return ERROR;
}

void CapturerInClientInner::SetApplicationCachePath(const std::string cachePath)
{
    cachePath_ = cachePath;
    AUDIO_INFO_LOG("SetApplicationCachePath to %{publid}s", cachePath_.c_str());
}

int32_t CapturerInClientInner::SetChannelBlendMode(ChannelBlendMode blendMode)
{
    // in plan
    return ERROR;
}

int32_t CapturerInClientInner::SetVolumeWithRamp(float volume, int32_t duration)
{
    // in plan
    return ERROR;
}

void CapturerInClientInner::SetStreamTrackerState(bool trackerRegisteredState)
{
    // in plan
    return;
}

void CapturerInClientInner::GetSwitchInfo(IAudioStream::SwitchInfo& info)
{
    // in plan
}

IAudioStream::StreamClass CapturerInClientInner::GetStreamClass()
{
    return PA_STREAM;
}
} // namespace AudioStandard
} // namespace OHOS
#endif // FAST_AUDIO_STREAM_H
