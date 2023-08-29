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

#include "audio_device_manager_impl.h"

#include <dlfcn.h>

#include "audio_device_adapter_impl.h"
#include "audio_errors.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
AudioDeviceManagerFactory& AudioDeviceManagerFactory::GetInstance()
{
    static AudioDeviceManagerFactory devMgr;
    return devMgr;
}

int32_t AudioDeviceManagerFactory::DestoryDeviceManager(const AudioDeviceManagerType audioMgrType)
{
    std::lock_guard<std::mutex> lock(devMgrFactoryMtx_);
    if (allHdiDevMgr_.find(audioMgrType) == allHdiDevMgr_.end()) {
        AUDIO_INFO_LOG("Audio manager is already destoried, audioMgrType %{publid}d.", audioMgrType);
        return SUCCESS;
    }

    int32_t ret = allHdiDevMgr_[audioMgrType]->Release();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Audio manager is busy, audioMgrType %{publid}d.", audioMgrType);

    allHdiDevMgr_.erase(audioMgrType);
    return SUCCESS;
}

std::shared_ptr<IAudioDeviceManager> AudioDeviceManagerFactory::CreatDeviceManager(
    const AudioDeviceManagerType audioMgrType)
{
    std::lock_guard<std::mutex> lock(devMgrFactoryMtx_);
    if (allHdiDevMgr_.find(audioMgrType) != allHdiDevMgr_.end()) {
        return allHdiDevMgr_[audioMgrType];
    }
    std::shared_ptr<IAudioDeviceManager> audioDevMgr = nullptr;
    switch (audioMgrType) {
        case LOCAL_DEV_MGR:
            audioDevMgr = InitLocalAudioMgr();
            break;
        case REMOTE_DEV_MGR:
            audioDevMgr = InitRemoteAudioMgr();
            break;
        case BLUETOOTH_DEV_MGR:
            audioDevMgr = InitBluetoothAudioMgr();
            break;
        default:
            AUDIO_ERR_LOG("Get audio manager of audioMgrType %{publid}d is not supported.", audioMgrType);
            return nullptr;
    }

    if (audioDevMgr == nullptr) {
        AUDIO_ERR_LOG("Get audio manager of audioMgrType %{publid}d fail.", audioMgrType);
        return nullptr;
    }
    allHdiDevMgr_[audioMgrType] = audioDevMgr;
    return allHdiDevMgr_[audioMgrType];
}

std::shared_ptr<IAudioDeviceManager> AudioDeviceManagerFactory::InitLocalAudioMgr()
{
    AUDIO_ERR_LOG("Get local audio manager is not supported.");
    return nullptr;
}

std::shared_ptr<IAudioDeviceManager> AudioDeviceManagerFactory::InitRemoteAudioMgr()
{
    AUDIO_INFO_LOG("AudioDeviceManagerFactory: Init remote audio manager proxy.");
#ifdef FEATURE_DISTRIBUTE_AUDIO
#ifdef __aarch64__
    char resolvedPath[100] = "/system/lib64/libdaudio_client.z.so";
#else
    char resolvedPath[100] = "/system/lib/libdaudio_client.z.so";
#endif
    struct AudioManager *(*GetAudioManagerFuncs)() = nullptr;

    void *handle_ = dlopen(resolvedPath, RTLD_LAZY);
    if (handle_ == nullptr) {
        AUDIO_ERR_LOG("Open daudio client shared object fail, resolvedPath [%{public}s].", resolvedPath);
        return nullptr;
    }

    GetAudioManagerFuncs = (struct AudioManager *(*)())(dlsym(handle_, "GetAudioManagerFuncs"));
    if (GetAudioManagerFuncs == nullptr) {
        AUDIO_ERR_LOG("Link the GetAudioManagerFuncs symbol in daudio client fail.");
        return nullptr;
    }

    struct AudioManager *audioMgr = GetAudioManagerFuncs();
#else
    struct AudioManager *audioMgr = nullptr;
#endif // FEATURE_DISTRIBUTE_AUDIO

    CHECK_AND_RETURN_RET_LOG((audioMgr != nullptr), nullptr, "Init remote audio manager proxy fail.");
    AUDIO_INFO_LOG("Init remote hdi manager proxy success.");
    return std::make_shared<AudioDeviceManagerImpl>(REMOTE_DEV_MGR, audioMgr);
}

std::shared_ptr<IAudioDeviceManager> AudioDeviceManagerFactory::InitBluetoothAudioMgr()
{
    AUDIO_ERR_LOG("Get local audio manager is not supported.");
    return nullptr;
}

int32_t AudioDeviceManagerImpl::GetAllAdapters(AudioAdapterDescriptor **descs, int32_t *size)
{
    CHECK_AND_RETURN_RET_LOG((audioMgr_ != nullptr), ERR_INVALID_HANDLE,
        "GetAllAdapters: Audio manager is null, audioMgrType %{publid}d.", audioMgrType_);
    int32_t ret = audioMgr_->GetAllAdapters(audioMgr_, descs, size);
    if (ret != 0 || size == 0 || descs == nullptr) {
        AUDIO_ERR_LOG("Audio manager proxy get all adapters fail, ret %{publid}d.", ret);
        return ERR_OPERATION_FAILED;
    }
    return SUCCESS;
}

