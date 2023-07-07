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

#include <chrono>
#include <thread>
#include <vector>

#include "audio_errors.h"
#include "audio_log.h"
#include "audio_utils.h"

#include "fast_audio_stream.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
FastAudioStream::FastAudioStream(AudioStreamType eStreamType, AudioMode eMode, int32_t appUid)
    : eStreamType_(eStreamType),
      eMode_(eMode),
      state_(NEW),
      renderMode_(RENDER_MODE_NORMAL),
      captureMode_(CAPTURE_MODE_NORMAL)
{
    AUDIO_DEBUG_LOG("FastAudioStream ctor, appUID = %{public}d", appUid);
}

FastAudioStream::~FastAudioStream()
{
    if (state_ != RELEASED && state_ != NEW) {
        ReleaseAudioStream(false);
    }
}

void FastAudioStream::SetClientID(int32_t clientPid, int32_t clientUid)
{
    //todo
    AUDIO_DEBUG_LOG("Set client PID: %{public}d, UID: %{public}d", clientPid, clientUid);
    clientPid_ = clientPid;
    clientUid_ = clientUid;
}

void FastAudioStream::SetRendererInfo(const AudioRendererInfo &rendererInfo)
{
    rendererInfo_ = rendererInfo;
}

void FastAudioStream::SetCapturerInfo(const AudioCapturerInfo &capturerInfo)
{
    capturerInfo_ = capturerInfo;
}

int32_t FastAudioStream::SetAudioStreamInfo(const AudioStreamParams info,
    const std::shared_ptr<AudioClientTracker> &proxyObj)
{
    AUDIO_INFO_LOG("FastAudioStreamInfo, Sampling rate: %{public}d, channels: %{public}d, format: %{public}d,"
        " stream type: %{public}d", info.samplingRate, info.channels, info.format, eStreamType_);
    if (state_ != NEW) {
        AUDIO_INFO_LOG("FastAudioStream: State is not new, release existing stream");
        StopAudioStream();
        ReleaseAudioStream(false);
    }
    if (eMode_ == AUDIO_MODE_PLAYBACK) {
        AUDIO_DEBUG_LOG("FastAudioStream: Initialize playback");
        AudioProcessConfig config;
        config.appInfo.appPid = clientPid_;
        config.appInfo.appUid = clientUid_;
        config.audioMode = eMode_;
        config.rendererInfo.contentType = CONTENT_TYPE_MUSIC;
        config.rendererInfo.streamUsage = STREAM_USAGE_MEDIA;
        config.rendererInfo.rendererFlags = 4; // 4 for test
        config.streamInfo.channels = STEREO;
        config.streamInfo.encoding = ENCODING_PCM;
        config.streamInfo.format = SAMPLE_S16LE;
        config.streamInfo.samplingRate = SAMPLE_RATE_48000;
        config.isRemote = true;
        spkProcessClient_ = AudioProcessInClient::Create(config);
        CHECK_AND_RETURN_RET_LOG(spkProcessClient_ != nullptr, ERR_INVALID_HANDLE,
            "Client test creat process client fail.");
    } else if (eMode_ == AUDIO_MODE_RECORD) {
        AUDIO_DEBUG_LOG("FastAudioStream: Initialize recording");
    } else {
        AUDIO_ERR_LOG("FastAudioStream: error eMode.");
        return ERR_INVALID_OPERATION;
    }

    state_ = PREPARED;
    return SUCCESS;
}

int32_t FastAudioStream::GetAudioStreamInfo(AudioStreamParams &audioStreamInfo)
{
    AUDIO_INFO_LOG("GetAudioStreamInfo in");

    return SUCCESS;
}

bool FastAudioStream::VerifyClientMicrophonePermission(uint32_t appTokenId, int32_t appUid, bool privacyFlag,
    AudioPermissionState state)
{
    AUDIO_INFO_LOG("VerifyClientPermission in");
    return true;
}

bool FastAudioStream::getUsingPemissionFromPrivacy(const std::string &permissionName, uint32_t appTokenId,
    AudioPermissionState state)
{
    AUDIO_INFO_LOG("getUsingPemissionFromPrivacy in");
    return true;
}

int32_t FastAudioStream::GetAudioSessionID(uint32_t &sessionID)
{
    AUDIO_INFO_LOG("GetAudioSessionID in");
    sessionID = 1; // test

    return SUCCESS;
}

