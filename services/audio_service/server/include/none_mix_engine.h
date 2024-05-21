/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef NONE_MIX_ENGINE_H
#define NONE_MIX_ENGINE_H
#include <mutex>
#include <atomic>
#include "audio_playback_engine.h"

namespace OHOS {
namespace AudioStandard {
class NoneMixEngine : public AudioPlaybackEngine {
public:
    NoneMixEngine(DeviceInfo type, bool isVoip);
    ~NoneMixEngine() override;

    int32_t Start() override;
    int32_t Stop() override;
    int32_t Pause() override;
    int32_t Flush() override;

    int32_t AddRenderer(const std::shared_ptr<IRendererStream> &stream) override;
    void RemoveRenderer(const std::shared_ptr<IRendererStream> &stream) override;
    bool IsPlaybackEngineRunning() const noexcept override;

protected:
    void MixStreams() override;

private:
    void StandbySleep();
    int32_t InitSink(const AudioStreamInfo &streamInfo);
    int32_t SwitchSink(const AudioStreamInfo &streamInfo, bool isVoip);
    void PauseAsync();

private:
    bool isVoip_;
    bool isStart_;
    bool isPause_;
    DeviceInfo device_;
    std::atomic<uint32_t> failedCount_;
    uint64_t writeCount_;
    uint64_t fwkSyncTime_;
    std::shared_ptr<IRendererStream> stream_;

    std::mutex startMutex;
};
} // namespace AudioStandard
} // namespace OHOS
#endif
