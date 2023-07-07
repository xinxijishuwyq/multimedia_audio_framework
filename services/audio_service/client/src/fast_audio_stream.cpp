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
    // audioStreamTracker_ =  std::make_unique<AudioStreamTracker>(eMode, appUid);
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
    // if (!IsFormatValid(info.format) || !IsSamplingRateValid(info.samplingRate) || !IsEncodingTypeValid(info.encoding)) {
    //     AUDIO_ERR_LOG("FastAudioStream: Unsupported audio parameter");
    //     return ERR_NOT_SUPPORTED;
    // }
    if (state_ != NEW) {
        AUDIO_INFO_LOG("FastAudioStream: State is not new, release existing stream");
        StopAudioStream();
        ReleaseAudioStream(false);
    }
    if (eMode_ == AUDIO_MODE_PLAYBACK) {
        AUDIO_DEBUG_LOG("FastAudioStream: Initialize playback");
        // if (!IsRendererChannelValid(info.channels)) {
        //     AUDIO_ERR_LOG("FastAudioStream: Invalid sink channel %{public}d", info.channels);
        //     return ERR_NOT_SUPPORTED;
        // }
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
        // if (!IsCapturerChannelValid(info.channels)) {
        //     AUDIO_ERR_LOG("FastAudioStream: Invalid source channel %{public}d", info.channels);
        //     return ERR_NOT_SUPPORTED;
        // }
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
    // if (GetAudioStreamParams(audioStreamInfo) != 0) {
    //     return ERR_OPERATION_FAILED;
    // }

    return SUCCESS;
}

bool FastAudioStream::VerifyClientMicrophonePermission(uint32_t appTokenId, int32_t appUid, bool privacyFlag,
    AudioPermissionState state)
{
    AUDIO_INFO_LOG("VerifyClientPermission in");
    // return AudioServiceClient::VerifyClientPermission(permissionName, appTokenId, appUid, privacyFlag, state);
    return true;
}

bool FastAudioStream::getUsingPemissionFromPrivacy(const std::string &permissionName, uint32_t appTokenId,
    AudioPermissionState state)
{
    AUDIO_INFO_LOG("getUsingPemissionFromPrivacy in");
    // return AudioServiceClient::getUsingPemissionFromPrivacy(permissionName, appTokenId, state);
    return true;
}

int32_t FastAudioStream::GetAudioSessionID(uint32_t &sessionID)
{
    // wuhaobo todo SessionId 如何实现？ AudioServiceClient::GetSessionID
    AUDIO_INFO_LOG("GetAudioSessionID in");
    sessionID = 1; // test
    // if ((state_ == RELEASED) || (state_ == NEW)) {
    //     return ERR_ILLEGAL_STATE;
    // }

    // if (GetSessionID(sessionID) != -1) {
    //     return ERR_INVALID_INDEX;
    // }

    // sessionId_ = sessionID;

    return SUCCESS;
}

State FastAudioStream::GetState()
{
    return state_;
}

bool FastAudioStream::GetAudioTime(Timestamp &timestamp, Timestamp::Timestampbase base)
{
    AUDIO_INFO_LOG("GetAudioTime in");
    // if (state_ == STOPPED) {
    //     return false;
    // }
    // uint64_t paTimeStamp = 0;
    // if (GetCurrentTimeStamp(paTimeStamp) == SUCCESS) {
    //     if (resetTime_) {
    //         AUDIO_INFO_LOG("AudioStream::GetAudioTime resetTime_ %{public}d", resetTime_);
    //         resetTime_ = false;
    //         resetTimestamp_ = paTimeStamp;
    //     }

    //     timestamp.time.tv_sec = static_cast<time_t>((paTimeStamp - resetTimestamp_) / TIME_CONVERSION_US_S);
    //     timestamp.time.tv_nsec
    //         = static_cast<time_t>(((paTimeStamp - resetTimestamp_) - (timestamp.time.tv_sec * TIME_CONVERSION_US_S))
    //                               * TIME_CONVERSION_NS_US);
    //     timestamp.time.tv_sec += baseTimestamp_.tv_sec;
    //     timestamp.time.tv_nsec += baseTimestamp_.tv_nsec;
    //     timestamp.time.tv_sec += (timestamp.time.tv_nsec / TIME_CONVERSION_NS_S);
    //     timestamp.time.tv_nsec = (timestamp.time.tv_nsec % TIME_CONVERSION_NS_S);

    //     return true;
    // }
    return false;
}