State FastAudioStream::GetState()
{
    return state_;
}

bool FastAudioStream::GetAudioTime(Timestamp &timestamp, Timestamp::Timestampbase base)
{
    AUDIO_INFO_LOG("GetAudioTime in");
    return false;
}

int32_t FastAudioStream::GetBufferSize(size_t &bufferSize)
{
    AUDIO_INFO_LOG("GetBufferSize in");

    return SUCCESS;
}

int32_t FastAudioStream::GetFrameCount(uint32_t &frameCount)
{
    AUDIO_INFO_LOG("GetFrameCount in");

    return SUCCESS;
}

int32_t FastAudioStream::GetLatency(uint64_t &latency)
{
    AUDIO_INFO_LOG("GetLatency in");
    return SUCCESS;
}

int32_t FastAudioStream::SetAudioStreamType(AudioStreamType audioStreamType)
{
    AUDIO_INFO_LOG("SetAudioStreamType in");
    return SUCCESS;
}

int32_t FastAudioStream::SetVolume(float volume)
{
    AUDIO_INFO_LOG("SetVolume in");
    return SUCCESS;
}

float FastAudioStream::GetVolume()
{
    return 0.0f;
}

int32_t FastAudioStream::SetRenderRate(AudioRendererRate renderRate)
{
    renderRate_ = renderRate;
    return SUCCESS;
}

AudioRendererRate FastAudioStream::GetRenderRate()
{
    return renderRate_;
}

int32_t FastAudioStream::SetStreamCallback(const std::shared_ptr<AudioStreamCallback> &callback)
{
    AUDIO_INFO_LOG("SetStreamCallback in");

    return SUCCESS;
}

int32_t FastAudioStream::SetRenderMode(AudioRenderMode renderMode)
{
    AUDIO_INFO_LOG("SetRenderMode in");
    renderMode_ = renderMode;
    return SUCCESS;
}

AudioRenderMode FastAudioStream::GetRenderMode()
{
    AUDIO_INFO_LOG("GetRenderMode in");
    return renderMode_;
}

int32_t FastAudioStream::SetRendererWriteCallback(const std::shared_ptr<AudioRendererWriteCallback> &callback)
{
    AUDIO_INFO_LOG("SetRendererWriteCallback in.");
    if (!callback || !spkProcessClient_) {
        AUDIO_ERR_LOG("SetRendererWriteCallback callback is nullptr");
        return ERR_INVALID_PARAM;
    }
    spkProcClientCb_ = std::make_shared<FastAudioStreamCallback>(spkProcessClient_, callback);
    int32_t ret = spkProcessClient_->SaveDataCallback(spkProcClientCb_);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Client test save data callback fail, ret %{public}d.", ret);
    return SUCCESS;
}

int32_t FastAudioStream::SetCaptureMode(AudioCaptureMode captureMode)
{
    AUDIO_INFO_LOG("SetCaptureMode in.");
    captureMode_ = captureMode;
    return SUCCESS;
}

AudioCaptureMode FastAudioStream::GetCaptureMode()
{
    return captureMode_;
}

int32_t FastAudioStream::SetCapturerReadCallback(const std::shared_ptr<AudioCapturerReadCallback> &callback)
{
    AUDIO_INFO_LOG("SetCapturerReadCallback in.");
    return SUCCESS;
}

int32_t FastAudioStream::GetBufferDesc(BufferDesc &bufDesc)
{
    AUDIO_INFO_LOG("GetBufferDesc in.");
    if (!spkProcessClient_) {
        AUDIO_ERR_LOG("spkClient is null.");
    }
    int32_t ret = spkProcessClient_->GetBufferDesc(bufDesc);
    if (ret != SUCCESS || bufDesc.buffer == nullptr || bufDesc.bufLength ==0) {
        AUDIO_ERR_LOG("GetBufferDesc failed.");
        return -1;
    }
    return SUCCESS;
}

int32_t FastAudioStream::GetBufQueueState(BufferQueueState &bufState)
{
    AUDIO_INFO_LOG("GetBufQueueState in.");

    return SUCCESS;
}

int32_t FastAudioStream::Enqueue(const BufferDesc &bufDesc)
{
    AUDIO_INFO_LOG("Enqueue in");
    if (!spkProcessClient_) {
        AUDIO_ERR_LOG("spkClient is null.");
    }
    int32_t ret = spkProcessClient_->Enqueue(bufDesc);
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("Enqueue failed.");
        return -1;
    }
    return SUCCESS;
}

