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
#include "audio_effect_chain_manager.h"
#include "audio_enhance_chain_adapter.h"

namespace OHOS {
namespace AudioStandard {

struct DataDescription {
    uint32_t frameLength;
    uint32_t sampleRate;
    uint32_t dataFormat;
    uint32_t micNum;
    uint32_t refNum;
    uint32_t outChannelNum;
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
    AudioEnhanceChain(std::string &scene);
    ~AudioEnhanceChain();
    void SetEnhanceMode(std::string &mode);
    void ReleaseEnhanceChain();
    void AddEnhanceHandle(AudioEffectHandle handle, AudioEffectLibrary *libHandle);
    void ApplyEnhanceChain(float *bufIn, float *bufOut, uint32_t frameLen);
    bool IsEmptyEnhanceHandles();

private:
    std::mutex reloadMutex_;
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
        std::unordered_map<std::string, std::string> &enhanceChainNameMap,
        std::vector<std::shared_ptr<AudioEffectLibEntry>> &enhanceLibraryList);
    int32_t CreateAudioEnhanceChainDynamic(std::string &sceneType,
        std::string &enhanceMode, std::string &upAndDownDevice);
    int32_t ReleaseAudioEnhanceChainDynamic(std::string &sceneType, std::string &upAndDownDevice);
    int32_t ApplyAudioEnhanceChain(std::string &sceneType,
        std::string &upAndDownDevice, BufferAttr *bufferAttr);
    std::string GetUpAndDownDevice();

private:
    int32_t SetAudioEnhanceChainDynamic(std::string &sceneType,
        std::string &enhanceMode, std::string &upAndDownDevice);
    
    std::map<std::string, AudioEnhanceChain*> sceneTypeToEnhanceChainMap_;
    std::map<std::string, std::string> sceneTypeAndModeToEnhanceChainNameMap_;
    std::map<std::string, std::vector<std::string>> enhanceChainToEnhancesMap_;
    std::map<std::string, AudioEffectLibEntry*> enhanceToLibraryEntryMap_;
    std::map<std::string, std::string> enhanceToLibraryNameMap_;
    std::mutex chainMutex_;
    bool isInitialized_;
    std::string upAndDownDevice_;
};

}  // namespace AudioStandard
}  // namespace OHOS
#endif // AUDIO_ENHANCE_CHAIN_MANAGER_H