int32_t FastAudioStream::GetBufferSize(size_t &bufferSize)
{
    AUDIO_INFO_LOG("GetBufferSize in");
    // if (GetMinimumBufferSize(bufferSize) != 0) {
    //     return ERR_OPERATION_FAILED;
    // }

    return SUCCESS;
}

int32_t FastAudioStream::GetFrameCount(uint32_t &frameCount)
{
    AUDIO_INFO_LOG("GetFrameCount in");
    // if (GetMinimumFrameCount(frameCount) != 0) {
    //     return ERR_OPERATION_FAILED;
    // }

    return SUCCESS;
}

int32_t FastAudioStream::GetLatency(uint64_t &latency)
{
    AUDIO_INFO_LOG("GetLatency in");
    // if (GetAudioLatency(latency) != SUCCESS) {
    //     return ERR_OPERATION_FAILED;
    // } else {
    //     return SUCCESS;
    // }
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
    // return SetStreamVolume(volume);
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
    // if (callback == nullptr) {
    //     AUDIO_ERR_LOG("AudioStream::SetStreamCallback failed. callback == nullptr");
    //     return ERR_INVALID_PARAM;
    // }

    // SaveStreamCallback(callback);

    return SUCCESS;
}

int32_t FastAudioStream::SetRenderMode(AudioRenderMode renderMode)
{
    AUDIO_INFO_LOG("SetRenderMode in");
    renderMode_ = renderMode;
    // int32_t ret = SetAudioRenderMode(renderMode);
    // if (ret) {
    //     AUDIO_ERR_LOG("AudioStream::SetRenderMode: renderMode: %{public}d failed", renderMode);
    //     return ERR_OPERATION_FAILED;
    // }
    // renderMode_ = renderMode;

    // lock_guard<mutex> lock(bufferQueueLock_);

    // for (int32_t i = 0; i < MAX_WRITECB_NUM_BUFFERS; ++i) {
    //     size_t length;
    //     GetMinimumBufferSize(length);
    //     AUDIO_INFO_LOG("AudioServiceClient:: GetMinimumBufferSize: %{public}zu", length);

    //     writeBufferPool_[i] = std::make_unique<uint8_t[]>(length);
    //     if (writeBufferPool_[i] == nullptr) {
    //         AUDIO_INFO_LOG(
    //             "AudioServiceClient::GetBufferDescriptor writeBufferPool_[i]==nullptr. Allocate memory failed.");
    //         return ERR_OPERATION_FAILED;
    //     }

    //     BufferDesc bufDesc {};
    //     bufDesc.buffer = writeBufferPool_[i].get();
    //     bufDesc.bufLength = length;
    //     freeBufferQ_.emplace(bufDesc);
    // }
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
    // int32_t ret = SetAudioCaptureMode(captureMode);
    // if (ret) {
    //     AUDIO_ERR_LOG("FastAudioStream::SetCaptureMode: captureMode: %{public}d failed", captureMode);
    //     return ERR_OPERATION_FAILED;
    // }
    // captureMode_ = captureMode;

    // lock_guard<mutex> lock(bufferQueueLock_);

    // for (int32_t i = 0; i < MAX_READCB_NUM_BUFFERS; ++i) {
    //     size_t length;
    //     GetMinimumBufferSize(length);
    //     AUDIO_INFO_LOG("FastAudioStream::SetCaptureMode: length %{public}zu", length);

    //     readBufferPool_[i] = std::make_unique<uint8_t[]>(length);
    //     if (readBufferPool_[i] == nullptr) {
    //         AUDIO_INFO_LOG("FastAudioStream::SetCaptureMode readBufferPool_[i]==nullptr. Allocate memory failed.");
    //         return ERR_OPERATION_FAILED;
    //     }

    //     BufferDesc bufDesc {};
    //     bufDesc.buffer = readBufferPool_[i].get();
    //     bufDesc.bufLength = length;
    //     freeBufferQ_.emplace(bufDesc);
    // }
    return SUCCESS;
}

AudioCaptureMode FastAudioStream::GetCaptureMode()
{
    // return GetAudioCaptureMode();
    return captureMode_;
}

