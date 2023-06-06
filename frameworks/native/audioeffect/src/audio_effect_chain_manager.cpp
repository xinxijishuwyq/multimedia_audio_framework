/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <cstddef>
#include <map>
#include <memory>
#include <vector>
#include <set>

#include "audio_effect_chain_adapter.h"
#include "audio_effect_chain_manager.h"
#include "audio_log.h"
#include "audio_errors.h"
#include "audio_effect.h"

using namespace OHOS::AudioStandard;

int32_t EffectChainManagerCreate(char *sceneType, BufferAttr *bufferAttr)
{
    AudioEffectChainManager *audioEffectChainManager = AudioEffectChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEffectChainManager != nullptr, ERR_INVALID_HANDLE, "null audioEffectChainManager");
    std::string sceneTypeString(sceneType);
    if (audioEffectChainManager->CreateAudioEffectChain(sceneTypeString, bufferAttr) != SUCCESS) {
        return ERROR;
    }
    return SUCCESS;
}

int32_t EffectChainManagerProcess(char *sceneType, BufferAttr *bufferAttr)
{
    AudioEffectChainManager *audioEffectChainManager = AudioEffectChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEffectChainManager != nullptr, ERR_INVALID_HANDLE, "null audioEffectChainManager");
    std::string sceneTypeString(sceneType);
    if (audioEffectChainManager->ApplyAudioEffectChain(sceneTypeString, bufferAttr) != SUCCESS) {
        return ERROR;
    }
    return SUCCESS;
}

int32_t EffectChainManagerGetFrameLen()
{
    AudioEffectChainManager *audioEffectChainManager = AudioEffectChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEffectChainManager != nullptr, ERR_INVALID_HANDLE, "null audioEffectChainManager");
    return audioEffectChainManager->GetFrameLen();
}

int32_t EffectChainManagerGetDeviceType()
{
    AudioEffectChainManager *audioEffectChainManager = AudioEffectChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEffectChainManager != nullptr, ERR_INVALID_HANDLE, "null audioEffectChainManager");
    return (int32_t)audioEffectChainManager->GetDeviceType();
}

namespace OHOS {
namespace AudioStandard {

AudioEffectChain::AudioEffectChain(std::string scene)
{
    sceneType = scene;
    effectMode = AUDIO_SUPPORTED_SCENE_MODES.find(EFFECT_DEFAULT)->second;
    audioBufIn.frameLength = 0;
    audioBufOut.frameLength = 0;
    ioBufferConfig.inputCfg.samplingRate = DEFAULT_SAMPLE_RATE;
    ioBufferConfig.inputCfg.channels = DEFAULT_NUM_CHANNEL;
    ioBufferConfig.inputCfg.format = DATA_FORMAT_F32;
    ioBufferConfig.outputCfg.samplingRate = DEFAULT_SAMPLE_RATE;
    ioBufferConfig.outputCfg.channels = DEFAULT_NUM_CHANNEL;
    ioBufferConfig.outputCfg.format = DATA_FORMAT_F32;
}

AudioEffectChain::~AudioEffectChain() {}

void AudioEffectChain::Dump()
{
    for (AudioEffectHandle handle: standByEffectHandles) {
        AUDIO_INFO_LOG("Dump standByEffectHandle for [%{public}s], handle address is %{public}p",
                       sceneType.c_str(), handle);
    }
}

std::string AudioEffectChain::GetEffectMode()
{
    return effectMode;
}

void AudioEffectChain::SetEffectMode(std::string mode)
{
    effectMode = mode;
}

void AudioEffectChain::AddEffectHandleBegin()
{
    standByEffectHandles.clear();
}

template <typename T>
int32_t GetKeyFromValue(const std::unordered_map<T, std::string> &map, std::string value)
{
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it->second == value) {
            return it->first;
        }
    }
    return -1;
}

