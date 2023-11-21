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

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <unistd.h>
#include "audio_log.h"
#include "audio_renderer.h"
#include "pcm2wav.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::AudioStandard;

namespace {
    constexpr int32_t ARGS_COUNTS = 3;
    constexpr int32_t PCMPATH_INDEX = 1;
    constexpr int32_t METAPATH_INDEX = 2;

    constexpr int32_t METABUF_MAX_SIZE = 20000;
    constexpr int32_t AVS3METADATA_SIZE = 19824;

    constexpr int32_t AUDIO_VIVID_SAMPLES = 1024;

    constexpr int32_t SUCCESS = 0;

    constexpr int32_t SAMPLE_FORMAT_U8 = 8;
    constexpr int32_t SAMPLE_FORMAT_S16LE = 16;
    constexpr int32_t SAMPLE_FORMAT_S24LE = 24;
    constexpr int32_t SAMPLE_FORMAT_S32LE = 32;

    std::map<uint8_t, int8_t> format2bps = {{SAMPLE_S16LE, sizeof(int16_t)},
                                            {SAMPLE_S24LE, sizeof(int16_t) + sizeof(int8_t)},
                                            {SAMPLE_S32LE, sizeof(int32_t)},
                                            {SAMPLE_F32LE, sizeof(int32_t)}};
}

class AudioRenderer3DCallbackTestImpl : public AudioRendererCallback {
public:
    void OnInterrupt(const InterruptEvent &interruptEvent) override {}
    void OnStateChange(const RendererState state, const StateChangeCmdType __attribute__((unused)) cmdType) override
    {
        AUDIO_DEBUG_LOG("AudioRenderer3DCallbackTestImpl:: OnStateChange");

        switch (state) {
            case RENDERER_PREPARED:
                AUDIO_DEBUG_LOG("AudioRenderer3DCallbackTestImpl: OnStateChange RENDERER_PREPARED");
                break;
            case RENDERER_RUNNING:
                AUDIO_DEBUG_LOG("AudioRenderer3DCallbackTestImpl: OnStateChange RENDERER_RUNNING");
                break;
            case RENDERER_STOPPED:
                AUDIO_DEBUG_LOG("AudioRenderer3DCallbackTestImpl: OnStateChange RENDERER_STOPPED");
                break;
            case RENDERER_PAUSED:
                AUDIO_DEBUG_LOG("AudioRenderer3DCallbackTestImpl: OnStateChange RENDERER_PAUSED");
                break;
            case RENDERER_RELEASED:
                AUDIO_DEBUG_LOG("AudioRenderer3DCallbackTestImpl: OnStateChange RENDERER_RELEASED");
                break;
            default:
                AUDIO_ERR_LOG("AudioRenderer3DCallbackTestImpl: OnStateChange NOT A VALID state");
                break;
        }
    }
};

class AudioRendererTest {
public:
    void CheckSupportedParams() const
    {
        vector<AudioSampleFormat> supportedFormatList = AudioRenderer::GetSupportedFormats();
        AUDIO_INFO_LOG("AudioRendererTest: Supported formats:");
        for (auto i = supportedFormatList.begin(); i != supportedFormatList.end(); ++i) {
            AUDIO_INFO_LOG("AudioRendererTest: Format %{public}d", *i);
        }

        vector<AudioChannel> supportedChannelList = AudioRenderer::GetSupportedChannels();
        AUDIO_INFO_LOG("AudioRendererTest: Supported channels:");
        for (auto i = supportedChannelList.begin(); i != supportedChannelList.end(); ++i) {
            AUDIO_INFO_LOG("AudioRendererTest: channel %{public}d", *i);
        }

        vector<AudioEncodingType> supportedEncodingTypes
                                    = AudioRenderer::GetSupportedEncodingTypes();
        AUDIO_INFO_LOG("AudioRendererTest: Supported encoding types:");
        for (auto i = supportedEncodingTypes.begin(); i != supportedEncodingTypes.end(); ++i) {
            AUDIO_INFO_LOG("AudioRendererTest: encoding type %{public}d", *i);
        }

        vector<AudioSamplingRate> supportedSamplingRates = AudioRenderer::GetSupportedSamplingRates();
        AUDIO_INFO_LOG("AudioRendererTest: Supported sampling rates:");
        for (auto i = supportedSamplingRates.begin(); i != supportedSamplingRates.end(); ++i) {
            AUDIO_INFO_LOG("AudioRendererTest: sampling rate %{public}d", *i);
        }
    }

