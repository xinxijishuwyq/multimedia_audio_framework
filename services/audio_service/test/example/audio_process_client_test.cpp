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

#include <cinttypes>
#include <condition_variable>
#include <cstdint>
#include <ctime>
#include <ostream>
#include <sstream>
#include <iostream>
#include <thread>
#include <mutex>
#include <map>

#include "audio_log.h"
#include "audio_error.h"
#include "audio_utils.h"
#include "audio_process_in_client.h"
#include "pcm2wav.h"

using namespace std;
using namespace OHOS::AudioStandard;
namespace {
    static const uint32_t ARGC_NUM_TWO = 2;
    static const uint32_t ARGC_NUM_THREE = 3;
    static const long WAV_HEADER_SIZE = 42;
    static const int32_t SUCCESS = 0;
    static const int32_t ERROR = -1;
    enum OperationCode : int32_t {
        INVALID_OPERATION = -1,
        INIT_PROCESS = 0,
        START_PROCESS,
        PAUSE_PROCESS,
        STOP_PROCESS,
        CHANGE_PROCESS_VOL,
        RELEASE_PROCESS
    };
}
std::map<int32_t, std::string> g_operationStringMap = {
    {INIT_PROCESS, "call init process"},
    {START_PROCESS, "call start process"},
    {PAUSE_PROCESS, "call pause process"},
    {STOP_PROCESS, "call stop process"},
    {CHANGE_PROCESS_VOL, "change process volume"},
    {RELEASE_PROCESS, "release process"}
};
std::string g_filePath = nullptr;
FILE *g_wavFile = nullptr;
mutex g_autoRunMutex;
condition_variable g_autoRunCV;

class AudioProcessTest : public AudioDataCallback, public enable_shared_from_this<AudioProcessTest> {
public:
    AudioProcessTest() = default;
    ~AudioProcessTest() = default;

    void OnHandleData(size_t length) override
    {
        Trace callBack("client_n");
        int32_t ret = processClient_->GetBufferDesc(buffer_);
        if (ret != SUCCESS || buffer_.buffer == nullptr || buffer_.bufLength ==0) {
            cout << "GetBufferDesc failed." << endl;
            return;
        }
        if (needSkipWavHeader_) {
            fseek(g_wavFile, WAV_HEADER_SIZE, SEEK_SET);
            needSkipWavHeader_ = false;
        }
        if (feof(g_wavFile)) {
            if (loopCount_ < 0) {
                fseek(g_wavFile, WAV_HEADER_SIZE, SEEK_SET); // infinite loop
            } else if (loopCount_ == 0) {
                renderFinish_ = true;
                g_autoRunCV.notify_all();
            } else {
                loopCount_--;
                fseek(g_wavFile, WAV_HEADER_SIZE, SEEK_SET);
            }
        }
        if (renderFinish_) {
            return;
        }
        fread(buffer_.buffer, 1, buffer_.bufLength, g_wavFile);
        processClient_->Enqueue(buffer_);
        callBack.End();
    }

    int32_t Init(int32_t loopCount)
    {
        if (loopCount < 0) {
            loopCount_ = 1; // loop once
        } else if (loopCount == 0) {
            loopCount_ = -1; // infinite loop
        } else {
            loopCount_ = loopCount;
        }

        AudioProcessConfig config;
        config.appInfo.appPid = getpid();
        config.appInfo.appUid = getuid();

        config.audioMode = AUDIO_MODE_PLAYBACK;

        config.rendererInfo.contentType = CONTENT_TYPE_MUSIC;
        config.rendererInfo.streamUsage = STREAM_USAGE_MEDIA;
        config.rendererInfo.rendererFlags = 4; // 4 for test

        config.streamInfo.channels = STEREO;
        config.streamInfo.encoding = ENCODING_PCM;
        config.streamInfo.format = SAMPLE_S16LE;
        config.streamInfo.samplingRate = SAMPLE_RATE_48000;

        processClient_ = AudioProcessInClient::Create(config);
        if (processClient_ == nullptr) {
            return ERROR;
        }
        processClient_->SaveDataCallback(shared_from_this());
        gIsInited = true;
        return SUCCESS;
    }

    bool Start()
    {
        int32_t ret = processClient_->Start();
        if (ret != SUCCESS) {
            return false;
        }
        return true;
    }

    bool Pause()
    {
        int32_t ret = processClient_->Pause();
        if (ret != SUCCESS) {
            return false;
        }
        return true;
    }

    bool SetVolume(int32_t vol)
    {
        int32_t ret = processClient_->SetVolume(vol);
        if (ret != SUCCESS) {
            return false;
        }
        return true;
    }

    bool Stop()
    {
        int32_t ret = processClient_->Stop();
        if (ret != SUCCESS) {
            return false;
        }
        return true;
    }

    bool Release()
    {
        int32_t ret = processClient_->Release();
        if (ret != SUCCESS) {
            return false;
        }
        return true;
    }

private:
    std::shared_ptr<AudioProcessInClient> processClient_ = nullptr;
    bool gIsInited = false;
    int32_t loopCount_ = -1; // for loop
    BufferDesc buffer_ = {nullptr, 0, 0};
    bool needSkipWavHeader_ = true;
    bool renderFinish_ = false;
};

inline int32_t GetArgs(std::string args)
{
    int32_t value = 0;
    stringstream valueStr;
    valueStr << args;
    valueStr >> value;
    return value;
}

