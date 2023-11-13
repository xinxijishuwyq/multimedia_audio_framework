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
#include <thread>
#include <iostream>
#include "securec.h"

#include "renderer_in_server.h"
#include "capturer_in_server.h"
#include "audio_errors.h"
#include "audio_info.h"
#include "audio_log.h"
#include "audio_utils.h"
#include "parameter.h"
#include "pcm2wav.h"

using namespace std;
namespace OHOS {
namespace AudioStandard {
enum AudioOptCode : int32_t {
    INVALID_CODE = -1,
    INIT_RENDERER = 0,
    START_PLAY = 1,
    PAUSE_PLAY = 2,
    FLUSH_PLAY = 3,
    DRAIN_PLAY = 4,
    STOP_PLAY = 5,

    RELEASE_SPK = 4,
    INIT_LOCAL_MIC = 5,
    INIT_REMOTE_MIC = 6,
    START_MIC = 7,
    STOP_MIC = 8,
    RELEASE_MIC = 9,
    EXIT_DEMO = 10,
};

std::map<int32_t, std::string> g_OptStrMap = {
    {INIT_RENDERER, "call local spk init process"},
    {START_PLAY, "call start spk process"},
    {PAUSE_PLAY, "call pause spk process"},
    {FLUSH_PLAY, "call flush spk process"},
    {DRAIN_PLAY, "call drain spk process"},
    {STOP_PLAY, "call stop spk process"},
    {RELEASE_SPK, "release spk process"},

    {INIT_LOCAL_MIC, "call local mic init process"},
    {INIT_REMOTE_MIC, "call remote mic init process"},
    {START_MIC, "call start mic process"},
    {STOP_MIC, "call stop mic process"},
    {RELEASE_MIC, "release mic process"},

    {EXIT_DEMO, "exit interactive run test"},
};


class PaStreamTest : public RendererListener, public std::enable_shared_from_this<PaStreamTest> {
public:
    virtual ~PaStreamTest() {};
    int32_t InitRenderer();
    int32_t StartPlay();
    int32_t PausePlay();
    int32_t FlushPlay();
    int32_t DrainPlay();
    int32_t StopPlay();
    void OnWriteEvent(BufferDesc& bufferesc) override;
    bool OpenSpkFile(const std::string &spkFilePath);
    void CloseSpkFile();

private:
    std::unique_ptr<RendererInServer> rendererInServer_ = nullptr;
    static constexpr long WAV_HEADER_SIZE = 44;
    FILE *spkWavFile_ = nullptr;
};

void PaStreamTest::OnWriteEvent(BufferDesc& bufferesc)
{

}

bool PaStreamTest::OpenSpkFile(const std::string &spkFilePath)
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

void PaStreamTest::CloseSpkFile()
{
    if (spkWavFile_ != nullptr) {
        fclose(spkWavFile_);
        spkWavFile_ = nullptr;
    }
}

int32_t PaStreamTest::InitRenderer()
{
    std::string path = "/data/test.wav";
    OpenSpkFile(path);
    wav_hdr wavHeader;
    size_t headerSize = sizeof(wav_hdr);
    size_t bytesRead = fread(&wavHeader, 1, headerSize, spkWavFile_);
    AUDIO_DEBUG_LOG("Init renderer, bytesRead: %{public}zu", bytesRead);
    AudioStreamParams audioParams;
    audioParams.format = SAMPLE_S16LE;
    audioParams.samplingRate = wavHeader.SamplesPerSec;
    audioParams.channels = 2;
    rendererInServer_ = std::make_unique<RendererInServer>(audioParams, STREAM_MEDIA);
    rendererInServer_->RegisterTestCallback(shared_from_this());
    AUDIO_INFO_LOG("Audio renderer create success.");
    return 0;
}

int32_t PaStreamTest::StartPlay()
{
    if (rendererInServer_ == nullptr) {
        AUDIO_ERR_LOG("Audiorenderer init failed.");
        return -1;
    }
    if (!rendererInServer_->Start()) {
        AUDIO_ERR_LOG("Audio renderer start failed.");
        return -1;
    }
    return 0;
}

int32_t PaStreamTest::PausePlay()
{
    if (rendererInServer_ == nullptr) {
        AUDIO_ERR_LOG("Audiorenderer init failed.");
        return -1;
    }
    if (!rendererInServer_->Pause()) {
        AUDIO_ERR_LOG("Audio renderer start failed.");
        return -1;
    }
    return 0;
}

int32_t PaStreamTest::FlushPlay()
{
    if (rendererInServer_ == nullptr) {
        AUDIO_ERR_LOG("Audiorenderer init failed.");
        return -1;
    }
    if (!rendererInServer_->Flush()) {
        AUDIO_ERR_LOG("Audio renderer start failed.");
        return -1;
    }
    return 0;
}

int32_t PaStreamTest::DrainPlay()
{
    if (rendererInServer_ == nullptr) {
        AUDIO_ERR_LOG("Audiorenderer init failed.");
        return -1;
    }
    if (!rendererInServer_->Drain()) {
        AUDIO_ERR_LOG("Audio renderer start failed.");
        return -1;
    }
    return 0;
}

int32_t PaStreamTest::StopPlay()
{
    if (rendererInServer_ == nullptr) {
        AUDIO_ERR_LOG("Audiorenderer init failed.");
        return -1;
    }
    if (!rendererInServer_->Stop()) {
        AUDIO_ERR_LOG("Audio renderer stop failed.");
        return -1;
    }
    return 0;
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
    cout << "[Pa Stream Test App]" << endl << endl;
    cout << "Supported Functionalities:" << endl;
    cout << "================================Usage=======================================" << endl << endl;
    cout << "  0: Init renderer." << endl;
    cout << "  1: Start play." << endl;
    cout << "  2: Pause play." << endl;
    cout << "  3: Flush play." << endl;
    cout << "  4: Drain play." << endl;
    cout << "  5: Stop play." << endl;
    
    cout << "  2: Stop play." << endl;
    cout << "  3: Release spk." << endl;
    cout << "  5: Init local mic." << endl;
    cout << "  6: Init remote mic." << endl;
    cout << "  7: Start record." << endl;
    cout << "  8: Stop record." << endl;
    cout << "  9: Release mic." << endl;
    cout << "  10: exit demo." << endl;
    cout << " Please input your choice: " << endl;
}

int32_t InitPlayback(std::shared_ptr<PaStreamTest> streamTest, bool isRemote)
{
    if (streamTest == nullptr) {
        cout << "Play test is nullptr, init spk error." << endl;
        return -1;
    }
    int32_t ret = streamTest->InitRenderer();
    if (ret != 0) {
        cout << "Init renderer error!" << endl;
        return -1;
    }
    return 0;
}

int32_t StartPlay(std::shared_ptr<PaStreamTest> streamTest)
{
    if (streamTest == nullptr) {
        cout << "Play test is nullptr, start play error." << endl;
        return -1;
    }
    int32_t ret = streamTest->StartPlay();
    if (ret != 0) {
        cout << "Start play error!" << endl;
        return -1;
    }
    return 0;
}

int32_t PausePlay(std::shared_ptr<PaStreamTest> streamTest)
{
    if (streamTest == nullptr) {
        cout << "Play test is nullptr, pause play error." << endl;
        return -1;
    }
    int32_t ret = streamTest->PausePlay();
    if (ret != 0) {
        cout << "Pause play error!" << endl;
        return -1;
    }
    return 0;
}

int32_t FlushPlay(std::shared_ptr<PaStreamTest> streamTest)
{
    if (streamTest == nullptr) {
        cout << "Play test is nullptr, Flush play error." << endl;
        return -1;
    }
    int32_t ret = streamTest->FlushPlay();
    if (ret != 0) {
        cout << "Flush play error!" << endl;
        return -1;
    }
    return 0;
}

int32_t DrainPlay(std::shared_ptr<PaStreamTest> streamTest)
{
    if (streamTest == nullptr) {
        cout << "Play test is nullptr, Drain play error." << endl;
        return -1;
    }
    int32_t ret = streamTest->DrainPlay();
    if (ret != 0) {
        cout << "Drain play error!" << endl;
        return -1;
    }
    return 0;
}

int32_t StopPlay(std::shared_ptr<PaStreamTest> streamTest)
{
    if (streamTest == nullptr) {
        cout << "Play test is nullptr, stop play error." << endl;
        return -1;
    }
    int32_t ret = streamTest->StopPlay();
    if (ret != 0) {
        cout << "Stop play error!" << endl;
        return -1;
    }
    return 0;
}

void Loop(std::shared_ptr<PaStreamTest> streamTest)
{
    bool isProcTestRun = true;
    while (isProcTestRun) {
        PrintUsage();
        AudioOptCode optCode = INVALID_CODE;
        int32_t res = GetUserInput();
        if (g_OptStrMap.count(res)) {
            optCode = static_cast<AudioOptCode>(res);
        }
        switch (optCode) {
            case INIT_RENDERER:
                InitPlayback(streamTest, false);
                break;
            case START_PLAY:
                StartPlay(streamTest);
                break;
            case PAUSE_PLAY:
                PausePlay(streamTest);
                break;
            case FLUSH_PLAY:
                FlushPlay(streamTest);
                break;
            case DRAIN_PLAY:
                DrainPlay(streamTest);
                break;
            case STOP_PLAY:
                StopPlay(streamTest);
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
    cout << "oh pa stream test." << endl;
    std::shared_ptr<PaStreamTest> streamTest = std::make_shared<PaStreamTest>();

    Loop(streamTest);
    streamTest->CloseSpkFile();
    return 0;
}
