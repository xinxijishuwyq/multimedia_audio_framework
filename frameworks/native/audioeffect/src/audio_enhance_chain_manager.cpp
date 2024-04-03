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

#undef LOG_TAG
#define LOG_TAG "AudioEnhanceChainManager"

#include "audio_enhance_chain_manager.h"

#include "audio_log.h"
#include "audio_errors.h"
#include "audio_enhance_chain_adapter.h"

using namespace OHOS::AudioStandard;

int32_t EnhanceChainManagerProcess(char *sceneType, BufferAttr *bufferAttr)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERR_INVALID_HANDLE, "null audioEnhanceChainManager");
    std::string sceneTypeString = "";
    if (sceneType) {
        sceneTypeString = sceneType;
    }
    std::string upAndDownDevice = audioEnhanceChainMananger->GetUpAndDownDevice();
    if (audioEnhanceChainMananger->ApplyAudioEnhanceChain(sceneTypeString, upAndDownDevice, bufferAttr) != SUCCESS) {
        return ERROR;
    }
    return SUCCESS;
}

int32_t EnhanceChainManagerCreateCb(const char *sceneType, const char *enhanceMode,
    const char *upDevice, const char *downDevice)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERR_INVALID_HANDLE, "null audioEnhanceChainManager");
    std::string sceneTypeString = "";
    std::string enhanceModeString = "";
    std::string upDeviceString = "";
    std::string downDeviceString = "";
    if (sceneType) {
        sceneTypeString = sceneType;
    }
    if (enhanceMode) {
        enhanceModeString = enhanceMode;
    }
    if (upDevice) {
        upDeviceString = upDevice;
    }
    if (downDevice) {
        downDeviceString = downDevice;
    }
    std::string upAndDownDeviceString = upDeviceString + "_&_" + downDeviceString;
    if (audioEnhanceChainMananger->CreateAudioEnhanceChainDynamic(sceneTypeString,
        enhanceModeString, upAndDownDeviceString) != SUCCESS) {
        return ERROR;
    }
    return SUCCESS;
}

int32_t EnhanceChainManagerReleaseCb(const char *sceneType, const char *enhanceMode,
    const char *upDevice, const char *downDevice)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERR_INVALID_HANDLE, "null audioEnhanceChainManager");
    std::string sceneTypeString = "";
    std::string upDeviceString = "";
    std::string downDeviceString = "";

    if (sceneType) {
        sceneTypeString = sceneType;
    }
    if (upDevice) {
        upDeviceString = upDevice;
    }
    if (downDevice) {
        downDeviceString = downDevice;
    }
    std::string upAndDownDeviceString = upDeviceString + "_&_" + downDeviceString;
    if (audioEnhanceChainMananger->ReleaseAudioEnhanceChainDynamic(sceneTypeString,
        upAndDownDeviceString) != SUCCESS) {
        return ERROR;
    }
    return SUCCESS;
}

namespace OHOS {
namespace AudioStandard {

AudioEnhanceChain::AudioEnhanceChain(std::string &scene)
{
    sceneType_ = scene;
    enhanceMode_ = "EFFECT_DEFAULT";
}

AudioEnhanceChain::~AudioEnhanceChain()
{
    ReleaseEnhanceChain();
}

void AudioEnhanceChain::SetEnhanceMode(std::string &mode)
{
    enhanceMode_ = mode;
}

void AudioEnhanceChain::ReleaseEnhanceChain()
{
    for (uint32_t i = 0; i < standByEnhanceHandles_.size() && i < enhanceLibHandles_.size(); i++) {
        if (!enhanceLibHandles_[i] || !standByEnhanceHandles_[i]) {
            continue;
        }
        if (!enhanceLibHandles_[i]->releaseEffect) {
            continue;
        }
        enhanceLibHandles_[i]->releaseEffect(standByEnhanceHandles_[i]);
    }
    standByEnhanceHandles_.clear();
    enhanceLibHandles_.clear();
}

void AudioEnhanceChain::AddEnhanceHandle(AudioEffectHandle handle, AudioEffectLibrary *libHandle)
{
    int32_t ret = 0;
    AudioEffectTransInfo cmdInfo = {};
    AudioEffectTransInfo replyInfo = {};

    ret = (*handle)->command(handle, EFFECT_CMD_INIT, &cmdInfo, &replyInfo);
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with [%{public}s], either one of libs EFFECT_CMD_INIT fail",
        sceneType_.c_str(), enhanceMode_.c_str());
    DataDescription desc = dataDesc;
    cmdInfo.data = static_cast<void *>(&desc);
    cmdInfo.size = sizeof(desc);

