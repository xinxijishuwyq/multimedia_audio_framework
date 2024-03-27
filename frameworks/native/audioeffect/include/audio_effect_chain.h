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

#ifndef AUDIO_EFFECT_CHAIN_H
#define AUDIO_EFFECT_CHAIN_H

#include <cstdio>
#include <cstdint>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <set>

#include "audio_effect.h"

#ifdef SENSOR_ENABLE
#include "audio_head_tracker.h"
#endif
#include "audio_effect_hdi_param.h"
#ifdef WINDOW_MANAGER_ENABLE
#include "audio_effect_rotation.h"
#endif
#include "audio_effect_volume.h"

namespace OHOS {
namespace AudioStandard {
struct AudioEffectProcInfo {
    bool headTrackingEnabled;
    bool offloadEnabled;
};

class AudioEffectChain {
public:
#ifdef SENSOR_ENABLE
    AudioEffectChain(std::string scene, std::shared_ptr<HeadTracker> headTracker);
#else
    AudioEffectChain(std::string scene);
#endif
    ~AudioEffectChain();
    std::string GetEffectMode();
    void SetEffectMode(std::string mode);
    void ReleaseEffectChain();
    void AddEffectHandle(AudioEffectHandle effectHandle, AudioEffectLibrary *libHandle, AudioEffectScene currSceneType);
    void ApplyEffectChain(float *bufIn, float *bufOut, uint32_t frameLen, AudioEffectProcInfo procInfo);
    void SetIOBufferConfig(bool isInput, uint32_t samplingRate, uint32_t channels);
    bool IsEmptyEffectHandles();
    void Dump();
    int32_t UpdateMultichannelIoBufferConfig(const uint32_t &channels, const uint64_t &channelLayout);
    void StoreOldEffectChainInfo(std::string &sceneMode, AudioEffectConfig &ioBufferConfig);
    AudioEffectConfig GetIoBufferConfig();
    void InitEffectChain();
    void SetHeadTrackingDisabled();
    uint32_t GetLatency();
    int32_t SetEffectParam(AudioEffectScene currSceneType);

private:
    std::mutex reloadMutex;
    std::string sceneType;
    std::string effectMode;
    uint32_t latency_ = 0;
    std::vector<AudioEffectHandle> standByEffectHandles;
    std::vector<AudioEffectLibrary *> libHandles;
    AudioEffectConfig ioBufferConfig;
    AudioBuffer audioBufIn;
    AudioBuffer audioBufOut;

#ifdef SENSOR_ENABLE
    std::shared_ptr<HeadTracker> headTracker_;
#endif
};

} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_EFFECT_CHAIN_H