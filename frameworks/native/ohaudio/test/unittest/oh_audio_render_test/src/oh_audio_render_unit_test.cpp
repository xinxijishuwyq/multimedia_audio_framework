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

#include "oh_audio_render_unit_test.h"

using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {

void OHAudioRenderUnitTest::SetUpTestCase(void) { }

void OHAudioRenderUnitTest::TearDownTestCase(void) { }

void OHAudioRenderUnitTest::SetUp(void) { }

void OHAudioRenderUnitTest::TearDown(void) { }

const int32_t g_samplingRate = 48000; // 48000:g_samplingRate value
const int32_t g_channelCount = 2; // 2:g_channelCount value
const int32_t g_latencyMode = 0;
const int32_t g_sampleFormat = 1;
const int32_t g_frameSize = 240; // 240:g_frameSize value
uint32_t g_flag = 0;
const float MAX_AUDIO_VOLUME = 1.0f; // volume range is between 0 to 1.
const float MIN_AUDIO_VOLUME = 0.0f; // volume range is between 0 to 1.
const int32_t DURATIONMS = 40; // 40:fade out latency ms

static int32_t AudioRendererOnWriteData(OH_AudioRenderer* capturer,
    void* userData,
    void* buffer,
    int32_t bufferLen)
{
    return 0;
}

static void AudioRendererOnMarkReachedCb(OH_AudioRenderer* renderer, uint32_t samplePos, void* userData)
{
    g_flag = samplePos;
    printf("AudioRendererOnMarkReachedCb samplePos: %d \n", samplePos);
}

OH_AudioStreamBuilder* OHAudioRenderUnitTest::CreateRenderBuilder()
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RENDERER;
    OH_AudioStreamBuilder_Create(&builder, type);
    return builder;
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_GenerateRenderer API via legal state.
* @tc.number: OH_Audio_Capture_Generate_001
* @tc.desc  : Test OH_AudioStreamBuilder_GenerateRenderer interface. Returns true, if the result is successful.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_Generate_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();

    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);

    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_GenerateRenderer API via illegal OH_AudioStream_Type.
* @tc.number: OH_Audio_Render_Generate_002
* @tc.desc  : Test OH_AudioStreamBuilder_GenerateRenderer interface. Returns error code, if the stream type is
*             AUDIOSTREAM_TYPE_CAPTURER.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_Generate_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);

    OH_AudioRenderer* audioRenderer;
    result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);
}

