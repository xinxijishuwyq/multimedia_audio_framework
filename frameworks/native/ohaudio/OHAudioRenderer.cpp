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
#undef LOG_TAG
#define LOG_TAG "OHAudioRenderer"

#include "OHAudioRenderer.h"
#include "audio_errors.h"

using OHOS::AudioStandard::Timestamp;

static const int64_t SECOND_TO_NANOSECOND = 1000000000;

static OHOS::AudioStandard::OHAudioRenderer *convertRenderer(OH_AudioRenderer *renderer)
{
    return (OHOS::AudioStandard::OHAudioRenderer*) renderer;
}

static OH_AudioStream_Result ConvertError(int32_t err)
{
    if (err == OHOS::AudioStandard::SUCCESS) {
        return AUDIOSTREAM_SUCCESS;
    } else if (err == OHOS::AudioStandard::ERR_INVALID_PARAM) {
        return AUDIOSTREAM_ERROR_INVALID_PARAM;
    } else if (err == OHOS::AudioStandard::ERR_ILLEGAL_STATE) {
        return AUDIOSTREAM_ERROR_ILLEGAL_STATE;
    }
    return AUDIOSTREAM_ERROR_SYSTEM;
}

OH_AudioStream_Result OH_AudioRenderer_Start(OH_AudioRenderer *renderer)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    if (audioRenderer->Start()) {
        return AUDIOSTREAM_SUCCESS;
    } else {
        return AUDIOSTREAM_ERROR_ILLEGAL_STATE;
    }
}

OH_AudioStream_Result OH_AudioRenderer_Pause(OH_AudioRenderer *renderer)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");

    if (audioRenderer->Pause()) {
        return AUDIOSTREAM_SUCCESS;
    } else {
        return AUDIOSTREAM_ERROR_ILLEGAL_STATE;
    }
}

OH_AudioStream_Result OH_AudioRenderer_Stop(OH_AudioRenderer *renderer)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");

    if (audioRenderer->Stop()) {
        return AUDIOSTREAM_SUCCESS;
    } else {
        return AUDIOSTREAM_ERROR_ILLEGAL_STATE;
    }
}

OH_AudioStream_Result OH_AudioRenderer_Flush(OH_AudioRenderer *renderer)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");

    if (audioRenderer->Flush()) {
        return AUDIOSTREAM_SUCCESS;
    } else {
        return AUDIOSTREAM_ERROR_ILLEGAL_STATE;
    }
}

OH_AudioStream_Result OH_AudioRenderer_Release(OH_AudioRenderer *renderer)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");

    if (audioRenderer->Release()) {
        delete audioRenderer;
        audioRenderer = nullptr;
        return AUDIOSTREAM_SUCCESS;
    } else {
        return AUDIOSTREAM_ERROR_ILLEGAL_STATE;
    }
}

