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
#define LOG_TAG "AudioRendererGateway"

#include "audio_errors.h"
#include "audio_policy_manager.h"
#include "audio_renderer_gateway.h"
#include "audio_container_stream_base.h"
#include "audio_log.h"
#include "audio_renderer.h"

namespace OHOS {
namespace AudioStandard {

static const int32_t MAX_VOLUME_LEVEL = 15;
static const int32_t CONST_FACTOR = 100;

static float VolumeToDb(int32_t volumeLevel)
{
    float value = static_cast<float>(volumeLevel) / MAX_VOLUME_LEVEL;
    float roundValue = static_cast<int>(value * CONST_FACTOR);

    return static_cast<float>(roundValue) / CONST_FACTOR;
}

std::map<pid_t, std::map<AudioStreamType, AudioInterrupt>> AudioRendererGateway::sharedInterrupts_;

AudioRenderer::~AudioRenderer() = default;
AudioRendererGateway::~AudioRendererGateway()
{
    RendererState state = GetStatus();
    if (state != RENDERER_RELEASED && state != RENDERER_NEW) {
        Release();
    }
}

AudioRendererGateway::AudioRendererGateway(AudioStreamType audioStreamType)
{
    audioStream_ = std::make_shared<AudioContainerRenderStream>(audioStreamType, AUDIO_MODE_PLAYBACK);
    audioInterrupt_.streamType = audioStreamType;
}

int32_t AudioRendererGateway::GetFrameCount(uint32_t &frameCount) const
{
    return audioStream_->GetFrameCount(frameCount);
}

int32_t AudioRendererGateway::GetLatency(uint64_t &latency) const
{
    return audioStream_->GetLatency(latency);
}

int32_t AudioRendererGateway::SetParams(const AudioRendererParams params)
{
    AudioStreamParams audioStreamParams;
    audioStreamParams.format = params.sampleFormat;
    audioStreamParams.samplingRate = params.sampleRate;
    audioStreamParams.channels = params.channelCount;
    audioStreamParams.encoding = params.encodingType;

    int32_t ret = audioStream_->SetAudioStreamInfo(audioStreamParams);

    CHECK_AND_RETURN_RET_LOG(!ret, ret, "SetAudioStreamInfo Failed");

    int32_t audioSessionID = audioStream_->GetAudioSessionID(sessionID_);
    CHECK_AND_RETURN_RET_LOG(audioSessionID == 0, ERR_INVALID_INDEX, "GetAudioSessionID Failed");
    audioInterrupt_.sessionId = sessionID_;

    if (audioInterruptCallback_ == nullptr) {
        audioInterruptCallback_ = std::make_shared<AudioInterruptCallbackGateway>(audioStream_, audioInterrupt_);
        CHECK_AND_RETURN_RET_LOG(audioInterruptCallback_ != nullptr, ERROR,
            "Failed to allocate memory for audioInterruptCallback_");
    }

    return AudioPolicyManager::GetInstance().SetAudioInterruptCallback(sessionID_, audioInterruptCallback_);
}

int32_t AudioRendererGateway::GetParams(AudioRendererParams &params) const
{
    AudioStreamParams audioStreamParams;
    int32_t result = audioStream_->GetAudioStreamInfo(audioStreamParams);
    if (!result) {
        params.sampleFormat = static_cast<AudioSampleFormat>(audioStreamParams.format);
        params.sampleRate = static_cast<AudioSamplingRate>(audioStreamParams.samplingRate);
        params.channelCount = static_cast<AudioChannel>(audioStreamParams.channels);
        params.encodingType = static_cast<AudioEncodingType>(audioStreamParams.encoding);
    }

    return result;
}

int32_t AudioRendererGateway::GetRendererInfo(AudioRendererInfo &rendererInfo) const
{
    rendererInfo = rendererInfo_;

    return SUCCESS;
}

int32_t AudioRendererGateway::GetStreamInfo(AudioStreamInfo &streamInfo) const
{
    AudioStreamParams audioStreamParams;
    int32_t result = audioStream_->GetAudioStreamInfo(audioStreamParams);
    if (!result) {
        streamInfo.format = static_cast<AudioSampleFormat>(audioStreamParams.format);
        streamInfo.samplingRate = static_cast<AudioSamplingRate>(audioStreamParams.samplingRate);
        streamInfo.channels = static_cast<AudioChannel>(audioStreamParams.channels);
        streamInfo.encoding = static_cast<AudioEncodingType>(audioStreamParams.encoding);
    }

    return result;
}

int32_t AudioRendererGateway::SetRendererCallback(const std::shared_ptr<AudioRendererCallback> &callback)
{
    // If the client is using the deprecated SetParams API. SetRendererCallback must be invoked, after SetParams.
    // In general, callbacks can only be set after the renderer state is PREPARED.
    RendererState state = GetStatus();
    CHECK_AND_RETURN_RET_LOG(state != RENDERER_NEW && state != RENDERER_RELEASED, ERR_ILLEGAL_STATE,
        "incorrect state:%{public}d to register cb", state);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_PARAM, "callback param is null");