    ret = (*handle)->command(handle, EFFECT_CMD_SET_CONFIG, &cmdInfo, &replyInfo);
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with [%{public}s], either one of libs EFFECT_CMD_SET_CONFIG fail",
        sceneType_.c_str(), enhanceMode_.c_str());
    standByEnhanceHandles_.emplace_back(handle);
    enhanceLibHandles_.emplace_back(libHandle);
}

void CopyBufferEnhance(const float *bufIn, float *bufOut, uint32_t totalLen)
{
    for (uint32_t i = 0; i < totalLen; i++) {
        bufOut[i] = bufIn[i];
    }
}

void AudioEnhanceChain::ApplyEnhanceChain(float *bufIn, float *bufOut, uint32_t frameLen)
{
    if (IsEmptyEnhanceHandles()) {
        AUDIO_ERR_LOG("audioEnhanceChain->standByEnhanceHandles is empty");
        CopyBufferEnhance(bufIn, bufOut, frameLen * dataDesc.outChannelNum);
        return;
    }
    std::lock_guard<std::mutex> lock(reloadMutex_);
    AudioBuffer audioBufIn_ = {};
    AudioBuffer audioBufOut_ = {};
    audioBufIn_.raw = static_cast<void *>(bufIn);
    audioBufOut_.raw = static_cast<void *>(bufOut);

    for (AudioEffectHandle handle : standByEnhanceHandles_) {
        int32_t ret = (*handle)->process(handle, &audioBufIn_, &audioBufOut_);
        CHECK_AND_CONTINUE_LOG(ret == 0, "[%{publc}s] with mode [%{public}s], either one of libs process fail",
            sceneType_.c_str(), enhanceMode_.c_str());
    }
}

bool AudioEnhanceChain::IsEmptyEnhanceHandles()
{
    std::lock_guard<std::mutex> lock(reloadMutex_);
    return standByEnhanceHandles_.size() == 0;
}

AudioEnhanceChainManager::AudioEnhanceChainManager()
{
    sceneTypeToEnhanceChainMap_.clear();
    sceneTypeAndModeToEnhanceChainNameMap_.clear();
    enhanceChainToEnhancesMap_.clear();
    enhanceToLibraryEntryMap_.clear();
    enhanceToLibraryNameMap_.clear();
    isInitialized_ = false;
    upAndDownDevice_ = "";
}

AudioEnhanceChainManager::~AudioEnhanceChainManager()
{
    for (auto item = sceneTypeToEnhanceChainMap_.begin(); item != sceneTypeToEnhanceChainMap_.end(); item++) {
        item->second->ReleaseEnhanceChain();
    }
}

AudioEnhanceChainManager *AudioEnhanceChainManager::GetInstance()
{
    static AudioEnhanceChainManager audioEnhanceChainManager;
    return &audioEnhanceChainManager;
}

int32_t FindEnhanceLib(std::string &enhance,
    const std::vector<std::unique_ptr<AudioEffectLibEntry>> &enhanceLibraryList,
    AudioEffectLibEntry **libEntry, std::string &libName)
{
    for (const std::unique_ptr<AudioEffectLibEntry> &lib : enhanceLibraryList) {
        if (lib->libraryName == enhance) {
            libName = lib->libraryName;
            *libEntry = lib.get();
            return SUCCESS;
        }
    }
    return ERROR;
}

int32_t CheckValidEnhanceLibEntry(AudioEffectLibEntry *libEntry, std::string &enhance,
    std::string &libName)
{
    CHECK_AND_RETURN_RET_LOG(libEntry, ERROR, "Enhance [%{public}s] in lib [%{public}s] is nullptr",
        enhance.c_str(), libName.c_str());
    CHECK_AND_RETURN_RET_LOG(libEntry->audioEffectLibHandle, ERROR,
        "AudioEffectLibHandle of Enhance [%{public}s] in lib [%{public}s] is nullptr",
        enhance.c_str(), libName.c_str());
    CHECK_AND_RETURN_RET_LOG(libEntry->audioEffectLibHandle->createEffect, ERROR,
        "CreateEffect function of Enhance [%{public}s] in lib [%{public}s] is nullptr",
        enhance.c_str(), libName.c_str());
    CHECK_AND_RETURN_RET_LOG(libEntry->audioEffectLibHandle->releaseEffect, ERROR,
        "ReleaseEffect function of Enhance [%{public}s] in lib [%{public}s] is nullptr",
        enhance.c_str(), libName.c_str());
    return SUCCESS;
}


