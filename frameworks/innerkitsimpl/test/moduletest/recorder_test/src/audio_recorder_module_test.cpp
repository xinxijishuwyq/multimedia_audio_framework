/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "audio_recorder_module_test.h"

#include "audio_errors.h"
#include "audio_info.h"
#include "audio_recorder.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {
namespace {
    const string AUDIO_RECORD_FILE1 = "/data/audiorecordtest_blocking.pcm";
    const string AUDIO_RECORD_FILE2 = "/data/audiorecordtest_nonblocking.pcm";
    const int32_t READ_BUFFERS_COUNT = 128;
    const int32_t VALUE_ZERO = 0;
} // namespace

void AudioRecorderModuleTest::SetUpTestCase(void) {}
void AudioRecorderModuleTest::TearDownTestCase(void) {}
void AudioRecorderModuleTest::SetUp(void) {}
void AudioRecorderModuleTest::TearDown(void) {}

int32_t AudioRecorderModuleTest::InitializeRecorder(unique_ptr<AudioRecorder> &audioRecorder)
{
    AudioRecorderParams recorderParams;
    recorderParams.audioSampleFormat = SAMPLE_S16LE;
    recorderParams.samplingRate = SAMPLE_RATE_44100;
    recorderParams.audioChannel = STEREO;
    recorderParams.audioEncoding = ENCODING_PCM;

    return audioRecorder->SetParams(recorderParams);
}