void AudioEffectChain::AddEffectHandle(AudioEffectHandle handle)
{
    int32_t ret;
    int32_t replyData = 0;
    AudioEffectTransInfo cmdInfo = {sizeof(AudioEffectConfig), &ioBufferConfig};
    AudioEffectTransInfo replyInfo = {sizeof(int32_t), &replyData};
    ret = (*handle)->command(handle, EFFECT_CMD_INIT, &cmdInfo, &replyInfo);
    if (ret != 0) {
        AUDIO_ERR_LOG("[%{public}s] with mode [%{public}s], either one of libs EFFECT_CMD_INIT fail",
            sceneType.c_str(), effectMode.c_str());
        return;
    }
    ret = (*handle)->command(handle, EFFECT_CMD_ENABLE, &cmdInfo, &replyInfo);
    if (ret != 0) {
        AUDIO_ERR_LOG("[%{public}s] with mode [%{public}s], either one of libs EFFECT_CMD_ENABLE fail",
            sceneType.c_str(), effectMode.c_str());
        return;
    }
    ret = (*handle)->command(handle, EFFECT_CMD_SET_CONFIG, &cmdInfo, &replyInfo);
    if (ret != 0) {
        AUDIO_ERR_LOG("[%{public}s] with mode [%{public}s], either one of libs EFFECT_CMD_SET_CONFIG fail",
            sceneType.c_str(), effectMode.c_str());
        return;
    }
    // Set param
    AudioEffectParam *effectParam = new AudioEffectParam[sizeof(AudioEffectParam) +
                                                                NUM_SET_EFFECT_PARAM * sizeof(int32_t)];
    effectParam->status = 0;
    effectParam->paramSize = sizeof(int32_t);
    effectParam->valueSize = 0;
    int32_t *data = &(effectParam->data[0]);
    *data++ = EFFECT_SET_PARAM;
    *data++ = GetKeyFromValue(AUDIO_SUPPORTED_SCENE_TYPES, sceneType);
    *data++ = GetKeyFromValue(AUDIO_SUPPORTED_SCENE_MODES, effectMode);
    cmdInfo = {sizeof(AudioEffectParam) + sizeof(int32_t) * NUM_SET_EFFECT_PARAM, effectParam};
    ret = (*handle)->command(handle, EFFECT_CMD_SET_PARAM, &cmdInfo, &replyInfo);
    delete[] effectParam;
    if (ret != 0) {
        AUDIO_ERR_LOG("[%{public}s] with mode [%{public}s], either one of libs EFFECT_CMD_SET_PARAM fail",
            sceneType.c_str(), effectMode.c_str());
        return;
    }
    standByEffectHandles.emplace_back(handle);
}

void AudioEffectChain::AddEffectHandleEnd()
{
    // used for crossfading in the near future
}

void CopyBuffer(float *bufIn, float *bufOut, uint32_t totalLen)
{
    for (uint32_t i = 0; i < totalLen; ++i) {
        bufOut[i] = bufIn[i];
    }
}

void AudioEffectChain::ApplyEffectChain(float *bufIn, float *bufOut, uint32_t frameLen)
{
    if (standByEffectHandles.empty()) {
        CopyBuffer(bufIn, bufOut, frameLen * ioBufferConfig.outputCfg.channels);
        return;
    }

    audioBufIn.frameLength = frameLen;
    audioBufOut.frameLength = frameLen;
    int ret;
    int count = 0;
    for (AudioEffectHandle handle: standByEffectHandles) {
        if (count % FACTOR_TWO == 0) {
            audioBufIn.raw = bufIn;
            audioBufOut.raw = bufOut;
        } else {
            audioBufOut.raw = bufIn;
            audioBufIn.raw = bufOut;
        }
        ret = (*handle)->process(handle, &audioBufIn, &audioBufOut);
        if (ret != 0) {
            AUDIO_ERR_LOG("[%{public}s] with mode [%{public}s], either one of libs process fail",
                sceneType.c_str(), effectMode.c_str());
            continue;
        }
        count++;
    }

    if (count % FACTOR_TWO == 0) {
        CopyBuffer(bufIn, bufOut, frameLen * ioBufferConfig.outputCfg.channels);
    }
}

void AudioEffectChain::SetIOBufferConfig(bool isInput, uint32_t samplingRate, uint32_t channels)
{
    if (isInput) {
        ioBufferConfig.inputCfg.samplingRate = samplingRate;
        ioBufferConfig.inputCfg.channels = channels;
    } else {
        ioBufferConfig.outputCfg.samplingRate = samplingRate;
        ioBufferConfig.outputCfg.channels = channels;
    }
}

int32_t FindEffectLib(std::string effect, std::vector<std::unique_ptr<AudioEffectLibEntry>> &effectLibraryList,
    AudioEffectLibEntry **libEntry, std::string &libName)
{
    for (const std::unique_ptr<AudioEffectLibEntry> &lib : effectLibraryList) {
        if (lib->libraryName == effect) {
            libName = lib->libraryName;
            *libEntry = lib.get();
            return SUCCESS;
        }
    }
    return ERROR;
}

int32_t CheckValidEffectLibEntry(AudioEffectLibEntry *libEntry, std::string effect, std::string libName)
{
    if (!libEntry) {
        AUDIO_ERR_LOG("Effect [%{public}s] in lib [%{public}s] is nullptr", effect.c_str(), libName.c_str());
        return ERROR;
    }
    if (!libEntry->audioEffectLibHandle) {
        AUDIO_ERR_LOG("AudioEffectLibHandle of Effect [%{public}s] in lib [%{public}s] is nullptr",
                      effect.c_str(), libName.c_str());
        return ERROR;
    }
    if (!libEntry->audioEffectLibHandle->createEffect) {
        AUDIO_ERR_LOG("CreateEffect function of Effect [%{public}s] in lib [%{public}s] is nullptr",
                      effect.c_str(), libName.c_str());
        return ERROR;
    }
    return SUCCESS;
}

