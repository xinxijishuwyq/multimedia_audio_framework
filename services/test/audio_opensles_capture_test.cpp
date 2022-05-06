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

#include <iostream>
#include <unistd.h>
#include <OpenSLES.h>
#include <OpenSLES_OpenHarmony.h>
#include <OpenSLES_Platform.h>
#include "audio_info.h"
#include "audio_log.h"

using namespace std;

static void BuqqerQueueCallback (SLOHBufferQueueItf bufferQueueItf, void *pContext, SLuint32 size);

static void CaptureStart(SLRecordItf recordItf, SLOHBufferQueueItf bufferQueueItf, FILE *wavFile);

static void CaptureStop(SLRecordItf recordItf, SLOHBufferQueueItf bufferQueueItf);

static void OpenSLCaptureTest();

const int PARAMETERS = 2;
FILE *wavFile_ = nullptr;
SLObjectItf engineObject = nullptr;
SLRecordItf  recordItf;
SLOHBufferQueueItf bufferQueueItf;
SLObjectItf pcmCapturerObject = nullptr;

int main(int argc, char *argv[])
{
    AUDIO_INFO_LOG("opensl es capture test in");
    if (argc != PARAMETERS) {
        AUDIO_ERR_LOG("Incorrect number of parameters.");
        return -1;
    }
    
    string filePath = argv[1];
    wavFile_ = fopen(filePath.c_str(), "wb");
    if (wavFile_ == nullptr) {
        AUDIO_INFO_LOG("OpenSLES record: Unable to open file");
        return -1;
    }

    OpenSLCaptureTest();
    while (wavFile_ != nullptr) {
        sleep(1);
    }
    fflush(wavFile_);
    CaptureStop(recordItf, bufferQueueItf);
    (*pcmCapturerObject)->Destroy(pcmCapturerObject);
    fclose(wavFile_);
    wavFile_ = nullptr;
}

static void OpenSLCaptureTest()
{
    AUDIO_ERR_LOG("OpenSLCaptureTest");
    engineObject = nullptr;
    SLEngineItf engineItf = nullptr;

    SLresult result = slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineItf);

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

    result = (*engineItf)->CreateAudioRecorder(engineItf, &pcmCapturerObject,
        &audioSource, &audioSink, 0, nullptr, nullptr);
    (*pcmCapturerObject)->Realize(pcmCapturerObject, SL_BOOLEAN_FALSE);
    
    (*pcmCapturerObject)->GetInterface(pcmCapturerObject, SL_IID_RECORD, &recordItf);
    (*pcmCapturerObject)->GetInterface(pcmCapturerObject, SL_IID_OH_BUFFERQUEUE, &bufferQueueItf);
    (*bufferQueueItf)->RegisterCallback(bufferQueueItf, BuqqerQueueCallback, wavFile_);
    
    CaptureStart(recordItf, bufferQueueItf, wavFile_);

    return;
}

static void BuqqerQueueCallback(SLOHBufferQueueItf bufferQueueItf, void *pContext, SLuint32 size)
{
    AUDIO_INFO_LOG("BuqqerQueueCallback");
    FILE *wavFile = (FILE *)pContext;
    if (wavFile != nullptr) {
        SLuint8 *buffer = nullptr;
        SLuint32 pSize = 0;
        (*bufferQueueItf)->GetBuffer(bufferQueueItf, &buffer, pSize);
        if (buffer != nullptr) {
            AUDIO_INFO_LOG("BuqqerQueueCallback, length, pSize:%{public}lu, size: %{public}lu.",
                           pSize, size);
            fwrite(buffer, 1, pSize, wavFile);
            (*bufferQueueItf)->Enqueue(bufferQueueItf, buffer, size);
        } else {
            AUDIO_INFO_LOG("BuqqerQueueCallback, buffer is null or pSize: %{public}lu, size: %{public}lu.",
                           pSize, size);
        }
    }

    return;
}

static void CaptureStart(SLRecordItf recordItf, SLOHBufferQueueItf bufferQueueItf, FILE *wavFile)
{
    AUDIO_INFO_LOG("CaptureStart");
    (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_RECORDING);
    if (wavFile != nullptr) {
        SLuint8* buffer = nullptr;
        SLuint32 pSize = 0;
        (*bufferQueueItf)->GetBuffer(bufferQueueItf, &buffer, pSize);
        if (buffer != nullptr) {
            AUDIO_INFO_LOG("CaptureStart, enqueue buffer length: %{public}lu.", pSize);
            fwrite(buffer, 1, pSize, wavFile);
            (*bufferQueueItf)->Enqueue(bufferQueueItf, buffer, pSize);
        } else {
            AUDIO_INFO_LOG("BuqqerQueueCallback, buffer is null or pSize: %{public}lu.", pSize);
        }
    }

    return;
}

static void CaptureStop(SLRecordItf recordItf, SLOHBufferQueueItf bufferQueueItf)
{
    AUDIO_INFO_LOG("CaptureStop");
    (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_STOPPED);
    return;
}
