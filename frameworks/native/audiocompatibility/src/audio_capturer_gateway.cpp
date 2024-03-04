/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define LOG_TAG "AudioCapturerGateway"

#include "audio_errors.h"
#include "audio_policy_manager.h"
#include "audio_capturer_gateway.h"
#include "audio_container_stream_base.h"
#include "audio_log.h"
#include "audio_capturer.h"

namespace OHOS {
namespace AudioStandard {
AudioCapturer::~AudioCapturer() = default;

AudioCapturerGateway::~AudioCapturerGateway()
{
    CapturerState state = GetStatus();
    if (state != CAPTURER_RELEASED && state != CAPTURER_NEW) {
        Release();
    }
}

AudioCapturerGateway::AudioCapturerGateway(AudioStreamType audioStreamType)
{
    audioStream_ = std::make_shared<AudioContainerCaptureStream>(audioStreamType, AUDIO_MODE_RECORD);
    if (audioStream_) {
    AUDIO_DEBUG_LOG("Audio stream created");
    }
}

int32_t AudioCapturerGateway::GetFrameCount(uint32_t &frameCount) const
{
    return audioStream_->GetFrameCount(frameCount);
}

int32_t AudioCapturerGateway::SetParams(const AudioCapturerParams params)
{
    AudioStreamParams audioStreamParams;
    audioStreamParams.format = params.audioSampleFormat;
    audioStreamParams.samplingRate = params.samplingRate;
    audioStreamParams.channels = params.audioChannel;
    audioStreamParams.encoding = params.audioEncoding;
    AUDIO_INFO_LOG("SetAudioStreamInfo");
    return audioStream_->SetAudioStreamInfo(audioStreamParams);
}

int32_t AudioCapturerGateway::SetCapturerCallback(const std::shared_ptr<AudioCapturerCallback> &callback)
{
    // If the client is using the deprecated SetParams API. SetCapturerCallback must be invoked, after SetParams.
    // In general, callbacks can only be set after the capturer state is  PREPARED.
    CapturerState state = GetStatus();
    CHECK_AND_RETURN_RET_LOG(state != CAPTURER_NEW && state != CAPTURER_RELEASED, ERR_ILLEGAL_STATE,
        "ncorrect state:%{public}d to register cb", state);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_PARAM,
        "callback param is null");

    // Save and Set reference for stream callback. Order is important here.
    if (audioStreamCallback_ == nullptr) {
        audioStreamCallback_ = std::make_shared<AudioStreamCapturerCallback>();
        CHECK_AND_RETURN_RET_LOG(audioStreamCallback_ != nullptr, ERROR,
            "Failed to allocate memory for audioStreamCallback_");
    }
    AUDIO_DEBUG_LOG("SetCapturerCallback");
    std::shared_ptr<AudioStreamCapturerCallback> cbStream =
        std::static_pointer_cast<AudioStreamCapturerCallback>(audioStreamCallback_);
    cbStream->SaveCallback(callback);
    (void)audioStream_->SetStreamCallback(audioStreamCallback_);

