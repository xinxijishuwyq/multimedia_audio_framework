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

#include "audio_opensles_unit_test.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {
namespace {
    const char * AUDIORENDER_TEST_FILE_PATH = "/data/test_44100_2.wav";
    FILE *wavFile_;
    wav_hdr wavHeader_;
    SLObjectItf engineObject_;
    SLObjectItf outputMixObject_;
    SLPlayItf playItf_;
    SLVolumeItf volumeItf_;
    SLOHBufferQueueItf bufferQueueItf_;
    SLObjectItf pcmPlayerObject_;
    SLEngineItf engineEngine_;
} // namespace

void AudioOpenslesUnitTest::SetUpTestCase(void)
{

}

void AudioOpenslesUnitTest::TearDownTestCase(void)
{

}

void AudioOpenslesUnitTest::SetUp(void)
{
    wavFile_ = fopen(AUDIORENDER_TEST_FILE_PATH, "rb");
    if (wavFile_ == nullptr) {
        MEDIA_INFO_LOG("AudioRendererTest: Unable to open wave file");
    }
    size_t headerSize = sizeof(wav_hdr);
    fread(&wavHeader_, 1, headerSize, wavFile_);
}

void AudioOpenslesUnitTest::TearDown(void)
{

}

HWTEST(AudioOpenslesUnitTest, Audio_Opensles_CreateEngine_001, TestSize.Level0)
{
    SLresult result = slCreateEngine(&engineObject_, 0, nullptr, 0, nullptr, nullptr);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesUnitTest, Audio_Opensles_CreateEngine_002, TestSize.Level0)
{
    SLresult result = (*engineObject_)->Realize(engineObject_, SL_BOOLEAN_FALSE);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesUnitTest, Audio_Opensles_CreateEngine_003, TestSize.Level0)
{
    SLresult result = (*engineObject_)->GetInterface(engineObject_, SL_IID_ENGINE, &engineEngine_);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesUnitTest, Audio_Opensles_CreateOutputMix_001, TestSize.Level0)
{
    SLresult result = (*engineEngine_)->CreateOutputMix(engineEngine_, &outputMixObject_, 0, nullptr, nullptr);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesUnitTest, Audio_Opensles_CreateOutputMix_002, TestSize.Level0)
{
    SLresult result = (*outputMixObject_)->Realize(outputMixObject_, SL_BOOLEAN_FALSE);;
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesUnitTest, Audio_Opensles_CreateAudioPlayer_001, TestSize.Level0)
{
    SLDataLocator_OutputMix slOutputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject_};
    SLDataSink slSink = {&slOutputMix, nullptr};
    SLDataLocator_BufferQueue slBufferQueue = {
        SL_DATALOCATOR_BUFFERQUEUE,
        0
    };
    SLDataFormat_PCM pcmFormat = {
        SL_DATAFORMAT_PCM,
        wavHeader_.NumOfChan,
        wavHeader_.SamplesPerSec * 1000,
        wavHeader_.bitsPerSample,
        0,
        0,
        0
    };
    SLDataSource slSource = {&slBufferQueue, &pcmFormat};
    SLresult result = (*engineEngine_)->CreateAudioPlayer(engineEngine_, &pcmPlayerObject_, &slSource, &slSink, 0, nullptr, nullptr);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesUnitTest, Audio_Opensles_CreateAudioPlayer_002, TestSize.Level0)
{
    SLresult result = (*pcmPlayerObject_)->Realize(pcmPlayerObject_, SL_BOOLEAN_FALSE);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesUnitTest, Audio_Opensles_CreateAudioPlayer_003, TestSize.Level0)
{
    SLresult result = (*pcmPlayerObject_)->GetInterface(pcmPlayerObject_, SL_IID_PLAY, &playItf_);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesUnitTest, Audio_Opensles_GetVoumeItf_001, TestSize.Level0)
{
    SLresult result = (*pcmPlayerObject_)->GetInterface(pcmPlayerObject_, SL_IID_VOLUME, &volumeItf_);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

HWTEST(AudioOpenslesUnitTest, Audio_Opensles_CetBufferQueue_001, TestSize.Level0)
{
    SLresult result = (*pcmPlayerObject_)->GetInterface(pcmPlayerObject_, SL_IID_OH_BUFFERQUEUE, &bufferQueueItf_);
    EXPECT_TRUE(result == SL_RESULT_SUCCESS);
}

} // namespace AudioStandard
} // namespace OHOS