void PrintInteractiveUsage()
{
    for (auto it = g_operationStringMap.begin(); it != g_operationStringMap.end(); it ++) {
        cout << "\t enter " << it->first << " : " << it->second << endl;
    }
}

void PrintUsage()
{
    cout << "[Audio Process Client Test App]" << endl << endl;
    cout << "Supported Functionalities:" << endl;
    cout << "  a) Automatically perform playback test." << endl;
    cout << "  a) Interactive execution of playback test." << endl;
    cout << "================================Usage=======================================" << endl << endl;

    cout << "-a\n\tAutomatically play" << endl;
    cout << "\tUsage : ./audio_process_client_test <wav-file-path> <play-loop-count>" << endl;
    cout << "\t       if <play-loop-count> equals to 0, it will loop infinitely." << endl;
    cout << "\tExample 1 : ./audio_process_client_test /data/data/48kHz_16bit.wav 0" << endl;
    cout << "\tExample 2 : ./audio_process_client_test /data/data/48kHz_16bit.wav 2" << endl << endl;

    cout << "-b\n\tInteractive play" << endl;
    cout << "\tUsage : ./audio_process_client_test <wav-file-path>" << endl;
    PrintInteractiveUsage();
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

shared_ptr<AudioProcessTest> g_audioProcessTest = nullptr;
void AutoRun(int32_t loopCount)
{
    g_audioProcessTest->Init(loopCount);
    g_audioProcessTest->Start();
    int volShift = 15; // helf of 1 << 16
    g_audioProcessTest->SetVolume(1 << volShift);
    unique_lock<mutex> lock(g_autoRunMutex);
    g_autoRunCV.wait(lock);
    cout << "AutoRun end" << endl;
    g_audioProcessTest->Stop();
    g_audioProcessTest->Release();
}

string ConfigTest()
{
    int32_t ret = g_audioProcessTest->Init(0);
    if (ret != SUCCESS) {
        return "init failed";
    }
    return "init SUCCESS";
}

string CallStart()
{
    if (!g_audioProcessTest->Start()) {
        return "start failed";
    }
    return "start SUCCESS";
}

string CallPause()
{
    if (!g_audioProcessTest->Pause()) {
        return "Pause failed";
    }
    return "Pause SUCCESS";
}

string CallStop()
{
    if (!g_audioProcessTest->Stop()) {
        return "Stop failed";
    }
    return "Stop SUCCESS";
}

string SetVolume()
{
    int32_t vol = GetUserInput();
    if (!g_audioProcessTest->SetVolume(vol)) {
        return "SetVolume failed";
    }
    return "SetVolume SUCCESS";
}

string CallRelease()
{
    if (!g_audioProcessTest->Release()) {
        return "Release failed";
    }
    return "Release SUCCESS";
}

void InteractiveRun()
{
    OperationCode optCode = INVALID_OPERATION; // invalid code
    while (true) {
        PrintInteractiveUsage();
        int32_t res = GetUserInput();
        if (g_operationStringMap.count(res)) {
            optCode = static_cast<OperationCode>(res);
        } else {
            optCode = INVALID_OPERATION;
        }
        switch (optCode) {
            case INIT_PROCESS:
                cout << ConfigTest() << endl;
                break;
            case START_PROCESS:
                cout << CallStart() << endl;
                break;
            case STOP_PROCESS:
                cout << CallStop() << endl;
                break;
            case PAUSE_PROCESS:
                cout << CallPause() << endl;
                break;
            case CHANGE_PROCESS_VOL:
                cout << SetVolume() << endl;
                break;
            case RELEASE_PROCESS:
                cout << CallRelease() << endl;
                break;
            default:
                cout << "invalid input :" << optCode << endl;
                break;
        }
    }
}

bool OpenFile()
{
    char path[PATH_MAX] = { 0x00 };
    if ((strlen(g_filePath.c_str()) > PATH_MAX) || (realpath(g_filePath.c_str(), path) == nullptr)) {
        return false;
    }
    AUDIO_INFO_LOG("path = %{public}s", path);
    g_wavFile = fopen(path, "rb");
    if (g_wavFile == nullptr) {
        AUDIO_ERR_LOG("Unable to open wave file");
        return false;
    }
    return true;
}

void CloseFile()
{
    if (g_wavFile != nullptr) {
        fclose(g_wavFile);
        g_wavFile = nullptr;
    }
}

int main(int argc, char *argv[])
{
    AUDIO_INFO_LOG("AudioProcessClientTest: Render test in");

    g_audioProcessTest = make_shared<AudioProcessTest>();
    if (argv == nullptr || argc > ARGC_NUM_THREE || argc < ARGC_NUM_TWO) {
        cout << "AudioProcessClientTest: argv is invalid" << endl;
        return 0;
    }
    g_filePath = argv[1];
    if (!OpenFile()) {
        cout << "open file path failed!" << g_filePath << endl;
        return 0;
    }

    PrintUsage();
    bool isAudioRun = false;
    int32_t loopCount = -1; // loop
    if (argc == ARGC_NUM_THREE && GetArgs(argv[ARGC_NUM_TWO]) >= 0) {
        loopCount = GetArgs(argv[ARGC_NUM_TWO]);
        isAudioRun = true;
    } else {
        isAudioRun = false;
    }

    if (!isAudioRun) {
        InteractiveRun();
    } else {
        AutoRun(loopCount);
    }
    CloseFile();

    return 0;
}
