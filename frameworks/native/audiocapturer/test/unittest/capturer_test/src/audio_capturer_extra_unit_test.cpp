/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include "audio_capturer_unit_test.h"

#include "audio_capturer.h"
#include "audio_errors.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {
/**
* @tc.name  : Test SetParams API via illegal input.
* @tc.number: Audio_Capturer_SetParams_008
* @tc.desc  : Test SetParams interface. Returns 0 {SUCCESS}, if the setting is successful.
*             capturerParams.audioSampleFormat = SAMPLE_S24LE;
*             capturerParams.samplingRate = SAMPLE_RATE_88200;
*             capturerParams.audioChannel = STEREO;
*             capturerParams.audioEncoding = ENCODING_PCM;
*/
HWTEST(AudioCapturerUnitTest, Audio_Capturer_SetParams_008, TestSize.Level1)
{
    unique_ptr<AudioCapturer> audioCapturer = AudioCapturer::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioCapturer);

    AudioCapturerParams capturerParams;
    capturerParams.audioSampleFormat = SAMPLE_S24LE;
    capturerParams.samplingRate = SAMPLE_RATE_88200;
    capturerParams.audioChannel = STEREO;
    capturerParams.audioEncoding = ENCODING_PCM;

    int32_t ret = audioCapturer->SetParams(capturerParams);
    EXPECT_EQ(SUCCESS, ret);
    audioCapturer->Release();
}

/**
* @tc.name  : Test SetParams API via illegal input.
* @tc.number: Audio_Capturer_SetParams_009
* @tc.desc  : Test SetParams interface. Returns 0 {SUCCESS}, if the setting is successful.
*             capturerParams.audioSampleFormat = SAMPLE_S24LE;
*             capturerParams.samplingRate = SAMPLE_RATE_176400;
*             capturerParams.audioChannel = STEREO;
*             capturerParams.audioEncoding = ENCODING_PCM;
*/
HWTEST(AudioCapturerUnitTest, Audio_Capturer_SetParams_009, TestSize.Level1)
{
    unique_ptr<AudioCapturer> audioCapturer = AudioCapturer::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioCapturer);

    AudioCapturerParams capturerParams;
    capturerParams.audioSampleFormat = SAMPLE_S24LE;
    capturerParams.samplingRate = SAMPLE_RATE_176400;
    capturerParams.audioChannel = STEREO;
    capturerParams.audioEncoding = ENCODING_PCM;

    int32_t ret = audioCapturer->SetParams(capturerParams);
    EXPECT_EQ(SUCCESS, ret);
    audioCapturer->Release();
}

/**
* @tc.name  : Test SetParams API via illegal input.
* @tc.number: Audio_Capturer_SetParams_010
* @tc.desc  : Test SetParams interface. Returns 0 {SUCCESS}, if the setting is successful.
*             capturerParams.audioSampleFormat = SAMPLE_S24LE;
*             capturerParams.samplingRate = SAMPLE_RATE_192000;
*             capturerParams.audioChannel = STEREO;
*             capturerParams.audioEncoding = ENCODING_PCM;
*/
HWTEST(AudioCapturerUnitTest, Audio_Capturer_SetParams_010, TestSize.Level1)
{
    unique_ptr<AudioCapturer> audioCapturer = AudioCapturer::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioCapturer);

    AudioCapturerParams capturerParams;
    capturerParams.audioSampleFormat = SAMPLE_S24LE;
    capturerParams.samplingRate = SAMPLE_RATE_192000;
    capturerParams.audioChannel = STEREO;
    capturerParams.audioEncoding = ENCODING_PCM;

    int32_t ret = audioCapturer->SetParams(capturerParams);
    EXPECT_EQ(SUCCESS, ret);
    audioCapturer->Release();
}
} // namespace AudioStandard
} // namespace OHOS