AudioEffectChainManager::AudioEffectChainManager() {}

AudioEffectChainManager::~AudioEffectChainManager() {}

AudioEffectChainManager *AudioEffectChainManager::GetInstance()
{
    static AudioEffectChainManager audioEffectChainManager;
    return &audioEffectChainManager;
}

int32_t AudioEffectChainManager::SetOutputDeviceSink(int32_t device, std::string &sinkName)
{
    deviceType = (DeviceType)device;
    deviceSink = sinkName;
    AUDIO_INFO_LOG("Set deviceType to [%{public}d] and corresponding sink is [%{public}s]",
                   device, sinkName.c_str());

    std::string sceneType;
    for (auto scene = AUDIO_SUPPORTED_SCENE_TYPES.begin(); scene != AUDIO_SUPPORTED_SCENE_TYPES.end();
        ++scene) {
        sceneType = scene->second;
        if (!SceneTypeToEffectChainMap.count(sceneType)) {
            AUDIO_ERR_LOG("Set effect chain for [%{public}s] but it does not exist", sceneType.c_str());
            continue;
        }
        AUDIO_INFO_LOG("Set effect chain for scene name %{public}s", sceneType.c_str());
        auto *audioEffectChain = SceneTypeToEffectChainMap[sceneType];
        if (SetAudioEffectChain(sceneType, audioEffectChain->GetEffectMode()) != SUCCESS) {
            AUDIO_ERR_LOG("Fail to set effect chain for [%{public}s]", sceneType.c_str());
            continue;
        }
    }
    return SUCCESS;
}

DeviceType AudioEffectChainManager::GetDeviceType()
{
    return deviceType;
}

std::string AudioEffectChainManager::GetDeviceTypeName()
{
    std::string name = SUPPORTED_DEVICE_TYPE.find(DEVICE_TYPE_INVALID)->second;
    auto device = SUPPORTED_DEVICE_TYPE.find(deviceType);
    if (device != SUPPORTED_DEVICE_TYPE.end()) {
        name = device->second;
    }
    return name;
}

int32_t AudioEffectChainManager::SetFrameLen(int32_t frameLength)
{
    frameLen = frameLength;
    return SUCCESS;
}

int32_t AudioEffectChainManager::GetFrameLen()
{
    return frameLen;
}

// Boot initialize
void AudioEffectChainManager::InitAudioEffectChainManager(std::vector<EffectChain> &effectChains,
    std::unordered_map<std::string, std::string> &map,
    std::vector<std::unique_ptr<AudioEffectLibEntry>> &effectLibraryList)
{
    std::set<std::string> effectSet;
    for (EffectChain efc: effectChains) {
        for (std::string effect: efc.apply) {
            effectSet.insert(effect);
        }
    }

    // Construct EffectToLibraryEntryMap that stores libEntry for each effect name
    AudioEffectLibEntry *libEntry = nullptr;
    int32_t ret;
    std::string libName;
    for (std::string effect: effectSet) {
        ret = FindEffectLib(effect, effectLibraryList, &libEntry, libName);
        if (ret == ERROR) {
            AUDIO_ERR_LOG("Couldn't find libEntry of effect %{public}s", effect.c_str());
            continue;
        }
        ret = CheckValidEffectLibEntry(libEntry, effect, libName);
        if (ret == ERROR) {
            AUDIO_ERR_LOG("Invalid libEntry of effect %{public}s", effect.c_str());
            continue;
        }
        EffectToLibraryEntryMap[effect] = libEntry;
        EffectToLibraryNameMap[effect] = libName;
    }
    
    // Construct EffectChainToEffectsMap that stores all effect names of each effect chain
    for (EffectChain efc: effectChains) {
        std::string key = efc.name;
        std::vector <std::string> effects;
        for (std::string effectName: efc.apply) {
            effects.emplace_back(effectName);
        }
        EffectChainToEffectsMap[key] = effects;
    }

    // Constrcut SceneTypeAndModeToEffectChainNameMap that stores effectMode associated with the effectChainName
    for (auto item = map.begin(); item != map.end(); ++item) {
        SceneTypeAndModeToEffectChainNameMap[item->first] = item->second;
    }

    AUDIO_INFO_LOG("EffectToLibraryEntryMap size %{public}zu", EffectToLibraryEntryMap.size());
    AUDIO_INFO_LOG("EffectChainToEffectsMap size %{public}zu", EffectChainToEffectsMap.size());
    AUDIO_INFO_LOG("SceneTypeAndModeToEffectChainNameMap size %{public}zu",
                   SceneTypeAndModeToEffectChainNameMap.size());
}