int32_t FastAudioStream::SetCapturerReadCallback(const std::shared_ptr<AudioCapturerReadCallback> &callback)
{
    AUDIO_INFO_LOG("SetCapturerReadCallback in.");
    return SUCCESS;
    // if (captureMode_ != CAPTURE_MODE_CALLBACK) {
    //     AUDIO_ERR_LOG("SetCapturerReadCallback not supported. Capture mode is not callback.");
    //     return ERR_INCORRECT_MODE;
    // }

    // if (!callback) {
    //     AUDIO_ERR_LOG("SetCapturerReadCallback callback is nullptr");
    //     return ERR_INVALID_PARAM;
    // }
    // return AudioServiceClient::SetCapturerReadCallback(callback);
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
    // if ((renderMode_ != RENDER_MODE_CALLBACK) && (captureMode_ != CAPTURE_MODE_CALLBACK)) {
    //     AUDIO_ERR_LOG("AudioStream::GetBufQueueState not supported. Render or Capture mode is not callback.");
    //     return ERR_INCORRECT_MODE;
    // }

    // lock_guard<mutex> lock(bufferQueueLock_);

    // if (renderMode_ == RENDER_MODE_CALLBACK) {
    //     bufState.numBuffers = filledBufferQ_.size();
    // }

    // if (captureMode_ == CAPTURE_MODE_CALLBACK) {
    //     bufState.numBuffers = freeBufferQ_.size();
    // }

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
    // if ((renderMode_ != RENDER_MODE_CALLBACK) && (captureMode_ != CAPTURE_MODE_CALLBACK)) {
    //     AUDIO_ERR_LOG("AudioStream::Clear not supported. Render or capture mode is not callback.");
    //     return ERR_INCORRECT_MODE;
    // }

    // lock_guard<mutex> lock(bufferQueueLock_);

    // while (!filledBufferQ_.empty()) {
    //     freeBufferQ_.emplace(filledBufferQ_.front());
    //     filledBufferQ_.pop();
    // }

    return SUCCESS;
}

int32_t FastAudioStream::SetLowPowerVolume(float volume)
{
    AUDIO_INFO_LOG("SetLowPowerVolume in.");
    // return SetStreamLowPowerVolume(volume);
    return 0.0f;
}

float FastAudioStream::GetLowPowerVolume()
{
    AUDIO_INFO_LOG("GetLowPowerVolume in.");
    // return GetStreamLowPowerVolume();
    return 0.0f;
}

float FastAudioStream::GetSingleStreamVolume()
{
    AUDIO_INFO_LOG("GetSingleStreamVolume in.");
    // return GetSingleStreamVol();
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
    // return SetStreamAudioEffectMode(effectMode);
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

    // int32_t ret = StartStream(cmdType);
    // if (ret != SUCCESS) {
    //     AUDIO_ERR_LOG("StartStream Start failed:%{public}d", ret);
    //     return false;
    // }

    // resetTime_ = true;
    // int32_t retCode = clock_gettime(CLOCK_MONOTONIC, &baseTimestamp_);
    // if (retCode != 0) {
    //     AUDIO_ERR_LOG("AudioStream::StartAudioStream get system elapsed time failed: %d", retCode);
    // }

    // isFirstRead_ = true;
    // isFirstWrite_ = true;
    // state_ = RUNNING;

    // if (renderMode_ == RENDER_MODE_CALLBACK) {
    //     isReadyToWrite_ = true;
    //     writeThread_ = std::make_unique<std::thread>(&AudioStream::WriteCbTheadLoop, this);
    // } else if (captureMode_ == CAPTURE_MODE_CALLBACK) {
    //     isReadyToRead_ = true;
    //     readThread_ = std::make_unique<std::thread>(&AudioStream::ReadCbThreadLoop, this);
    // }

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

    // if (captureMode_ == CAPTURE_MODE_CALLBACK) {
    //     isReadyToRead_ = false;
    //     if (readThread_ && readThread_->joinable()) {
    //         readThread_->join();
    //     }
    // }

    // // Ends the WriteCb thread
    // if (renderMode_ == RENDER_MODE_CALLBACK) {
    //     isReadyToWrite_ = false;
    //     // wake write thread to make pause faster
    //     bufferQueueCV_.notify_all();
    //     if (writeThread_ && writeThread_->joinable()) {
    //         writeThread_->join();
    //     }
    // }

    // AUDIO_DEBUG_LOG("AudioStream::PauseAudioStream:renderMode_ : %{public}d state_: %{public}d", renderMode_, state_);
    // int32_t ret = PauseStream(cmdType);
    // if (ret != SUCCESS) {
    //     AUDIO_DEBUG_LOG("StreamPause fail,ret:%{public}d", ret);
    //     state_ = oldState;
    //     return false;
    // }

    AUDIO_INFO_LOG("PauseAudioStream SUCCESS, sessionId: %{public}d", sessionId_);

    // flush stream after stream paused
    // FlushAudioStream();

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
    // if (captureMode_ == CAPTURE_MODE_CALLBACK) {
    //     isReadyToRead_ = false;
    //     if (readThread_ && readThread_->joinable()) {
    //         readThread_->join();
    //     }
    // }

    // if (renderMode_ == RENDER_MODE_CALLBACK) {
    //     isReadyToWrite_ = false;
    //     if (writeThread_ && writeThread_->joinable()) {
    //         writeThread_->join();
    //     }
    // }

    // int32_t ret = StopStream();
    // if (ret != SUCCESS) {
    //     AUDIO_DEBUG_LOG("StreamStop fail,ret:%{public}d", ret);
    //     state_ = oldState;
    //     return false;
    // }

    AUDIO_INFO_LOG("StopAudioStream SUCCESS, sessionId: %{public}d", sessionId_);
    return true;
}