void AudioEnhanceChainManager::InitAudioEnhanceChainManager(std::vector<EffectChain> &enhanceChains,
    std::unordered_map<std::string, std::string> &enhanceChainNameMap,
    std::vector<std::unique_ptr<AudioEffectLibEntry>> &enhanceLibraryList)
{
    std::lock_guard<std::mutex> lock(chainMutex_);
    std::set<std::string> enhanceSet;
    for (EffectChain enhanceChain : enhanceChains) {
        for (std::string enhance : enhanceChain.apply) {
            enhanceSet.insert(enhance);
        }
    }
    // Construct enhanceToLibraryEntryMap_ that stores libEntry for each effect name
    AudioEffectLibEntry* libEntry = nullptr;
    std::string libName;
    for (std::string enhance : enhanceSet) {
        int32_t ret = FindEnhanceLib(enhance, enhanceLibraryList, &libEntry, libName);
        CHECK_AND_CONTINUE_LOG(ret != ERROR, "Couldn't find libEntry of effect %{public}s", enhance.c_str());
        ret = CheckValidEnhanceLibEntry(libEntry, enhance, libName);
        enhanceToLibraryEntryMap_[enhance] = libEntry;
        enhanceToLibraryNameMap_[enhance] = libName;
    }
    // Construct enhanceChainToEnhancesMap_ that stores all effect names of each effect chain
    for (EffectChain enhanceChain : enhanceChains) {
        std::string key = enhanceChain.name;
        std::vector<std::string> enhances;
        for (std::string enhanceName : enhanceChain.apply) {
            enhances.emplace_back(enhanceName);
        }
        enhanceChainToEnhancesMap_[key] = enhances;
    }
    // Construct sceneTypeAndModeToEnhanceChainNameMap_ that stores effectMode associated with the effectChainName
    for (auto item = enhanceChainNameMap.begin(); item != enhanceChainNameMap.end(); item++) {
        sceneTypeAndModeToEnhanceChainNameMap_[item->first] = item->second;
    }
    AUDIO_INFO_LOG("enhanceToLibraryEntryMap_ size %{public}zu \
        enhanceToLibraryNameMap_ size %{public}zu \
        sceneTypeAndModeToEnhanceChainNameMap_ size %{public}zu",
        enhanceToLibraryEntryMap_.size(),
        enhanceChainToEnhancesMap_.size(),
        sceneTypeAndModeToEnhanceChainNameMap_.size());
    isInitialized_ = true;
}

