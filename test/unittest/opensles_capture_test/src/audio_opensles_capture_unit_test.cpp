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

#include "audio_opensles_capture_unit_test.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {
namespace {
    const char *AUDIORENDER_TEST_FILE_PATH = "/data/test_capture.pcm";
    FILE *wavFile_;
    SLObjectItf engineObject_;
    SLRecordItf captureItf_;
    SLOHBufferQueueItf bufferQueueItf_;
    SLObjectItf pcmCapturerObject_;
    SLEngineItf engineEngine_;
} // namespace

static void BuqqerQueueCallback(SLOHBufferQueueItf bufferQueueItf, void *pContext, SLuint32 size)
{
    FILE *wavFile = (FILE *)pContext;
    if (wavFile != nullptr) {
        SLuint8 *buffer = nullptr;
        SLuint32 pSize = 0;
        (*bufferQueueItf)->GetBuffer(bufferQueueItf, &buffer, pSize);
        if (buffer != nullptr) {
            fwrite(buffer, 1, pSize, wavFile);
            (*bufferQueueItf)->Enqueue(bufferQueueItf, buffer, size);
        }
    }

    return;
}

void AudioOpenslesCaptureUnitTest::SetUpTestCase(void) { }

void AudioOpenslesCaptureUnitTest::TearDownTestCase(void) { }

void AudioOpenslesCaptureUnitTest::SetUp(void) { }

void AudioOpenslesCaptureUnitTest::TearDown(void) { }

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_CreateEngine_001, TestSize.Level0)
{
    SLresult result = slCreateEngine(&engineObject_, 0, nullptr, 0, nullptr, nullptr);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_CreateEngine_002, TestSize.Level0)
{
    SLresult result = (*engineObject_)->Realize(engineObject_, SL_BOOLEAN_FALSE);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_CreateEngine_003, TestSize.Level0)
{
    SLresult result = (*engineObject_)->GetInterface(engineObject_, SL_IID_ENGINE, &engineEngine_);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_CreateAudioRecorder_001, TestSize.Level0)
{
    wavFile_ = fopen(AUDIORENDER_TEST_FILE_PATH, "wb");
    if (wavFile_ == nullptr) {
        AUDIO_INFO_LOG("AudioCaptureTest: Unable to open record file.");
    }

    SLDataLocator_IODevice io_device = {
        SL_DATALOCATOR_IODEVICE,
        SL_IODEVICE_AUDIOINPUT,
        SL_DEFAULTDEVICEID_AUDIOINPUT,
        NULL
    };

    SLDataSource audioSource = {
        &io_device,
        NULL
    };

    SLDataLocator_BufferQueue buffer_queue = {
        SL_DATALOCATOR_BUFFERQUEUE,
        3
    };

    SLDataFormat_PCM format_pcm = {
        SL_DATAFORMAT_PCM,
        OHOS::AudioStandard::AudioChannel::MONO,
        OHOS::AudioStandard::AudioSamplingRate::SAMPLE_RATE_44100,
        SL_PCMSAMPLEFORMAT_FIXED_16,
        0,
        0,
        0
    };

    SLDataSink audioSink = {
        &buffer_queue,
        &format_pcm
    };

    SLresult result = (*engineEngine_)->CreateAudioRecorder(engineEngine_, &pcmCapturerObject_, &audioSource,
                                                            &audioSink, 0, nullptr, nullptr);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_CreateAudioRecorder_002, TestSize.Level0)
{
    SLresult result = (*pcmCapturerObject_)->Realize(pcmCapturerObject_, SL_BOOLEAN_FALSE);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_CreateAudioRecorder_003, TestSize.Level0)
{
    SLresult result = (*pcmCapturerObject_)->GetInterface(pcmCapturerObject_, SL_IID_RECORD, &captureItf_);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_GetBufferQueue_001, TestSize.Level0)
{
    SLresult result = (*pcmCapturerObject_)->GetInterface(pcmCapturerObject_, SL_IID_OH_BUFFERQUEUE, &bufferQueueItf_);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_RegisterCallback_001, TestSize.Level0)
{
    SLresult result = (*bufferQueueItf_)->RegisterCallback(bufferQueueItf_, BuqqerQueueCallback, wavFile_);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_SetRecordState_001, TestSize.Level0)
{
    SLresult result = (*captureItf_)->SetRecordState(captureItf_, SL_RECORDSTATE_RECORDING);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_001, TestSize.Level0)
{
    if (wavFile_ != nullptr) {
        SLuint8* buffer = nullptr;
        SLuint32 size = 0;
        SLresult result = (*bufferQueueItf_)->GetBuffer(bufferQueueItf_, &buffer, size);
        EXPECT_TRUE(result == SL_RESULT_SUCCESS);
        if (buffer != nullptr) {
            fwrite(buffer, 1, size, wavFile_);
            result = (*bufferQueueItf_)->Enqueue(bufferQueueItf_, buffer, size);
            EXPECT_TRUE(result == SL_RESULT_SUCCESS);
        }
    }
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_GetState_001, TestSize.Level0)
{
    SLOHBufferQueueState state;
    SLresult result = (*bufferQueueItf_)->GetState(bufferQueueItf_, &state);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_SetRecordState_002, TestSize.Level0)
{
    SLresult result = (*captureItf_)->SetRecordState(captureItf_, SL_RECORDSTATE_PAUSED);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_SetRecordState_003, TestSize.Level0)
{
    SLresult result = (*captureItf_)->SetRecordState(captureItf_, SL_RECORDSTATE_STOPPED);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_GetRecordState_001, TestSize.Level0)
{
    SLuint32 state;
    SLresult result = (*captureItf_)->GetRecordState(captureItf_, &state);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_clear_001, TestSize.Level0)
{
    SLresult result = (*bufferQueueItf_)->Clear(bufferQueueItf_);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_Destroy_001, TestSize.Level0)
{
    (*pcmCapturerObject_)->Destroy(pcmCapturerObject_);
    EXPECT_TRUE(true);
}