bool FastAudioStream::FlushAudioStream()
{
    AUDIO_INFO_LOG("FlushAudioStream in.");
    // Trace trace("AudioStream::FlushAudioStream");
    // if ((state_ != RUNNING) && (state_ != PAUSED) && (state_ != STOPPED)) {
    //     AUDIO_ERR_LOG("FlushAudioStream: State is not RUNNING. Illegal state:%{public}u", state_);
    //     return false;
    // }

    // int32_t ret = FlushStream();
    // if (ret != SUCCESS) {
    //     AUDIO_DEBUG_LOG("Flush stream fail,ret:%{public}d", ret);
    //     return false;
    // }

    // AUDIO_INFO_LOG("Flush stream SUCCESS, sessionId: %{public}d", sessionId_);
    return true;
}

bool FastAudioStream::DrainAudioStream()
{
    AUDIO_INFO_LOG("DrainAudioStream in.");
    // if (state_ != RUNNING) {
    //     AUDIO_ERR_LOG("DrainAudioStream: State is not RUNNING. Illegal  state:%{public}u", state_);
    //     return false;
    // }

    // int32_t ret = DrainStream();
    // if (ret != SUCCESS) {
    //     AUDIO_DEBUG_LOG("Drain stream fail,ret:%{public}d", ret);
    //     return false;
    // }

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
    // if (userSize <= 0) {
    //     AUDIO_ERR_LOG("Invalid userSize:%{public}zu", userSize);
    //     return ERR_INVALID_PARAM;
    // }

    // if (state_ != RUNNING) {
    //     AUDIO_ERR_LOG("Read: State is not RUNNNIG. Illegal  state:%{public}u", state_);
    //     return ERR_ILLEGAL_STATE;
    // }

    // if (isFirstRead_) {
    //     FlushAudioStream();
    //     isFirstRead_ = false;
    // }

    // StreamBuffer stream;
    // stream.buffer = &buffer;
    // stream.bufferLen = userSize;
    // int32_t readLen = ReadStream(stream, isBlockingRead);
    // if (readLen < 0) {
    //     AUDIO_ERR_LOG("ReadStream fail,ret:%{public}d", readLen);
    //     return ERR_INVALID_READ;
    // }

    // return readLen;
}

size_t FastAudioStream::Write(uint8_t *buffer, size_t buffer_size)
{
    AUDIO_INFO_LOG("Write in.");
    // Trace trace("AudioStream::Write");
    // if (renderMode_ == RENDER_MODE_CALLBACK) {
    //     AUDIO_ERR_LOG("AudioStream::Write not supported. RenderMode is callback");
    //     return ERR_INCORRECT_MODE;
    // }

    // if ((buffer == nullptr) || (buffer_size <= 0)) {
    //     AUDIO_ERR_LOG("Invalid buffer size:%{public}zu", buffer_size);
    //     return ERR_INVALID_PARAM;
    // }

    // if (state_ != RUNNING) {
    //     AUDIO_ERR_LOG("Write: Illegal  state:%{public}u", state_);
    //     // To allow context switch for APIs running in different thread contexts
    //     std::this_thread::sleep_for(std::chrono::microseconds(WRITE_RETRY_DELAY_IN_US));
    //     return ERR_ILLEGAL_STATE;
    // }

    // int32_t writeError;
    // StreamBuffer stream;
    // stream.buffer = buffer;
    // stream.bufferLen = buffer_size;

    // if (isFirstWrite_) {
    //     if (RenderPrebuf(stream.bufferLen)) {
    //         return ERR_WRITE_FAILED;
    //     }
    //     isFirstWrite_ = false;
    // }

    // size_t bytesWritten = WriteStream(stream, writeError);
    // if (writeError != 0) {
    //     AUDIO_ERR_LOG("WriteStream fail,writeError:%{public}d", writeError);
    //     return ERR_WRITE_FAILED;
    // }
    // return bytesWritten;
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
