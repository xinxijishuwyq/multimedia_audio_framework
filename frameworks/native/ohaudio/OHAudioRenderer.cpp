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
#include "OHAudioRenderer.h"
#include "audio_errors.h"

using OHOS::AudioStandard::Timestamp;

static OHOS::AudioStandard::OHAudioRenderer *convertRenderer(OH_AudioRenderer* renderer)
{
    return (OHOS::AudioStandard::OHAudioRenderer*) renderer;
}

OH_AudioStream_Result OH_AudioRenderer_Start(OH_AudioRenderer* renderer)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    if (audioRenderer->Start()) {
        return AUDIOSTREAM_SUCCESS;
    } else {
        return AUDIOSTREAM_ERROR_ILLEGAL_STATE;
    }
}

OH_AudioStream_Result OH_AudioRenderer_Pause(OH_AudioRenderer* renderer)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");

    if (audioRenderer->Pause()) {
        return AUDIOSTREAM_SUCCESS;
    } else {
        return AUDIOSTREAM_ERROR_ILLEGAL_STATE;
    }
}

OH_AudioStream_Result OH_AudioRenderer_Stop(OH_AudioRenderer* renderer)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");

    if (audioRenderer->Stop()) {
        return AUDIOSTREAM_SUCCESS;
    } else {
        return AUDIOSTREAM_ERROR_ILLEGAL_STATE;
    }
}

OH_AudioStream_Result OH_AudioRenderer_Flush(OH_AudioRenderer* renderer)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");

    if (audioRenderer->Flush()) {
        return AUDIOSTREAM_SUCCESS;
    } else {
        return AUDIOSTREAM_ERROR_ILLEGAL_STATE;
    }
}

OH_AudioStream_Result OH_AudioRenderer_Release(OH_AudioRenderer* renderer)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");

    if (audioRenderer->Release()) {
        return AUDIOSTREAM_SUCCESS;
    } else {
        return AUDIOSTREAM_ERROR_ILLEGAL_STATE;
    }
}

OH_AudioStream_Result OH_AudioRenderer_GetCurrentState(OH_AudioRenderer* renderer, OH_AudioStream_State* state)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");

    OHOS::AudioStandard::RendererState rendererState = audioRenderer->GetStatus();
    *state = (OH_AudioStream_State)rendererState;
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetSamplingRate(OH_AudioRenderer* renderer, int32_t* rate)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");

    *rate = audioRenderer->GetSamplingRate();
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetStreamId(OH_AudioRenderer* renderer, uint32_t* streamId)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    audioRenderer->GetStreamId(*streamId);
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetChannelCount(OH_AudioRenderer* renderer, int32_t* channelCount)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    *channelCount = audioRenderer->GetChannelCount();
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetSampleFormat(OH_AudioRenderer* renderer,
    OH_AudioStream_SampleFormat* sampleFormat)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    *sampleFormat = (OH_AudioStream_SampleFormat)audioRenderer->GetSampleFormat();
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetLatencyMode(OH_AudioRenderer* renderer,
    OH_AudioStream_LatencyMode* latencyMode)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    OHOS::AudioStandard::AudioRendererInfo rendererInfo;
    audioRenderer->GetRendererInfo(rendererInfo);
    *latencyMode = (OH_AudioStream_LatencyMode)rendererInfo.rendererFlags;

    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetRendererInfo(OH_AudioRenderer* renderer,
    OH_AudioStream_Usage* usage, OH_AudioStream_Content* content)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");

    OHOS::AudioStandard::AudioRendererInfo rendererInfo;
    audioRenderer->GetRendererInfo(rendererInfo);
    *usage = (OH_AudioStream_Usage)rendererInfo.streamUsage;
    *content = (OH_AudioStream_Content)rendererInfo.contentType;
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result OH_AudioRenderer_GetEncodingType(OH_AudioRenderer* renderer,
    OH_AudioStream_EncodingType* encodingType)
{
    OHOS::AudioStandard::OHAudioRenderer *audioRenderer = convertRenderer(renderer);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, AUDIOSTREAM_ERROR_INVALID_PARAM, "convert renderer failed");
    *encodingType = (OH_AudioStream_EncodingType)audioRenderer->GetEncodingType();
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

    std::string cacheDir = "/data/storage/el2/base/haps/entry/files";
    audioRenderer_ = AudioRenderer::Create(cacheDir, rendererOptions);
    return audioRenderer_ != nullptr;
}

bool OHAudioRenderer::Start()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->Start();
}

bool OHAudioRenderer::Pause()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->Pause();
}

bool OHAudioRenderer::Stop()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->Stop();
}

bool OHAudioRenderer::Flush()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->Flush();
}

bool OHAudioRenderer::Release()
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, ERROR, "renderer client is nullptr");
    return audioRenderer_->Release();
}

RendererState OHAudioRenderer::GetStatus()
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

int32_t OHAudioRenderer::GetFramesWritten()
{
    return 0;
}

void OHAudioRenderer::GetAudioTime(Timestamp &timestamp, Timestamp::Timestampbase base)
{
    CHECK_AND_RETURN_LOG(audioRenderer_ != nullptr, "renderer client is nullptr");
    audioRenderer_->GetAudioTime(timestamp, base);
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

void OHAudioRenderer::SetRendererWriteCallback(OH_AudioRenderer_Callbacks callbacks, void* userData)
{
    CHECK_AND_RETURN_LOG(audioRenderer_ != nullptr, "renderer client is nullptr");
    audioRenderer_->SetRenderMode(RENDER_MODE_CALLBACK);

    if (callbacks.OH_AudioRenderer_OnWriteData != nullptr) {
        std::shared_ptr<AudioRendererWriteCallback> callback = std::make_shared<OHAudioRendererModeCallback>(callbacks,
                (OH_AudioRenderer*)this, userData);
        audioRenderer_->SetRendererWriteCallback(callback);
    } else {
        AUDIO_ERR_LOG("write callback is nullptr");
    }
}

void OHAudioRendererModeCallback::OnWriteData(size_t length)
{
    OHAudioRenderer* audioRenderer = (OHAudioRenderer*)ohAudioRenderer_;
    CHECK_AND_RETURN_LOG(audioRenderer != nullptr, "renderer client is nullptr");
    CHECK_AND_RETURN_LOG(callbacks_.OH_AudioRenderer_OnWriteData != nullptr, "pointer to the fuction is nullptr");
    BufferDesc bufDesc;
    audioRenderer->GetBufferDesc(bufDesc);
    callbacks_.OH_AudioRenderer_OnWriteData(ohAudioRenderer_,
        userData_,
        (void*)bufDesc.buffer,
        bufDesc.bufLength);
    audioRenderer->Enqueue(bufDesc);
}
}  // namespace AudioStandard
}  // namespace OHOS
