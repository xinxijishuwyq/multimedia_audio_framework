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
#include "audio_server_death_recipient.h"
#include "audio_stream_tracker.h"
#include "audio_utils.h"
#include "ipc_stream_listener_stub.h"

namespace OHOS {
namespace AudioStandard {
namespace {
static const int32_t CREATE_TIMEOUT_IN_SECOND = 5; // 5S
}
class RendererInClientInner : public RendererInClient , public IpcStreamListenerStub {
public:
    RendererInClientInner(AudioStreamType eStreamType, int32_t appUid);
    ~RendererInClientInner();

    // IpcStreamListenerStub
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

    int32_t DeinitIpcStream();

    int32_t InitIpcStream();

    const AudioProcessConfig ConstructConfig();

    int32_t InitSharedBuffer();
    int32_t InitCacheBuffer(size_t targetSize);
private:
    AudioStreamType eStreamType_;
    int32_t appUid_;
    uint32_t sessionId_;

    std::unique_ptr<AudioStreamTracker> audioStreamTracker_;

    AudioRendererInfo audioRendererInfo_;
    AudioCapturerInfo capturerInfo_; // not in use
    int32_t clientPid_ = -1;
    int32_t clientUid_ = -1;

    AudioPrivacyType privacyType_ = PRIVACY_TYPE_PUBLIC;
    bool streamTrackerRegistered_ = false;

    AudioStreamParams streamParams_;
    std::atomic<State> state_;

    size_t cacheSizeInByte_ = 0;
    size_t clientSpanSizeInByte_ = 0;
    size_t sizePerFrameInByte_ = 4; // 16bit 2ch as default

    // ipc stream related
    sptr<IpcStream> ipcStream_ = nullptr;
    std::shared_ptr<OHAudioBuffer> clientBuffer_ = nullptr;

    std::unique_ptr<AudioRingCache> ringCache_ = nullptr;
};

std::shared_ptr<RendererInClient> RendererInClient::GetInstance(AudioStreamType eStreamType, int32_t appUid)
{
    return std::make_shared<RendererInClientInner>(eStreamType, appUid);
}

RendererInClientInner::RendererInClientInner(AudioStreamType eStreamType, int32_t appUid)
    : eStreamType_(eStreamType), appUid_(appUid)
{
    AUDIO_INFO_LOG("RendererInClientInner() with appUid:%{public}d", appUid_);
    audioStreamTracker_ =  std::make_unique<AudioStreamTracker>(AUDIO_MODE_PLAYBACK, appUid);
    state_ = NEW;
}

RendererInClientInner::~RendererInClientInner()
{
    AUDIO_INFO_LOG("~RendererInClientInner()");
}

int32_t RendererInClientInner::OnOperationHandled(Operation operation, int64_t result)
{
    // in plan: read/write operation may print many log, use debug log later
    AUDIO_INFO_LOG("OnOperationHandled() recv operation:%{public}d result:%{public}" PRId64".", operation, result);
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
    audioRendererInfo_ = rendererInfo;
    AUDIO_INFO_LOG("SetRendererInfo with flag %{public}d", audioRendererInfo_.rendererFlags);
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
        audioStreamTracker_->RegisterTracker(sessionId_, state_, audioRendererInfo_, capturerInfo_, proxyObj);
        streamTrackerRegistered_ = true;
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
    // in plan
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

    config.rendererInfo = audioRendererInfo_;
    if (audioRendererInfo_.rendererFlags != 0) {
        AUDIO_WARNING_LOG("ConstructConfig find renderer flag invalid:%{public}d", audioRendererInfo_.rendererFlags);
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

    int32_t ret = ipcStream_->RegisterStreamListener(this->AsObject());
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
    // in plan
    return ERROR;
}

State RendererInClientInner::GetState()
{
    // in plan
    return state_;
}

bool RendererInClientInner::GetAudioTime(Timestamp &timestamp, Timestamp::Timestampbase base)
{
    // in plan
    return false;
}

int32_t RendererInClientInner::GetBufferSize(size_t &bufferSize)
{
    // in plan
    return ERROR;
}

int32_t RendererInClientInner::GetFrameCount(uint32_t &frameCount)
{
    // in plan
    return ERROR;
}

int32_t RendererInClientInner::GetLatency(uint64_t &latency)
{
    // in plan
    return ERROR;
}

int32_t RendererInClientInner::SetAudioStreamType(AudioStreamType audioStreamType)
{
    // in plan
    return ERROR;
}

int32_t RendererInClientInner::SetVolume(float volume)
{
    // in plan
    return ERROR;
}

float RendererInClientInner::GetVolume()
{
    // in plan
    return 0.0;
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
    // in plan
    return ERROR;
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
    // in plan
    return ERROR;
}

AudioCaptureMode RendererInClientInner::GetCaptureMode()
{
    // in plan
    return CAPTURE_MODE_NORMAL; // not supported
}

int32_t RendererInClientInner::SetCapturerReadCallback(const std::shared_ptr<AudioCapturerReadCallback> &callback)
{
    // in plan
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
    return ERROR;
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
    // in plan
}

void RendererInClientInner::SetWakeupCapturerState(bool isWakeupCapturer)
{
    // in plan
}

void RendererInClientInner::SetCapturerSource(int capturerSource)
{
    // in plan
}

void RendererInClientInner::SetPrivacyType(AudioPrivacyType privacyType)
{
    privacyType_ = privacyType;
    // should we update it after create?
}

bool RendererInClientInner::StartAudioStream(StateChangeCmdType cmdType)
{
    // in plan
    return false;
}

bool RendererInClientInner::PauseAudioStream(StateChangeCmdType cmdType)
{
    // in plan
    return false;
}

bool RendererInClientInner::StopAudioStream()
{
    // in plan
    return false;
}

bool RendererInClientInner::ReleaseAudioStream(bool releaseRunner)
{
    // in plan
    return false;
}

bool RendererInClientInner::FlushAudioStream()
{
    // in plan
    return false;
}



bool RendererInClientInner::DrainAudioStream()
{
    // in plan
    return false;
}

int32_t RendererInClientInner::Write(uint8_t *buffer, size_t buffer_size)
{
    // in plan
    return ERROR;
}

int32_t RendererInClientInner::Read(uint8_t &buffer, size_t userSize, bool isBlockingRead)
{
    // in plan
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
    // in plan
}

void RendererInClientInner::UnsetCapturerPositionCallback()
{
    // in plan
}

void RendererInClientInner::SetCapturerPeriodPositionCallback(int64_t markPosition,
        const std::shared_ptr<CapturerPeriodPositionCallback> &callback)
{
    // in plan
}

void RendererInClientInner::UnsetCapturerPeriodPositionCallback()
{
    // in plan
}

int32_t RendererInClientInner::SetRendererSamplingRate(uint32_t sampleRate)
{
    // in plan
    return ERROR;
}

uint32_t RendererInClientInner::GetRendererSamplingRate()
{
    // in plan
    return 0;
}

int32_t RendererInClientInner::SetBufferSizeInMsec(int32_t bufferSizeInMsec)
{
    // in plan
    return ERROR;
}

void RendererInClientInner::SetApplicationCachePath(const std::string cachePath)
{
    // in plan
}

int32_t RendererInClientInner::SetChannelBlendMode(ChannelBlendMode blendMode)
{
    // in plan
    return ERROR;
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