    return SUCCESS;
}

int32_t AudioCapturerGateway::GetParams(AudioCapturerParams &params) const
{
    AudioStreamParams audioStreamParams;
    int32_t result = audioStream_->GetAudioStreamInfo(audioStreamParams);
    if (SUCCESS == result) {
        params.audioSampleFormat = static_cast<AudioSampleFormat>(audioStreamParams.format);
        params.samplingRate = static_cast<AudioSamplingRate>(audioStreamParams.samplingRate);
        params.audioChannel = static_cast<AudioChannel>(audioStreamParams.channels);
        params.audioEncoding = static_cast<AudioEncodingType>(audioStreamParams.encoding);
    }

    return result;
}

int32_t AudioCapturerGateway::GetCapturerInfo(AudioCapturerInfo &capturerInfo) const
{
    capturerInfo = capturerInfo_;

    return SUCCESS;
}

int32_t AudioCapturerGateway::GetStreamInfo(AudioStreamInfo &streamInfo) const
{
    AudioStreamParams audioStreamParams;
    int32_t result = audioStream_->GetAudioStreamInfo(audioStreamParams);
    if (SUCCESS == result) {
        streamInfo.format = static_cast<AudioSampleFormat>(audioStreamParams.format);
        streamInfo.samplingRate = static_cast<AudioSamplingRate>(audioStreamParams.samplingRate);
        streamInfo.channels = static_cast<AudioChannel>(audioStreamParams.channels);
        streamInfo.encoding = static_cast<AudioEncodingType>(audioStreamParams.encoding);
    }

    return result;
}


int32_t AudioCapturerGateway::SetCapturerPositionCallback(int64_t markPosition,
    const std::shared_ptr<CapturerPositionCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG((callback != nullptr) && (markPosition > 0),
        ERR_INVALID_PARAM, "input param is invalid");

    audioStream_->SetCapturerPositionCallback(markPosition, callback);

    return SUCCESS;
}

void AudioCapturerGateway::UnsetCapturerPositionCallback()
{
    audioStream_->UnsetCapturerPositionCallback();
}

int32_t AudioCapturerGateway::SetCapturerPeriodPositionCallback(int64_t frameNumber,
    const std::shared_ptr<CapturerPeriodPositionCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG((callback != nullptr) && (frameNumber > 0), ERR_INVALID_PARAM,
        "input param is invalid");

    audioStream_->SetCapturerPeriodPositionCallback(frameNumber, callback);

    return SUCCESS;
}

void AudioCapturerGateway::UnsetCapturerPeriodPositionCallback()
{
    audioStream_->UnsetCapturerPeriodPositionCallback();
}

bool AudioCapturerGateway::Start() const
{
    return audioStream_->StartAudioStream();
}

int32_t AudioCapturerGateway::Read(uint8_t &buffer, size_t userSize, bool isBlockingRead) const
{
    return audioStream_->Read(buffer, userSize, isBlockingRead);
}

CapturerState AudioCapturerGateway::GetStatus() const
{
    return (CapturerState)audioStream_->GetState();
}

bool AudioCapturerGateway::GetAudioTime(Timestamp &timestamp, Timestamp::Timestampbase base) const
{
    return audioStream_->GetAudioTime(timestamp, base);
}

bool AudioCapturerGateway::Pause() const
{
    return audioStream_->PauseAudioStream();
}

bool AudioCapturerGateway::Stop() const
{
    return audioStream_->StopAudioStream();
}

bool AudioCapturerGateway::Flush() const
{
    return audioStream_->FlushAudioStream();
}

bool AudioCapturerGateway::Release() const
{
    return audioStream_->ReleaseAudioStream();
}

int32_t AudioCapturerGateway::GetBufferSize(size_t &bufferSize) const
{
    return audioStream_->GetBufferSize(bufferSize);
}

int32_t AudioCapturerGateway::GetAudioStreamId(uint32_t &sessionID) const
{
    return audioStream_->GetAudioSessionID(sessionID);
}

int32_t AudioCapturerGateway::SetBufferDuration(uint64_t bufferDuration) const
{
    CHECK_AND_RETURN_RET_LOG(bufferDuration >= MINIMUM_BUFFER_SIZE_MSEC && bufferDuration <= MAXIMUM_BUFFER_SIZE_MSEC,
        ERR_INVALID_PARAM, "Error: Please set the buffer duration between 5ms ~ 20ms");
    return audioStream_->SetBufferSizeInMsec(bufferDuration);
}

void AudioCapturerGateway::SetApplicationCachePath(const std::string cachePath)
{
    audioStream_->SetApplicationCachePath(cachePath);
}

void AudioStreamCapturerCallback::SaveCallback(const std::weak_ptr<AudioCapturerCallback> &callback)
{
    std::shared_ptr<AudioCapturerCallback> cb = callback.lock();
    AUDIO_DEBUG_LOG("SaveCallback cb");
    callback_ = callback;
}

void AudioStreamCapturerCallback::OnStateChange(const State state, StateChangeCmdType cmdType)
{
    std::shared_ptr<AudioCapturerCallback> cb = callback_.lock();
    CHECK_AND_RETURN_RET_LOG(cb != nullptr, "cb == nullptr.");
    AUDIO_DEBUG_LOG("OnStateChange cb");
    cb->OnStateChange(static_cast<CapturerState>(state));
}

int32_t AudioCapturerGateway::SetCaptureMode(AudioCaptureMode captureMode) const
{
    return audioStream_->SetCaptureMode(captureMode);
}

AudioCaptureMode AudioCapturerGateway::GetCaptureMode() const
{
    return audioStream_->GetCaptureMode();
}

int32_t AudioCapturerGateway::SetCapturerReadCallback(const std::shared_ptr<AudioCapturerReadCallback> &callback)
{
    return audioStream_->SetCapturerReadCallback(callback);
}

int32_t AudioCapturerGateway::GetBufferDesc(BufferDesc &bufDesc) const
{
    return audioStream_->GetBufferDesc(bufDesc);
}

int32_t AudioCapturerGateway::Enqueue(const BufferDesc &bufDesc) const
{
    return audioStream_->Enqueue(bufDesc);
}

int32_t AudioCapturerGateway::Clear() const
{
    return audioStream_->Clear();
}

int32_t AudioCapturerGateway::GetBufQueueState(BufferQueueState &bufState) const
{
    return audioStream_->GetBufQueueState(bufState);
}

}  // namespace AudioStandard
}  // namespace OHOS
