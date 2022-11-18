/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include <thread>
#include "audio_errors.h"
#include "audio_info.h"
#include "audio_stream_unit_test.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {
const uint32_t DEFAULT_SAMPLING_RATE = 44100;
const uint8_t DEFAULT_CHANNEL_COUNT = 2;
const uint8_t DEFAULT_SAMPLE_SIZE = 2;
const uint32_t DEFAULT_STREAM_VOLUME = 0;
void AudioStreamUnitTest::SetUpTestCase(void) {}
void AudioStreamUnitTest::TearDownTestCase(void) {}
void AudioStreamUnitTest::SetUp(void) {}
void AudioStreamUnitTest::TearDown(void) {}

void AudioStreamUnitTest::InitAudioStream(std::shared_ptr<AudioStream> &audioStream){
    AppInfo appInfo_ = {};
    if (!(appInfo_.appPid)) {
        appInfo_.appPid = getpid();
    }

    if (appInfo_.appUid < 0) {
        appInfo_.appUid = static_cast<int32_t>(getuid());
    }
    
    audioStream = std::make_shared<AudioStream>(STREAM_NOTIFICATION, AUDIO_MODE_PLAYBACK, appInfo_.appUid);
    if (audioStream) {
        AUDIO_DEBUG_LOG("AudioRendererPrivate::Audio stream created");
    }
}