struct AudioAdapterDescriptor *AudioDeviceManagerImpl::GetTargetAdapterDesc(const std::string &adapterName,
    bool isMmap)
{
    int32_t size = 0;
    struct AudioAdapterDescriptor *descs = nullptr;
    int32_t ret = GetAllAdapters(&descs, &size);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, nullptr,
        "Get all adapters fail, audioMgrType %{publid}d, ret %{publid}d.", audioMgrType_, ret);
    AUDIO_INFO_LOG("Get audioMgrType total adapters num: %{publid}d, audioMgrType %{publid}d, isMmap %{publid}d.",
        size, audioMgrType_, isMmap);

    int32_t targetIdx = INVALID_INDEX;
    for (int32_t index = 0; index < size; index++) {
        struct AudioAdapterDescriptor *desc = &descs[index];
        if (desc == nullptr || desc->adapterName == nullptr) {
            continue;
        }
        if (adapterName.compare(desc->adapterName)) {
            AUDIO_INFO_LOG("[%{public}d] is not target adapter", index);
            continue;
        }
        targetIdx = index;
        if (desc->portNum <= 0) {
            AUDIO_WARNING_LOG("The ports number of audio adapter %{public}d is %{public}d.", index, desc->portNum);
        }
    }
    CHECK_AND_RETURN_RET_LOG((targetIdx >= 0), nullptr, "can not find target adapter.");
    return &descs[targetIdx];
}

std::shared_ptr<IAudioDeviceAdapter> AudioDeviceManagerImpl::LoadAdapters(const std::string &adapterName, bool isMmap)
{
    struct AudioAdapterDescriptor *desc = GetTargetAdapterDesc(adapterName, isMmap);
    CHECK_AND_RETURN_RET_LOG(desc != nullptr, nullptr, "can not find target adapter.");

    std::lock_guard<std::mutex> lock(mtx_);
    if (enableAdapters_.find(std::string(desc->adapterName)) != enableAdapters_.end()) {
        AUDIO_INFO_LOG("LoadAdapters: Audio adapter already inited.");
        return enableAdapters_[std::string(desc->adapterName)].devAdp;
    }

    CHECK_AND_RETURN_RET_LOG((audioMgr_ != nullptr), nullptr,
        "LoadAdapters: Audio manager is null, audioMgrType %{publid}d.", audioMgrType_);
    struct AudioAdapter *audioAdapter = nullptr;
    int32_t ret = audioMgr_->LoadAdapter(audioMgr_, desc, &audioAdapter);
    if (ret != 0 || audioAdapter == nullptr) {
        AUDIO_ERR_LOG("Load audio adapter fail, audioMgrType %{publid}d, ret %{publid}d.", audioMgrType_, ret);
        return nullptr;
    }
    auto audioDevAdp = std::make_shared<AudioDeviceAdapterImpl>(std::string(desc->adapterName), audioAdapter);
    ret = audioDevAdp->Init();
    CHECK_AND_RETURN_RET_LOG((ret == SUCCESS), nullptr, "LoadAdapters: Init all ports fail, ret %{publid}d.", ret);

    DeviceAdapterInfo adpInfo = {audioDevAdp, audioAdapter};
    enableAdapters_[std::string(desc->adapterName)] = adpInfo;
    AUDIO_INFO_LOG("Load adapter end.");
    return audioDevAdp;
}

int32_t AudioDeviceManagerImpl::UnloadAdapter(const std::string &adapterName)
{
    AUDIO_INFO_LOG("Unload adapter, audioMgrType %{publid}d.", audioMgrType_);
    std::lock_guard<std::mutex> lock(mtx_);
    if (enableAdapters_.find(adapterName) == enableAdapters_.end()) {
        AUDIO_INFO_LOG("Adapter is already unloaded.");
        return SUCCESS;
    }

    auto adpInfo = enableAdapters_[adapterName];
    CHECK_AND_RETURN_RET_LOG((audioMgr_ != nullptr && adpInfo.audioAdapter != nullptr), ERR_INVALID_HANDLE,
        "UnloadAdapter: Audio manager or audio adapter is null, audioMgrType %{publid}d.", audioMgrType_);
    if (adpInfo.devAdp != nullptr) {
        int32_t ret = adpInfo.devAdp->Release();
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Adapter release fail, ret %{publid}d.", ret);
        AUDIO_DEBUG_LOG("Device adapter release OK, audioMgrType %{publid}d.", audioMgrType_);
    }

    audioMgr_->UnloadAdapter(audioMgr_, adpInfo.audioAdapter);
    enableAdapters_.erase(adapterName);
    return SUCCESS;
}

int32_t AudioDeviceManagerImpl::Release()
{
    AUDIO_INFO_LOG("Release enter.");
    std::lock_guard<std::mutex> lock(mtx_);
    if (!enableAdapters_.empty()) {
        AUDIO_ERR_LOG("Audio manager has some adapters busy, audioMgrType %{publid}d.", audioMgrType_);
        return ERR_ILLEGAL_STATE;
    }

    audioMgr_ = nullptr;
    return SUCCESS;
}
}  // namespace AudioStandard
}  // namespace OHOS