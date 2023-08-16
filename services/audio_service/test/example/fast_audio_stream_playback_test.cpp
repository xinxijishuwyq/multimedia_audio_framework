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
#include <cstdio>
#include <iostream>

#include "audio_capturer.h"
#include "audio_errors.h"
#include "audio_info.h"
#include "audio_log.h"
#include "audio_renderer.h"
#include "audio_utils.h"
#include "parameter.h"
#include "pcm2wav.h"

namespace OHOS {
namespace AudioStandard {
std::map<int32_t, std::string> g_OptStrMap = {
    {INIT_LOCAL_SPK, "call local spk init process"},
    {INIT_REMOTE_SPK, "call remote spk init process"},
    {START_SPK, "call start spk process"},
    {STOP_SPK, "call stop spk process"},
    {RELEASE_SPK, "release spk process"},

    {INIT_LOCAL_MIC, "call local mic init process"},
    {INIT_REMOTE_MIC, "call remote mic init process"},
    {START_MIC, "call start mic process"},
    {STOP_MIC, "call stop mic process"},
    {RELEASE_MIC, "release mic process"},

    {EXIT_DEMO, "exit interactive run test"},
};

enum AudioOptCode : int32_t {
    INIT_LOCAL_SPK = 0,
    INIT_REMOTE_SPK = 1,
    START_SPK = 2,
    STOP_SPK = 3,
    RELEASE_SPK = 4,
    INIT_LOCAL_MIC = 5,
    INIT_REMOTE_MIC = 6,
    START_MIC = 7,
    STOP_MIC = 8,
    RELEASE_MIC = 9,
    EXIT_DEMO = 10,
};

class PlaybackTest : public AudioRendererWriteCallback,
    public AudioCapturerReadCallback,
    public std::enable_shared_from_this<PlaybackTest> {
public:
    int32_t InitRenderer();
    int32_t StartPlay();
    int32_t StopPlay();
    int32_t InitCapturer();
    int32_t StartCapture();
    int32_t StopCapture();
    void OnWriteData(size_t length) override;
    void OnReadData(size_t length) override;
    bool OpenSpkFile(const std::string &spkFilePath);
    bool OpenMicFile(const std::string &micFilePath);
    void CloseSpkFile();
    void CloseMicFile();

private:
    std::unique_ptr<AudioStandard::AudioRenderer> audioRenderer_ = nullptr;
    std::unique_ptr<AudioStandard::AudioCapturer> audioCapturer_ = nullptr;
    static constexpr long WAV_HEADER_SIZE = 44;
    bool needSkipWavHeader_ = true;
    FILE *spkWavFile_ = nullptr;
    FILE *micWavFile_ = nullptr;
};

void PlaybackTest::OnWriteData(size_t length)
{
    BufferDesc bufDesc;
    if (audioRenderer_ == nullptr) {
        AUDIO_ERR_LOG("audioRenderer is nullptr.");
        return;
    }
    audioRenderer_->GetBufferDesc(bufDesc);

    if (spkWavFile_ == nullptr) {
        AUDIO_ERR_LOG("spkWavFile_ is nullptr.");
        return;
    }
    if (needSkipWavHeader_) {
        fseek(spkWavFile_, WAV_HEADER_SIZE, SEEK_SET);
        needSkipWavHeader_ = false;
    }
    if (feof(spkWavFile_)) {
        fseek(spkWavFile_, WAV_HEADER_SIZE, SEEK_SET); // infinite loop
    }
    if (renderFinish_) {
        AUDIO_INFO_LOG("%{public}s render finish.", __func__);
        return;
    }
    fread(bufDesc.buffer, 1, bufDesc.bufLength, spkWavFile_);
    audioRenderer_->Enqueue(bufDesc);
}

bool PlaybackTest::OpenSpkFile(const std::string &spkFilePath)
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

void PlaybackTest::CloseSpkFile()
{
    if (spkWavFile_ != nullptr) {
        fclose(spkWavFile_);
        spkWavFile_ = nullptr;
    }
}

int32_t PlaybackTest::InitRenderer()
{
    AudioStandard::AudioRendererOptions rendererOptions = {
        {
            AudioStandard::AudioSamplingRate::SAMPLE_RATE_48000,
            AudioStandard::AudioEncodingType::ENCODING_PCM,
            AudioStandard::AudioSampleFormat::SAMPLE_S16LE,
            AudioStandard::AudioChannel::STEREO,
        },
        {
            AudioStandard::ContentType::CONTENT_TYPE_MUSIC,
            AudioStandard::StreamUsage::STREAM_USAGE_MEDIA,
            AudioStandard::STREAM_FLAG_FAST, // fast audio stream
        }
    };
    audioRenderer_ = AudioStandard::AudioRenderer::Create(rendererOptions);
    if (audioRenderer_ == nullptr) {
        AUDIO_ERR_LOG("Audio renderer create failed.");
        return -1;
    }
    std::string path = "/data/test.wav";
    OpenSpkFile(path);
    int32_t ret = audioRenderer_->SetRendererWriteCallback(shared_from_this());
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Client test save data callback fail, ret %{public}d.", ret);
    AUDIO_INFO_LOG("Audio renderer create success.");
    return 0;
}

int32_t PlaybackTest::StartPlay()
{
    if (audioRenderer_ == nullptr) {
        AUDIO_ERR_LOG("Audiorenderer init failed.");
        return -1;
    }
    if (!audioRenderer_->Start()) {
        AUDIO_ERR_LOG("Audio renderer start failed.");
        return -1;
    }
    return 0;
}

int32_t PlaybackTest::StopPlay()
{
    if (audioRenderer_ == nullptr) {
        AUDIO_ERR_LOG("Audiorenderer init failed.");
        return -1;
    }
    if (!audioRenderer_->Stop()) {
        AUDIO_ERR_LOG("Audio renderer stop failed.");
        return -1;
    }
    return 0;
}

void PlaybackTest::OnReadData(size_t length)
{
    BufferDesc bufDesc;
    if (audioCapturer_ == nullptr) {
        AUDIO_ERR_LOG("audioCapturer is nullptr.");
        return;
    }
    int32_t ret = audioCapturer_->GetBufferDesc(bufDesc);
    if (ret != 0 || bufDesc.buffer == nullptr || bufDesc.bufLength == 0) {
        AUDIO_ERR_LOG("Get buffer desc failed. On read data.");
        return;
    }
    if (micWavFile_ == nullptr) {
        AUDIO_ERR_LOG("micWavFile_ is nullptr.");
        return;
    }
    size_t cnt = fwrite(bufDesc.buffer, 1, bufDesc.bufLength, micWavFile_);
    if (cnt != bufDesc.bufLength) {
        AUDIO_ERR_LOG("fwrite fail, cnt %{public}zu, bufLength %{public}zu.", cnt, bufDesc.bufLength);
        return;
    }
    audioCapturer_->Enqueue(bufDesc);
}

bool PlaybackTest::OpenMicFile(const std::string &micFilePath)
{
    if (micWavFile_ != nullptr) {
        AUDIO_ERR_LOG("Mic file has been opened, micFilePath %{public}s.", micFilePath.c_str());
        return true;
    }

    char path[PATH_MAX] = { 0x00 };
    if ((strlen(micFilePath.c_str()) > PATH_MAX) || (realpath(micFilePath.c_str(), path) == nullptr)) {
        AUDIO_ERR_LOG("micFilePath is not valid.");
        return false;
    }
    AUDIO_INFO_LOG("mic path = %{public}s.", path);
    micWavFile_ = fopen(path, "ab+");
    if (micWavFile_ == nullptr) {
        AUDIO_ERR_LOG("Unable to open wave file");
        return false;
    }
    return true;
}

void PlaybackTest::CloseMicFile()
{
    if (micWavFile_ != nullptr) {
        fclose(micWavFile_);
        micWavFile_ = nullptr;
    }
}

int32_t PlaybackTest::InitCapturer()
{
    AudioStandard::AudioCapturerOptions capturerOptions = {
        {
            AudioStandard::AudioSamplingRate::SAMPLE_RATE_48000,
            AudioStandard::AudioEncodingType::ENCODING_PCM,
            AudioStandard::AudioSampleFormat::SAMPLE_S16LE,
            AudioStandard::AudioChannel::STEREO,
        },
        {
            AudioStandard::SourceType::SOURCE_TYPE_MIC,
            AudioStandard::STREAM_FLAG_FAST,
        }
    };
    audioCapturer_ = AudioStandard::AudioCapturer::Create(capturerOptions);
    if (audioCapturer_ == nullptr) {
        AUDIO_ERR_LOG("Audio capturer create failed.");
        return -1;
    }
    std::string path = "/data/mic.pcm";
    OpenMicFile(path);
    int32_t ret = audioCapturer_->SetCapturerReadCallback(shared_from_this());
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Client test save data callback fail, ret %{public}d.", ret);
    AUDIO_INFO_LOG("Audio capturer create success.");
    return 0;
}

int32_t PlaybackTest::StartCapture()
{
    if (audioCapturer_ == nullptr) {
        AUDIO_ERR_LOG("audioCapturer init failed.");
        return -1;
    }
    if (!audioCapturer_->Start()) {
        AUDIO_ERR_LOG("Audio capture start failed.");
        return -1;
    }
    return 0;
}

int32_t PlaybackTest::StopCapture()
{
    if (audioCapturer_ == nullptr) {
        AUDIO_ERR_LOG("audioCapturer init failed.");
        return -1;
    }
    if (!audioCapturer_->Stop()) {
        AUDIO_ERR_LOG("Audio capture stop failed.");
        return -1;
    }
    return 0;
}

bool SetSysPara(const std::string &key, int32_t &value)
{
    auto res = SetParameter(key.c_str(), std::to_string(value).c_str());
    if (res < 0) {
        AUDIO_WARNING_LOG("SetSysPara fail, key:%{public}s res:%{public}d", key.c_str(), res);
        return false;
    }
    AUDIO_INFO_LOG("SetSysPara success.");
    return true;
}

int32_t GetUserInput()
{
    int32_t res = -1; // result
    size_t count = 3; // try three time
    cout << ">>";
    cin >> res;
    while (cin.fail() && count-- > 0) {
        cin.clear();
        cin.ignore();
        cout << "invalid input, not a number! Please retry with a number." << endl;
        cout << ">>";
        cin >> res;
    }
    return res;
}

void PrintUsage()
{
    cout << "[Fast Audio Stream Test App]" << endl << endl;
    cout << "Supported Functionalities:" << endl;
    cout << "================================Usage=======================================" << endl << endl;
    cout << "  0: Init local spk." << endl;
    cout << "  1: Init remote spk." << endl;
    cout << "  2: Start play." << endl;
    cout << "  3: Stop play." << endl;
    cout << "  4: Release spk." << endl;
    cout << "  5: Init local mic." << endl;
    cout << "  6: Init remote mic." << endl;
    cout << "  7: Start record." << endl;
    cout << "  8: Stop record." << endl;
    cout << "  9: Release mic." << endl;
    cout << "  10: exit demo." << endl;
    cout << " Please input your choice: " << endl;
}

int32_t InitPlayback(std:share_ptr<PlaybackTest> playTest, bool isRemote) {
    if (playTest == nullptr) {
        cout << "Play test is nullptr, init spk error." << endl;
        return -1;
    }
    int32_t ret = playTest->InitRenderer();
    if (ret != 0) {
        cout << "Init renderer error!" << endl;
        return -1;
    }
    return 0;
}

int32_t StartPlay(std:share_ptr<PlaybackTest> playTest) {
    if (playTest == nullptr) {
        cout << "Play test is nullptr, start play error." << endl;
        return -1;
    }
    int32_t ret = playTest->StartPlay();
    if (ret != 0) {
        cout << "Start play error!" << endl;
        return -1;
    }
    return 0;
}

int32_t StopPlay(std:share_ptr<PlaybackTest> playTest) {
    if (playTest == nullptr) {
        cout << "Play test is nullptr, stop play error." << endl;
        return -1;
    }
    int32_t ret = playTest->StopPlay();
    if (ret != 0) {
        cout << "Stop play error!" << endl;
        return -1;
    }
    return 0;
}

int32_t InitMic(std:share_ptr<PlaybackTest> playTest, bool isRemote) {
    if (playTest == nullptr) {
        cout << "Play test is nullptr, init mic error." << endl;
        return -1;
    }
    int32_t ret = playTest->InitCapturer();
    if (ret != 0) {
        cout << "Init capturer error!" << endl;
        return -1;
    }
    return 0;
}

int32_t StartCpature(std:share_ptr<PlaybackTest> playTest) {
    if (playTest == nullptr) {
        cout << "Play test is nullptr, start capture error." << endl;
        return -1;
    }
    int32_t ret = playTest->StartCapture();
    if (ret != 0) {
        cout << "Start capture error!" << endl;
        return -1;
    }
    return 0;
}

int32_t StopCapture(std:share_ptr<PlaybackTest> playTest) {
    if (playTest == nullptr) {
        cout << "Play test is nullptr, stop capture error." << endl;
        return -1;
    }
    int32_t ret = playTest->StopCapture();
    if (ret != 0) {
        cout << "Stop capture error!" << endl;
        return -1;
    }
    return 0;
}

void Loop(std::shared_ptr<PlaybackTest> playTest)
{
    bool isProcTestRun = true;
    while (isProcTestRun) {
        PrintUsage();
        AudioOptCode optCode = -1;
        int32_t res = GetUserInput();
        if (g_optStrMap.count(res)) {
            optCode = static_cast<AudioOptCode>(res);
        }
        switch (optCode) {
            case INIT_LOCAL_SPK:
                InitPlayback(playTest, false);
                break;
            case INIT_REMOTE_SPK:
                InitPlayback(playTest, true);
                break;
            case START_SPK:
                StartPlay(playTest);
                break;
            case STOP_SPK:
                StopPlay(playTest);
                break;
            case RELEASE_SPK:
                break;
            case INIT_LOCAL_MIC:
                InitMic(playTest, false);
                break;
            case INIT_REMOTE_MIC:
                InitMic(playTest, true);
                break;
            case START_MIC:
                StartCapture(playTest);
                break;
            case STOP_MIC:
                StopCapture(playTest);
                break;
            case RELEASE_MIC:
                break;
            case EXIT_DEMO:
                isProcTestRun = false;
                break;
            default:
                cout << "invalid input: " << res << endl;
                break;
        }
    }
}
}
}

using namespace OHOS::AudioStandard;
using namespace std;
int main(int argc, char* argv[])
{
    cout << "oh fast audio stream test." << endl;
    std::shared_ptr<PlaybackTest> playTest = std::make_shared<PlaybackTest>();
    int32_t val = 1;
    std::string key = "persist.multimedia.audio.mmap.enable";
    SetSysPara(key, val);

    Loop(playTest);
    playTest->CloseSpkFile();
    playTest->CloseMicFile();
    return 0;
}