int32_t AudioEnhanceChainManager::CreateAudioEnhanceChainDynamic(std::string &sceneType,
    std::string &enhanceMode, std::string &upAndDownDevice)
{
    std::lock_guard<std::mutex> lock(chainMutex_);
    upAndDownDevice_ = upAndDownDevice;
    std::string sceneTypeAndDeviceKey = sceneType + "_&_" + upAndDownDevice;
    AudioEnhanceChain *audioEnhanceChain;
    if (sceneTypeToEnhanceChainMap_.count(sceneTypeAndDeviceKey)) {
        return SUCCESS;
    } else {
        audioEnhanceChain = new AudioEnhanceChain(sceneType);
        sceneTypeToEnhanceChainMap_.insert(std::make_pair(sceneTypeAndDeviceKey, audioEnhanceChain));
    }
    if (SetAudioEnhanceChainDynamic(sceneType, enhanceMode, upAndDownDevice) != SUCCESS) {
        return ERROR;
    }
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::SetAudioEnhanceChainDynamic(std::string &sceneType,
    std::string &enhanceMode, std::string &upAndDownDevice)
{
    std::string sceneTypeAndDeviceKey = sceneType + "_&_" + upAndDownDevice;
    CHECK_AND_RETURN_RET_LOG(sceneTypeToEnhanceChainMap_.count(sceneTypeAndDeviceKey), ERROR,
        "SceneType [%{public}s] does not exist, fail to set.", sceneType.c_str());
    
    AudioEnhanceChain *audioEnhanceChain = sceneTypeToEnhanceChainMap_[sceneTypeAndDeviceKey];

    std::string enhanceChain;
    std::string enhanceChainKey = sceneType + "_&_" + enhanceMode;
    std::string enhanceNone = AUDIO_SUPPORTED_SCENE_MODES.find(EFFECT_NONE)->second;
    if (!sceneTypeAndModeToEnhanceChainNameMap_.count(enhanceChainKey)) {
        AUDIO_ERR_LOG("EnhanceChain key [%{public}s] does not exist, auto set to %{public}s",
            enhanceChainKey.c_str(), enhanceNone.c_str());
        enhanceChain = enhanceNone;
    } else {
        enhanceChain = sceneTypeAndModeToEnhanceChainNameMap_[enhanceChainKey];
    }

    if (enhanceChain != enhanceNone && !enhanceChainToEnhancesMap_.count(enhanceChain)) {
        AUDIO_ERR_LOG("EnhanceChain name [%{public}s] does not exist, auto set to %{public}s",
            enhanceChain.c_str(), enhanceNone.c_str());
            enhanceChain = enhanceNone;
    }

    audioEnhanceChain->SetEnhanceMode(enhanceMode);
    for (std::string enhance : enhanceChainToEnhancesMap_[enhanceChain]) {
        AudioEffectHandle handle = nullptr;
        AudioEffectDescriptor descriptor;
        descriptor.libraryName = enhanceToLibraryNameMap_[enhance];
        descriptor.effectName = enhance;

        AUDIO_INFO_LOG("libraryName: %{public}s effectName:%{public}s",
            descriptor.libraryName.c_str(), descriptor.effectName.c_str());
        int32_t ret = enhanceToLibraryEntryMap_[enhance]->audioEffectLibHandle->createEffect(descriptor, &handle);
        CHECK_AND_CONTINUE_LOG(ret == 0, "EnhanceToLibraryEntryMap[%{public}s] createEffect fail",
            enhance.c_str());
        audioEnhanceChain->AddEnhanceHandle(handle, enhanceToLibraryEntryMap_[enhance]->audioEffectLibHandle);
    }

    if (audioEnhanceChain->IsEmptyEnhanceHandles()) {
        AUDIO_ERR_LOG("EnhanceChain is empty, copy bufIn to bufOut like EFFECT_NONE mode");
        return ERROR;
    }
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::ReleaseAudioEnhanceChainDynamic(std::string &sceneType,
    std::string &upAndDownDevice)
{
    std::lock_guard<std::mutex> lock(chainMutex_);
    CHECK_AND_RETURN_RET_LOG(isInitialized_, ERROR, "has not been initialized");
    CHECK_AND_RETURN_RET_LOG(sceneType != "", ERROR, "null sceneType");

    AudioEnhanceChain *audioEnhanceChain;
    std::string sceneTypeAndDeviceKey = sceneType + "_&_" + upAndDownDevice;
    if (!sceneTypeToEnhanceChainMap_.count(sceneTypeAndDeviceKey)) {
        return SUCCESS;
    } else {
        audioEnhanceChain = sceneTypeToEnhanceChainMap_[sceneTypeAndDeviceKey];
    }
    if (audioEnhanceChain != nullptr) {
        delete audioEnhanceChain;
        audioEnhanceChain = nullptr;
    }
    sceneTypeToEnhanceChainMap_.erase(sceneTypeAndDeviceKey);
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::ApplyAudioEnhanceChain(std::string &sceneType,
    std::string &upAndDownDevice, BufferAttr *bufferAttr)
{
    std::lock_guard<std::mutex> lock(chainMutex_);
    std::string sceneTypeAndDeviceKey = sceneType + "_&_" + upAndDownDevice;
    if (!sceneTypeToEnhanceChainMap_.count(sceneTypeAndDeviceKey)) {
        CopyBufferEnhance(bufferAttr->bufIn, bufferAttr->bufOut, bufferAttr->frameLen * bufferAttr->numChanIn);
        return ERROR;
    }
    AudioEnhanceChain *audioEnhanceChain = sceneTypeToEnhanceChainMap_[sceneTypeAndDeviceKey];
    audioEnhanceChain->ApplyEnhanceChain(bufferAttr->bufIn, bufferAttr->bufOut, bufferAttr->frameLen);
    return SUCCESS;
}

std::string AudioEnhanceChainManager::GetUpAndDownDevice()
{
    return upAndDownDevice_;
}

} // namespace AudioStandard
} // namespace OHOS