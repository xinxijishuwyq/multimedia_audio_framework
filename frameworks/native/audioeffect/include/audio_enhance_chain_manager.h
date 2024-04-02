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

#ifndef AUDIO_ENHANCE_CHAIN_MANAGER_H
#define AUDIO_ENHANCE_CHAIN_MANAGER_H

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
#include "audio_enhance_chain_adapter.h"
#include "audio_effect_chain_manager.h"

namespace OHOS {
namespace AudioStandard {

struct DataDescription {
    uint32_t frameLength; // 10ms, 20ms
    uint32_t sampleRate; // 16000 48000
    uint32_t dataFormat; // 16, 24, 32
    uint32_t micNum; // 1, 2, 4
    uint32_t refNum; // 2, 4
    uint32_t outChannelNum; // 1, 2, 3, 4
} __attribute__((packed));

constexpr DataDescription dataDesc = {
    .frameLength = 20,
    .sampleRate = 48000,
    .dataFormat = 16,
    .micNum = 2,
    .refNum = 0,
    .outChannelNum = 2,
};

class AudioEnhanceChain {
public:
    AudioEnhanceChain(std::string scene);
    ~AudioEnhanceChain();
    void SetEnhanceMode(std::string mode);
    void ReleaseEnhanceChain();
    void AddEnhanceHandle(AudioEffectHandle handle, AudioEffectLibrary *libHandle);
    void ApplyEnhanceChain(float *bufIn, float *bufOut, uint32_t frameLen);
    bool IsEmptyEnhanceHandles();

private:
    std::string sceneType_;
    std::string enhanceMode_;
    std::vector<AudioEffectHandle> standByEnhanceHandles_;
    std::vector<AudioEffectLibrary*> enhanceLibHandles_;
};

class AudioEnhanceChainManager {
public:
    AudioEnhanceChainManager();
    ~AudioEnhanceChainManager();
    static AudioEnhanceChainManager* GetInstance();
    void InitAudioEnhanceChainManager(std::vector<EffectChain> &enhanceChains,
        std::unordered_map<std::string, std::string> &map,
        std::vector<std::unique_ptr<AudioEffectLibEntry>> &enhanceLibraryList);
    int32_t CreateAudioEnhanceChainDynamic(std::string sceneType, std::string enhanceMode, std::string upAndDownDevice);
    int32_t SetAudioEnhanceChainDynamic(std::string sceneType, std::string enhanceMode, std::string upAndDownDevice);
    int32_t ReleaseAudioEnhanceChainDynamic(std::string sceneType, std::string upAndDownDevice);
    int32_t ApplyAudioEnhanceChain(std::string sceneType, std::string upAndDownDevice, BufferAttr *bufferAttr);
    std::string GetUpAndDownDevice();

private:
    std::map<std::string, AudioEnhanceChain*> SceneTypeToEnhanceChainMap_;
    std::map<std::string, std::string> SceneTypeAndModeToEnhanceChainNameMap_;
    std::map<std::string, std::vector<std::string>> EnhanceChainToEnhancesMap_;
    std::map<std::string, AudioEffectLibEntry*> EnhanceToLibraryEntryMap_;
    std::map<std::string, std::string> EnhanceToLibraryNameMap_;
    bool isInitialized_ = false;
    std::string upAndDownDevice_;
};

}  // namespace AudioStandard
}  // namespace OHOS
#endif // AUDIO_ENHANCE_CHAIN_MANAGER_H