OH_AudioStream_Result OH_AudioRenderer_GetCurrentState(OH_AudioRenderer *renderer, OH_AudioStream_State *state)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");

    OHOS::AudioStandard::RendererState rendererState = audioRenderer->GetCurrentState();
    *state = (OH_AudioStream_State)rendererState;
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetSamplingRate(OH_AudioRenderer *renderer, int32_t *rate)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");

    *rate = audioRenderer->GetSamplingRate();
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetStreamId(OH_AudioRenderer *renderer, uint32_t *streamId)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    audioRenderer->GetStreamId(*streamId);
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetChannelCount(OH_AudioRenderer *renderer, int32_t *channelCount)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    *channelCount = audioRenderer->GetChannelCount();
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetSampleFormat(OH_AudioRenderer *renderer,
    OH_AudioStream_SampleFormat *sampleFormat)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    *sampleFormat = (OH_AudioStream_SampleFormat)audioRenderer->GetSampleFormat();
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetLatencyMode(OH_AudioRenderer *renderer,
    OH_AudioStream_LatencyMode *latencyMode)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    OHOS::AudioStandard::AudioRendererInfo rendererInfo;
    audioRenderer->GetRendererInfo(rendererInfo);
    *latencyMode = (OH_AudioStream_LatencyMode)rendererInfo.rendererFlags;

    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetRendererInfo(OH_AudioRenderer *renderer,
    OH_AudioStream_Usage *usage)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");

    OHOS::AudioStandard::AudioRendererInfo rendererInfo;
    audioRenderer->GetRendererInfo(rendererInfo);
    *usage = (OH_AudioStream_Usage)rendererInfo.streamUsage;
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetEncodingType(OH_AudioRenderer *renderer,
    OH_AudioStream_EncodingType *encodingType)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    *encodingType = (OH_AudioStream_EncodingType)audioRenderer->GetEncodingType();
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetFramesWritten(OH_AudioRenderer *renderer, int64_t *frames)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    *frames = audioRenderer->GetFramesWritten();
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetTimestamp(OH_AudioRenderer *renderer,
    clockid_t clockId, int64_t *framePosition, int64_t *timestamp)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    CHECK_AND_RETURN_RET_LOG(clockId == CLOCK_MONOTONIC, AUDIOSTREAM_ERROR_INVALID_PARAM, "error clockId value");
    Timestamp stamp;
    Timestamp::Timestampbase base = Timestamp::Timestampbase::MONOTONIC;
    bool ret = audioRenderer->GetAudioTime(stamp, base);
    if (!ret) {
        AUDIO_ERR_LOG("GetAudioTime error!");
        return AUDIOSTREAM_ERROR_ILLEGAL_STATE;
    }
    *framePosition = stamp.framePosition;
    *timestamp = stamp.time.tv_sec * SECOND_TO_NANOSECOND + stamp.time.tv_nsec;
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetFrameSizeInCallback(OH_AudioRenderer *renderer, int32_t *frameSize)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    *frameSize = audioRenderer->GetFrameSizeInCallback();
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetSpeed(OH_AudioRenderer *renderer, float *speed)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    *speed = audioRenderer->GetSpeed();
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_SetSpeed(OH_AudioRenderer *renderer, float speed)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    audioRenderer->SetSpeed(speed);
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_SetVolume(OH_AudioRenderer *renderer, float volume)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    CHECK_AND_RETURN_RET_LOG(((volume >= 0) && (volume <= 1)), AUDIOSTREAM_ERROR_INVALID_PARAM, "volume set invalid");
    int32_t err = audioRenderer->SetVolume(volume);
    return ConvertError(err);
}

OH_AudioStream_Result OH_AudioRenderer_SetVolumeWithRamp(OH_AudioRenderer *renderer, float volume, int32_t durationMs)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    CHECK_AND_RETURN_RET_LOG(((volume >= 0) && (volume <= 1)), AUDIOSTREAM_ERROR_INVALID_PARAM, "volume set invalid");
    int32_t err = audioRenderer->SetVolumeWithRamp(volume, durationMs);
    return ConvertError(err);
}

OH_AudioStream_Result OH_AudioRenderer_GetVolume(OH_AudioRenderer *renderer, float *volume)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    CHECK_AND_RETURN_RET_LOG(volume != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "volume is nullptr");
    *volume = audioRenderer->GetVolume();
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_SetMarkPosition(OH_AudioRenderer *renderer, uint32_t samplePos,
    OH_AudioRenderer_OnMarkReachedCallback callback, void *userData)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    CHECK_AND_RETURN_RET_LOG(samplePos > 0, AUDIOSTREAM_ERROR_INVALID_PARAM, "framePos set invalid");
    int32_t err = audioRenderer->SetRendererPositionCallback(callback, samplePos, userData);
    return ConvertError(err);
}

OH_AudioStream_Result OH_AudioRenderer_CancelMark(OH_AudioRenderer *renderer)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    audioRenderer->UnsetRendererPositionCallback();
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetChannelLayout(OH_AudioRenderer *renderer,
    OH_AudioChannelLayout *channelLayout)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    *channelLayout = (OH_AudioChannelLayout)audioRenderer->GetChannelLayout();
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetEffectMode(OH_AudioRenderer *renderer,
    OH_AudioStream_AudioEffectMode *effectMode)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    *effectMode = (OH_AudioStream_AudioEffectMode)audioRenderer->GetEffectMode();
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_SetEffectMode(OH_AudioRenderer *renderer,
    OH_AudioStream_AudioEffectMode effectMode)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    audioRenderer->SetEffectMode((OHOS::AudioStandard::AudioEffectMode)effectMode);
    return AUDIOSTREAM_SUCCESS;
}

