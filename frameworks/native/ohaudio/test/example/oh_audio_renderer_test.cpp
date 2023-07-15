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

#include <iostream>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>
#include <ctime>
#include "common/native_audiostreambuilder.h"
#include "native_audiorenderer.h"

#ifdef __cplusplus
extern "C" {
#endif
namespace AudioTestConstants {
    constexpr int32_t FIRST_ARG_IDX = 1;
    constexpr int32_t SECOND_ARG_IDX = 2;
    constexpr int32_t THIRD_ARG_IDX = 3;
    constexpr int32_t FOUR_ARG_IDX = 4;
    constexpr int32_t WAIT_INTERVAL = 1000;
}

std::string g_filePath = "/data/data/oh_test_audio.pcm";
FILE* g_file = nullptr;
bool g_readEnd = false;
int32_t g_samplingRate = 48000;
int32_t g_channelCount = 2;
int32_t g_latencyMode = 0;

static int32_t AudioRendererOnWriteData(OH_AudioRenderer* capturer,
    void* userData,
    void* buffer,
    int32_t bufferLen)
{
    size_t readCount = fread(buffer, bufferLen, 1, g_file);
    if (!readCount) {
        g_readEnd = true;
        if (ferror(g_file)) {
            printf("Error reading myfile");
        } else if (feof(g_file)) {
            printf("EOF found");
        }
    }

    return 0;
}

void PlayerTest(char *argv[])
{
    OH_AudioStream_Result ret;

    // 1. create builder
    OH_AudioStreamBuilder* builder;
    OH_AudioStream_Type type = AUDIOSTREAM_TYPE_RENDERER;
    ret = OH_AudioStreamBuilder_Create(&builder, type);
    printf("createcallback ret: %d \n", ret);

    // 2. set params and callbacks
    OH_AudioStreamBuilder_SetSamplingRate(builder, g_samplingRate);
    OH_AudioStreamBuilder_SetChannelCount(builder, g_channelCount);
    OH_AudioStreamBuilder_SetLatencyMode(builder, (OH_AudioStream_LatencyMode)g_channelCount);

    OH_AudioRenderer_Callbacks callbacks;
    callbacks.OH_AudioRenderer_OnWriteData = AudioRendererOnWriteData;
    ret = OH_AudioStreamBuilder_SetRendererCallback(builder, callbacks, nullptr);
    printf("setcallback ret: %d \n", ret);

    // 3. create OH_AudioRenderer
    OH_AudioRenderer* audioRenderer;
    ret = OH_AudioStreamBuilder_GenerateRenderer(builder, &audioRenderer);
    printf("create renderer client, ret: %d \n", ret);

    // 4. start
    ret = OH_AudioRenderer_Start(audioRenderer);
    printf("start ret: %d \n", ret);
    int32_t frameSize;
    OH_AudioRenderer_GetFrameSizeInCallback(audioRenderer, &frameSize);
    printf("framesize: %d \n", frameSize);

    int timer = 0;
    while (!g_readEnd) {
        std::this_thread::sleep_for(std::chrono::milliseconds(AudioTestConstants::WAIT_INTERVAL));
        int64_t frames;
        OH_AudioRenderer_GetFramesWritten(audioRenderer, &frames);
        printf("Wait for the audio to finish playing.(..%d s) frames:%ld\n", ++timer, frames);
        int64_t framePosition;
        int64_t timestamp;
        OH_AudioRenderer_GetTimestamp(audioRenderer, CLOCK_MONOTONIC, &framePosition, &timestamp);
        printf("framePosition %ld timestamp:%ld\n", framePosition, timestamp);
    }
    // 5. stop and release client
    ret = OH_AudioRenderer_Stop(audioRenderer);
    printf("stop ret: %d \n", ret);
    ret = OH_AudioRenderer_Release(audioRenderer);
    printf("release ret: %d \n", ret);

    // 6. destroy the builder
    ret = OH_AudioStreamBuilder_Destroy(builder);
    printf("destroy builder ret: %d \n", ret);
}

int main(int argc, char *argv[])
{
    printf("start \n");
    if ((argv == nullptr) || (argc < AudioTestConstants::THIRD_ARG_IDX)) {
        printf("input parms wrong. input format: filePath samplingRate channelCount latencyMode\n");
        printf("input demo: ./oh_audio_renderer_test ./oh_test_audio.pcm 48000 2 1 \n");
        return 0;
    }
    printf("argc=%d ", argc);
    printf("argv[1]=%s ", argv[AudioTestConstants::FIRST_ARG_IDX]);
    printf("argv[2]=%s ", argv[AudioTestConstants::SECOND_ARG_IDX]);
    printf("argv[3]=%s \n", argv[AudioTestConstants::THIRD_ARG_IDX]);

    g_samplingRate = atoi(argv[AudioTestConstants::SECOND_ARG_IDX]);
    g_channelCount = atoi(argv[AudioTestConstants::THIRD_ARG_IDX]);
    g_latencyMode = atoi(argv[AudioTestConstants::FOUR_ARG_IDX]);

    g_filePath = argv[AudioTestConstants::FIRST_ARG_IDX];
    printf("filePATH: %s \n", g_filePath.c_str());

    g_file = fopen(g_filePath.c_str(), "rb");
    if (g_file == nullptr) {
        printf("OHAudioRendererTest: Unable to open file \n");
        return 0;
    }

    PlayerTest(argv);

    fclose(g_file);
    g_file = nullptr;
    return 0;
}

#ifdef __cplusplus
}
#endif
