/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef AUDIO_INTERRUPT_TEST_H
#define AUDIO_INTERRUPT_TEST_H

#include <thread>

#include "audio_renderer.h"
#include "audio_system_manager.h"

namespace OHOS {
namespace AudioStandard {
class AudioInterruptTest {
public:
    static AudioInterruptTest& GetInstance();
    static void StartRender();
    void WriteBuffer();
    int32_t TestPlayback(const AudioStreamType &streamType);

    FILE *wavFile_ = nullptr;
    bool isRenderPaused_ = false;
    bool isRenderingCompleted_ = false;
    std::thread renderThread_;
    std::unique_ptr<AudioRenderer> audioRenderer_ = nullptr;
private:
    bool InitRender() const;
    bool GetBufferLen(size_t &bufferLen) const;

    AudioSystemManager *audioSystemMgr_ = nullptr;
};

class AudioInterruptCallbackTest : public AudioManagerCallback {
    void OnInterrupt(const InterruptAction &interruptAction) override;
};
} // AudioStandard
} // OHOS
#endif // AUDIO_INTERRUPT_TEST_H