/**
* @tc.name  : Test GetSupportedFormats API
* @tc.number: Audio_Recorder_GetSupportedFormats_001
* @tc.desc  : Test GetSupportedFormats interface. Returns supported Formats on success.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetSupportedFormats_001, TestSize.Level1)
{
    vector<AudioSampleFormat> supportedFormatList = AudioRecorder::GetSupportedFormats();
    EXPECT_EQ(AUDIO_SUPPORTED_FORMATS.size(), supportedFormatList.size());
}

/**
* @tc.name  : Test GetSupportedChannels API
* @tc.number: Audio_Recorder_GetSupportedChannels_001
* @tc.desc  : Test GetSupportedChannels interface. Returns supported Channels on success.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetSupportedChannels_001, TestSize.Level1)
{
    vector<AudioChannel> supportedChannelList = AudioRecorder::GetSupportedChannels();
    EXPECT_EQ(AUDIO_SUPPORTED_CHANNELS.size(), supportedChannelList.size());
}

/**
* @tc.name  : Test GetSupportedEncodingTypes API
* @tc.number: Audio_Recorder_GetSupportedEncodingTypes_001
* @tc.desc  : Test GetSupportedEncodingTypes interface. Returns supported Encoding types on success.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetSupportedEncodingTypes_001, TestSize.Level1)
{
    vector<AudioEncodingType> supportedEncodingTypes
                                        = AudioRecorder::GetSupportedEncodingTypes();
    EXPECT_EQ(AUDIO_SUPPORTED_ENCODING_TYPES.size(), supportedEncodingTypes.size());
}

/**
* @tc.name  : Test GetSupportedSamplingRates API
* @tc.number: Audio_Recorder_GetSupportedSamplingRates_001
* @tc.desc  : Test GetSupportedSamplingRates interface. Returns supported Sampling rates on success.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetSupportedSamplingRates_001, TestSize.Level1)
{
    vector<AudioSamplingRate> supportedSamplingRates = AudioRecorder::GetSupportedSamplingRates();
    EXPECT_EQ(AUDIO_SUPPORTED_SAMPLING_RATES.size(), supportedSamplingRates.size());
}

/**
* @tc.name  : Test Create API via legal input.
* @tc.number: Audio_Recorder_Create_001
* @tc.desc  : Test Create interface with STREAM_MUSIC. Returns audioRecorder instance, if create is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Create_001, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    EXPECT_NE(nullptr, audioRecorder);
}

/**
* @tc.name  : Test Create API via legal input.
* @tc.number: Audio_Recorder_Create_002
* @tc.desc  : Test Create interface with STREAM_RING. Returns audioRecorder instance, if create is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Create_002, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_RING);
    EXPECT_NE(nullptr, audioRecorder);
}

/**
* @tc.name  : Test Create API via legal input.
* @tc.number: Audio_Recorder_Create_003
* @tc.desc  : Test Create interface with STREAM_VOICE_CALL. Returns audioRecorder instance if create is successful.
*             Note: instance will be created but functional support for STREAM_VOICE_CALL not available yet.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Create_003, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_VOICE_CALL);
    EXPECT_NE(nullptr, audioRecorder);
}

/**
* @tc.name  : Test Create API via legal input.
* @tc.number: Audio_Recorder_Create_004
* @tc.desc  : Test Create interface with STREAM_SYSTEM. Returns audioRecorder instance, if create is successful.
*             Note: instance will be created but functional support for STREAM_SYSTEM not available yet.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Create_004, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_SYSTEM);
    EXPECT_NE(nullptr, audioRecorder);
}

/**
* @tc.name  : Test Create API via legal input.
* @tc.number: Audio_Recorder_Create_005
* @tc.desc  : Test Create interface with STREAM_BLUETOOTH_SCO. Returns audioRecorder instance, if create is successful.
*             Note: instance will be created but functional support for STREAM_BLUETOOTH_SCO not available yet
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Create_005, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_BLUETOOTH_SCO);
    EXPECT_NE(nullptr, audioRecorder);
}

/**
* @tc.name  : Test Create API via legal input.
* @tc.number: Audio_Recorder_Create_006
* @tc.desc  : Test Create interface with STREAM_ALARM. Returns audioRecorder instance, if create is successful.
*             Note: instance will be created but functional support for STREAM_ALARM not available yet.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Create_006, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_ALARM);
    EXPECT_NE(nullptr, audioRecorder);
}

/**
* @tc.name  : Test Create API via legal input.
* @tc.number: Audio_Recorder_Create_007
* @tc.desc  : Test Create interface with STREAM_NOTIFICATION. Returns audioRecorder instance, if create is successful.
*             Note: instance will be created but functional support for STREAM_NOTIFICATION not available yet.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Create_007, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_NOTIFICATION);
    EXPECT_NE(nullptr, audioRecorder);
}

/**
* @tc.name  : Test SetParams API via legal input
* @tc.number: Audio_Recorder_SetParams_001
* @tc.desc  : Test SetParams interface. Returns 0 {SUCCESS}, if the setting is successful.
*             recorderParams.audioSampleFormat = SAMPLE_S16LE;
*             recorderParams.samplingRate = SAMPLE_RATE_44100;
*             recorderParams.audioChannel = STEREO;
*             recorderParams.audioEncoding = ENCODING_PCM;
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_SetParams_001, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    AudioRecorderParams recorderParams;
    recorderParams.audioSampleFormat = SAMPLE_S16LE;
    recorderParams.samplingRate = SAMPLE_RATE_44100;
    recorderParams.audioChannel = STEREO;
    recorderParams.audioEncoding = ENCODING_PCM;

    int32_t ret = audioRecorder->SetParams(recorderParams);
    EXPECT_EQ(SUCCESS, ret);
    audioRecorder->Release();
}

/**
* @tc.name  : Test SetParams API via legal input.
* @tc.number: Audio_Recorder_SetParams_002
* @tc.desc  : Test SetParams interface. Returns 0 {SUCCESS}, if the setting is successful.
*             recorderParams.audioSampleFormat = SAMPLE_S16LE;
*             recorderParams.samplingRate = SAMPLE_RATE_8000;
*             recorderParams.audioChannel = MONO;
*             recorderParams.audioEncoding = ENCODING_PCM;
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_SetParams_002, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    AudioRecorderParams recorderParams;
    recorderParams.audioSampleFormat = SAMPLE_S16LE;
    recorderParams.samplingRate = SAMPLE_RATE_8000;
    recorderParams.audioChannel = MONO;
    recorderParams.audioEncoding = ENCODING_PCM;

    int32_t ret = audioRecorder->SetParams(recorderParams);
    EXPECT_EQ(SUCCESS, ret);
    audioRecorder->Release();
}

/**
* @tc.name  : Test SetParams API via legal input.
* @tc.number: Audio_Recorder_SetParams_003
* @tc.desc  : Test SetParams interface. Returns 0 {SUCCESS}, if the setting is successful.
*             recorderParams.audioSampleFormat = SAMPLE_S16LE;
*             recorderParams.samplingRate = SAMPLE_RATE_11025;
*             recorderParams.audioChannel = STEREO;
*             recorderParams.audioEncoding = ENCODING_PCM;
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_SetParams_003, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    AudioRecorderParams recorderParams;
    recorderParams.audioSampleFormat = SAMPLE_S16LE;
    recorderParams.samplingRate = SAMPLE_RATE_11025;
    recorderParams.audioChannel = STEREO;
    recorderParams.audioEncoding = ENCODING_PCM;

    int32_t ret = audioRecorder->SetParams(recorderParams);
    EXPECT_EQ(SUCCESS, ret);
    audioRecorder->Release();
}

/**
* @tc.name  : Test SetParams API via legal input.
* @tc.number: Audio_Recorder_SetParams_004
* @tc.desc  : Test SetParams interface. Returns 0 {SUCCESS}, if the setting is successful.
*             recorderParams.audioSampleFormat = SAMPLE_S16LE;
*             recorderParams.samplingRate = SAMPLE_RATE_22050;
*             recorderParams.audioChannel = MONO;
*             recorderParams.audioEncoding = ENCODING_PCM;
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_SetParams_004, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    AudioRecorderParams recorderParams;
    recorderParams.audioSampleFormat = SAMPLE_S16LE;
    recorderParams.samplingRate = SAMPLE_RATE_22050;
    recorderParams.audioChannel = MONO;
    recorderParams.audioEncoding = ENCODING_PCM;

    int32_t ret = audioRecorder->SetParams(recorderParams);
    EXPECT_EQ(SUCCESS, ret);
    audioRecorder->Release();
}

/**
* @tc.name  : Test SetParams API via legal input.
* @tc.number: Audio_Recorder_SetParams_005
* @tc.desc  : Test SetParams interface. Returns 0 {SUCCESS}, if the setting is successful.
*             recorderParams.audioSampleFormat = SAMPLE_S16LE;
*             recorderParams.samplingRate = SAMPLE_RATE_96000;
*             recorderParams.audioChannel = MONO;
*             recorderParams.audioEncoding = ENCODING_PCM;
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_SetParams_005, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    AudioRecorderParams recorderParams;
    recorderParams.audioSampleFormat = SAMPLE_S16LE;
    recorderParams.samplingRate = SAMPLE_RATE_96000;
    recorderParams.audioChannel = MONO;
    recorderParams.audioEncoding = ENCODING_PCM;

    int32_t ret = audioRecorder->SetParams(recorderParams);
    EXPECT_EQ(SUCCESS, ret);
}

/**
* @tc.name  : Test SetParams API via legal input.
* @tc.number: Audio_Recorder_SetParams_006
* @tc.desc  : Test SetParams interface. Returns 0 {SUCCESS}, if the setting is successful.
*             recorderParams.audioSampleFormat = SAMPLE_S24LE;
*             recorderParams.samplingRate = SAMPLE_RATE_64000;
*             recorderParams.audioChannel = MONO;
*             recorderParams.audioEncoding = ENCODING_PCM;
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_SetParams_006, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    AudioRecorderParams recorderParams;
    recorderParams.audioSampleFormat = SAMPLE_S24LE;
    recorderParams.samplingRate = SAMPLE_RATE_64000;
    recorderParams.audioChannel = MONO;
    recorderParams.audioEncoding = ENCODING_PCM;

    int32_t ret = audioRecorder->SetParams(recorderParams);
    EXPECT_EQ(SUCCESS, ret);
}

/**
* @tc.name  : Test SetParams API via illegal input.
* @tc.number: Audio_Recorder_SetParams_007
* @tc.desc  : Test SetParams interface. Returns 0 {SUCCESS}, if the setting is successful.
*             recorderParams.audioSampleFormat = SAMPLE_S16LE;
*             recorderParams.samplingRate = SAMPLE_RATE_16000;
*             recorderParams.audioChannel = STEREO;
*             recorderParams.audioEncoding = ENCODING_PCM;
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_SetParams_007, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    AudioRecorderParams recorderParams;
    recorderParams.audioSampleFormat = SAMPLE_S16LE;
    recorderParams.samplingRate = SAMPLE_RATE_16000;
    recorderParams.audioChannel = STEREO;
    recorderParams.audioEncoding = ENCODING_PCM;

    int32_t ret = audioRecorder->SetParams(recorderParams);
    EXPECT_EQ(SUCCESS, ret);
    audioRecorder->Release();
}

/**
* @tc.name  : Test GetParams API via legal input.
* @tc.number: Audio_Recorder_GetParams_001
* @tc.desc  : Test GetParams interface. Returns 0 {SUCCESS}, if the getting is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetParams_001, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    AudioRecorderParams recorderParams;
    recorderParams.audioSampleFormat = SAMPLE_S16LE;
    recorderParams.samplingRate = SAMPLE_RATE_44100;
    recorderParams.audioChannel = STEREO;
    recorderParams.audioEncoding = ENCODING_PCM;
    ret = audioRecorder->SetParams(recorderParams);
    EXPECT_EQ(SUCCESS, ret);

    AudioRecorderParams getRecorderParams;
    ret = audioRecorder->GetParams(getRecorderParams);
    EXPECT_EQ(SUCCESS, ret);
    EXPECT_EQ(recorderParams.audioSampleFormat, getRecorderParams.audioSampleFormat);
    EXPECT_EQ(recorderParams.samplingRate, getRecorderParams.samplingRate);
    EXPECT_EQ(recorderParams.audioChannel, getRecorderParams.audioChannel);
    EXPECT_EQ(recorderParams.audioEncoding, getRecorderParams.audioEncoding);

    audioRecorder->Release();
}

/**
* @tc.name  : Test GetParams API via legal state, RECORDER_RUNNING: GetParams after Start.
* @tc.number: Audio_Recorder_GetParams_002
* @tc.desc  : Test GetParams interface. Returns 0 {SUCCESS} if the getting is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetParams_002, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    AudioRecorderParams recorderParams;
    recorderParams.audioSampleFormat = SAMPLE_S16LE;
    recorderParams.samplingRate = SAMPLE_RATE_44100;
    recorderParams.audioChannel = MONO;
    recorderParams.audioEncoding = ENCODING_PCM;
    ret = audioRecorder->SetParams(recorderParams);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    AudioRecorderParams getRecorderParams;
    ret = audioRecorder->GetParams(getRecorderParams);
    EXPECT_EQ(SUCCESS, ret);

    audioRecorder->Release();
}

/**
* @tc.name  : Test GetParams API via illegal state, RECORDER_NEW: Call GetParams without SetParams.
* @tc.number: Audio_Recorder_GetParams_003
* @tc.desc  : Test GetParams interface. Returns error code, if the recorder state is RECORDER_NEW.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetParams_003, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    AudioRecorderParams recorderParams;
    recorderParams.audioSampleFormat = SAMPLE_S16LE;
    recorderParams.samplingRate = SAMPLE_RATE_44100;
    recorderParams.audioChannel = MONO;
    recorderParams.audioEncoding = ENCODING_PCM;

    AudioRecorderParams getRecorderParams;
    ret = audioRecorder->GetParams(getRecorderParams);
    EXPECT_EQ(ERR_OPERATION_FAILED, ret);
}

/**
* @tc.name  : Test GetParams API via illegal state, RECORDER_RELEASED: Call GetParams after Release.
* @tc.number: Audio_Recorder_GetParams_004
* @tc.desc  : Test GetParams interface. Returns error code, if the recorder state is RECORDER_RELEASED.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetParams_004, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(true, isReleased);

    AudioRecorderParams getRecorderParams;
    ret = audioRecorder->GetParams(getRecorderParams);
    EXPECT_EQ(ERR_OPERATION_FAILED, ret);
}

/**
* @tc.name  : Test GetParams API via legal state, RECORDER_STOPPED: GetParams after Stop.
* @tc.number: Audio_Recorder_GetParams_005
* @tc.desc  : Test GetParams interface. Returns 0 {SUCCESS}, if the getting is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetParams_005, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(true, isStopped);

    AudioRecorderParams getRecorderParams;
    ret = audioRecorder->GetParams(getRecorderParams);
    EXPECT_EQ(SUCCESS, ret);

    audioRecorder->Release();
}

/**
* @tc.name  : Test GetBufferSize API via legal input.
* @tc.number: Audio_Recorder_GetBufferSize_001
* @tc.desc  : Test GetBufferSize interface. Returns 0 {SUCCESS}, if the getting is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetBufferSize_001, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(SUCCESS, ret);

    audioRecorder->Release();
}

/**
* @tc.name  : Test GetBufferSize API via illegal state, RECORDER_NEW: without initializing the recorder.
* @tc.number: Audio_Recorder_GetBufferSize_002
* @tc.desc  : Test GetBufferSize interface. Returns error code, if the recorder state is RECORDER_NEW.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetBufferSize_002, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(ERR_OPERATION_FAILED, ret);
}

/**
* @tc.name  : Test GetBufferSize API via illegal state, RECORDER_RELEASED: call Release before GetBufferSize
* @tc.number: Audio_Recorder_GetBufferSize_003
* @tc.desc  : Test GetBufferSize interface. Returns error code, if the recorder state is RECORDER_RELEASED.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetBufferSize_003, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(true, isReleased);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(ERR_OPERATION_FAILED, ret);
}

/**
* @tc.name  : Test GetBufferSize API via legal state, RECORDER_STOPPED: call Stop before GetBufferSize
* @tc.number: Audio_Recorder_GetBufferSize_004
* @tc.desc  : Test GetBufferSize interface. Returns 0 {SUCCESS}, if the getting is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetBufferSize_004, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(true, isStopped);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(SUCCESS, ret);

    audioRecorder->Release();
}

/**
* @tc.name  : Test GetBufferSize API via legal state, RECORDER_RUNNING: call Start before GetBufferSize
* @tc.number: Audio_Recorder_GetBufferSize_005
* @tc.desc  : test GetBufferSize interface. Returns 0 {SUCCESS}, if the getting is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetBufferSize_005, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(SUCCESS, ret);

    audioRecorder->Release();
}

/**
* @tc.name  : Test GetFrameCount API via legal input.
* @tc.number: Audio_Recorder_GetFrameCount_001
* @tc.desc  : test GetFrameCount interface, Returns 0 {SUCCESS}, if the getting is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetFrameCount_001, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    uint32_t frameCount;
    ret = audioRecorder->GetFrameCount(frameCount);
    EXPECT_EQ(SUCCESS, ret);

    audioRecorder->Release();
}

/**
* @tc.name  : Test GetFrameCount API via illegal state, RECORDER_NEW: without initialiing the recorder.
* @tc.number: Audio_Recorder_GetFrameCount_002
* @tc.desc  : Test GetFrameCount interface. Returns error code, if the recorder state is RECORDER_NEW.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetFrameCount_002, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    uint32_t frameCount;
    ret = audioRecorder->GetFrameCount(frameCount);
    EXPECT_EQ(ERR_OPERATION_FAILED, ret);
}

/**
* @tc.name  : Test GetFrameCount API via legal state, RECORDER_RUNNING: call Start before GetFrameCount.
* @tc.number: Audio_Recorder_GetFrameCount_003
* @tc.desc  : Test GetFrameCount interface. Returns 0 {SUCCESS}, if the getting is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetFrameCount_003, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    uint32_t frameCount;
    ret = audioRecorder->GetFrameCount(frameCount);
    EXPECT_EQ(SUCCESS, ret);

    audioRecorder->Release();
}

/**
* @tc.name  : Test GetFrameCount API via legal state, RECORDER_STOPPED: call Stop before GetFrameCount
* @tc.number: Audio_Recorder_GetFrameCount_004
* @tc.desc  : Test GetFrameCount interface. Returns 0 {SUCCESS}, if the getting is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetFrameCount_004, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(true, isStopped);

    uint32_t frameCount;
    ret = audioRecorder->GetFrameCount(frameCount);
    EXPECT_EQ(SUCCESS, ret);

    audioRecorder->Release();
}

/**
* @tc.name  : Test GetFrameCount API via illegal state, RECORDER_RELEASED: call Release before GetFrameCount
* @tc.number: Audio_Recorder_GetFrameCount_005
* @tc.desc  : Test GetFrameCount interface.  Returns error code, if the state is RECORDER_RELEASED.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetFrameCount_005, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(true, isReleased);

    uint32_t frameCount;
    ret = audioRecorder->GetFrameCount(frameCount);
    EXPECT_EQ(ERR_OPERATION_FAILED, ret);
}

/**
* @tc.name  : Test Start API via legal state, RECORDER_PREPARED.
* @tc.number: Audio_Recorder_Start_001
* @tc.desc  : Test Start interface. Returns true if start is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Start_001, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    audioRecorder->Release();
}

/**
* @tc.name  : Test Start API via illegal state, RECORDER_NEW: without initializing the recorder.
* @tc.number: Audio_Recorder_Start_002
* @tc.desc  : Test Start interface. Returns false, if the recorder state is RECORDER_NEW.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Start_002, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(false, isStarted);
}

/**
* @tc.name  : Test Start API via illegal state, RECORDER_RELEASED: call Start after Release
* @tc.number: Audio_Recorder_Start_003
* @tc.desc  : Test Start interface. Returns false, if the recorder state is RECORDER_RELEASED.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Start_003, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(true, isReleased);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(false, isStarted);

    audioRecorder->Release();
}

/**
* @tc.name  : Test Start API via legal state, RECORDER_STOPPED: Start Stop and then Start again
* @tc.number: Audio_Recorder_Start_004
* @tc.desc  : Test Start interface. Returns true, if the start is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Start_004, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(true, isStopped);

    isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    audioRecorder->Release();
}

/**
* @tc.name  : Test Start API via illegal state, RECORDER_RUNNING : call Start repeatedly
* @tc.number: Audio_Recorder_Start_005
* @tc.desc  : Test Start interface. Returns false, if the recorder state is RECORDER_RUNNING.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Start_005, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    isStarted = audioRecorder->Start();
    EXPECT_EQ(false, isStarted);

    audioRecorder->Release();
}

/**
* @tc.name  : Test Read API via isBlockingRead = true.
* @tc.number: Audio_Recorder_Read_001
* @tc.desc  : Test Read interface. Returns number of bytes read, if the read is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Read_001, TestSize.Level1)
{
    int32_t ret = -1;
    bool isBlockingRead = true;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(SUCCESS, ret);

    uint8_t *buffer = (uint8_t *) malloc(bufferLen);
    ASSERT_NE(nullptr, buffer);
    FILE *recFile = fopen(AUDIO_RECORD_FILE1.c_str(), "wb");
    ASSERT_NE(nullptr, recFile);

    size_t size = 1;
    int32_t bytesRead = 0;
    int32_t numBuffersToRecord = READ_BUFFERS_COUNT;

    while (numBuffersToRecord) {
        bytesRead = audioRecorder->Read(*buffer, bufferLen, isBlockingRead);
        if (bytesRead < 0) {
            break;
        } else if (bytesRead > 0) {
            fwrite(buffer, size, bytesRead, recFile);
            numBuffersToRecord--;
        }
    }

    audioRecorder->Flush();
    audioRecorder->Stop();
    audioRecorder->Release();

    free(buffer);
    fclose(recFile);
}

/**
* @tc.name  : Test Read API via isBlockingRead = false.
* @tc.number: Audio_Recorder_Read_002
* @tc.desc  : Test Read interface. Returns number of bytes read, if the read is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Read_002, TestSize.Level1)
{
    int32_t ret = -1;
    bool isBlockingRead = false;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(SUCCESS, ret);

    uint8_t *buffer = (uint8_t *) malloc(bufferLen);
    ASSERT_NE(nullptr, buffer);
    FILE *recFile = fopen(AUDIO_RECORD_FILE2.c_str(), "wb");
    ASSERT_NE(nullptr, recFile);

    size_t size = 1;
    int32_t bytesRead = 0;
    int32_t numBuffersToRecord = READ_BUFFERS_COUNT;

    while (numBuffersToRecord) {
        bytesRead = audioRecorder->Read(*buffer, bufferLen, isBlockingRead);
        if (bytesRead < 0) {
            break;
        } else if (bytesRead > 0) {
            fwrite(buffer, size, bytesRead, recFile);
            numBuffersToRecord--;
        }
    }

    audioRecorder->Flush();
    audioRecorder->Stop();
    audioRecorder->Release();

    free(buffer);
    fclose(recFile);
}


/**
* @tc.name  : Test Read API via illegl state, RECORDER_NEW : without Initializing the recorder.
* @tc.number: Audio_Recorder_Read_003
* @tc.desc  : Test Read interface. Returns error code, if the recorder state is RECORDER_NEW.
*           : bufferLen is invalid here, firstly bufferLen is validated in Read. So it returns ERR_INVALID_PARAM.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Read_003, TestSize.Level1)
{
    int32_t ret = -1;
    bool isBlockingRead = true;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(false, isStarted);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(ERR_OPERATION_FAILED, ret);

    uint8_t *buffer = (uint8_t *) malloc(bufferLen);
    ASSERT_NE(nullptr, buffer);

    int32_t bytesRead = audioRecorder->Read(*buffer, bufferLen, isBlockingRead);
    EXPECT_EQ(ERR_INVALID_PARAM, bytesRead);

    audioRecorder->Flush();
    audioRecorder->Stop();
    audioRecorder->Release();

    free(buffer);
}

/**
* @tc.name  : Test Read API via illegl state, RECORDER_PREPARED : Read without Start.
* @tc.number: Audio_Recorder_Read_004
* @tc.desc  : Test Read interface. Returns error code, if the recorder state is not RECORDER_RUNNING.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Read_004, TestSize.Level1)
{
    int32_t ret = -1;
    bool isBlockingRead = true;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(SUCCESS, ret);

    uint8_t *buffer = (uint8_t *) malloc(bufferLen);
    ASSERT_NE(nullptr, buffer);

    int32_t bytesRead = audioRecorder->Read(*buffer, bufferLen, isBlockingRead);
    EXPECT_EQ(ERR_ILLEGAL_STATE, bytesRead);

    audioRecorder->Flush();
    audioRecorder->Stop();
    audioRecorder->Release();

    free(buffer);
}

/**
* @tc.name  : Test Read API via illegal input, bufferLength = 0.
* @tc.number: Audio_Recorder_Read_005
* @tc.desc  : Test Read interface. Returns error code, if the bufferLength <= 0.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Read_005, TestSize.Level1)
{
    int32_t ret = -1;
    bool isBlockingRead = true;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    size_t bufferLen = 0;

    uint8_t *buffer = (uint8_t *) malloc(bufferLen);
    ASSERT_NE(nullptr, buffer);

    int32_t bytesRead = audioRecorder->Read(*buffer, bufferLen, isBlockingRead);
    EXPECT_EQ(ERR_INVALID_PARAM, bytesRead);

    audioRecorder->Flush();
    audioRecorder->Stop();
    audioRecorder->Release();

    free(buffer);
}

/**
* @tc.name  : Test Read API via illegal state, RECORDER_STOPPED: Read after Stop.
* @tc.number: Audio_Recorder_Read_006
* @tc.desc  : Test Read interface. Returns error code, if the recorder state is not RECORDER_RUNNING.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Read_006, TestSize.Level1)
{
    int32_t ret = -1;
    bool isBlockingRead = true;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(SUCCESS, ret);

    uint8_t *buffer = (uint8_t *) malloc(bufferLen);
    ASSERT_NE(nullptr, buffer);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(true, isStopped);

    int32_t bytesRead = audioRecorder->Read(*buffer, bufferLen, isBlockingRead);
    EXPECT_EQ(ERR_ILLEGAL_STATE, bytesRead);

    audioRecorder->Release();

    free(buffer);
}

/**
* @tc.name  : Test Read API via illegal state, RECORDER_RELEASED: Read after Release.
* @tc.number: Audio_Recorder_Read_007
* @tc.desc  : Test Read interface. Returns error code, if the recorder state is not RECORDER_RUNNING.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Read_007, TestSize.Level1)
{
    int32_t ret = -1;
    bool isBlockingRead = true;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(SUCCESS, ret);

    uint8_t *buffer = (uint8_t *) malloc(bufferLen);
    ASSERT_NE(nullptr, buffer);

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(true, isReleased);

    int32_t bytesRead = audioRecorder->Read(*buffer, bufferLen, isBlockingRead);
    EXPECT_EQ(ERR_ILLEGAL_STATE, bytesRead);

    free(buffer);
}

/**
* @tc.name  : Test GetAudioTime API via legal input.
* @tc.number: Audio_Recorder_GetAudioTime_001
* @tc.desc  : Test GetAudioTime interface. Returns true, if the getting is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetAudioTime_001, TestSize.Level1)
{
    int32_t ret = -1;
    bool isBlockingRead = true;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(SUCCESS, ret);

    uint8_t *buffer = (uint8_t *) malloc(bufferLen);
    ASSERT_NE(nullptr, buffer);
    int32_t bytesRead = audioRecorder->Read(*buffer, bufferLen, isBlockingRead);
    EXPECT_GE(bytesRead, VALUE_ZERO);

    Timestamp timeStamp;
    bool getAudioTime = audioRecorder->GetAudioTime(timeStamp, Timestamp::Timestampbase::MONOTONIC);
    EXPECT_EQ(true, getAudioTime);
    EXPECT_GE(timeStamp.time.tv_sec, (const long)VALUE_ZERO);
    EXPECT_GE(timeStamp.time.tv_nsec, (const long)VALUE_ZERO);

    audioRecorder->Flush();
    audioRecorder->Stop();
    audioRecorder->Release();

    free(buffer);
}

/**
* @tc.name  : Test GetAudioTime API via illegal state, RECORDER_NEW: GetAudioTime without initializing the recorder.
* @tc.number: Audio_Recorder_GetAudioTime_002
* @tc.desc  : Test GetAudioTime interface. Returns false, if the recorder state is RECORDER_NEW.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetAudioTime_002, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    Timestamp timeStamp;
    bool getAudioTime = audioRecorder->GetAudioTime(timeStamp, Timestamp::Timestampbase::MONOTONIC);
    EXPECT_EQ(false, getAudioTime);
}

/**
* @tc.name  : Test GetAudioTime API via legal state, RECORDER_RUNNING.
* @tc.number: Audio_Recorder_GetAudioTime_003
* @tc.desc  : test GetAudioTime interface. Returns true, if the getting is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetAudioTime_003, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    Timestamp timeStamp;
    bool getAudioTime = audioRecorder->GetAudioTime(timeStamp, Timestamp::Timestampbase::MONOTONIC);
    EXPECT_EQ(true, getAudioTime);

    audioRecorder->Release();
}

/**
* @tc.name  : Test GetAudioTime API via legal state, RECORDER_STOPPED.
* @tc.number: Audio_Recorder_GetAudioTime_004
* @tc.desc  : Test GetAudioTime interface.  Returns true, if the getting is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetAudioTime_004, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(true, isStopped);

    Timestamp timeStamp;
    bool getAudioTime = audioRecorder->GetAudioTime(timeStamp, Timestamp::Timestampbase::MONOTONIC);
    EXPECT_EQ(true, getAudioTime);

    audioRecorder->Release();
}

/**
* @tc.name  : Test GetAudioTime API via illegal state, RECORDER_RELEASED: GetAudioTime after Release.
* @tc.number: Audio_Recorder_GetAudioTime_005
* @tc.desc  : Test GetAudioTime interface. Returns false, if the recorder state is RECORDER_RELEASED
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetAudioTime_005, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(true, isStopped);

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(true, isReleased);

    Timestamp timeStamp;
    bool getAudioTime = audioRecorder->GetAudioTime(timeStamp, Timestamp::Timestampbase::MONOTONIC);
    EXPECT_EQ(false, getAudioTime);
}

/**
* @tc.name  : Test Flush API.
* @tc.number: Audio_Recorder_Flush_001
* @tc.desc  : Test Flush interface. Returns true, if the flush is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Flush_001, TestSize.Level1)
{
    int32_t ret = -1;
    bool isBlockingRead = true;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(SUCCESS, ret);

    uint8_t *buffer = (uint8_t *) malloc(bufferLen);
    ASSERT_NE(nullptr, buffer);

    int32_t bytesRead = audioRecorder->Read(*buffer, bufferLen, isBlockingRead);
    EXPECT_GE(bytesRead, VALUE_ZERO);

    bool isFlushed = audioRecorder->Flush();
    EXPECT_EQ(true, isFlushed);

    audioRecorder->Stop();
    audioRecorder->Release();

    free(buffer);
}

/**
* @tc.name  : Test Flush API via illegal state, RECORDER_NEW: Without initializing the recorder.
* @tc.number: Audio_Recorder_Flush_002
* @tc.desc  : Test Flush interface. Returns false, if the recorder state is not RECORDER_RUNNING.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Flush_002, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    bool isFlushed = audioRecorder->Flush();
    EXPECT_EQ(false, isFlushed);

    audioRecorder->Release();
}

/**
* @tc.name  : Test Flush API via illegal state, RECORDER_PREPARED: Without Start.
* @tc.number: Audio_Recorder_Flush_003
* @tc.desc  : Test Flush interface. Returns false, if the recorder state is not RECORDER_RUNNING.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Flush_003, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isFlushed = audioRecorder->Flush();
    EXPECT_EQ(false, isFlushed);

    audioRecorder->Release();
}

/**
* @tc.name  : Test Flush API via illegal state, RECORDER_STOPPED: call Stop before Flush.
* @tc.number: Audio_Recorder_Flush_004
* @tc.desc  : Test Flush interface. Returns false, if the recorder state is not RECORDER_RUNNING.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Flush_004, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(true, isStopped);

    bool isFlushed = audioRecorder->Flush();
    EXPECT_EQ(false, isFlushed);

    audioRecorder->Release();
}

/**
* @tc.name  : Test Flush API via illegal state, RECORDER_RELEASED: call Release before Flush.
* @tc.number: Audio_Recorder_Flush_005
* @tc.desc  : Test Flush interface. Returns false, if the recorder state is not RECORDER_RUNNING.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Flush_005, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(true, isReleased);

    bool isFlushed = audioRecorder->Flush();
    EXPECT_EQ(false, isFlushed);

    audioRecorder->Release();
}

/**
* @tc.name  : Test Stop API.
* @tc.number: Audio_Recorder_Stop_001
* @tc.desc  : Test Stop interface. Returns true, if the stop is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Stop_001, TestSize.Level1)
{
    int32_t ret = -1;
    bool isBlockingRead = true;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(SUCCESS, ret);

    uint8_t *buffer = (uint8_t *) malloc(bufferLen);
    ASSERT_NE(nullptr, buffer);

    int32_t bytesRead = audioRecorder->Read(*buffer, bufferLen, isBlockingRead);
    EXPECT_GE(bytesRead, VALUE_ZERO);

    audioRecorder->Flush();

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(true, isStopped);

    audioRecorder->Release();

    free(buffer);
}

/**
* @tc.name  : Test Stop API via illegal state, RECORDER_NEW: call Stop without Initializing the recorder.
* @tc.number: Audio_Recorder_Stop_002
* @tc.desc  : Test Stop interface. Returns false, if the recorder state is not RECORDER_RUNNING.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Stop_002, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(false, isStopped);
}

/**
* @tc.name  : Test Stop API via illegal state, RECORDER_PREPARED: call Stop without Start.
* @tc.number: Audio_Recorder_Stop_003
* @tc.desc  : Test Stop interface. Returns false, if the recorder state is not RECORDER_RUNNING.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Stop_003, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(false, isStopped);

    audioRecorder->Release();
}

/**
* @tc.name  : Test Stop API via illegal state, RECORDER_RELEASED: call Stop after Release.
* @tc.number: Audio_Recorder_Stop_004
* @tc.desc  : Test Stop interface. Returns false, if the recorder state is not RECORDER_RUNNING.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Stop_004, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(true, isReleased);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(false, isStopped);
}

/**
* @tc.name  : Test Stop API via legal state. call Start, Stop, Start and Stop again
* @tc.number: Audio_Recorder_Stop_005
* @tc.desc  : Test Stop interface. Returns true , if the stop is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Stop_005, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(true, isStopped);

    isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    isStopped = audioRecorder->Stop();
    EXPECT_EQ(true, isStopped);
}

/**
* @tc.name  : Test Release API.
* @tc.number: Audio_Recorder_Release_001
* @tc.desc  : Test Release interface. Returns true, if the release is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Release_001, TestSize.Level1)
{
    int32_t ret = -1;
    bool isBlockingRead = true;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    size_t bufferLen;
    ret = audioRecorder->GetBufferSize(bufferLen);
    EXPECT_EQ(SUCCESS, ret);

    uint8_t *buffer = (uint8_t *) malloc(bufferLen);
    ASSERT_NE(nullptr, buffer);

    int32_t bytesRead = audioRecorder->Read(*buffer, bufferLen, isBlockingRead);
    EXPECT_GE(bytesRead, VALUE_ZERO);

    audioRecorder->Flush();
    audioRecorder->Stop();

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(true, isReleased);

    free(buffer);
}

/**
* @tc.name  : Test Release API via illegal state, RECORDER_NEW: Call Release without initializing the recorder.
* @tc.number: Audio_Recorder_Release_002
* @tc.desc  : Test Release interface, Returns false, if the state is RECORDER_NEW.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Release_002, TestSize.Level1)
{
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(false, isReleased);
}

/**
* @tc.name  : Test Release API via illegal state, RECORDER_RELEASED: call Release repeatedly.
* @tc.number: Audio_Recorder_Release_003
* @tc.desc  : Test Release interface. Returns false, if the state is already RECORDER_RELEASED.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Release_003, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(true, isReleased);

    isReleased = audioRecorder->Release();
    EXPECT_EQ(false, isReleased);
}

/**
* @tc.name  : Test Release API via legal state, RECORDER_RUNNING: call Release after Start
* @tc.number: Audio_Recorder_Release_004
* @tc.desc  : Test Release interface. Returns true, if the release is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Release_004, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(true, isReleased);
}

/**
* @tc.name  : Test Release API via legal state, RECORDER_STOPPED: call release after Start and Stop
* @tc.number: Audio_Recorder_Release_005
* @tc.desc  : Test Release interface. Returns true, if the release is successful.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_Release_005, TestSize.Level1)
{
    int32_t ret = -1;
    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(true, isStopped);

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(true, isReleased);
}

/**
* @tc.name  : Test GetStatus API.
* @tc.number: Audio_Recorder_GetStatus_001
* @tc.desc  : Test GetStatus interface. Returns correct state on success.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetStatus_001, TestSize.Level1)
{
    int32_t ret = -1;
    RecorderState state = RECORDER_INVALID;

    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);
    state = audioRecorder->GetStatus();
    EXPECT_EQ(RECORDER_NEW, state);

    AudioRecorderParams recorderParams;
    recorderParams.audioSampleFormat = SAMPLE_S16LE;
    recorderParams.samplingRate = SAMPLE_RATE_44100;
    recorderParams.audioChannel = STEREO;
    recorderParams.audioEncoding = ENCODING_PCM;
    ret = audioRecorder->SetParams(recorderParams);
    EXPECT_EQ(SUCCESS, ret);
    state = audioRecorder->GetStatus();
    EXPECT_EQ(RECORDER_PREPARED, state);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);
    state = audioRecorder->GetStatus();
    EXPECT_EQ(RECORDER_RUNNING, state);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(true, isStopped);
    state = audioRecorder->GetStatus();
    EXPECT_EQ(RECORDER_STOPPED, state);

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(true, isReleased);
    state = audioRecorder->GetStatus();
    EXPECT_EQ(RECORDER_RELEASED, state);
}
/**
* @tc.name  : Test GetStatus API, call Start without Initializing the recorder
* @tc.number: Audio_Recorder_GetStatus_002
* @tc.desc  : Test GetStatus interface. Not changes to RECORDER_RUNNING, if the current state is RECORDER_NEW.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetStatus_002, TestSize.Level1)
{
    RecorderState state = RECORDER_INVALID;

    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(false, isStarted);
    state = audioRecorder->GetStatus();
    EXPECT_NE(RECORDER_RUNNING, state);
    EXPECT_EQ(RECORDER_NEW, state);
}

/**
* @tc.name  : Test GetStatus API, call Stop without Start
* @tc.desc  : Test GetStatus interface. Not changes to RECORDER_STOPPED, if the current state is RECORDER_PREPARED.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetStatus_003, TestSize.Level1)
{
    int32_t ret = -1;
    RecorderState state = RECORDER_INVALID;

    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(false, isStopped);
    state = audioRecorder->GetStatus();
    EXPECT_NE(RECORDER_STOPPED, state);
    EXPECT_EQ(RECORDER_PREPARED, state);

    audioRecorder->Release();
}

/**
* @tc.name  : Test GetStatus API, call Start, Stop and then Start again
* @tc.number: Audio_Recorder_GetStatus_004
* @tc.desc  : Test GetStatus interface.  Returns correct state on success.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetStatus_004, TestSize.Level1)
{
    int32_t ret = -1;
    RecorderState state = RECORDER_INVALID;

    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    ret = AudioRecorderModuleTest::InitializeRecorder(audioRecorder);
    EXPECT_EQ(SUCCESS, ret);

    bool isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);
    state = audioRecorder->GetStatus();
    EXPECT_EQ(RECORDER_RUNNING, state);

    bool isStopped = audioRecorder->Stop();
    EXPECT_EQ(true, isStopped);
    state = audioRecorder->GetStatus();
    EXPECT_EQ(RECORDER_STOPPED, state);

    isStarted = audioRecorder->Start();
    EXPECT_EQ(true, isStarted);
    state = audioRecorder->GetStatus();
    EXPECT_EQ(RECORDER_RUNNING, state);

    audioRecorder->Release();
}

/**
* @tc.name  : Test GetStatus API, call Release without initializing
* @tc.number: Audio_Recorder_GetStatus_005
* @tc.desc  : Test GetStatus interface. Not changes to RECORDER_RELEASED, if the current state is RECORDER_NEW.
*/
HWTEST(AudioRecorderModuleTest, Audio_Recorder_GetStatus_005, TestSize.Level1)
{
    RecorderState state = RECORDER_INVALID;

    unique_ptr<AudioRecorder> audioRecorder = AudioRecorder::Create(STREAM_MUSIC);
    ASSERT_NE(nullptr, audioRecorder);

    bool isReleased = audioRecorder->Release();
    EXPECT_EQ(false, isReleased);
    state = audioRecorder->GetStatus();
    EXPECT_NE(RECORDER_RELEASED, state);
    EXPECT_EQ(RECORDER_NEW, state);
}
} // namespace AudioStandard
} // namespace OHOS