    void GetRendererStreamInfo(const unique_ptr<AudioRenderer> &audioRenderer) const
    {
        AUDIO_INFO_LOG("AudioRendererTest: GetRendererInfo:");
        AudioRendererInfo rendererInfo;
        if (audioRenderer->GetRendererInfo(rendererInfo) ==  SUCCESS) {
            AUDIO_INFO_LOG("AudioRendererTest: Get ContentType: %{public}d", rendererInfo.contentType);
            AUDIO_INFO_LOG("AudioRendererTest: Get StreamUsage: %{public}d", rendererInfo.streamUsage);
        } else {
            AUDIO_ERR_LOG("AudioRendererTest: GetStreamInfo failed");
        }

        AUDIO_INFO_LOG("AudioRendererTest: GetStreamInfo:");
        AudioStreamInfo streamInfo;
        if (audioRenderer->GetStreamInfo(streamInfo) ==  SUCCESS) {
            AUDIO_INFO_LOG("AudioRendererTest: Get AudioSamplingRate: %{public}d", streamInfo.samplingRate);
            AUDIO_INFO_LOG("AudioRendererTest: Get AudioEncodingType: %{public}d", streamInfo.encoding);
            AUDIO_INFO_LOG("AudioRendererTest: Get AudioSampleFormat: %{public}d", streamInfo.format);
            AUDIO_INFO_LOG("AudioRendererTest: Get AudioChannel: %{public}d", streamInfo.channels);
        } else {
            AUDIO_ERR_LOG("AudioRendererTest: GetStreamInfo failed");
        }
    }

    bool InitRender(const unique_ptr<AudioRenderer> &audioRenderer) const
    {
        AUDIO_INFO_LOG("AudioRendererTest: Starting renderer");
        if (!audioRenderer->Start()) {
            AUDIO_ERR_LOG("AudioRendererTest: Start failed");
            if (!audioRenderer->Release()) {
                AUDIO_ERR_LOG("AudioRendererTest: Release failed");
            }
            return false;
        }
        AUDIO_INFO_LOG("AudioRendererTest: Playback started");
        return true;
    }

    bool StartRender(const unique_ptr<AudioRenderer> &audioRenderer, FILE* wavFile, FILE* metaFile) const
    {
        size_t bufferLen = 0;
        if (audioRenderer->GetBufferSize(bufferLen) != SUCCESS) {
            return false;
        }

        auto buffer = std::make_unique<uint8_t[]>(bufferLen);
        auto metaBuffer = std::make_unique<uint8_t[]>(METABUF_MAX_SIZE);
        if (buffer == nullptr)
        {
            AUDIO_ERR_LOG("AudioRendererTest: Failed to allocate buffer");
            return false;
        }

        size_t bytesToWrite = 0;

        while (!feof(wavFile)) {
            bytesToWrite = fread(buffer.get(), 1, bufferLen, wavFile);
            fread(metaBuffer.get(), 1, AVS3METADATA_SIZE, metaFile);
            AUDIO_INFO_LOG("AudioRendererTest: Bytes to write: %{public}zu", bytesToWrite);
            audioRenderer->Write(buffer.get(), bytesToWrite, metaBuffer.get(), AVS3METADATA_SIZE);
        }

        if (!audioRenderer->Drain()) {
            AUDIO_ERR_LOG("AudioRendererTest: Drain failed");
        }
        return true;
    }

