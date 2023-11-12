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

#include "ipc_stream.h"
#include "audio_log.h"
#include "audio_errors.h"

namespace OHOS {
namespace AudioStandard {
class CapturerInClientInner : public CapturerInClient {
public:
    CapturerInClientInner(AudioStreamType eStreamType, int32_t appUid);
    ~CapturerInClientInner();

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

private:
    AudioStreamType eStreamType_;
    int32_t appUid_;
};

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

void CapturerInClientInner::SetClientID(int32_t clientPid, int32_t clientUid)
{
    // in plan
    return;
}

void CapturerInClientInner::SetRendererInfo(const AudioRendererInfo &rendererInfo)
{
    return;
}

void CapturerInClientInner::SetCapturerInfo(const AudioCapturerInfo &capturerInfo)
{
    // in plan
    return;
}

int32_t CapturerInClientInner::SetAudioStreamInfo(const AudioStreamParams info,
        const std::shared_ptr<AudioClientTracker> &proxyObj)
{
    // in plan
    return ERROR;
}

int32_t CapturerInClientInner::GetAudioStreamInfo(AudioStreamParams &info)
{
    // in plan
    return ERROR;
}

bool CapturerInClientInner::CheckRecordingCreate(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid)
{
    // in plan
    return false;
}

bool CapturerInClientInner::CheckRecordingStateChange(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid,
    AudioPermissionState state)
{
    // in plan
    return false;
}

int32_t CapturerInClientInner::GetAudioSessionID(uint32_t &sessionID)
{
    // in plan
    return ERROR;
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
    // in plan
    return;
}

void CapturerInClientInner::SetWakeupCapturerState(bool isWakeupCapturer)
{
    // in plan
    return;
}

void CapturerInClientInner::SetCapturerSource(int capturerSource)
{
    // in plan
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

int32_t CapturerInClientInner::Write(uint8_t *buffer, size_t buffer_size)
{
    // in plan
    return ERROR;
}

int32_t CapturerInClientInner::Read(uint8_t &buffer, size_t userSize, bool isBlockingRead)
{
    // in plan
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
    // in plan
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