namespace OHOS {
namespace AudioStandard {
OHAudioRenderer::OHAudioRenderer()
{
    AUDIO_INFO_LOG("OHAudioRenderer created!");
}

OHAudioRenderer::~OHAudioRenderer()
{
    AUDIO_INFO_LOG("OHAudioRenderer destroyed!");
}

bool OHAudioRenderer::Initialize(const AudioRendererOptions &rendererOptions)
{
    std::string cacheDir = "/data/storage/el2/base/temp";
    audioRenderer_ = AudioRenderer::Create(cacheDir, rendererOptions);
    return audioRenderer_ != nullptr;
}

bool OHAudioRenderer::Start()
{
    if (audioRenderer_ == nullptr) {
        AUDIO_ERR_LOG("renderer client is nullptr");
        return false;
    }
    return audioRenderer_->Start();
}

bool OHAudioRenderer::Pause()
{
    if (audioRenderer_ == nullptr) {
        AUDIO_ERR_LOG("renderer client is nullptr");
        return false;
    }
    return audioRenderer_->Pause();
}

bool OHAudioRenderer::Stop()
{
    if (audioRenderer_ == nullptr) {
        AUDIO_ERR_LOG("renderer client is nullptr");
        return false;
    }
    return audioRenderer_->Stop();
}

bool OHAudioRenderer::Flush()
{
    if (audioRenderer_ == nullptr) {
        AUDIO_ERR_LOG("renderer client is nullptr");
        return false;
    }
    return audioRenderer_->Flush();
}

bool OHAudioRenderer::Release()
{
    if (audioRenderer_ == nullptr) {
        AUDIO_ERR_LOG("renderer client is nullptr");
        return false;
    }

    if (!audioRenderer_->Release()) {
        return false;
    }
    audioRenderer_ = nullptr;
    audioRendererCallback_ = nullptr;
    return true;
}

RendererState OHAudioRenderer::GetCurrentState()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, RENDERER_INVALID, "renderer client is nullptr");
    return audioRenderer_->GetStatus();
}

void OHAudioRenderer::GetStreamId(uint32_t &streamId)
{
    CHECK_AND_RETURN_LOG(audioRenderer_ != nullptr, "renderer client is nullptr");
    audioRenderer_->GetAudioStreamId(streamId);
}

AudioChannel OHAudioRenderer::GetChannelCount()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, MONO, "renderer client is nullptr");
    AudioRendererParams params;
    audioRenderer_->GetParams(params);
    return params.channelCount;
}

int32_t OHAudioRenderer::GetSamplingRate()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, MONO, "renderer client is nullptr");
    AudioRendererParams params;
    audioRenderer_->GetParams(params);
    return params.sampleRate;
}

AudioSampleFormat OHAudioRenderer::GetSampleFormat()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, INVALID_WIDTH, "renderer client is nullptr");
    AudioRendererParams params;
    audioRenderer_->GetParams(params);
    return params.sampleFormat;
}

void OHAudioRenderer::GetRendererInfo(AudioRendererInfo& rendererInfo)
{
    CHECK_AND_RETURN_LOG(audioRenderer_ != nullptr, "renderer client is nullptr");
    audioRenderer_->GetRendererInfo(rendererInfo);
}

AudioEncodingType OHAudioRenderer::GetEncodingType()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ENCODING_INVALID, "renderer client is nullptr");
    AudioRendererParams params;
    audioRenderer_->GetParams(params);
    return params.encodingType;
}

int64_t OHAudioRenderer::GetFramesWritten()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->GetFramesWritten();
}

bool OHAudioRenderer::GetAudioTime(Timestamp &timestamp, Timestamp::Timestampbase base)
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, false, "renderer client is nullptr");
    return audioRenderer_->GetAudioPosition(timestamp, base);
}

