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

#include <benchmark/benchmark.h>
#include <string>
#include <vector>

#include "audio_info.h"
#include "audio_renderer.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::AudioStandard;

namespace {
class AudioRendererTest : public benchmark::Fixture {
public:
    AudioRendererTest()
    {
        Iterations(iterations);
        Repetitions(repetitions);
        ReportAggregatesOnly();
    }

    ~AudioRendererTest() override = default;

    void SetUp(const ::benchmark::State &state) override
    {
        sleep(1);
        AudioRendererOptions rendererOptions;
        rendererOptions.streamInfo.samplingRate = AudioSamplingRate::SAMPLE_RATE_96000;
        rendererOptions.streamInfo.encoding = AudioEncodingType::ENCODING_PCM;
        rendererOptions.streamInfo.format = AudioSampleFormat::SAMPLE_U8;
        rendererOptions.streamInfo.channels = AudioChannel::MONO;
        rendererOptions.rendererInfo.contentType = ContentType::CONTENT_TYPE_MUSIC;
        rendererOptions.rendererInfo.streamUsage = StreamUsage::STREAM_USAGE_MEDIA;
        rendererOptions.rendererInfo.rendererFlags = RENDERER_FLAG;
        audioRenderer = AudioRenderer::Create(rendererOptions);
        sleep(1);
    }

    void TearDown(const ::benchmark::State &state) override
    {
    }

protected:
    unique_ptr<AudioRenderer> audioRenderer;
    const string AUDIORENDER_TEST_FILE_PATH = "/data/test_44100_2.wav";
    const int32_t RENDERER_FLAG = 0;
    const int32_t repetitions = 16;
    const int32_t iterations = 1;
};

// StartAbility
BENCHMARK_F(AudioRendererTest, StartAbilityTestCase)(
    benchmark::State &state)
{
    while (state.KeepRunning()) {
         bool isStarted = audioRenderer->Start();
         if(isStarted==false){
            state.SkipWithError("StartAbilityTestCase audioRenderer start failed.");
         }

         state.PauseTiming();
         bool isStopped = audioRenderer->Stop();
         if(isStopped==false){
            state.SkipWithError("StartAbilityTestCase audioRenderer start failed.");
         }
         state.ResumeTiming();
    }
}

// PauseAbility
BENCHMARK_F(AudioRendererTest, PauseAbilityTestCase)(
    benchmark::State &state)
{
    while (state.KeepRunning()) {
         state.PauseTiming();
         bool isStarted = audioRenderer->Start();
         if(isStarted==false){
            state.SkipWithError("PauseAbilityTestCase audioRenderer start failed.");
         }
         
        bool isPaused = audioRenderer->Pause();
        if(isPaused==false){
            state.SkipWithError("PauseAbilityTestCase audioRenderer pause failed.");
         } 

         state.PauseTiming();
         bool isStopped = audioRenderer->Stop();
         if(isStopped==false){
            state.SkipWithError("PauseAbilityTestCase audioRenderer start failed.");
         }  
         state.ResumeTiming();
    }
}

// StopAbility
BENCHMARK_F(AudioRendererTest, StopAbilityTestCase)(
    benchmark::State &state)
{
    while (state.KeepRunning()) {
         state.PauseTiming();
         bool isStarted = audioRenderer->Start();
         if(isStarted==false){
            state.SkipWithError("StopAbilityTestCase audioRenderer start failed.");
         }
         state.ResumeTiming();

         bool isStopped = audioRenderer->Stop();
         if(isStopped==false){
            state.SkipWithError("StopAbilityTestCase audioRenderer start failed.");
         }  
    }
}
}

// Run the benchmark
BENCHMARK_MAIN();
