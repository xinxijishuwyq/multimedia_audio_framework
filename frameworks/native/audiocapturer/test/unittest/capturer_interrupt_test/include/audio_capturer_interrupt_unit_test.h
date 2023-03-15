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

#ifndef AUDIO_CAPTURER_INTERRUPT_UNIT_TEST_H
#define AUDIO_CAPTURER_INTERRUPT_UNIT_TEST_H

#include "gtest/gtest.h"
#include "audio_capturer.h"
#include "audio_renderer.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
enum TestType {
    CAPTURE_CAPTURE = 1,
    CAPTURE_RANDER = 2,
    RANDER_CAPTURE = 3,
};

class AudioCapturerInterruptCallbackTest : public AudioCapturerCallback {
public:
    void OnInterrupt(const InterruptEvent &interruptEvent) override
    {
        AUDIO_DEBUG_LOG("AudioCapturerInterruptCallbackTest: OnInterrupt Hint: %{public}d eventType: %{public}d,\
            rceType: %{public}d", interruptEvent.hintType, interruptEvent.eventType, interruptEvent.forceType);
    }
    void OnStateChange(const CapturerState state) override {}
};

class AudioRendererInterruptCallbackTest : public AudioRendererCallback {
public:
    void OnInterrupt(const InterruptEvent &interruptEvent) override
    {
        AUDIO_DEBUG_LOG("AudioRendererInterruptCallbackTest: OnInterrupt Hint: %{public}d eventType: %{public}d,\
            forceType: %{public}d", interruptEvent.hintType, interruptEvent.eventType, interruptEvent.forceType);
    }
    void OnStateChange(const RendererState state, const StateChangeCmdType cmdType) override {}
};

class AudioCapturerInterruptUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

    static void UTCreateAudioCapture(std::unique_ptr<AudioCapturer> &audioCapturer,
        SourceType sourceType);
    static void UTCreateAudioRender(std::unique_ptr<AudioRenderer> &audioRenderer,
        AudioRendererInfo renderInfo);
    static void AudioInterruptUnitTestFunc(TestType tType, bool bothRun, SourceType sourceTypte1,
        SourceType sourceTypte2, AudioRendererInfo renderInfo);
};
} // namespace AudioStandard
} // namespace OHOS

#endif // AUDIO_CAPTURER_INTERRUPT_UNIT_TEST_H
