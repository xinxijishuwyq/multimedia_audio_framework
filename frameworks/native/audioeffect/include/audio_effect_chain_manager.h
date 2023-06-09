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


#ifndef AUDIO_EFFECT_CHAIN_MANAGER_H
#define AUDIO_EFFECT_CHAIN_MANAGER_H

#include <cstdio>
#include <cstdint>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "audio_effect_chain_adapter.h"
#include "audio_effect.h"

namespace OHOS {
namespace AudioStandard {

const uint32_t NUM_SET_EFFECT_PARAM = 3;
const uint32_t DEFAULT_FRAMELEN = 480;
const uint32_t DEFAULT_SAMPLE_RATE = 48000;
const uint32_t DEFAULT_NUM_CHANNEL = 2;
const uint32_t FACTOR_TWO = 2;
const std::string DEFAULT_DEVICE_SINK = "Speaker";

class AudioEffectChain {
public:
    AudioEffectChain(std::string scene);
    ~AudioEffectChain();
    std::string GetEffectMode();
    void SetEffectMode(std::string mode);
    void AddEffectHandleBegin();
    void AddEffectHandleEnd();
    void AddEffectHandle(AudioEffectHandle effectHandle, AudioEffectLibrary *libHandle);
    void ApplyEffectChain(float *bufIn, float *bufOut, uint32_t frameLen);
    void SetIOBufferConfig(bool isInput, uint32_t samplingRate, uint32_t channels);
    bool IsEmptyEffectHandles();
    void Dump();
private:
    std::string sceneType;
    std::string effectMode;
    std::vector<AudioEffectHandle> standByEffectHandles;
    std::vector<AudioEffectLibrary*> libHandles;
    AudioEffectConfig ioBufferConfig;
    AudioBuffer audioBufIn;
    AudioBuffer audioBufOut;
};

class AudioEffectChainManager {
public:
    AudioEffectChainManager();
    ~AudioEffectChainManager();
    static AudioEffectChainManager *GetInstance();
    void InitAudioEffectChainManager(std::vector<EffectChain> &effectChains,
        std::unordered_map<std::string, std::string> &map,
        std::vector<std::unique_ptr<AudioEffectLibEntry>> &effectLibraryList);
    int32_t CreateAudioEffectChain(std::string sceneType, BufferAttr *bufferAttr);
    int32_t SetAudioEffectChain(std::string sceneType, std::string effectChain);
    bool ExistAudioEffectChain(std::string sceneType, std::string effectMode);
    int32_t ApplyAudioEffectChain(std::string sceneType, BufferAttr *bufferAttr);
    int32_t SetOutputDeviceSink(int32_t device, std::string &sinkName);
    DeviceType GetDeviceType();
    std::string GetDeviceTypeName();
    int32_t GetFrameLen();
    int32_t SetFrameLen(int32_t frameLen);
    void Dump();
private:
    std::map<std::string, AudioEffectLibEntry*> EffectToLibraryEntryMap;
    std::map<std::string, std::string> EffectToLibraryNameMap;
    std::map<std::string, std::vector<std::string>> EffectChainToEffectsMap;
    std::map<std::string, std::string> SceneTypeAndModeToEffectChainNameMap;
    std::map<std::string, AudioEffectChain*> SceneTypeToEffectChainMap;
    uint32_t frameLen = DEFAULT_FRAMELEN;
    DeviceType deviceType = DEVICE_TYPE_SPEAKER;
    std::string deviceSink = DEFAULT_DEVICE_SINK;
};

}  // namespace AudioStandard
}  // namespace OHOS
#endif // AUDIO_EFFECT_CHAIN_MANAGER_H
