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

#include <cinttypes>
#include <thread>
#include <stdint.h>
#include <time.h>

#include "audio_log.h"
#include "audio_utils.h"
#include "fast/fast_audio_renderer_sink.h"
#include "pcm2wav.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::AudioStandard;

class AudioHdiDeviceTest {
public:
    void RenderFrameFromFile()
    {
        if (hdiRenderSink_ == nullptr) {
            AUDIO_ERR_LOG("RenderFrameFromFile hdiRenderSink_ null");
            return;
        }

        size_t tempBufferSize = hdiRenderSink_->eachReadFrameSize_ * hdiRenderSink_->frameSizeInByte_;
        char *buffer = (char *)malloc(tempBufferSize);
        if (buffer == nullptr) {
            AUDIO_ERR_LOG("AudioHdiDeviceTest: failed to malloc");
            return;
        }

        uint64_t frameCount = 0;
        int64_t timeSec = 0;
        int64_t timeNanoSec = 0;

        int64_t periodMicrTime = 5000; // 5ms
        int64_t timeStampNext = GetNowTimeUs();
        int64_t timeToSleep = periodMicrTime;

        uint64_t written = 0;
        int32_t ret = 0;
        while (!stopThread && !feof(wavFile)) {
            fread(buffer, 1, tempBufferSize, wavFile);
            ret = hdiRenderSink_->RenderFrame(*buffer, tempBufferSize, written);

            hdiRenderSink_->GetMmapHandlePosition(frameCount, timeSec, timeNanoSec);

            timeToSleep = periodMicrTime - (GetNowTimeUs() - timeStampNext);
            if (timeToSleep > 0 && timeToSleep < periodMicrTime) {
                usleep(timeToSleep);
            } else {
                usleep(periodMicrTime);
            }

            timeStampNext = GetNowTimeUs();
        }
        free(buffer);
    }

    bool InitHdiRender()
    {
        hdiRenderSink_ = FastAudioRendererSink::GetInstance();
        AudioSinkAttr attr = {};
        attr.adapterName = "primary";
        attr.sampleRate = 48000; // 48000hz
        attr.channel = 2;
        attr.format = AUDIO_FORMAT_PCM_16_BIT; // 0x2u

        hdiRenderSink_->Init(attr);

        return true;
    }

    void StartHdiRender()
    {
        if (hdiRenderSink_ == nullptr) {
            AUDIO_ERR_LOG("StartHdiRender hdiRenderSink_ null");
            return;
        }

        hdiRenderSink_->Start();
        int32_t ret = hdiRenderSink_->SetVolume(0.5, 0.5);
        AUDIO_INFO_LOG("AudioHdiDeviceTest set volume to 0.5, ret %{public}d", ret);

        timeThread_ = make_unique<thread>(&AudioHdiDeviceTest::RenderFrameFromFile, this);

        int32_t sleepTime = 7;

        sleep(sleepTime);

        stopThread = true;
        timeThread_->join();
        hdiRenderSink_->Stop();
        hdiRenderSink_->DeInit();
    }

    bool TestPlayback(int argc, char *argv[])
    {
        AUDIO_INFO_LOG("TestPlayback in");

        wav_hdr wavHeader;
        size_t headerSize = sizeof(wav_hdr);
        char *inputPath = argv[1];
        char path[PATH_MAX + 1] = {0x00};
        if ((strlen(inputPath) > PATH_MAX) || (realpath(inputPath, path) == nullptr)) {
            AUDIO_ERR_LOG("Invalid path");
            return false;
        }
        AUDIO_INFO_LOG("AudioHdiDeviceTest: path = %{public}s", path);
        wavFile = fopen(path, "rb");
        if (wavFile == nullptr) {
            AUDIO_INFO_LOG("AudioHdiDeviceTest: Unable to open wave file");
            return false;
        }
        size_t bytesRead = fread(&wavHeader, 1, headerSize, wavFile);
        AUDIO_INFO_LOG("AudioHdiDeviceTest: Header Read in bytes %{public}zu", bytesRead);

        InitHdiRender();
        StartHdiRender();

        return true;
    }
private:
    FastAudioRendererSink *hdiRenderSink_ = nullptr;
    unique_ptr<thread> timeThread_ = nullptr;
    bool stopThread = false;
    FILE* wavFile = nullptr;
};

int main(int argc, char *argv[])
{
    AUDIO_INFO_LOG("AudioHdiDeviceTest: Render test in");

    if (argv == nullptr) {
        AUDIO_ERR_LOG("AudioHdiDeviceTest: argv is null");
        return 0;
    }
    int32_t argsCountTwo_ = 2;
    int32_t argsCountThree_ = 3;
    if (argc < argsCountTwo_ || argc >= argsCountThree_) {
        AUDIO_ERR_LOG("AudioHdiDeviceTest: incorrect argc. Enter 2 args");
        return 0;
    }

    AUDIO_INFO_LOG("AudioHdiDeviceTest: argc=%{public}d", argc);
    AUDIO_INFO_LOG("file path argv[1]=%{public}s", argv[1]);

    AudioHdiDeviceTest testObj;
    bool ret = testObj.TestPlayback(argc, argv);

    return ret;
}