/**
* @tc.name  : Test Audio_Stream_GetSamplingRate_001 via legal state
* @tc.number: Audio_Stream_GetSamplingRate_001
* @tc.desc  : Test GetSamplingRate interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_GetSamplingRate_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    uint32_t samplingRate = audioStream_->GetSamplingRate();
    EXPECT_EQ(DEFAULT_SAMPLING_RATE, samplingRate);
}

/**
* @tc.name  : Test Audio_Stream_GetChannelCount_001 via legal state
* @tc.number: Audio_Stream_GetChannelCount_001
* @tc.desc  : Test GetChannelCount interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_GetChannelCount_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    uint8_t channelCount = audioStream_->GetChannelCount();
    EXPECT_EQ(DEFAULT_CHANNEL_COUNT, channelCount);
}

/**
* @tc.name  : Test Audio_Stream_GetSampleSize_001 via legal state
* @tc.number: Audio_Stream_GetSampleSize_001
* @tc.desc  : Test GetSampleSize interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_GetSampleSize_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    uint8_t sampleSize = audioStream_->GetSampleSize();
    EXPECT_EQ(DEFAULT_SAMPLE_SIZE, sampleSize);
}

/**
* @tc.name  : Test Audio_Stream_GetStreamVolume_001 via legal state
* @tc.number: Audio_Stream_GetStreamVolume_001
* @tc.desc  : Test GetStreamVolume interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_GetStreamVolume_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    uint32_t sessionID = 0;
    uint8_t streamVolume = audioStream_->GetStreamVolume(sessionID);
    EXPECT_EQ(DEFAULT_STREAM_VOLUME, streamVolume);
}

/**
* @tc.name  : Test Audio_Stream_RegisterAudioRendererCallbacks_001 via legal state
* @tc.number: Audio_Stream_RegisterAudioRendererCallbacks_001
* @tc.desc  : Test RegisterAudioRendererCallbacks interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_RegisterAudioRendererCallbacks_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    RenderCallbackTest renderCallback;
    audioStream_->RegisterAudioRendererCallbacks(renderCallback);
}

/**
* @tc.name  : Test Audio_Stream_RegisterAudioCapturerCallbacks_001 via legal state
* @tc.number: Audio_Stream_RegisterAudioCapturerCallbacks_001
* @tc.desc  : Test RegisterAudioCapturerCallbacks interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_RegisterAudioCapturerCallbacks_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    CapturterCallbackTest capturerCallback;
    audioStream_->RegisterAudioCapturerCallbacks(capturerCallback);
}

/**
* @tc.name  : Test Audio_Stream_SetStreamType_001 via illegal state
* @tc.number: Audio_Stream_SetStreamType_001
* @tc.desc  : Test SetStreamType interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_SetStreamType_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    AudioStreamType audioStreamType = AudioStreamType::STREAM_MEDIA;
    int32_t ret = audioStream_->SetStreamType(audioStreamType);
    EXPECT_EQ(OPEN_PORT_FAILURE, ret);
}

/**
* @tc.name  : Test Audio_Stream_GetStreamRenderRate_001 via legal state
* @tc.number: Audio_Stream_GetStreamRenderRate_001
* @tc.desc  : Test GetStreamRenderRate interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_GetStreamRenderRate_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    AudioRendererRate renderRate = audioStream_->GetStreamRenderRate();
    EXPECT_EQ(AudioRendererRate::RENDER_RATE_NORMAL, renderRate);
}

/**
* @tc.name  : Test Audio_Stream_GetSupportedFormats_001 via legal state
* @tc.number: Audio_Stream_GetSupportedFormats_001
* @tc.desc  : Test GetSupportedFormats interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_GetSupportedFormats_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    vector<AudioSampleFormat> supportedFormatList = audioStream_->GetSupportedFormats();
    EXPECT_EQ(AUDIO_SUPPORTED_FORMATS.size(), supportedFormatList.size());
}

/**
* @tc.name  : Test Audio_Stream_GetSupportedEncodingTypes_001 via legal state
* @tc.number: Audio_Stream_GetSupportedEncodingTypes_001
* @tc.desc  : Test GetSupportedEncodingTypes interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_GetSupportedEncodingTypes_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    vector<AudioEncodingType> supportedEncodingTypes = audioStream_->GetSupportedEncodingTypes();
    EXPECT_EQ(AUDIO_SUPPORTED_ENCODING_TYPES.size(), supportedEncodingTypes.size());
}

/**
* @tc.name  : Test Audio_Stream_GetSupportedSamplingRates_001 via legal state
* @tc.number: Audio_Stream_GetSupportedSamplingRates_001
* @tc.desc  : Test GetSupportedSamplingRates interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_GetSupportedSamplingRates_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    vector<AudioSamplingRate> supportedSamplingRates = audioStream_->GetSupportedSamplingRates();
    EXPECT_EQ(AUDIO_SUPPORTED_SAMPLING_RATES.size(), supportedSamplingRates.size());
}

/**
* @tc.name  : Test Audio_Stream_SetAudioStreamType_001 via illegal state
* @tc.number: Audio_Stream_SetAudioStreamType_001
* @tc.desc  : Test SetAudioStreamType interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_SetAudioStreamType_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    AudioStreamType audioStreamType = AudioStreamType::STREAM_RING;
    int32_t ret = audioStream_->SetAudioStreamType(audioStreamType);
    EXPECT_EQ(OPEN_PORT_FAILURE, ret);
}

/**
* @tc.name  : Test Audio_Stream_SetRenderRate_001 via legal state
* @tc.number: Audio_Stream_SetRenderRate_001
* @tc.desc  : Test SetRenderRate interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_SetRenderRate_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    AudioRendererRate renderRate = AudioRendererRate::RENDER_RATE_NORMAL;
    int32_t ret = audioStream_->SetRenderRate(renderRate);
    EXPECT_EQ(SUCCESS, ret);
}

/**
* @tc.name  : Test Audio_Stream_GetRenderRate_001 via legal state
* @tc.number: Audio_Stream_GetRenderRate_001
* @tc.desc  : Test GetRenderRate interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_GetRenderRate_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    AudioRendererRate renderRate = audioStream_->GetRenderRate();
    EXPECT_EQ(renderRate, AudioRendererRate::RENDER_RATE_NORMAL);
}

/**
* @tc.name  : Test Audio_Stream_GetCaptureMode_001 via legal state
* @tc.number: Audio_Stream_GetCaptureMode_001
* @tc.desc  : Test GetCaptureMode interface. Returns success.
*/
HWTEST(AudioStreamUnitTest, Audio_Stream_GetCaptureMode_001, TestSize.Level1)
{
    std::shared_ptr<AudioStream> audioStream_;
    AudioStreamUnitTest::InitAudioStream(audioStream_);
    AudioCaptureMode captureMode = audioStream_->GetCaptureMode();
    EXPECT_EQ(captureMode, AudioCaptureMode::CAPTURE_MODE_NORMAL);
}
} // namespace AudioStandard
} // namespace OHOS 