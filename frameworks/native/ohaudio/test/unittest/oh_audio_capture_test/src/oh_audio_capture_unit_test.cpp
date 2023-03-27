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

#include "oh_audio_capture_unit_test.h"

using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {
void OHAudioCaptureUnitTest::SetUpTestCase(void) { }

void OHAudioCaptureUnitTest::TearDownTestCase(void) { }

void OHAudioCaptureUnitTest::SetUp(void) { }

void OHAudioCaptureUnitTest::TearDown(void) { }

OH_AudioStreamBuilder* OHAudioCaptureUnitTest::CreateCapturerBuilder()
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_CAPTURER;
    OH_AudioStreamBuilder_Create(&builder, type);
    return builder;
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_GenerateCapturer API via legal state.
* @tc.number: OH_Audio_Capture_Generate_001
* @tc.desc  : Test OH_AudioStreamBuilder_GenerateCapturer interface. Returns true, if the result is successful.
*/
HWTEST(OHAudioCaptureUnitTest, OH_Audio_Capture_Generate_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioCaptureUnitTest::CreateCapturerBuilder();

    OH_AudioCapturer* audioCapturer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateCapturer(builder, &audioCapturer);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);

    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioStreamBuilder_GenerateCapturer API via illegal OH_AudioStream_Type.
* @tc.number: OH_Audio_Capture_Generate_002
* @tc.desc  : Test OH_AudioStreamBuilder_GenerateCapturer interface. Returns error code, if the stream type is
*             AUDIOSTREAM_TYPE_RERNDERER.
*/
HWTEST(OHAudioCaptureUnitTest, OH_Audio_Capture_Generate_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RERNDERER;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&builder, type);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);

    OH_AudioCapturer* audioCapturer;
    result = OH_AudioStreamBuilder_GenerateCapturer(builder, &audioCapturer);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_INVALID_PARAM);

    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioCapturer_Start API via legal state.