HWTEST(AudioOpenslesCaptureUnitTest, Audio_Opensles_Capture_Destroy_002, TestSize.Level0)
{
    (*engineObject_)->Destroy(engineObject_);
    EXPECT_TRUE(true);
}

HWTEST(AudioOpenslesCaptureUnitTest, Prf_Audio_Opensles_Capture_CreateEngine_001, TestSize.Level0)
{
    struct timespec tv1 = {0};
    struct timespec tv2 = {0};
    int64_t performanceTestTimes = 10;
    int64_t usecTimes = 1000000000;
    int64_t totalTime = 0;
    for (int32_t i = 0; i < performanceTestTimes; i++) {
        clock_gettime(CLOCK_REALTIME, &tv1);
        slCreateEngine(&engineObject_, 0, nullptr, 0, nullptr, nullptr);
        clock_gettime(CLOCK_REALTIME, &tv2);
        totalTime += tv2.tv_sec * usecTimes + tv2.tv_nsec - (tv1.tv_sec * usecTimes + tv1.tv_nsec);
    }
    int64_t expectTime = 1000000000;
    EXPECT_TRUE(totalTime <= expectTime * performanceTestTimes);
}

HWTEST(AudioOpenslesCaptureUnitTest, Prf_Audio_Opensles_Capture_DestoryEngine_001, TestSize.Level0)
{
    struct timespec tv1 = {0};
    struct timespec tv2 = {0};
    int64_t performanceTestTimes = 10;
    int64_t usecTimes = 1000000000;
    int64_t totalTime = 0;
    for (int32_t i = 0; i < performanceTestTimes; i++) {
        engineObject_ = {};
        slCreateEngine(&engineObject_, 0, nullptr, 0, nullptr, nullptr);
        clock_gettime(CLOCK_REALTIME, &tv1);
        (*engineObject_)->Destroy(engineObject_);
        clock_gettime(CLOCK_REALTIME, &tv2);
        totalTime += tv2.tv_sec * usecTimes + tv2.tv_nsec - (tv1.tv_sec * usecTimes + tv1.tv_nsec);
    }
    int64_t expectTime = 1000000000;
    EXPECT_TRUE(totalTime <= expectTime * performanceTestTimes);
}