int32_t OHAudioRenderer::GetFrameSizeInCallback()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    uint32_t frameSize;
    audioRenderer_->GetFrameCount(frameSize);
    return static_cast<int32_t>(frameSize);
}

int32_t OHAudioRenderer::GetBufferDesc(BufferDesc &bufDesc) const
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->GetBufferDesc(bufDesc);
}

int32_t OHAudioRenderer::Enqueue(const BufferDesc &bufDesc) const
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->Enqueue(bufDesc);
}

int32_t OHAudioRenderer::SetSpeed(float speed)
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->SetSpeed(speed);
}

float OHAudioRenderer::GetSpeed()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->GetSpeed();
}

int32_t OHAudioRenderer::SetVolume(float volume) const
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->SetVolume(volume);
}

float OHAudioRenderer::GetVolume() const
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->GetVolume();
}

int32_t OHAudioRenderer::SetVolumeWithRamp(float volume, int32_t duration)
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->SetVolumeWithRamp(volume, duration);
}

int32_t OHAudioRenderer::SetRendererPositionCallback(OH_AudioRenderer_OnMarkReachedCallback callback,
    uint32_t markPosition, void *userData)
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERROR, "callback is nullptr");
    rendererPositionCallback_ = std::make_shared<OHRendererPositionCallback>(callback,
        reinterpret_cast<OH_AudioRenderer*>(this), userData);
    return audioRenderer_->SetRendererPositionCallback(markPosition, rendererPositionCallback_);
}

void OHAudioRenderer::UnsetRendererPositionCallback()
{
    CHECK_AND_RETURN_LOG(audioRenderer_ != nullptr, "renderer client is nullptr");
    audioRenderer_->UnsetRendererPositionCallback();
}

void OHRendererPositionCallback::OnMarkReached(const int64_t &framePosition)
{
    CHECK_AND_RETURN_LOG(ohAudioRenderer_ != nullptr, "renderer client is nullptr");
    CHECK_AND_RETURN_LOG(callback_ != nullptr, "pointer to the fuction is nullptr");
    callback_(ohAudioRenderer_, framePosition, userData_);
}

AudioChannelLayout OHAudioRenderer::GetChannelLayout()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, CH_LAYOUT_UNKNOWN, "renderer client is nullptr");
    AudioRendererParams params;
    audioRenderer_->GetParams(params);
    return params.channelLayout;
}

AudioEffectMode OHAudioRenderer::GetEffectMode()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, EFFECT_NONE, "renderer client is nullptr");
    return audioRenderer_->GetAudioEffectMode();
}

int32_t OHAudioRenderer::SetEffectMode(AudioEffectMode effectMode)
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->SetAudioEffectMode(effectMode);
}