    // Save reference for interrupt callback
    CHECK_AND_RETURN_RET_LOG(audioInterruptCallback_ != nullptr, ERROR, "audioInterruptCallback_ == nullptr");
    std::shared_ptr<AudioInterruptCallbackGateway> cbInterrupt =
        std::static_pointer_cast<AudioInterruptCallbackGateway>(audioInterruptCallback_);
    cbInterrupt->SaveCallback(callback);

    // Save and Set reference for stream callback. Order is important here.
    if (audioStreamCallback_ == nullptr) {
        audioStreamCallback_ = std::make_shared<AudioStreamRenderCallback>();
        CHECK_AND_RETURN_RET_LOG(audioStreamCallback_ != nullptr, ERROR,
            "Failed to allocate memory for audioStreamCallback_");
    }

    std::shared_ptr<AudioStreamRenderCallback> cbStream =
        std::static_pointer_cast<AudioStreamRenderCallback>(audioStreamCallback_);
    cbStream->SaveCallback(callback);
    AUDIO_INFO_LOG("callback audioStreamCallback_");
    (void)audioStream_->SetStreamCallback(audioStreamCallback_);

    return SUCCESS;
}

int32_t AudioRendererGateway::SetRendererPositionCallback(int64_t markPosition,
    const std::shared_ptr<RendererPositionCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG((callback != nullptr) && (markPosition > 0), ERR_INVALID_PARAM, "input param is invalid");

    audioStream_->SetRendererPositionCallback(markPosition, callback);

    return SUCCESS;
}

void AudioRendererGateway::UnsetRendererPositionCallback()
{
    audioStream_->UnsetRendererPositionCallback();
}

int32_t AudioRendererGateway::SetRendererPeriodPositionCallback(int64_t frameNumber,
    const std::shared_ptr<RendererPeriodPositionCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG((callback != nullptr) && (frameNumber > 0), ERR_INVALID_PARAM, "input param is invalid");

    audioStream_->SetRendererPeriodPositionCallback(frameNumber, callback);

    return SUCCESS;
}

void AudioRendererGateway::UnsetRendererPeriodPositionCallback()
{
    audioStream_->UnsetRendererPeriodPositionCallback();
}

bool AudioRendererGateway::Start(StateChangeCmdType cmdType) const
{
    AUDIO_INFO_LOG("AudioRendererGateway::Start()");
    RendererState state = GetStatus();
    CHECK_AND_RETURN_RET_LOG((state == RENDERER_PREPARED) || (state == RENDERER_STOPPED) || (state == RENDERER_PAUSED),
        false, "Illegal state:%{public}u, Start failed", state);
    AudioInterrupt audioInterrupt;
    switch (mode_) {
        case InterruptMode::SHARE_MODE:
            audioInterrupt = sharedInterrupt_;
            break;
        case InterruptMode::INDEPENDENT_MODE:
            audioInterrupt = audioInterrupt_;
            break;
        default:
            break;
    }

    if (audioInterrupt_.streamType == STREAM_DEFAULT || audioInterrupt_.sessionId == INVALID_SESSION) {
        return false;
    }

    int32_t ret = AudioPolicyManager::GetInstance().ActivateAudioInterrupt(audioInterrupt);
    CHECK_AND_RETURN_RET_LOG(ret == 0, false, "ActivateAudioInterrupt Failed");

    return audioStream_->StartAudioStream();
}

int32_t AudioRendererGateway::Write(uint8_t *buffer, size_t bufferSize)
{
    return audioStream_->Write(buffer, bufferSize);
}

int32_t AudioRendererGateway::Write(uint8_t *pcmBuffer, size_t pcmSize, uint8_t *metaBuffer, size_t metaSize)
{
    AUDIO_ERR_LOG("AudioRendererGateway::Write with meta do not supported");
    return ERR_INVALID_OPERATION;
}

RendererState AudioRendererGateway::GetStatus() const
{
    return static_cast<RendererState>(audioStream_->GetState());
}

bool AudioRendererGateway::GetAudioTime(Timestamp &timestamp, Timestamp::Timestampbase base) const
{
    return audioStream_->GetAudioTime(timestamp, base);
}

bool AudioRendererGateway::Drain() const
{
    return audioStream_->DrainAudioStream();
}

bool AudioRendererGateway::Flush() const
{
    return audioStream_->FlushAudioStream();
}

bool AudioRendererGateway::Pause(StateChangeCmdType cmdType) const
{
    bool result = audioStream_->PauseAudioStream();
    AudioInterrupt audioInterrupt;
    switch (mode_) {
        case InterruptMode::SHARE_MODE:
            audioInterrupt = sharedInterrupt_;
            break;
        case InterruptMode::INDEPENDENT_MODE:
            audioInterrupt = audioInterrupt_;
            break;
        default:
            break;
    }
    // When user is intentionally pausing , Deactivate to remove from active/pending owners list
    int32_t ret = AudioPolicyManager::GetInstance().DeactivateAudioInterrupt(audioInterrupt);
    if (ret != 0) {
        AUDIO_ERR_LOG("DeactivateAudioInterrupt Failed");
    }

    return result;
}

bool AudioRendererGateway::Stop() const
{
    bool result = audioStream_->StopAudioStream();
    AudioInterrupt audioInterrupt;
    switch (mode_) {
        case InterruptMode::SHARE_MODE:
            audioInterrupt = sharedInterrupt_;
            break;
        case InterruptMode::INDEPENDENT_MODE:
            audioInterrupt = audioInterrupt_;
            break;
        default:
            break;
    }
    int32_t ret = AudioPolicyManager::GetInstance().DeactivateAudioInterrupt(audioInterrupt);
    if (ret != 0) {
        AUDIO_ERR_LOG("DeactivateAudioInterrupt Failed");
    }

    return result;
}

bool AudioRendererGateway::Release() const
{
    // If Stop call was skipped, Release to take care of Deactivation
    (void)AudioPolicyManager::GetInstance().DeactivateAudioInterrupt(audioInterrupt_);

    // Unregister the callaback in policy server
    (void)AudioPolicyManager::GetInstance().UnsetAudioInterruptCallback(sessionID_);

    return audioStream_->ReleaseAudioStream();
}

int32_t AudioRendererGateway::GetBufferSize(size_t &bufferSize) const
{
    return audioStream_->GetBufferSize(bufferSize);
}

int32_t AudioRendererGateway::GetAudioStreamId(uint32_t &sessionID) const
{
    return audioStream_->GetAudioSessionID(sessionID);
}

int32_t AudioRendererGateway::SetAudioRendererDesc(AudioRendererDesc audioRendererDesc) const
{
    ContentType contentType = audioRendererDesc.contentType;
    StreamUsage streamUsage = audioRendererDesc.streamUsage;
    AudioStreamType audioStreamType = audioStream_->GetStreamType(contentType, streamUsage);
    return audioStream_->SetAudioStreamType(audioStreamType);
}

int32_t AudioRendererGateway::SetStreamType(AudioStreamType audioStreamType) const
{
    return audioStream_->SetAudioStreamType(audioStreamType);
}

int32_t AudioRendererGateway::SetVolume(float volume) const
{
    return audioStream_->SetVolume(volume);
}

float AudioRendererGateway::GetVolume() const
{
    return audioStream_->GetVolume();
}

int32_t AudioRendererGateway::SetRenderRate(AudioRendererRate renderRate) const
{
    return audioStream_->SetRenderRate(renderRate);
}

AudioRendererRate AudioRendererGateway::GetRenderRate() const
{
    return audioStream_->GetRenderRate();
}

int32_t AudioRendererGateway::SetBufferDuration(uint64_t bufferDuration) const
{
    CHECK_AND_RETURN_RET_LOG(bufferDuration >= MINIMUM_BUFFER_SIZE_MSEC && bufferDuration <= MAXIMUM_BUFFER_SIZE_MSEC,
        ERR_INVALID_PARAM, "Error: Please set the buffer duration between 5ms ~ 20ms");

    return audioStream_->SetBufferSizeInMsec(bufferDuration);
}

AudioInterruptCallbackGateway::AudioInterruptCallbackGateway(
    const std::shared_ptr<AudioContainerRenderStream> &audioStream, const AudioInterrupt &audioInterrupt)
    : audioStream_(audioStream), audioInterrupt_(audioInterrupt)
{
    AUDIO_INFO_LOG("AudioInterruptCallbackGateway constructor");
}

AudioInterruptCallbackGateway::~AudioInterruptCallbackGateway()
{
    AUDIO_DEBUG_LOG("AudioInterruptCallbackGateway: instance destroy");
}

void AudioInterruptCallbackGateway::SaveCallback(const std::weak_ptr<AudioRendererCallback> &callback)
{
    callback_ = callback;
}

void AudioInterruptCallbackGateway::NotifyEvent(const InterruptEvent &interruptEvent)
{
    AUDIO_DEBUG_LOG("NotifyEvent: Hint: %{public}d, eventType: %{public}d",
        interruptEvent.hintType, interruptEvent.eventType);

    if (cb_ != nullptr) {
        cb_->OnInterrupt(interruptEvent);
        AUDIO_DEBUG_LOG("OnInterrupt : NotifyEvent to app complete");
    } else {
        AUDIO_DEBUG_LOG("cb_ == nullptr cannont NotifyEvent to app");
    }
}

bool AudioInterruptCallbackGateway::HandleForceDucking(const InterruptEventInternal &interruptEvent)
{
    int32_t systemVolumeLevel = AudioPolicyManager::GetInstance().GetSystemVolumeLevel(audioInterrupt_.streamType);
    float systemVolumeDb = VolumeToDb(systemVolumeLevel);
    float duckVolumeDb = interruptEvent.duckVolume;
    int32_t ret = 0;

    if (systemVolumeDb <= duckVolumeDb || FLOAT_COMPARE_EQ(systemVolumeDb, 0.0f)) {
        AUDIO_DEBUG_LOG("systemVolumeDb: %{public}f <= duckVolumeDb: %{public}f",
            systemVolumeDb, duckVolumeDb);
        AUDIO_DEBUG_LOG("No need to duck further return");
        return false;
    }

    instanceVolBeforeDucking_ = audioStream_->GetVolume();
    float duckInstanceVolume = duckVolumeDb / systemVolumeDb;
    CHECK_AND_RETURN_RET_LOG(!FLOAT_COMPARE_EQ(instanceVolBeforeDucking_, 0.0f) &&
        instanceVolBeforeDucking_ >= duckInstanceVolume, false, "No need to duck further return");

    ret = audioStream_->SetVolume(duckInstanceVolume);
    CHECK_AND_RETURN_RET_LOG(!ret, false, "set duckVolumeDb(instance) %{pubic}f failed", duckInstanceVolume);

    AUDIO_INFO_LOG("set duckVolumeDb(instance) %{pubic}f succeeded", duckInstanceVolume);
    return true;
}

void AudioInterruptCallbackGateway::NotifyForcePausedToResume(const InterruptEventInternal &interruptEvent)
{
    // Change InterruptForceType to Share, Since app will take care of resuming
    InterruptEvent interruptEventResume {interruptEvent.eventType, INTERRUPT_SHARE,
                                         interruptEvent.hintType};
    NotifyEvent(interruptEventResume);
}

void AudioInterruptCallbackGateway::HandleAndNotifyForcedEvent(const InterruptEventInternal &interruptEvent)
{
    InterruptHint hintType = interruptEvent.hintType;
    AUDIO_DEBUG_LOG("ForceType: INTERRUPT_FORCE, Force handle the event and notify the app");
    AUDIO_DEBUG_LOG("Hint: %{public}d eventType: %{public}d",
        interruptEvent.hintType, interruptEvent.eventType);

    switch (hintType) {
        case INTERRUPT_HINT_PAUSE:
            State state = audioStream_->GetState();
            CHECK_AND_RETURN_LOG(state == RUNNING, "OnInterrupt state is not running no need to pause");
            (void)audioStream_->PauseAudioStream(); // Just Pause, do not deactivate here
            isForcePaused_ = true;
            break;
        case INTERRUPT_HINT_RESUME:
            State state_ = audioStream_->GetState();
            CHECK_AND_RETURN_LOG(state_ == PAUSED && isForcePaused_,
                "OnInterrupt state is not paused or not forced paused");
            isForcePaused_ = false;
            NotifyForcePausedToResume(interruptEvent);
            return; // return, sending callback is taken care in NotifyForcePausedToResume
        case INTERRUPT_HINT_STOP:
            (void)audioStream_->StopAudioStream();
            break;
        case INTERRUPT_HINT_DUCK:
            bool forceDucking = HandleForceDucking(interruptEvent);
            CHECK_AND_RETURN_LOG(forceDucking, "It is not forced ducked, no need notify app, return");
            isForceDucked_ = true;
            break;
        case INTERRUPT_HINT_UNDUCK:
            CHECK_AND_RETURN_LOG(isForceDucked_, "It is not forced ducked, no need to unduck or notify app");
            (void)audioStream_->SetVolume(instanceVolBeforeDucking_);
            AUDIO_DEBUG_LOG("unduck Volume(instance) complete: %{public}f",
                            instanceVolBeforeDucking_);
            isForceDucked_ = false;
            break;
        default:
            break;
    }
    // Notify valid forced event callbacks to app
    InterruptEvent interruptEventForced {interruptEvent.eventType, interruptEvent.forceType, interruptEvent.hintType};
    NotifyEvent(interruptEventForced);
}

void AudioInterruptCallbackGateway::OnInterrupt(const InterruptEventInternal &interruptEvent)
{
    cb_ = callback_.lock();
    InterruptForceType forceType = interruptEvent.forceType;
    AUDIO_DEBUG_LOG("OnInterrupt InterruptForceType: %{public}d", forceType);

    if (forceType != INTERRUPT_FORCE) { // INTERRUPT_SHARE
        AUDIO_DEBUG_LOG("ForceType: INTERRUPT_SHARE. Let app handle the event");
        InterruptEvent interruptEventShared {interruptEvent.eventType, interruptEvent.forceType,
                                             interruptEvent.hintType};
        NotifyEvent(interruptEventShared);
        return;
    }

    CHECK_AND_RETURN_LOG(audioStream_ != nullptr, "stream is null. No need to take forced action");

    HandleAndNotifyForcedEvent(interruptEvent);
}

void AudioStreamRenderCallback::SaveCallback(const std::weak_ptr<AudioRendererCallback> &callback)
{
    std::shared_ptr<AudioRendererCallback> cb = callback.lock();
    AUDIO_ERR_LOG("AudioStreamRenderCallback::SaveCallback cb");
    callback_ = callback;
}

void AudioStreamRenderCallback::OnStateChange(const State state, StateChangeCmdType cmdType)
{
    std::shared_ptr<AudioRendererCallback> cb = callback_.lock();
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb == nullptr.");
    AUDIO_ERR_LOG("OnStateChange cb");
    cb->OnStateChange(static_cast<RendererState>(state));
}

int32_t AudioRendererGateway::SetRenderMode(AudioRenderMode renderMode) const
{
    return audioStream_->SetRenderMode(renderMode);
}

AudioRenderMode AudioRendererGateway::GetRenderMode() const
{
    return audioStream_->GetRenderMode();
}

int32_t AudioRendererGateway::GetBufferDesc(BufferDesc &bufDesc) const
{
    return audioStream_->GetBufferDesc(bufDesc);
}

int32_t AudioRendererGateway::Enqueue(const BufferDesc &bufDesc) const
{
    return audioStream_->Enqueue(bufDesc);
}

int32_t AudioRendererGateway::Clear() const
{
    return audioStream_->Clear();
}

int32_t AudioRendererGateway::GetBufQueueState(BufferQueueState &bufState) const
{
    return audioStream_->GetBufQueueState(bufState);
}

void AudioRendererGateway::SetApplicationCachePath(const std::string cachePath)
{
    audioStream_->SetApplicationCachePath(cachePath);
}

int32_t AudioRendererGateway::SetRendererWriteCallback(const std::shared_ptr<AudioRendererWriteCallback> &callback)
{
    return audioStream_->SetRendererWriteCallback(callback);
}

void AudioRendererGateway::SetInterruptMode(InterruptMode mode)
{
    AUDIO_INFO_LOG("InterruptMode %{pubilc}d", mode);
    mode_ = mode;
}

int32_t AudioRendererGateway::SetLowPowerVolume(float volume) const
{
    return 0;
}

float AudioRendererGateway::GetLowPowerVolume() const
{
    return 0;
}

float AudioRendererGateway::GetSingleStreamVolume() const
{
    return 0;
}

void AudioRendererGateway::SetPreferredFrameSize(int32_t frameSize)
{
}

bool AudioRendererGateway::IsFastRenderer()
{
    return false;
}
}  // namespace AudioStandard
}  // namespace OHOS