int32_t FastAudioStream::Clear()
{
    AUDIO_INFO_LOG("Clear in.");

    return SUCCESS;
}

int32_t FastAudioStream::SetLowPowerVolume(float volume)
{
    AUDIO_INFO_LOG("SetLowPowerVolume in.");
    return 0.0f;
}

float FastAudioStream::GetLowPowerVolume()
{
    AUDIO_INFO_LOG("GetLowPowerVolume in.");
    return 0.0f;
}

float FastAudioStream::GetSingleStreamVolume()
{
    AUDIO_INFO_LOG("GetSingleStreamVolume in.");
    return 0.0f;
}

AudioEffectMode FastAudioStream::GetAudioEffectMode()
{
    AUDIO_INFO_LOG("GetAudioEffectMode in.");
    return effectMode_;
}

int32_t FastAudioStream::SetAudioEffectMode(AudioEffectMode effectMode)
{
    AUDIO_INFO_LOG("SetAudioEffectMode in.");
    return SUCCESS;
}

int64_t FastAudioStream::GetFramesWritten()
{
    AUDIO_INFO_LOG("GetFramesWritten in.");
    return SUCCESS;
}

int64_t FastAudioStream::GetFramesRead()
{
    AUDIO_INFO_LOG("GetFramesRead in.");
    return SUCCESS;
}

bool FastAudioStream::StartAudioStream(StateChangeCmdType cmdType)
{
    AUDIO_INFO_LOG("StartAudioStream in.");
    if ((state_ != PREPARED) && (state_ != STOPPED) && (state_ != PAUSED)) {
        AUDIO_ERR_LOG("StartAudioStream Illegal state:%{public}u", state_);
        return false;
    }

    CHECK_AND_RETURN_RET_LOG(spkProcessClient_ != nullptr, false, "%{public}s process client is null.", __func__);
    int32_t ret = spkProcessClient_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "Client test stop fail, ret %{public}d.", ret);
    state_ = RUNNING;

    AUDIO_INFO_LOG("StartAudioStream SUCCESS, sessionId: %{public}d", sessionId_);
    return true;
}

bool FastAudioStream::PauseAudioStream(StateChangeCmdType cmdType)
{
    AUDIO_INFO_LOG("PauseAudioStream in");
    if (state_ != RUNNING) {
        AUDIO_ERR_LOG("PauseAudioStream: State is not RUNNING. Illegal state:%{public}u", state_);
        return false;
    }
    State oldState = state_;
    // Update state to stop write thread
    state_ = PAUSED;
    CHECK_AND_RETURN_RET_LOG(spkProcessClient_ != nullptr, false, "%{public}s process client is null.", __func__);
    int32_t ret = spkProcessClient_->Pause();
    if (ret != SUCCESS) {
        AUDIO_DEBUG_LOG("StreamPause fail,ret:%{public}d", ret);
        state_ = oldState;
        return false;
    }

    AUDIO_INFO_LOG("PauseAudioStream SUCCESS, sessionId: %{public}d", sessionId_);

    return true;
}

bool FastAudioStream::StopAudioStream()
{
    AUDIO_INFO_LOG("StopAudioStream in.");
    if ((state_ != RUNNING) && (state_ != PAUSED)) {
        AUDIO_ERR_LOG("StopAudioStream: State is not RUNNING. Illegal state:%{public}u", state_);
        return false;
    }
    State oldState = state_;
    state_ = STOPPED; // Set it before stopping as Read/Write and Stop can be called from different threads

    CHECK_AND_RETURN_RET_LOG(spkProcessClient_ != nullptr, false, "%{public}s process client is null.", __func__);
    int32_t ret = spkProcessClient_->Stop();
    if (ret != SUCCESS) {
        AUDIO_DEBUG_LOG("StreamStop fail,ret:%{public}d", ret);
        state_ = oldState;
        return false;
    }

    AUDIO_INFO_LOG("StopAudioStream SUCCESS, sessionId: %{public}d", sessionId_);
    return true;
}

bool FastAudioStream::FlushAudioStream()
{
    AUDIO_INFO_LOG("FlushAudioStream in.");
    return true;
}

bool FastAudioStream::DrainAudioStream()
{
    AUDIO_INFO_LOG("DrainAudioStream in.");

    AUDIO_INFO_LOG("Drain stream SUCCESS");
    return true;
}