void OHAudioRenderer::SetRendererCallback(OH_AudioRenderer_Callbacks callbacks, void *userData,
    OH_AudioRenderer_WriteDataWithMetadataCallback writeDataWithMetadataCallback, void *metadataUserData)
{
    CHECK_AND_RETURN_LOG(audioRenderer_ != nullptr, "renderer client is nullptr");
    audioRenderer_->SetRenderMode(RENDER_MODE_CALLBACK);

    AudioEncodingType encodingType = GetEncodingType();
    if ((encodingType == ENCODING_AUDIOVIVID) && (writeDataWithMetadataCallback != nullptr)) {
        std::shared_ptr<AudioRendererWriteCallback> callback = std::make_shared<OHAudioRendererModeCallback>(
            writeDataWithMetadataCallback, (OH_AudioRenderer*)this, metadataUserData, encodingType);
        audioRenderer_->SetRendererWriteCallback(callback);
        AUDIO_INFO_LOG("The write callback function is for AudioVivid type");
    } else if ((encodingType == ENCODING_PCM) && (callbacks.OH_AudioRenderer_OnWriteData != nullptr)) {
        std::shared_ptr<AudioRendererWriteCallback> callback = std::make_shared<OHAudioRendererModeCallback>(
            callbacks, (OH_AudioRenderer*)this, userData, encodingType);
        audioRenderer_->SetRendererWriteCallback(callback);
        AUDIO_INFO_LOG("The write callback function is for PCM type");
    } else {
        AUDIO_WARNING_LOG("The write callback function is not set");
    }

    if (callbacks.OH_AudioRenderer_OnStreamEvent != nullptr) {
        std::shared_ptr<AudioRendererDeviceChangeCallback> callback =
            std::make_shared<OHAudioRendererDeviceChangeCallback>(callbacks, (OH_AudioRenderer*)this, userData);
        int32_t clientPid = getpid();
        audioRenderer_->RegisterAudioRendererEventListener(clientPid, callback);
    } else {
        AUDIO_WARNING_LOG("The stream event callback function is not set");
    }

    if (callbacks.OH_AudioRenderer_OnInterruptEvent != nullptr) {
        audioRendererCallback_ =
            std::make_shared<OHAudioRendererCallback>(callbacks, (OH_AudioRenderer*)this, userData);
        audioRenderer_->SetRendererCallback(audioRendererCallback_);
    } else {
        AUDIO_WARNING_LOG("The audio renderer interrupt callback function is not set");
    }

    if (callbacks.OH_AudioRenderer_OnError != nullptr) {
        std::shared_ptr<AudioRendererPolicyServiceDiedCallback> callback =
            std::make_shared<OHServiceDiedCallback>(callbacks, (OH_AudioRenderer*)this, userData);
        int32_t clientPid = getpid();
        audioRenderer_->RegisterAudioPolicyServerDiedCb(clientPid, callback);

        std::shared_ptr<AudioRendererErrorCallback> errorCallback =
            std::make_shared<OHAudioRendererErrorCallback>(callbacks, (OH_AudioRenderer*)this, userData);
        audioRenderer_->SetAudioRendererErrorCallback(errorCallback);
    } else {
        AUDIO_WARNING_LOG("The audio renderer error callback function is not set");
    }
}

void OHAudioRenderer::SetRendererOutputDeviceChangeCallback(OH_AudioRenderer_OutputDeviceChangeCallback callback,
    void *userData)
{
    CHECK_AND_RETURN_LOG(audioRenderer_ != nullptr, "renderer client is nullptr");
    CHECK_AND_RETURN_LOG(callback != nullptr, "callback is nullptr");
    audioRendererDeviceChangeCallbackWithInfo_ =
        std::make_shared<OHAudioRendererDeviceChangeCallbackWithInfo> (callback,
        reinterpret_cast<OH_AudioRenderer*>(this), userData);
    audioRenderer_->RegisterOutputDeviceChangeWithInfoCallback(audioRendererDeviceChangeCallbackWithInfo_);
}

void OHAudioRenderer::SetPreferredFrameSize(int32_t frameSize)
{
    audioRenderer_->SetPreferredFrameSize(frameSize);
}

bool OHAudioRenderer::IsFastRenderer()
{
    return audioRenderer_->IsFastRenderer();
}

uint32_t OHAudioRenderer::GetUnderflowCount()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->GetUnderflowCount();
}

void OHAudioRendererModeCallback::OnWriteData(size_t length)
{
    OHAudioRenderer *audioRenderer = (OHAudioRenderer*)ohAudioRenderer_;
    CHECK_AND_RETURN_LOG(audioRenderer != nullptr, "renderer client is nullptr");
    CHECK_AND_RETURN_LOG(callbacks_.OH_AudioRenderer_OnWriteData != nullptr, "pointer to the fuction is nullptr");
    BufferDesc bufDesc;
    audioRenderer->GetBufferDesc(bufDesc);
    if (encodingType_ == ENCODING_AUDIOVIVID) {
        writeDataWithMetadataCallback_(ohAudioRenderer_, metadataUserData_, (void*)bufDesc.buffer, bufDesc.bufLength,
            (void*)bufDesc.metaBuffer, bufDesc.metaLength);
    } else {
        callbacks_.OH_AudioRenderer_OnWriteData(ohAudioRenderer_, userData_, (void*)bufDesc.buffer, bufDesc.bufLength);
    }
    audioRenderer->Enqueue(bufDesc);
}