/**
* @tc.name  : Test OH_AudioRenderer_Start API via legal state.
* @tc.number: Audio_Capturer_Start_001
* @tc.desc  : Test OH_AudioRenderer_Start interface. Returns true if start is successful.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_Start_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();

    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    result = OH_AudioRenderer_Start(audioRenderer);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);

    OH_AudioRenderer_Release(audioRenderer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_Start API via illegal state.
* @tc.number: Audio_Capturer_Start_002
* @tc.desc  : Test OH_AudioRenderer_Start interface. Returns error code, if Start interface is called twice.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_Start_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();

    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    result = OH_AudioRenderer_Start(audioRenderer);

    result = OH_AudioRenderer_Start(audioRenderer);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_ILLEGAL_STATE);

    OH_AudioRenderer_Release(audioRenderer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_Pause API via legal state.
* @tc.number: OH_Audio_Render_Pause_001
* @tc.desc  : Test OH_AudioRenderer_Pause interface. Returns true if Pause is successful.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_Pause_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();

    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    result = OH_AudioRenderer_Start(audioRenderer);

    result = OH_AudioRenderer_Pause(audioRenderer);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);

    OH_AudioRenderer_Release(audioRenderer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_Pause API via illegal state, Pause without Start first.
* @tc.number: OH_Audio_Render_Pause_002
* @tc.desc  : Test OH_AudioRenderer_Pause interface. Returns error code, if Pause without Start first.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_Pause_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();

    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    result = OH_AudioRenderer_Pause(audioRenderer);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_ILLEGAL_STATE);

    OH_AudioRenderer_Release(audioRenderer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_Stop API via legal state.
* @tc.number: OH_Audio_Render_Stop_001
* @tc.desc  : Test OH_AudioRenderer_Stop interface. Returns true if Stop is successful.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_Stop_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();

    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    result = OH_AudioRenderer_Start(audioRenderer);

    result = OH_AudioRenderer_Stop(audioRenderer);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);

    OH_AudioRenderer_Release(audioRenderer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_Stop API via illegal state, Stop without Start first.
* @tc.number: OH_Audio_Render_Stop_002
* @tc.desc  : Test OH_AudioRenderer_Stop interface. Returns error code, if Stop without Start first.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_Stop_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();

    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    result = OH_AudioRenderer_Stop(audioRenderer);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_ILLEGAL_STATE);

    OH_AudioRenderer_Release(audioRenderer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_Flush API via legal state.
* @tc.number: OH_Audio_Render_Flush_001
* @tc.desc  : Test OH_AudioRenderer_Flush interface. Returns true if Flush is successful.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_Flush_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();

    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    result = OH_AudioRenderer_Start(audioRenderer);

    result = OH_AudioRenderer_Flush(audioRenderer);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);

    OH_AudioRenderer_Release(audioRenderer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_Flush API via illegal state.
* @tc.number: OH_Audio_Render_Flush_002
* @tc.desc  : Test OH_AudioRenderer_Flush interface. Returns error code, if Flush without Start first.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_Flush_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();

    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    result = OH_AudioRenderer_Flush(audioRenderer);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_ILLEGAL_STATE);

    OH_AudioRenderer_Release(audioRenderer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_Release API via legal state.
* @tc.number: OH_Audio_Render_Release_001
* @tc.desc  : Test OH_AudioRenderer_Release interface. Returns true if Release is successful.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_Release_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();

    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    result = OH_AudioRenderer_Start(audioRenderer);

    result = OH_AudioRenderer_Release(audioRenderer);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);

    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_Release API via illegal state.
* @tc.number: OH_Audio_Render_Release_002
* @tc.desc  : Test OH_AudioRenderer_Release interface. Returns error code, if stream is released twice.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_Release_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();

    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    OH_AudioRenderer_Release(audioRenderer);
    result = OH_AudioRenderer_Release(audioRenderer);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_ILLEGAL_STATE);

    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetCurrentState API via legal state.
* @tc.number: OH_AudioRenderer_GetCurrentState_001
* @tc.desc  : Test OH_AudioRenderer_GetCurrentState interface. Return true if the result state is
*             AUDIOSTREAM_STATE_PREPARED.
*/
HWTEST(OHAudioRenderUnitTest, OH_AudioRenderer_GetCurrentState_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    OH_AudioStream_State state;
    result = OH_AudioRenderer_GetCurrentState(audioRenderer, &state);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_TRUE(state == AUDIOSTREAM_STATE_PREPARED);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetCurrentState API via legal state.
* @tc.number: OH_AudioRenderer_GetCurrentState_002
* @tc.desc  : Test OH_AudioRenderer_GetCurrentState interface. Return true if the result state is
*             AUDIOSTREAM_STATE_RUNNING.
*/
HWTEST(OHAudioRenderUnitTest, OH_AudioRenderer_GetCurrentState_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    OH_AudioRenderer_Start(audioRenderer);

    OH_AudioStream_State state;
    result = OH_AudioRenderer_GetCurrentState(audioRenderer, &state);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_TRUE(state == AUDIOSTREAM_STATE_RUNNING);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetCurrentState API via legal state.
* @tc.number: OH_AudioRenderer_GetCurrentState_003
* @tc.desc  : Test OH_AudioRenderer_GetCurrentState interface. Return true if the result state is
*             AUDIOSTREAM_STATE_PAUSED.
*/
HWTEST(OHAudioRenderUnitTest, OH_AudioRenderer_GetCurrentState_003, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    OH_AudioRenderer_Start(audioRenderer);
    OH_AudioRenderer_Pause(audioRenderer);

    OH_AudioStream_State state;
    result = OH_AudioRenderer_GetCurrentState(audioRenderer, &state);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_TRUE(state == AUDIOSTREAM_STATE_PAUSED);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetCurrentState API via legal state.
* @tc.number: OH_AudioRenderer_GetCurrentState_004
* @tc.desc  : Test OH_AudioRenderer_GetCurrentState interface. Return true if the result state is
*             AUDIOSTREAM_STATE_STOPPED.
*/
HWTEST(OHAudioRenderUnitTest, OH_AudioRenderer_GetCurrentState_004, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    OH_AudioRenderer_Start(audioRenderer);
    OH_AudioRenderer_Stop(audioRenderer);

    OH_AudioStream_State state;
    result = OH_AudioRenderer_GetCurrentState(audioRenderer, &state);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_TRUE(state == AUDIOSTREAM_STATE_STOPPED);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetCurrentState API via legal state.
* @tc.number: OH_AudioRenderer_GetCurrentState_005
* @tc.desc  : Test OH_AudioRenderer_GetCurrentState interface. Return true if the result state is
*             AUDIOSTREAM_STATE_RELEASED.
*/
HWTEST(OHAudioRenderUnitTest, OH_AudioRenderer_GetCurrentState_005, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    OH_AudioRenderer_Start(audioRenderer);
    OH_AudioRenderer_Release(audioRenderer);

    OH_AudioStream_State state;
    result = OH_AudioRenderer_GetCurrentState(audioRenderer, &state);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_TRUE(state == AUDIOSTREAM_STATE_RELEASED);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetLatencyMode API via legal state.
* @tc.number: OH_Audio_Render_GetParameter_001
* @tc.desc  : Test OH_AudioRenderer_GetLatencyMode interface. Returns true if latencyMode is
*             AUDIOSTREAM_LATENCY_MODE_NORMAL,because default latency mode is AUDIOSTREAM_LATENCY_MODE_NORMAL.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_GetParameter_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    OH_AudioStream_LatencyMode latencyMode = AUDIOSTREAM_LATENCY_MODE_NORMAL;
    result = OH_AudioRenderer_GetLatencyMode(audioRenderer, &latencyMode);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_TRUE(latencyMode == AUDIOSTREAM_LATENCY_MODE_NORMAL);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetStreamId API via legal state.
* @tc.number: OH_Audio_Render_GetParameter_002
* @tc.desc  : Test OH_AudioRenderer_GetStreamId interface. Returns true if the result is AUDIOSTREAM_SUCCESS.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_GetParameter_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    uint32_t streamId;
    result = OH_AudioRenderer_GetStreamId(audioRenderer, &streamId);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetSamplingRate API via legal state.
* @tc.number: OH_Audio_Render_GetSamplingRate_001
* @tc.desc  : Test OH_AudioRenderer_GetSamplingRate interface. Returns true if samplingRate is
*             SAMPLE_RATE_48000,because default samplingRate is SAMPLE_RATE_48000.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_GetSamplingRate_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    int32_t rate;
    result = OH_AudioRenderer_GetSamplingRate(audioRenderer, &rate);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_TRUE(rate == SAMPLE_RATE_48000);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetSampleFormat API via legal state.
* @tc.number: OH_Audio_Render_GetSampleFormat_001
* @tc.desc  : Test OH_AudioRenderer_GetSampleFormat interface. Returns true if sampleFormat is
*             AUDIOSTREAM_SAMPLE_S16LE,because default sampleFormat is AUDIOSTREAM_SAMPLE_S16LE.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_GetSampleFormat, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    OH_AudioStream_SampleFormat sampleFormat;
    result = OH_AudioRenderer_GetSampleFormat(audioRenderer, &sampleFormat);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_TRUE(sampleFormat == AUDIOSTREAM_SAMPLE_S16LE);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetEncodingType API via legal state.
* @tc.number: OH_Audio_Render_GetEncodingType_001
* @tc.desc  : Test OH_AudioRenderer_GetEncodingType interface. Returns true if encodingType is
*             ENCODING_PCM,because default encodingType is ENCODING_PCM.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_GetEncodingType_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    OH_AudioStream_EncodingType encodingType;
    result = OH_AudioRenderer_GetEncodingType(audioRenderer, &encodingType);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_TRUE(encodingType == AUDIOSTREAM_ENCODING_TYPE_RAW);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetRendererInfo API via legal state.
* @tc.number: OH_Audio_Render_GetRendererInfo_001
* @tc.desc  : Test OH_AudioRenderer_GetRendererInfo interface. Returns true if usage is STREAM_USAGE_MEDIA and content
*             is CONTENT_TYPE_MUSIC.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_GetRendererInfo_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    OH_AudioStream_Usage usage;
    result = OH_AudioRenderer_GetRendererInfo(audioRenderer, &usage);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_TRUE(usage == AUDIOSTREAM_USAGE_MEDIA);
    EXPECT_TRUE(content == AUDIOSTREAM_CONTENT_TYPE_MUSIC);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetChannelLayout API via legal state.
* @tc.number: OH_AudioRenderer_GetChannelLayout_001
* @tc.desc  : Test OH_AudioRenderer_GetChannelLayout interface. Returns true if channelLayout is
*             CH_LAYOUT_UNKNOWN, because default channelLayout is CH_LAYOUT_UNKNOWN.
*/
HWTEST(OHAudioRenderUnitTest, OH_AudioRenderer_GetChannelLayout_001, TestSize.Level0)
{
    OH_AudioStreamBuilder *builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer *audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    OH_AudioChannelLayout channelLayout;
    result = OH_AudioRenderer_GetEncodingType(audioRenderer, &channelLayout);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_TRUE(channelLayout == CH_LAYOUT_UNKNOWN);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetEffectMode API via legal state.
* @tc.number: OH_AudioRenderer_GetEffectMode_001
* @tc.desc  : Test OH_AudioRenderer_GetEffectMode interface. Returns true if effect mode is the same as set.
*/
HWTEST(OHAudioRenderUnitTest, OH_AudioRenderer_GetEffectMode_001, TestSize.Level0)
{
    OH_AudioStreamBuilder *builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer *audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    OH_AudioStream_AudioEffectMode effectMode;
    result = OH_AudioRenderer_SetEffectMode(audioRenderer, EFFECT_DEFAULT);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    result = OH_AudioRenderer_GetEffectMode(audioRenderer, &effectMode);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_TRUE(effectMode == EFFECT_DEFAULT);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetVolume API via illegal state.
* @tc.number: OH_Audio_Render_GetVolume_001
* @tc.desc  : Test OH_AudioRenderer_GetVolume interface with nullptr audioRenderer.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_GetVolume_001, TestSize.Level0)
{
    OH_AudioRenderer* audioRenderer = nullptr;
    float volume;
    OH_AudioStream_Result result = OH_AudioRenderer_GetVolume(audioRenderer, &volume);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetVolume API via legal state.
* @tc.number: OH_Audio_Render_GetVolume_002
* @tc.desc  : Test OH_AudioRenderer_GetVolume interface.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_GetVolume_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    float volume;
    result = OH_AudioRenderer_GetVolume(audioRenderer, &volume);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetVolume API via legal state.
* @tc.number: OH_Audio_Render_GetVolume_003
* @tc.desc  : Test OH_AudioRenderer_GetVolume interface after set volume call.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_GetVolume_003, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    float volumeSet = 0.5;
    result = OH_AudioRenderer_SetVolume(audioRenderer, volumeSet);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    float volumeGet;
    result = OH_AudioRenderer_GetVolume(audioRenderer, &volumeGet);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_EQ(volumeGet, 0.5);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_GetVolume API via legal state.
* @tc.number: OH_Audio_Render_GetVolume_004
* @tc.desc  : Test OH_AudioRenderer_GetVolume interface after set volume fails.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_GetVolume_004, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    float volumeSet = 0.5;
    result = OH_AudioRenderer_SetVolume(audioRenderer, volumeSet);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    volumeSet = 1.5;
    result = OH_AudioRenderer_SetVolume(audioRenderer, volumeSet);
    EXPECT_EQ(result, AUDIOSTREAM_ERROR_INVALID_PARAM);
    float volumeGet;
    result = OH_AudioRenderer_GetVolume(audioRenderer, &volumeGet);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_EQ(volumeGet, 0.5);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_SetVolume API via illegal state.
* @tc.number: OH_Audio_Render_SetVolume_001
* @tc.desc  : Test OH_AudioRenderer_SetVolume interface with nullptr audioRenderer.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_SetVolume_001, TestSize.Level0)
{
    OH_AudioRenderer* audioRenderer = nullptr;
    float volumeSet = 0.5;
    OH_AudioStream_Result result = OH_AudioRenderer_SetVolume(audioRenderer, volumeSet);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);
}

/**
* @tc.name  : Test OH_AudioRenderer_SetVolume API via legal state.
* @tc.number: OH_Audio_Render_SetVolume_002
* @tc.desc  : Test OH_AudioRenderer_SetVolume interface between minimum and maximum volumes.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_SetVolume_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    float volumeSet = 0.5;
    result = OH_AudioRenderer_SetVolume(audioRenderer, volumeSet);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_SetVolume API via legal state.
* @tc.number: OH_Audio_Render_SetVolume_003
* @tc.desc  : Test OH_AudioRenderer_SetVolume interface for minimum and maximum volumes.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_SetVolume_003, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    float volumeSet = MIN_AUDIO_VOLUME;
    result = OH_AudioRenderer_SetVolume(audioRenderer, volumeSet);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    volumeSet = MAX_AUDIO_VOLUME;
    result = OH_AudioRenderer_SetVolume(audioRenderer, volumeSet);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_SetVolume API via illegal state.
* @tc.number: OH_Audio_Render_SetVolume_004
* @tc.desc  : Test OH_AudioRenderer_SetVolume interface out of volumes range.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_SetVolume_004, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    float volumeSet = -0.5;
    result = OH_AudioRenderer_SetVolume(audioRenderer, volumeSet);
    EXPECT_EQ(result, AUDIOSTREAM_ERROR_INVALID_PARAM);
    volumeSet = -1.5;
    result = OH_AudioRenderer_SetVolume(audioRenderer, volumeSet);
    EXPECT_EQ(result, AUDIOSTREAM_ERROR_INVALID_PARAM);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_SetVolumeWithRamp API via illegal state.
* @tc.number: OH_Audio_Render_SetVolumeWithRamp_001
* @tc.desc  : Test OH_AudioRenderer_SetVolumeWithRamp interface with nullptr audioRenderer.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_SetVolumeWithRamp_001, TestSize.Level0)
{
    OH_AudioRenderer* audioRenderer = nullptr;
    float volumeSet = MIN_AUDIO_VOLUME;
    int32_t durationMs = DURATIONMS;
    OH_AudioStream_Result result = OH_AudioRenderer_SetVolumeWithRamp(audioRenderer, volumeSet, durationMs);
    EXPECT_EQ(result, AUDIOSTREAM_ERROR_INVALID_PARAM);
}

/**
* @tc.name  : Test OH_AudioRenderer_SetVolumeWithRamp API via legal state.
* @tc.number: OH_Audio_Render_SetVolumeWithRamp_002
* @tc.desc  : Test OH_AudioRenderer_SetVolumeWithRamp interface.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_SetVolumeWithRamp_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    float volumeSet = MIN_AUDIO_VOLUME;
    int32_t durationMs = DURATIONMS;
    result = OH_AudioRenderer_SetVolumeWithRamp(audioRenderer, volumeSet, durationMs);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_SetMarkPosition API via illegal state.
* @tc.number: OH_Audio_Render_SetMarkPosition_001
* @tc.desc  : Test OH_AudioRenderer_SetMarkPosition interface with nullptr audioRenderer.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_SetMarkPosition_001, TestSize.Level0)
{
    OH_AudioRenderer* audioRenderer = nullptr;
    uint32_t samplePos = 1;
    OH_AudioRenderer_OnMarkReachedCallback callback = AudioRendererOnMarkReachedCb;
    OH_AudioStream_Result result = OH_AudioRenderer_SetMarkPosition(audioRenderer, samplePos, callback, nullptr);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);
}

/**
* @tc.name  : Test OH_AudioRenderer_SetMarkPosition API via legal state.
* @tc.number: OH_Audio_Render_SetMarkPosition_002
* @tc.desc  : Test OH_AudioRenderer_SetMarkPosition interface.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_SetMarkPosition_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    uint32_t samplePos = 1;
    OH_AudioRenderer_OnMarkReachedCallback callback = AudioRendererOnMarkReachedCb;
    result = OH_AudioRenderer_SetMarkPosition(audioRenderer, samplePos, callback, nullptr);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_SetMarkPosition API via illegal state.
* @tc.number: OH_Audio_Render_SetMarkPosition_003
* @tc.desc  : Test OH_AudioRenderer_SetMarkPosition interface with incorrect samplepos value.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_SetMarkPosition_003, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);

    uint32_t samplePos = 0;
    OH_AudioRenderer_OnMarkReachedCallback callback = AudioRendererOnMarkReachedCb;
    result = OH_AudioRenderer_SetMarkPosition(audioRenderer, samplePos, callback, nullptr);
    EXPECT_EQ(result, AUDIOSTREAM_ERROR_INVALID_PARAM);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_SetMarkPosition API via legal state.
* @tc.number: OH_Audio_Render_SetMarkPosition_004
* @tc.desc  : Test OH_AudioRenderer_SetMarkPosition interface with callback.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_SetMarkPosition_004, TestSize.Level0)
{
    // 1. create
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();

    // 2. set params and callbacks
    OH_AudioStreamBuilder_SetSamplingRate(builder, g_samplingRate);
    OH_AudioStreamBuilder_SetChannelCount(builder, g_channelCount);
    OH_AudioStreamBuilder_SetLatencyMode(builder, (OH_AudioStream_LatencyMode)g_latencyMode);
    OH_AudioStreamBuilder_SetSampleFormat(builder, (OH_AudioStream_SampleFormat)g_sampleFormat);
    OH_AudioRenderer_Callbacks callbacks;
    callbacks.OH_AudioRenderer_OnWriteData = AudioRendererOnWriteData;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_SetRendererCallback(builder, callbacks, nullptr);
    // 3. set buffer size to g_frameSize
    result = OH_AudioStreamBuilder_SetFrameSizeInCallback(builder, g_frameSize);

    OH_AudioRenderer* audioRenderer;
    result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    EXPECT_EQ(g_flag, 0);
    uint32_t samplePos = 1;
    OH_AudioRenderer_OnMarkReachedCallback callback = AudioRendererOnMarkReachedCb;
    result = OH_AudioRenderer_SetMarkPosition(audioRenderer, samplePos, callback, nullptr);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    // 4. start
    result = OH_AudioRenderer_Start(audioRenderer);
    sleep(2);
    EXPECT_EQ(g_flag, 1);
    // 5. stop and release client
    result = OH_AudioRenderer_Stop(audioRenderer);
    result = OH_AudioRenderer_Release(audioRenderer);

    // 6. destroy the builder
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_SetMarkPosition API via legal state.
* @tc.number: OH_Audio_Render_SetMarkPosition_005
* @tc.desc  : Test OH_AudioRenderer_SetMarkPosition interface multiple times.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_SetMarkPosition_005, TestSize.Level0)
{
    // 1. create
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();

    // 2. set params and callbacks
    OH_AudioStreamBuilder_SetSamplingRate(builder, g_samplingRate);
    OH_AudioStreamBuilder_SetChannelCount(builder, g_channelCount);
    OH_AudioStreamBuilder_SetLatencyMode(builder, (OH_AudioStream_LatencyMode)g_latencyMode);
    OH_AudioStreamBuilder_SetSampleFormat(builder, (OH_AudioStream_SampleFormat)g_sampleFormat);
    OH_AudioRenderer_Callbacks callbacks;
    callbacks.OH_AudioRenderer_OnWriteData = AudioRendererOnWriteData;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_SetRendererCallback(builder, callbacks, nullptr);
    // 3. set buffer size to g_frameSize
    result = OH_AudioStreamBuilder_SetFrameSizeInCallback(builder, g_frameSize);

    OH_AudioRenderer* audioRenderer;
    result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    uint32_t samplePos = 1;
    OH_AudioRenderer_OnMarkReachedCallback callback = AudioRendererOnMarkReachedCb;
    result = OH_AudioRenderer_SetMarkPosition(audioRenderer, samplePos, callback, nullptr);
    result = OH_AudioRenderer_SetMarkPosition(audioRenderer, samplePos, callback, nullptr);
    result = OH_AudioRenderer_SetMarkPosition(audioRenderer, samplePos, callback, nullptr);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    // 4. start
    result = OH_AudioRenderer_Start(audioRenderer);
    sleep(2);
    EXPECT_EQ(g_flag, 1);
    // 5. stop and release client
    result = OH_AudioRenderer_Stop(audioRenderer);
    result = OH_AudioRenderer_Release(audioRenderer);

    // 6. destroy the builder
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_CancelMark API via illegal state.
* @tc.number: OH_Audio_Render_CancelMark_001
* @tc.desc  : Test OH_AudioRenderer_CancelMark interface with nullptr audioRenderer.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_CancelMark_001, TestSize.Level0)
{
    OH_AudioRenderer* audioRenderer = nullptr;
    OH_AudioStream_Result result = OH_AudioRenderer_CancelMark(audioRenderer);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);
}

/**
* @tc.name  : Test OH_AudioRenderer_CancelMark API via legal state.
* @tc.number: OH_Audio_Render_CancelMark_002
* @tc.desc  : Test OH_AudioRenderer_CancelMark interface without callback.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_CancelMark_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();
    OH_AudioRenderer* audioRenderer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    result = OH_AudioRenderer_CancelMark(audioRenderer);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioRenderer_CancelMark API via legal state.
* @tc.number: OH_Audio_Render_CancelMark_003
* @tc.desc  : Test OH_AudioRenderer_CancelMark interface with callback.
*/
HWTEST(OHAudioRenderUnitTest, OH_Audio_Render_CancelMark_003, TestSize.Level0)
{
    // 1. create
    OH_AudioStreamBuilder* builder = OHAudioRenderUnitTest::CreateRenderBuilder();

    // 2. set params and callbacks
    OH_AudioStreamBuilder_SetSamplingRate(builder, g_samplingRate);
    OH_AudioStreamBuilder_SetChannelCount(builder, g_channelCount);
    OH_AudioStreamBuilder_SetLatencyMode(builder, (OH_AudioStream_LatencyMode)g_latencyMode);
    OH_AudioStreamBuilder_SetSampleFormat(builder, (OH_AudioStream_SampleFormat)g_sampleFormat);
    OH_AudioRenderer_Callbacks callbacks;
    callbacks.OH_AudioRenderer_OnWriteData = AudioRendererOnWriteData;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_SetRendererCallback(builder, callbacks, nullptr);
    // 3. set buffer size to g_frameSize
    result = OH_AudioStreamBuilder_SetFrameSizeInCallback(builder, g_frameSize);

    OH_AudioRenderer* audioRenderer;
    result = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    uint32_t samplePos = 2;
    OH_AudioRenderer_OnMarkReachedCallback callback = AudioRendererOnMarkReachedCb;
    result = OH_AudioRenderer_SetMarkPosition(audioRenderer, samplePos, callback, nullptr);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    // 4. start
    result = OH_AudioRenderer_Start(audioRenderer);
    sleep(2);
    EXPECT_EQ(g_flag, 2);

    // CancelMark
    result = OH_AudioRenderer_CancelMark(audioRenderer);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    // 5. stop and release client
    result = OH_AudioRenderer_Stop(audioRenderer);
    result = OH_AudioRenderer_Release(audioRenderer);

    // 6. destroy the builder
    OH_AudioStreamBuilder_Destroy(builder);
}
} // namespace AudioStandard
} // namespace OHOS
