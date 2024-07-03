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

#ifndef AUDIO_ENHANCE_CHAIN_H
#define AUDIO_ENHANCE_CHAIN_H

#include <vector>
#include <mutex>
#include <map>

#include "audio_effect.h"

namespace OHOS {
namespace AudioStandard {

struct EnhanceBuffer {
    std::vector<uint8_t> ecBuffer;  // voip: ref data = mic data * 2
    std::vector<uint8_t> micBufferIn; // mic data input
    std::vector<uint8_t> micBufferOut; // mic data output
    uint32_t length;  // mic length
    uint32_t lengthEc;  // EC length
};

struct AlgoAttr {
    uint32_t bitDepth;
    uint32_t batchLen;
    uint32_t byteLenPerFrame;
};

struct AlgoConfig {
    uint32_t frameLength;
    uint32_t sampleRate;
    uint32_t dataFormat;
    uint32_t micNum;
    uint32_t refNum;
    uint32_t outNum;
};

class AudioEnhanceChain {
public:
    AudioEnhanceChain(const std::string &scene, const std::string &mode);
    ~AudioEnhanceChain();
    void AddEnhanceHandle(AudioEffectHandle handle, AudioEffectLibrary *libHandle);
    bool IsEmptyEnhanceHandles();
    void GetAlgoConfig(AudioBufferConfig &algoConfig);
    uint32_t GetAlgoBufferSize();
    uint32_t GetAlgoBufferSizeEc();

private:
    void InitAudioEnhanceChain();
    void ReleaseEnhanceChain();

    bool setConfigFlag_;
    std::mutex chainMutex_;
    std::string sceneType_;
    std::string enhanceMode_;
    AlgoAttr algoAttr_;
    AlgoConfig algoSupportedConfig_;
    std::vector<AudioEffectHandle> standByEnhanceHandles_;
    std::vector<AudioEffectLibrary*> enhanceLibHandles_;
};

}  // namespace AudioStandard
}  // namespace OHOS
#endif // AUDIO_ENHANCE_CHAIN_H