void OHAudioRendererDeviceChangeCallback::OnStateChange(const DeviceInfo &deviceInfo)
{
    CHECK_AND_RETURN_LOG(ohAudioRenderer_ != nullptr, "renderer client is nullptr");
    CHECK_AND_RETURN_LOG(callbacks_.OH_AudioRenderer_OnStreamEvent != nullptr, "pointer to the fuction is nullptr");

    OH_AudioStream_Event event =  AUDIOSTREAM_EVENT_ROUTING_CHANGED;
    callbacks_.OH_AudioRenderer_OnStreamEvent(ohAudioRenderer_, userData_, event);
}

void OHAudioRendererCallback::OnInterrupt(const InterruptEvent &interruptEvent)
{
    CHECK_AND_RETURN_LOG(ohAudioRenderer_ != nullptr, "renderer client is nullptr");
    CHECK_AND_RETURN_LOG(callbacks_.OH_AudioRenderer_OnInterruptEvent != nullptr, "pointer to the fuction is nullptr");
    OH_AudioInterrupt_ForceType type = (OH_AudioInterrupt_ForceType)(interruptEvent.forceType);
    OH_AudioInterrupt_Hint hint = OH_AudioInterrupt_Hint(interruptEvent.hintType);
    callbacks_.OH_AudioRenderer_OnInterruptEvent(ohAudioRenderer_, userData_, type, hint);
}

void OHServiceDiedCallback::OnAudioPolicyServiceDied()
{
    CHECK_AND_RETURN_LOG(ohAudioRenderer_ != nullptr, "renderer client is nullptr");
    CHECK_AND_RETURN_LOG(callbacks_.OH_AudioRenderer_OnError != nullptr, "pointer to the fuction is nullptr");
    OH_AudioStream_Result error = AUDIOSTREAM_ERROR_SYSTEM;
    callbacks_.OH_AudioRenderer_OnError(ohAudioRenderer_, userData_, error);
}

OH_AudioStream_Result OHAudioRendererErrorCallback::GetErrorResult(AudioErrors errorCode) const
{
    switch (errorCode) {
        case ERROR_ILLEGAL_STATE:
            return AUDIOSTREAM_ERROR_ILLEGAL_STATE;
        case ERROR_INVALID_PARAM:
            return AUDIOSTREAM_ERROR_INVALID_PARAM;
        case ERROR_SYSTEM:
            return AUDIOSTREAM_ERROR_SYSTEM;
        default:
            return AUDIOSTREAM_ERROR_SYSTEM;
    }
}

OH_AudioStream_Result OH_AudioRenderer_GetUnderflowCount(OH_AudioRenderer* renderer, uint32_t* count)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    CHECK_AND_RETURN_RET_LOG(count != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "count is nullptr");
    *count = audioRenderer->GetUnderflowCount();
    return AUDIOSTREAM_SUCCESS;
}

void OHAudioRendererErrorCallback::OnError(AudioErrors errorCode)
{
    CHECK_AND_RETURN_LOG(ohAudioRenderer_ != nullptr && callbacks_.OH_AudioRenderer_OnError != nullptr,
        "renderer client or error callback funtion is nullptr");
    OH_AudioStream_Result error = GetErrorResult(errorCode);
    callbacks_.OH_AudioRenderer_OnError(ohAudioRenderer_, userData_, error);
}

void OHAudioRendererDeviceChangeCallbackWithInfo::OnOutputDeviceChange(const DeviceInfo &deviceInfo,
    const AudioStreamDeviceChangeReason reason)
{
    CHECK_AND_RETURN_LOG(ohAudioRenderer_ != nullptr, "renderer client is nullptr");
    CHECK_AND_RETURN_LOG(callback_ != nullptr, "pointer to the fuction is nullptr");

    callback_(ohAudioRenderer_, userData_, static_cast<OH_AudioStream_DeviceChangeReason>(reason));
}

void OHAudioRenderer::SetInterruptMode(InterruptMode mode)
{
    CHECK_AND_RETURN_LOG(audioRenderer_ != nullptr, "renderer client is nullptr");
    audioRenderer_->SetInterruptMode(mode);
}
}  // namespace AudioStandard
}  // namespace OHOS