HWTEST(AudioOpenslesCaptureUnitTest, Prf_Audio_Opensles_Capture_Realize_001, TestSize.Level0)
{
    struct timespec tv1 = {0};
    struct timespec tv2 = {0};
    int64_t performanceTestTimes = 10;
    int64_t usecTimes = 1000000000;
    int64_t totalTime = 0;
    engineObject_ = {};
    slCreateEngine(&engineObject_, 0, nullptr, 0, nullptr, nullptr);
    for (int32_t i = 0; i < performanceTestTimes; i++) {
        clock_gettime(CLOCK_REALTIME, &tv1);
        (*engineObject_)->Realize(engineObject_, SL_BOOLEAN_FALSE);
        clock_gettime(CLOCK_REALTIME, &tv2);
        totalTime += tv2.tv_sec * usecTimes + tv2.tv_nsec - (tv1.tv_sec * usecTimes + tv1.tv_nsec);
    }
    int64_t expectTime = 1000000000;
    EXPECT_TRUE(totalTime <= expectTime * performanceTestTimes);
}

HWTEST(AudioOpenslesCaptureUnitTest, Prf_Audio_Opensles_Capture_GetInterface_001, TestSize.Level0)
{
    struct timespec tv1 = {0};
    struct timespec tv2 = {0};
    int64_t performanceTestTimes = 10;
    int64_t usecTimes = 1000000000;
    int64_t totalTime = 0;
    for (int32_t i = 0; i < performanceTestTimes; i++) {
        clock_gettime(CLOCK_REALTIME, &tv1);
        (*engineObject_)->GetInterface(engineObject_, SL_IID_ENGINE, &engineEngine_);
        clock_gettime(CLOCK_REALTIME, &tv2);
        totalTime += tv2.tv_sec * usecTimes + tv2.tv_nsec - (tv1.tv_sec * usecTimes + tv1.tv_nsec);
    }
    int64_t expectTime = 1000000000;
    EXPECT_TRUE(totalTime <= expectTime * performanceTestTimes);
}

HWTEST(AudioOpenslesCaptureUnitTest, Prf_Audio_Opensles_CreateAudioRecorder_001, TestSize.Level0)
{
    wavFile_ = fopen(AUDIORENDER_TEST_FILE_PATH, "wb");
    if (wavFile_ == nullptr) {
        AUDIO_INFO_LOG("AudioCaptureTest: Unable to open record file.");
    }

    SLDataLocator_IODevice io_device = {
        SL_DATALOCATOR_IODEVICE,
        SL_IODEVICE_AUDIOINPUT,
        SL_DEFAULTDEVICEID_AUDIOINPUT,
        NULL
    };

    SLDataSource audioSource = {
        &io_device,
        NULL
    };

    SLDataLocator_BufferQueue buffer_queue = {
        SL_DATALOCATOR_BUFFERQUEUE,
        3
    };

    SLDataFormat_PCM format_pcm = {
        SL_DATAFORMAT_PCM,
        OHOS::AudioStandard::AudioChannel::MONO,
        OHOS::AudioStandard::AudioSamplingRate::SAMPLE_RATE_44100,
        OHOS::AudioStandard::AudioSampleFormat::SAMPLE_S16LE,
        0,
        0,
        0
    };

    SLDataSink audioSink = {
        &buffer_queue,
        &format_pcm
    };

    struct timespec tv1 = {0};
    struct timespec tv2 = {0};
    int64_t performanceTestTimes = 10;
    int64_t usecTimes = 1000000000;
    int64_t totalTime = 0;
    for (int32_t i = 0; i < performanceTestTimes; i++) {
        clock_gettime(CLOCK_REALTIME, &tv1);
        (*engineEngine_)->CreateAudioRecorder(engineEngine_, &pcmCapturerObject_, &audioSource,
                                              &audioSink, 0, nullptr, nullptr);
        clock_gettime(CLOCK_REALTIME, &tv2);
        totalTime += tv2.tv_sec * usecTimes + tv2.tv_nsec - (tv1.tv_sec * usecTimes + tv1.tv_nsec);
    }
    int64_t expectTime = 1000000000;
    EXPECT_TRUE(totalTime <= expectTime * performanceTestTimes);
}
} // namespace AudioStandard
} // namespace OHOS
