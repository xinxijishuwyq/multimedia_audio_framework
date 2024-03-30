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

#include "oh_audio_stream_builder_unit_test.h"

using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {

namespace {
    const int32_t SAMPLING_RATE_44100 = 44100;
    const int32_t SAMPLING_RATE_48000 = 48000;
    const int32_t SAMPLING_RATE_WRONG_NUM = -1;

    const int32_t CHANNEL_COUNT_1 = 1;
    const int32_t CHANNEL_COUNT_2 = 2;
    const int32_t CHANNEL_COUNT_10 = 10;

    FILE *g_file;
    bool g_readEnd = false;
}

static int32_t AudioRendererOnWriteData(OH_AudioRenderer* capturer,
    void* userData,
    void* buffer,
    int32_t bufferLen)
{
    size_t readCount = fread(buffer, bufferLen, 1, g_file);
    if (!readCount) {
        g_readEnd = true;
        if (ferror(g_file)) {
            printf("Error reading myfile");
        } else if (feof(g_file)) {
            printf("EOF found");
        }
    }

    return 0;
}

static int32_t AudioRendererWriteDataWithMetadataCallback(OH_AudioRenderer *renderer,
    void *userData, void *audioData, int32_t audioDataSize, void *metadata, int32_t metadataSize)
{
    size_t readCount = fread(audioData, audioDataSize, 1, g_file);
    if (!readCount) {
        g_readEnd = true;
        if (ferror(g_file)) {
            printf("Error reading myfile");
        } else if (feof(g_file)) {
            printf("EOF found");
        }
        return 0;
    }
    readCount = fread(metadata, metadataSize, 1, g_file);
    if (!readCount) {
        g_readEnd = true;
        if (ferror(g_file)) {
            printf("Error reading myfile");
        } else if (feof(g_file)) {
            printf("EOF found");
        }
    }
    return 0;
}

static int32_t AudioCapturerOnReadData(
    OH_AudioCapturer* capturer,
    void* userData,
    void* buffer,
    int32_t bufferLen)
{
    printf("callback success\n");
    return 0;
}

void OHAudioStreamBuilderUnitTest::SetUpTestCase(void) { }

void OHAudioStreamBuilderUnitTest::TearDownTestCase(void) { }

void OHAudioStreamBuilderUnitTest::SetUp(void) { }

void OHAudioStreamBuilderUnitTest::TearDown(void) { }

/**
* @tc.name  : Test OH_AudioStreamBuilder_Create API via legal state, AUDIOSTREAM_TYPE_RENDERER.
* @tc.number: OH_AudioStreamBuilder_Create_001
* @tc.desc  : Test OH_AudioStreamBuilder_Create interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_Create_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = nullptr;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RENDERER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_Create API via legal state, AUDIOSTREAM_TYPE_CAPTURER.
* @tc.number: OH_AudioStreamBuilder_Create_001
* @tc.desc  : Test OH_AudioStreamBuilder_Create interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_Create_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_Destroy API via illegal state.
* @tc.number: OH_AudioStreamBuilder_Destroy_001
* @tc.desc  : Test OH_AudioStreamBuilder_Destroy interface. Returns error code, if builder pointer is nullptr.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_Destroy_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = nullptr;

    OH_AudioStream_Result result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetSamplingRate API via legal state, samplingRate is 48000.
* @tc.number: OH_AudioStreamBuilder_SetParameter_001
* @tc.desc  : Test OH_AudioStreamBuilder_SetSamplingRate interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    int32_t samplingRate = SAMPLING_RATE_48000;
    result = OH_AudioStreamBuilder_SetSamplingRate(builder, samplingRate);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetSamplingRate API via legal state, samplingRate is 44100.
* @tc.number: OH_AudioStreamBuilder_SetParameter_002
* @tc.desc  : Test OH_AudioStreamBuilder_SetSamplingRate interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    int32_t samplingRate = SAMPLING_RATE_44100;
    result = OH_AudioStreamBuilder_SetSamplingRate(builder, samplingRate);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetSamplingRate API via legal state, samplingRate is 88200.
* @tc.number: OH_AudioStreamBuilder_SetParameter_003
* @tc.desc  : Test OH_AudioStreamBuilder_SetSamplingRate interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_003, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    int32_t samplingRate = SAMPLING_RATE_88200;
    result = OH_AudioStreamBuilder_SetSamplingRate(builder, samplingRate);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetSamplingRate API via legal state, samplingRate is 176400.
* @tc.number: OH_AudioStreamBuilder_SetParameter_004
* @tc.desc  : Test OH_AudioStreamBuilder_SetSamplingRate interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_004, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    int32_t samplingRate = SAMPLING_RATE_176400;
    result = OH_AudioStreamBuilder_SetSamplingRate(builder, samplingRate);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetSamplingRate API via legal state, samplingRate is 192000.
* @tc.number: OH_AudioStreamBuilder_SetParameter_005
* @tc.desc  : Test OH_AudioStreamBuilder_SetSamplingRate interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_005, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    int32_t samplingRate = SAMPLING_RATE_192000;
    result = OH_AudioStreamBuilder_SetSamplingRate(builder, samplingRate);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetSamplingRate API via illegal state.
* @tc.number: OH_AudioStreamBuilder_SetParameter_006
* @tc.desc  : Test OH_AudioStreamBuilder_SetSamplingRate interface. Returns error code, if samplingRate is -1.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_006, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    int32_t samplingRate = SAMPLING_RATE_WRONG_NUM;
    result = OH_AudioStreamBuilder_SetSamplingRate(builder, samplingRate);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetChannelCount API via legal state, channelCount is 1.
* @tc.number: OH_AudioStreamBuilder_SetParameter_004
* @tc.desc  : Test OH_AudioStreamBuilder_SetChannelCount interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_004, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    int32_t channelCount = CHANNEL_COUNT_1;
    result = OH_AudioStreamBuilder_SetChannelCount(builder, channelCount);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetChannelCount API via legal state, channelCount is 2.
* @tc.number: OH_AudioStreamBuilder_SetParameter_005
* @tc.desc  : Test OH_AudioStreamBuilder_SetChannelCount interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_005, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    int32_t channelCount = CHANNEL_COUNT_2;
    result = OH_AudioStreamBuilder_SetChannelCount(builder, channelCount);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetChannelCount API via illegal state.
* @tc.number: OH_AudioStreamBuilder_SetParameter_006
* @tc.desc  : Test OH_AudioStreamBuilder_SetChannelCount interface. Returns error code, if channelCount is 8.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_006, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    int32_t channelCount = CHANNEL_COUNT_10;
    result = OH_AudioStreamBuilder_SetChannelCount(builder, channelCount);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetEncodingType API via legal state, AUDIOSTREAM_ENCODING_TYPE_RAW.
* @tc.number: OH_AudioStreamBuilder_SetParameter_007
* @tc.desc  : Test OH_AudioStreamBuilder_SetEncodingType interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_007, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_EncodingType encodingType = AUDIOSTREAM_ENCODING_TYPE_RAW;
    result = OH_AudioStreamBuilder_SetEncodingType(builder, encodingType);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetRendererInfo API via illegal state.
* @tc.number: OH_AudioStreamBuilder_SetParameter_008
* @tc.desc  : Test OH_AudioStreamBuilder_SetRendererInfo interface. Returns error code, if stream type is
*             AUDIOSTREAM_TYPE_CAPTURER.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_008, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_Usage usage = AUDIOSTREAM_USAGE_MEDIA;
    result = OH_AudioStreamBuilder_SetRendererInfo(builder, usage);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetRendererInfo API via legal state.
* @tc.number: OH_AudioStreamBuilder_SetParameter_009
* @tc.desc  : Test OH_AudioStreamBuilder_SetRendererInfo interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_009, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RENDERER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_Usage usage = AUDIOSTREAM_USAGE_COMMUNICATION;
    result = OH_AudioStreamBuilder_SetRendererInfo(builder, usage);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetRendererInfo API via illegal state.
* @tc.number: OH_AudioStreamBuilder_SetParameter_010
* @tc.desc  : Test OH_AudioStreamBuilder_SetRendererInfo interface. Returns error code, if builder type
*             is AUDIOSTREAM_TYPE_CAPTURER.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_010, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_Usage usage = AUDIOSTREAM_USAGE_COMMUNICATION;
    result = OH_AudioStreamBuilder_SetRendererInfo(builder, usage);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetRendererCallback API via legal state.
* @tc.number: OH_AudioStreamBuilder_SetParameter_011
* @tc.desc  : Test OH_AudioStreamBuilder_SetRendererCallback interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_011, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RENDERER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioRenderer_Callbacks callbacks;
    callbacks.OH_AudioRenderer_OnWriteData = AudioRendererOnWriteData;
    result = OH_AudioStreamBuilder_SetRendererCallback(builder, callbacks, NULL);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetRendererCallback API via illegal state.
* @tc.number: OH_AudioStreamBuilder_SetParameter_012
* @tc.desc  : Test OH_AudioStreamBuilder_SetRendererCallback interface. Returns error code, builder type
*             is AUDIOSTREAM_TYPE_CAPTURER.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_012, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioRenderer_Callbacks callbacks;
    callbacks.OH_AudioRenderer_OnWriteData = AudioRendererOnWriteData;
    result = OH_AudioStreamBuilder_SetRendererCallback(builder, callbacks, NULL);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetCapturerInfo API via legal state,
*             sourceType is AUDIOSTREAM_SOURCE_TYPE_MIC.
* @tc.number: OH_AudioStreamBuilder_SetParameter_013
* @tc.desc  : Test OH_AudioStreamBuilder_SetCapturerInfo interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_013, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_SourceType sourceType = AUDIOSTREAM_SOURCE_TYPE_MIC;
    result = OH_AudioStreamBuilder_SetCapturerInfo(builder, sourceType);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetCapturerInfo API via legal state,
*             sourceType is AUDIOSTREAM_SOURCE_TYPE_VOICE_RECOGNITION.
* @tc.number: OH_AudioStreamBuilder_SetParameter_014
* @tc.desc  : Test OH_AudioStreamBuilder_SetCapturerInfo interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_014, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_SourceType sourceType = AUDIOSTREAM_SOURCE_TYPE_VOICE_RECOGNITION ;
    result = OH_AudioStreamBuilder_SetCapturerInfo(builder, sourceType);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetCapturerInfo API via illegal state.
* @tc.number: OH_AudioStreamBuilder_SetParameter_015
* @tc.desc  : Test OH_AudioStreamBuilder_SetCapturerInfo interface. Returns error code, if the builder type is
*             AUDIOSTREAM_TYPE_RENDERER.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_015, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RENDERER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_SourceType sourceType = AUDIOSTREAM_SOURCE_TYPE_VOICE_RECOGNITION ;
    result = OH_AudioStreamBuilder_SetCapturerInfo(builder, sourceType);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetCapturerCallback API via legal state.
* @tc.number: OH_AudioStreamBuilder_SetParameter_016
* @tc.desc  : Test OH_AudioStreamBuilder_SetCapturerCallback interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_016, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioCapturer_Callbacks callbacks;
    callbacks.OH_AudioCapturer_OnReadData = AudioCapturerOnReadData;
    result = OH_AudioStreamBuilder_SetCapturerCallback(builder, callbacks, NULL);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetCapturerCallback API via illegal state.
* @tc.number: OH_AudioStreamBuilder_SetParameter_017
* @tc.desc  : Test OH_AudioStreamBuilder_SetCapturerCallback interface. Returns error code, if the builder type is
*             AUDIOSTREAM_TYPE_RENDERER.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetParameter_017, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RENDERER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioCapturer_Callbacks callbacks;
    callbacks.OH_AudioCapturer_OnReadData = AudioCapturerOnReadData;
    result = OH_AudioStreamBuilder_SetCapturerCallback(builder, callbacks, NULL);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetSampleFormat API via legal state. Test value is AUDIOSTREAM_SAMPLE_U8.
* @tc.number: OH_AudioStreamBuilder_SetSampleFormat_001
* @tc.desc  : Test OH_AudioStreamBuilder_SetSampleFormat interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetSampleFormat_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_SampleFormat format = AUDIOSTREAM_SAMPLE_U8;
    result = OH_AudioStreamBuilder_SetSampleFormat(builder, format);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetSampleFormat API via legal state. Test value is AUDIOSTREAM_SAMPLE_S16LE.
* @tc.number: OH_AudioStreamBuilder_SetSampleFormat_002
* @tc.desc  : Test OH_AudioStreamBuilder_SetSampleFormat interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetSampleFormat_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_SampleFormat format = AUDIOSTREAM_SAMPLE_S16LE;
    result = OH_AudioStreamBuilder_SetSampleFormat(builder, format);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetSampleFormat API via legal state. Test value is AUDIOSTREAM_SAMPLE_S24LE.
* @tc.number: OH_AudioStreamBuilder_SetSampleFormat_003
* @tc.desc  : Test OH_AudioStreamBuilder_SetSampleFormat interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetSampleFormat_003, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_SampleFormat format = AUDIOSTREAM_SAMPLE_S24LE;
    result = OH_AudioStreamBuilder_SetSampleFormat(builder, format);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetSampleFormat API via legal state. Test value is AUDIOSTREAM_SAMPLE_S32LE.
* @tc.number: OH_AudioStreamBuilder_SetSampleFormat_004
* @tc.desc  : Test OH_AudioStreamBuilder_SetSampleFormat interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetSampleFormat_004, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_SampleFormat format = AUDIOSTREAM_SAMPLE_S32LE;
    result = OH_AudioStreamBuilder_SetSampleFormat(builder, format);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetSampleFormat API via legal state. Test value is AUDIOSTREAM_SAMPLE_F32LE.
* @tc.number: OH_AudioStreamBuilder_SetSampleFormat_005
* @tc.desc  : Test OH_AudioStreamBuilder_SetSampleFormat interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetSampleFormat_005, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_SampleFormat format = AUDIOSTREAM_SAMPLE_S32LE;
    result = OH_AudioStreamBuilder_SetSampleFormat(builder, format);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetLatencyMode API via legal state, AUDIOSTREAM_LATENCY_MODE_NORMAL.
* @tc.number: OH_AudioStreamBuilder_SetLatencyMode_001
* @tc.desc  : Test OH_AudioStreamBuilder_SetLatencyMode interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetLatencyMode_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_LatencyMode latencyMode = AUDIOSTREAM_LATENCY_MODE_NORMAL;
    result = OH_AudioStreamBuilder_SetLatencyMode(builder, latencyMode);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetCapturerInfo API via illegal state,
*             sourceType is AUDIOSTREAM_SOURCE_TYPE_INVALID.
* @tc.number: OH_AudioStreamBuilder_SetCapturerInfo_001
* @tc.desc  : Test OH_AudioStreamBuilder_SetCapturerInfo interface. Returns error code, if source type is
*             AUDIOSTREAM_SOURCE_TYPE_INVALID.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetCapturerInfo_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_SourceType sourceType = AUDIOSTREAM_SOURCE_TYPE_INVALID;
    result = OH_AudioStreamBuilder_SetCapturerInfo(builder, sourceType);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetCapturerInfo API via legal state,
*             sourceType is AUDIOSTREAM_SOURCE_TYPE_VOICE_COMMUNICATION.
* @tc.number: OH_AudioStreamBuilder_SetCapturerInfo_002
* @tc.desc  : Test OH_AudioStreamBuilder_SetCapturerInfo interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetCapturerInfo_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_SourceType sourceType = AUDIOSTREAM_SOURCE_TYPE_VOICE_COMMUNICATION;
    result = OH_AudioStreamBuilder_SetCapturerInfo(builder, sourceType);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetRendererInfo API via illegal state.
* @tc.number: OH_AudioStreamBuilder_SetRendererInfo_001
* @tc.desc  : Test OH_AudioStreamBuilder_SetRendererInfo interface. Returns error code, if usage type is
*             AUDIOSTREAM_USAGE_UNKNOWN.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetRendererInfo_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RENDERER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_Usage usage = AUDIOSTREAM_USAGE_UNKNOWN;
    result = OH_AudioStreamBuilder_SetRendererInfo(builder, usage);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetRendererInfo API via illegal state.
* @tc.number: OH_AudioStreamBuilder_SetRendererInfo_002
* @tc.desc  : Test OH_AudioStreamBuilder_SetRendererInfo interface. Returns error code, if content type is
*             AUDIOSTREAM_CONTENT_TYPE_UNKNOWN.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetRendererInfo_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RENDERER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_Usage usage = AUDIOSTREAM_USAGE_MEDIA;
    result = OH_AudioStreamBuilder_SetRendererInfo(builder, usage);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetRendererInfo API via legal state.
* @tc.number: OH_AudioStreamBuilder_SetRendererInfo_003
* @tc.desc  : Test OH_AudioStreamBuilder_SetRendererInfo interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetRendererInfo_003, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RENDERER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioStream_Usage usage = AUDIOSTREAM_USAGE_MEDIA;
    result = OH_AudioStreamBuilder_SetRendererInfo(builder, usage);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetChannelLayout API via legal state, AUDIOSTREAM_LATENCY_MODE_NORMAL.
* @tc.number: OH_AudioStreamBuilder_SetChannelLayout_001
* @tc.desc  : Test OH_AudioStreamBuilder_SetChannelLayout interface. Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetChannelLayout_001, TestSize.Level0)
{
    OH_AudioStreamBuilder *builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RENDERER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioChannelLayout channelLayout = CH_LAYOUT_UNKNOWN;
    result = OH_AudioStreamBuilder_SetChannelLayout(builder, channelLayout);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_SetWriteDataWithMetadataCallback API via legal state.
* @tc.number: OH_AudioStreamBuilder_SetWriteDataWithMetadataCallback_001
* @tc.desc  : Test OH_AudioStreamBuilder_SetWriteDataWithMetadataCallback interface.
              Returns true if result is successful.
*/
HWTEST(OHAudioStreamBuilderUnitTest, OH_AudioStreamBuilder_SetWriteDataWithMetadataCallback_001, TestSize.Level0)
{
    OH_AudioStreamBuilder *builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RENDERER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    OH_AudioRenderer_WriteDataWithMetadataCallback callback = AudioRendererWriteDataWithMetadataCallback;
    result = OH_AudioStreamBuilder_SetWriteDataWithMetadataCallback(builder, callbacks, nullptr);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);

    result = OH_AudioStreamBuilder_Destroy(builder);
    EXPECT_EQ(result, AUDIOSTREAM_SUCCESS);
}
} // namespace AudioStandard
} // namespace OHOS