int32_t AudioEffectChainManager::CreateAudioEffectChain(std::string sceneType, BufferAttr *bufferAttr)
{
    AudioEffectChain *audioEffectChain;
    if (SceneTypeToEffectChainMap.count(sceneType)) {
        audioEffectChain = SceneTypeToEffectChainMap[sceneType];
    } else {
        audioEffectChain = new AudioEffectChain(sceneType);
        SceneTypeToEffectChainMap[sceneType] = audioEffectChain;
    }

    audioEffectChain->SetIOBufferConfig(true, bufferAttr->samplingRate, bufferAttr->numChanIn); // input
    audioEffectChain->SetIOBufferConfig(false, bufferAttr->samplingRate, bufferAttr->numChanOut); // output

    std::string effectMode = AUDIO_SUPPORTED_SCENE_MODES.find(EFFECT_DEFAULT)->second;
    if (SetAudioEffectChain(sceneType, effectMode) != SUCCESS) {
        return ERROR;
    }
    return SUCCESS;
}

int32_t AudioEffectChainManager::SetAudioEffectChain(std::string sceneType, std::string effectMode)
{
    if (!SceneTypeToEffectChainMap.count(sceneType)) {
        AUDIO_ERR_LOG("SceneType [%{public}s] does not exist, failed to set", sceneType.c_str());
        return ERROR;
    }
    AudioEffectChain *audioEffectChain = SceneTypeToEffectChainMap[sceneType];

    std::string effectChain;
    std::string effectChainKey = sceneType + "_&_" + effectMode + "_&_" + GetDeviceTypeName();
    std::string effectNone = AUDIO_SUPPORTED_SCENE_MODES.find(EFFECT_NONE)->second;
    if (!SceneTypeAndModeToEffectChainNameMap.count(effectChainKey)) {
        AUDIO_ERR_LOG("EffectChain key [%{public}s] does not exist, auto set to %{public}s",
                      effectChainKey.c_str(), effectNone.c_str());
        effectChain = effectNone;
    } else {
        effectChain = SceneTypeAndModeToEffectChainNameMap[effectChainKey];
    }

    if (effectChain != effectNone && !EffectChainToEffectsMap.count(effectChain)) {
        AUDIO_ERR_LOG("EffectChain name [%{public}s] does not exist, auto set to %{public}s",
                      effectChain.c_str(), effectNone.c_str());
        effectChain = effectNone;
    }

    int ret;
    audioEffectChain->SetEffectMode(effectMode);
    audioEffectChain->AddEffectHandleBegin();
    for (std::string effect: EffectChainToEffectsMap[effectChain]) {
        AudioEffectHandle handle = nullptr;
        AudioEffectDescriptor descriptor;
        descriptor.libraryName = EffectToLibraryNameMap[effect];
        descriptor.effectName = effect;
        ret = EffectToLibraryEntryMap[effect]->audioEffectLibHandle->createEffect(descriptor, &handle);
        if (ret != 0) {
            AUDIO_ERR_LOG("EffectToLibraryEntryMap[%{public}s] createEffect fail", effect.c_str());
            continue;
        }
        audioEffectChain->AddEffectHandle(handle);
    }
    audioEffectChain->AddEffectHandleEnd();
    
    return SUCCESS;
}

int32_t AudioEffectChainManager::ApplyAudioEffectChain(std::string sceneType, BufferAttr *bufferAttr)
{
    if (!SceneTypeToEffectChainMap.count(sceneType)) {
        AUDIO_ERR_LOG("Scene type [%{public}s] does not exist", sceneType.c_str());
        CopyBuffer(bufferAttr->bufIn, bufferAttr->bufOut, bufferAttr->frameLen * bufferAttr->numChanIn);
        return ERROR;
    }

    auto *audioEffectChain = SceneTypeToEffectChainMap[sceneType];
    audioEffectChain->ApplyEffectChain(bufferAttr->bufIn, bufferAttr->bufOut, bufferAttr->frameLen);
    
    return SUCCESS;
}

void AudioEffectChainManager::Dump()
{
    AUDIO_INFO_LOG("<-------- AudioEffectChainManager::Dump START -------->");
    for (auto item = SceneTypeToEffectChainMap.begin(); item != SceneTypeToEffectChainMap.end(); ++item) {
        AudioEffectChain *audioEffectChain = item->second;
        audioEffectChain->Dump();
    }
    AUDIO_INFO_LOG("<-------- AudioEffectChainManager::Dump END -------->");
}

}
}