bool FastAudioStream::ReleaseAudioStream(bool releaseRunner)
{
    AUDIO_INFO_LOG("ReleaseAudiostream in");
    if (state_ == RELEASED || state_ == NEW) {
        AUDIO_ERR_LOG("Illegal state: state = %{public}u", state_);
        return false;
    }
    // If state_ is RUNNING try to Stop it first and Release
    if (state_ == RUNNING) {
        StopAudioStream();
    }

    CHECK_AND_RETURN_RET_LOG(spkProcessClient_ != nullptr, false, "%{public}s process client is null.", __func__);
    spkProcessClient_->Release();
    state_ = RELEASED;
    AUDIO_INFO_LOG("ReleaseAudiostream SUCCESS, sessionId: %{public}d", sessionId_);
    return true;
}

int32_t FastAudioStream::Read(uint8_t &buffer, size_t userSize, bool isBlockingRead)
{
    AUDIO_INFO_LOG("Write in.");
    return 1;
}

size_t FastAudioStream::Write(uint8_t *buffer, size_t buffer_size)
{
    AUDIO_INFO_LOG("Write in.");
    return 1;
}

uint32_t FastAudioStream::GetUnderflowCount()
{
    AUDIO_INFO_LOG("GetUnderflowCount in.");
    return underFlowCount_;
}

void FastAudioStream::SetRendererPositionCallback(int64_t markPosition,
    const std::shared_ptr<RendererPositionCallback> &callback)
{
    AUDIO_INFO_LOG("Registering render frame position callback mark position");
}

void FastAudioStream::UnsetRendererPositionCallback()
{
    AUDIO_INFO_LOG("Unregistering render frame position callback");
}

void FastAudioStream::SetRendererPeriodPositionCallback(int64_t periodPosition,
    const std::shared_ptr<RendererPeriodPositionCallback> &callback)
{
    AUDIO_INFO_LOG("Registering render period position callback");
}

void FastAudioStream::UnsetRendererPeriodPositionCallback()
{
    AUDIO_INFO_LOG("Unregistering render period position callback");
}

void FastAudioStream::SetCapturerPositionCallback(int64_t markPosition,
    const std::shared_ptr<CapturerPositionCallback> &callback)
{
    AUDIO_INFO_LOG("Registering capture frame position callback, mark position");
}

void FastAudioStream::UnsetCapturerPositionCallback()
{
    AUDIO_INFO_LOG("Unregistering capture frame position callback");
}

void FastAudioStream::SetCapturerPeriodPositionCallback(int64_t periodPosition,
    const std::shared_ptr<CapturerPeriodPositionCallback> &callback)
{
    AUDIO_INFO_LOG("Registering period position callback");
}

void FastAudioStream::UnsetCapturerPeriodPositionCallback()
{
    AUDIO_INFO_LOG("Unregistering period position callback");
}

int32_t FastAudioStream::SetRendererSamplingRate(uint32_t sampleRate)
{
    AUDIO_INFO_LOG("SetStreamRendererSamplingRate %{public}d", sampleRate);
    if (sampleRate <= 0) {
        return -1;
    }
    rendererSampleRate_ = sampleRate;
    return SUCCESS;
}

uint32_t FastAudioStream::GetRendererSamplingRate()
{
    AUDIO_DEBUG_LOG("GetRendererSamplingRate in");
    return rendererSampleRate_;
}

int32_t FastAudioStream::SetBufferSizeInMsec(int32_t bufferSizeInMsec)
{
    AUDIO_DEBUG_LOG("SetBufferSizeInMsec in");
    return SUCCESS;
}

void FastAudioStream::SetApplicationCachePath(const std::string cachePath)
{
    AUDIO_DEBUG_LOG("SetApplicationCachePath in");
    cachePath_ = cachePath;
}
void FastAudioStream::SetInnerCapturerState(bool isInnerCapturer)
{
    AUDIO_DEBUG_LOG("SetInnerCapturerState in");
}

void FastAudioStream::SetPrivacyType(AudioPrivacyType privacyType)
{
    AUDIO_DEBUG_LOG("SetPrivacyType in");
}

void FastAudioStreamCallback::OnHandleData(size_t length)
{
    CHECK_AND_RETURN_LOG(renderCallback_!= nullptr, "%{public}s renderCallback_ is null.", __func__);
    renderCallback_->OnWriteData(length);
}
} // namespace AudioStandard
} // namespace OHOS
