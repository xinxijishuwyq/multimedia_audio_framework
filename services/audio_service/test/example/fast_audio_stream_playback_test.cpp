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
#include <cstdio>
#include <iostream>

#include "audio_renderer.h"
#include "audio_log.h"
#include "audio_utils.h"
#include "audio_errors.h"
#include "parameter.h"
#include "pcm2wav.h"

namespace OHOS {
namespace AudioStandard {
class PlaybackTest : public AudioRendererWriteCallback,
    public std::enable_shared_from_this<PlaybackTest> {
public:
    int32_t InitRenderer();
    int32_t StartPlay();
    int32_t StopPlay();
    void OnWriteData(size_t length) override;
    bool OpenSpkFile(const std::string spkFilePath);

private:
    std::unique_ptr<AudioStandard::AudioRenderer> audioRenderer_ = nullptr;
    static constexpr long WAV_HEADER_SIZE = 42;
    int32_t loopCount_ = 2; // play 2 times
    bool needSkipWavHeader_ = true;
    bool renderFinish_ = false;
    FILE *spkWavFile_ = nullptr;
};

void PlaybackTest::OnWriteData(size_t length)
{
    BufferDesc bufDesc;
    if (audioRenderer_ == nullptr) {
        AUDIO_ERR_LOG("audioRenderer is nullptr.");
        return;
    }
    audioRenderer_ -> GetBufferDesc(bufDesc);

    if (spkWavFile_ == nullptr) {
        AUDIO_ERR_LOG("spkWavFile_ is nullptr.");
        return;
    }
    if (needSkipWavHeader_) {
        fseek(spkWavFile_, WAV_HEADER_SIZE, SEEK_SET);
        needSkipWavHeader_ = false;
    }
    if (feof(spkWavFile_)) {
        loopCount_--;
        if (loopCount_ < 0) {
            fseek(spkWavFile_, WAV_HEADER_SIZE, SEEK_SET); // infinite loop
        } else if (loopCount_ == 0) {
            renderFinish_ = true;
            // g_autoRunCV.notify_all();
        } else {
            fseek(spkWavFile_, WAV_HEADER_SIZE, SEEK_SET);
        }
    }
    if (renderFinish_) {
        AUDIO_INFO_LOG("%{public}s render finish.", __func__);
        return;
    }
    fread(bufDesc.buffer, 1, bufDesc.bufLength, spkWavFile_);
    audioRenderer_->Enqueue(bufDesc);
}

bool PlaybackTest::OpenSpkFile(const std::string spkFilePath)
{
    if (spkWavFile_ != nullptr) {
        AUDIO_ERR_LOG("Spk file has been opened, spkFilePath %{public}s", spkFilePath.c_str());
        return true;
    }

    char path[PATH_MAX] = { 0x00 };
    if ((strlen(spkFilePath.c_str()) > PATH_MAX) || (realpath(spkFilePath.c_str(), path) == nullptr)) {
        return false;
    }
    AUDIO_INFO_LOG("spk path = %{public}s", path);
    spkWavFile_ = fopen(path, "rb");
    if (spkWavFile_ == nullptr) {
        AUDIO_ERR_LOG("Unable to open wave file");
        return false;
    }
    return true;
}

int32_t PlaybackTest::InitRenderer()
{
    AudioStandard::AudioRendererOptions rendererOptions = {
        {
            static_cast<AudioStandard::AudioSamplingRate>(48000),
            AudioStandard::AudioEncodingType::ENCODING_PCM,
            static_cast<AudioStandard::AudioSampleFormat>(1),
            static_cast<AudioStandard::AudioChannel>(2),
        },
        {
            static_cast<AudioStandard::ContentType>(2),
            static_cast<AudioStandard::StreamUsage>(1),
            4, // fast audio stream
        }
    };
    audioRenderer_ = AudioStandard::AudioRenderer::Create(rendererOptions);
    if (audioRenderer_ == nullptr) {
        AUDIO_ERR_LOG("wuhaobo Audio renderer create failed.");
        return -1;
    }
    // obTestCb_ = std::make_shared<OHTestCallback>();
    std::string path = "/data/test.wav";
    OpenSpkFile(path);
    int32_t ret = audioRenderer_ -> SetRendererWriteCallback(shared_from_this());
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Client test save data callback fail, ret %{public}d.", ret);
    AUDIO_INFO_LOG("wuhaobo Audio renderer create success.");
    return 0;
}

int32_t PlaybackTest::StartPlay()
{
    if (audioRenderer_ == nullptr) {
        AUDIO_ERR_LOG("wuhaobo Audiorenderer init failed.");
        return -1;
    }
    if (!audioRenderer_->Start()) {
        AUDIO_ERR_LOG("wuhaobo Audio renderer start failed.");
        return -1;
    }
    return 0;
}

int32_t PlaybackTest::StopPlay()
{
    if (audioRenderer_ == nullptr) {
        AUDIO_ERR_LOG("wuhaobo Audiorenderer init failed.");
        return -1;
    }
    if (!audioRenderer_->Stop()) {
        AUDIO_ERR_LOG("wuhaobo Audio renderer stop failed.");
        return -1;
    }
    return 0;
}

bool SetSysPara(std::string key, int32_t &value)
{
    auto res = SetParameter(key.c_str(), std::to_string(value).c_str());
    if (res < 0) {
        AUDIO_WARNING_LOG("SetSysPara fail, key:%{public}s res:%{public}d", key.c_str(), res);
        return false;
    }
    AUDIO_INFO_LOG("SetSysPara success.");
    return true;
}
}
}

using namespace OHOS::AudioStandard;
using namespace std;
int main(int argc, char* argv[])
{
    cout << "oh audio stream test." << endl;
    std::shared_ptr<PlaybackTest> playTest = std::make_shared<PlaybackTest>();
    int32_t val = 1;
    SetSysPara("persist.multimedia.audio.mmap.enable", val);

    int32_t ret = playTest->InitRenderer();
    if (ret != 0) {
        cout << "Init Renderer error !" << endl;
        return -1;
    }
    cout << "Init Renderer success !" << endl;
    ret = playTest->StartPlay();
    if (ret != 0) {
        cout << "Start Play error !" << endl;
        return -1;
    }
    cout << "Playing ..." << endl;
    usleep(2000000000);
    return 0;
}