* @tc.number: Audio_Capturer_Start_001
* @tc.desc  : Test OH_AudioCapturer_Start interface. Returns true if start is successful.
*/
HWTEST(OHAudioCaptureUnitTest, OH_Audio_Capture_Start_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioCaptureUnitTest::CreateCapturerBuilder();

    OH_AudioCapturer* audioCapturer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateCapturer(builder, &audioCapturer);

    result = OH_AudioCapturer_Start(audioCapturer);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);

    OH_AudioCapturer_Release(audioCapturer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioCapturer_Start API via illegal state.
* @tc.number: Audio_Capturer_Start_002
* @tc.desc  : Test OH_AudioCapturer_Start interface. Returns error code, if Start interface is called twice.
*/
HWTEST(OHAudioCaptureUnitTest, OH_Audio_Capture_Start_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioCaptureUnitTest::CreateCapturerBuilder();

    OH_AudioCapturer* audioCapturer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateCapturer(builder, &audioCapturer);

    result = OH_AudioCapturer_Start(audioCapturer);

    result = OH_AudioCapturer_Start(audioCapturer);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_ILLEGAL_STATE);

    OH_AudioCapturer_Release(audioCapturer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioCapturer_Pause API via legal state.
* @tc.number: OH_Audio_Capture_Pause_001
* @tc.desc  : Test OH_AudioCapturer_Pause interface. Returns true if Pause is successful.
*/
HWTEST(OHAudioCaptureUnitTest, OH_Audio_Capture_Pause_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioCaptureUnitTest::CreateCapturerBuilder();

    OH_AudioCapturer* audioCapturer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateCapturer(builder, &audioCapturer);
    result = OH_AudioCapturer_Start(audioCapturer);

    result = OH_AudioCapturer_Pause(audioCapturer);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);

    OH_AudioCapturer_Release(audioCapturer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioCapturer_Pause API via illegal state, Pause without Start first.
* @tc.number: OH_Audio_Capture_Pause_002
* @tc.desc  : Test OH_AudioCapturer_Pause interface. Returns error code, if Pause without Start first.
*/
HWTEST(OHAudioCaptureUnitTest, OH_Audio_Capture_Pause_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioCaptureUnitTest::CreateCapturerBuilder();

    OH_AudioCapturer* audioCapturer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateCapturer(builder, &audioCapturer);

    result = OH_AudioCapturer_Pause(audioCapturer);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_ILLEGAL_STATE);

    OH_AudioCapturer_Release(audioCapturer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioCapturer_Stop API via legal state.
* @tc.number: OH_Audio_Capture_Stop_001
* @tc.desc  : Test OH_AudioCapturer_Stop interface. Returns true if Stop is successful.
*/
HWTEST(OHAudioCaptureUnitTest, OH_Audio_Capture_Stop_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioCaptureUnitTest::CreateCapturerBuilder();

    OH_AudioCapturer* audioCapturer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateCapturer(builder, &audioCapturer);
    result = OH_AudioCapturer_Start(audioCapturer);

    result = OH_AudioCapturer_Stop(audioCapturer);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);

    OH_AudioCapturer_Release(audioCapturer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioCapturer_Stop API via illegal state, Stop without Start first.
* @tc.number: OH_Audio_Capture_Stop_002
* @tc.desc  : Test OH_AudioCapturer_Stop interface. Returns error code, if Stop without Start first.
*/
HWTEST(OHAudioCaptureUnitTest, OH_Audio_Capture_Stop_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioCaptureUnitTest::CreateCapturerBuilder();

    OH_AudioCapturer* audioCapturer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateCapturer(builder, &audioCapturer);

    result = OH_AudioCapturer_Stop(audioCapturer);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_ILLEGAL_STATE);

    OH_AudioCapturer_Release(audioCapturer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioCapturer_Flush API via legal state.
* @tc.number: OH_Audio_Capture_Flush_001
* @tc.desc  : Test OH_AudioCapturer_Flush interface. Returns true if Flush is successful.
*/
HWTEST(OHAudioCaptureUnitTest, OH_Audio_Capture_Flush_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioCaptureUnitTest::CreateCapturerBuilder();

    OH_AudioCapturer* audioCapturer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateCapturer(builder, &audioCapturer);
    result = OH_AudioCapturer_Start(audioCapturer);

    result = OH_AudioCapturer_Flush(audioCapturer);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);

    OH_AudioCapturer_Release(audioCapturer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioCapturer_Flush API via illegal state.
* @tc.number: OH_Audio_Capture_Flush_002
* @tc.desc  : Test OH_AudioCapturer_Flush interface. Returns error code, if Flush without Start first.
*/
HWTEST(OHAudioCaptureUnitTest, OH_Audio_Capture_Flush_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioCaptureUnitTest::CreateCapturerBuilder();

    OH_AudioCapturer* audioCapturer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateCapturer(builder, &audioCapturer);

    result = OH_AudioCapturer_Flush(audioCapturer);
    EXPECT_TRUE(result == AUDIOSTREAM_ERROR_ILLEGAL_STATE);

    OH_AudioCapturer_Release(audioCapturer);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioCapturer_Release API via legal state.
* @tc.number: OH_Audio_Capture_Release_001
* @tc.desc  : Test OH_AudioCapturer_Release interface. Returns true if Release is successful.
*/
HWTEST(OHAudioCaptureUnitTest, OH_Audio_Capture_Release_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioCaptureUnitTest::CreateCapturerBuilder();

    OH_AudioCapturer* audioCapturer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateCapturer(builder, &audioCapturer);
    result = OH_AudioCapturer_Start(audioCapturer);

    result = OH_AudioCapturer_Release(audioCapturer);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);

    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioCapturer_GetLatencyMode API via legal state.
* @tc.number: OH_Audio_Capture_GetParameter_001
* @tc.desc  : Test OH_AudioCapturer_GetLatencyMode interface. Returns true if latencyMode is
*             AUDIOSTREAM_LATENCY_MODE_NORMAL,because default latency mode is AUDIOSTREAM_LATENCY_MODE_NORMAL.
*/
HWTEST(OHAudioCaptureUnitTest, OH_Audio_Capture_GetParameter_001, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioCaptureUnitTest::CreateCapturerBuilder();
    OH_AudioCapturer* audioCapturer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateCapturer(builder, &audioCapturer);

    OH_AudioStream_LatencyMode latencyMode = AUDIOSTREAM_LATENCY_MODE_NORMAL;
    result = OH_AudioCapturer_GetLatencyMode(audioCapturer, &latencyMode);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    EXPECT_TRUE(latencyMode == AUDIOSTREAM_LATENCY_MODE_NORMAL);
    OH_AudioStreamBuilder_Destroy(builder);
}

/**
* @tc.name  : Test OH_AudioCapturer_GetLatencyMode API via legal state.
* @tc.number: OH_Audio_Capture_GetParameter_002
* @tc.desc  : Test OH_AudioCapturer_GetLatencyMode interface. Returns true if latencyMode is
*             AUDIOSTREAM_LATENCY_MODE_NORMAL,because default latency mode is AUDIOSTREAM_LATENCY_MODE_NORMAL.
*/
HWTEST(OHAudioCaptureUnitTest, OH_Audio_Capture_GetParameter_002, TestSize.Level0)
{
    OH_AudioStreamBuilder* builder = OHAudioCaptureUnitTest::CreateCapturerBuilder();
    OH_AudioCapturer* audioCapturer;
    OH_AudioStream_Result result = OH_AudioStreamBuilder_GenerateCapturer(builder, &audioCapturer);

    uint32_t streamId;
    result = OH_AudioCapturer_GetStreamId(audioCapturer, &streamId);
    EXPECT_TRUE(result == AUDIOSTREAM_SUCCESS);
    OH_AudioStreamBuilder_Destroy(builder);
}
} // namespace AudioStandard
} // namespace OHOS