    bool TestPlayback(int argc, char *argv[]) const
    {
        AUDIO_INFO_LOG("AudioRenderer3DTest: TestPlayback start ");

        char *inputPath = argv[PCMPATH_INDEX];
        char *metaPath = argv[METAPATH_INDEX];
        char path[PATH_MAX] = {0x00};
        if ((strlen(inputPath) >= PATH_MAX) || (realpath(inputPath, path) == nullptr)) {
            AUDIO_ERR_LOG("Invalid path");
            return false;
        }
        AUDIO_INFO_LOG("AudioRenderer3DTest: pcm path = %{public}s", path);
        FILE* pcmFile = fopen(path, "rb");
        if (pcmFile == nullptr) {
            AUDIO_INFO_LOG("AudioRenderer3DTest: Unable to open pcm file");
            return false;
        }
        if ((strlen(metaPath) >= PATH_MAX) || (realpath(metaPath, path) == nullptr)) {
            AUDIO_ERR_LOG("Invalid path");
            return false;
        }
        AUDIO_INFO_LOG("AudioRenderer3DTest: metadata path = %{public}s", path);
        FILE* metaFile = fopen(path, "rb");
        if (metaFile == nullptr) {
            fclose(pcmFile);
            AUDIO_INFO_LOG("AudioRenderer3DTest: Unable to open metadata file");
            return false;
        }

        ContentType contentType = ContentType::CONTENT_TYPE_MUSIC;
        StreamUsage streamUsage = StreamUsage::STREAM_USAGE_MEDIA;

        AudioRendererOptions rendererOptions = {};
        rendererOptions.streamInfo.encoding = AudioEncodingType::ENCODING_AUDIOVIVID;
        rendererOptions.streamInfo.samplingRate = SAMPLE_RATE_48000;
        rendererOptions.streamInfo.format = SAMPLE_S32LE;
        rendererOptions.streamInfo.channels = CHANNEL_10;
        rendererOptions.rendererInfo.contentType = contentType;
        rendererOptions.rendererInfo.streamUsage = streamUsage;
        rendererOptions.rendererInfo.rendererFlags = 0;

        unique_ptr<AudioRenderer> audioRenderer = AudioRenderer::Create(rendererOptions);

        if (audioRenderer == nullptr) {
            AUDIO_ERR_LOG("AudioRenderer3DTest: Create failed");
            fclose(pcmFile);
            fclose(metaFile);
            return false;
        }

        int32_t ret = 0;
        shared_ptr<AudioRendererCallback> cb1 = make_shared<AudioRenderer3DCallbackTestImpl>();
        ret = audioRenderer->SetRendererCallback(cb1);
        if (ret) {
            AUDIO_ERR_LOG("AudioRenderer3DTest: SetRendererCallback failed %{public}d", ret);
            fclose(pcmFile);
            fclose(metaFile);
            return false;
        }

        GetRendererStreamInfo(audioRenderer);

        CheckSupportedParams();
        AUDIO_ERR_LOG("AudioRenderer3DTest: buffermsec = %{public}d", bufferMsec);
        int32_t status = audioRenderer->SetBufferDuration(bufferMsec);
        if (status) {
            AUDIO_ERR_LOG("Failed to set buffer duration");
        }

        if (!InitRender(audioRenderer)) {
            AUDIO_ERR_LOG("AudioRenderer3DTest: Init render failed");
            fclose(pcmFile);
            fclose(metaFile);
            return false;
        }

        if (!StartRender(audioRenderer, pcmFile, metaFile)) {
            AUDIO_ERR_LOG("AudioRenderer3DTest: Start render failed");
            fclose(pcmFile);
            fclose(metaFile);
            return false;
        }

        if (!audioRenderer->Stop()) {
            AUDIO_ERR_LOG("AudioRenderer3DTest: Stop failed");
        }

        if (!audioRenderer->Release()) {
            AUDIO_ERR_LOG("AudioRenderer3DTest: Release failed");
        }

        fclose(pcmFile);
        fclose(metaFile);
        AUDIO_INFO_LOG("AudioRendererTest: TestPlayback end");

        return true;
    }
};

int main(int argc, char *argv[])
{
    AUDIO_INFO_LOG("AudioRenderer3DTest: Render test in");

    if (argv == nullptr) {
        AUDIO_ERR_LOG("AudioRenderer3DTest: argv is null");
        return 0;
    }

    if (argc != ARGS_COUNTS) {
        AUDIO_ERR_LOG("AudioRenderer3DTest: incorrect argc. Enter 3 args");
        return 0;
    }

    AUDIO_INFO_LOG("AudioRenderer3DTest: argc=%{public}d", argc);
    AUDIO_INFO_LOG("pcm file path=%{public}s", argv[PCMPATH_INDEX]);
    AUDIO_INFO_LOG("metadata file path=%{public}s", argv[METAPATH_INDEX]);

    AudioRendererTest testObj;
    bool ret = testObj.TestPlayback(argc, argv);

    return ret;
}
