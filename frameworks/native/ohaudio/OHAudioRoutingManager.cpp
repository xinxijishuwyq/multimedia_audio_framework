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

#include "OHAudioRoutingManager.h"

using OHOS::AudioStandard::OHAudioRoutingManager;
using OHOS::AudioStandard::OHAudioDeviceDescriptor;
using OHOS::AudioStandard::AudioRoutingManager;
using OHOS::AudioStandard::DeviceFlag;

static OHOS::AudioStandard::OHAudioRoutingManager *convertManager(OH_AudioRoutingManager* manager)
{
    return (OHAudioRoutingManager*) manager;
}

OH_AudioCommon_Result OH_AudioManager_GetAudioRoutingManager(OH_AudioRoutingManager **audioRoutingManager)
{
    OHAudioRoutingManager* ohAudioRoutingManager = OHAudioRoutingManager::GetInstance();
    *audioRoutingManager = (OH_AudioRoutingManager*)ohAudioRoutingManager;
    return AUDIOCOMMON_RESULT_SUCCESS;
}

OH_AudioCommon_Result OH_AudioRoutingManager_GetDevices(OH_AudioRoutingManager *audioRoutingManager,
    OH_AudioDevice_Flag deviceFlag, OH_AudioDeviceDescriptorArray **audioDeviceDescriptorArray)
{
    OHAudioRoutingManager* ohAudioRoutingManager = convertManager(audioRoutingManager);
    CHECK_AND_RETURN_RET_LOG(ohAudioRoutingManager != nullptr,
        AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM, "ohAudioRoutingManager is nullptr");
    CHECK_AND_RETURN_RET_LOG((
        deviceFlag == AUDIO_DEVICE_FLAG_NONE ||
        deviceFlag == AUDIO_DEVICE_FLAG_OUTPUT ||
        deviceFlag == AUDIO_DEVICE_FLAG_INPUT ||
        deviceFlag == AUDIO_DEVICE_FLAG_ALL),
        AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM, "deviceFlag is invalid");
    CHECK_AND_RETURN_RET_LOG(audioDeviceDescriptorArray != nullptr,
        AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM, "audioDeviceDescriptorArray is nullptr");
    DeviceFlag flag = static_cast<DeviceFlag>(deviceFlag);
    *audioDeviceDescriptorArray = ohAudioRoutingManager->GetDevices(flag);
    CHECK_AND_RETURN_RET_LOG(*audioDeviceDescriptorArray != nullptr,
        AUDIOCOMMON_RESULT_ERROR_NO_MEMORY, "*audioDeviceDescriptorArray is nullptr");
    return AUDIOCOMMON_RESULT_SUCCESS;
}

OH_AudioCommon_Result OH_AudioRoutingManager_RegisterDeviceChangeCallback(
    OH_AudioRoutingManager *audioRoutingManager, OH_AudioDevice_Flag deviceFlag,
    OH_AudioRoutingManager_OnDeviceChangedCallback callback)
{
    OHAudioRoutingManager* ohAudioRoutingManager = convertManager(audioRoutingManager);
    CHECK_AND_RETURN_RET_LOG(ohAudioRoutingManager != nullptr,
        AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM, "audioRoutingManager is nullptr");
    CHECK_AND_RETURN_RET_LOG((
        deviceFlag == AUDIO_DEVICE_FLAG_NONE ||
        deviceFlag == AUDIO_DEVICE_FLAG_OUTPUT ||
        deviceFlag == AUDIO_DEVICE_FLAG_INPUT ||
        deviceFlag == AUDIO_DEVICE_FLAG_ALL),
        AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM, "deviceFlag is invalid");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM, "callback is nullptr");
    DeviceFlag flag = static_cast<DeviceFlag>(deviceFlag);
    ohAudioRoutingManager->SetDeviceChangeCallback(flag, callback);
    return AUDIOCOMMON_RESULT_SUCCESS;
}

OH_AudioCommon_Result OH_AudioRoutingManager_UnregisterDeviceChangeCallback(
    OH_AudioRoutingManager *audioRoutingManager,
    OH_AudioRoutingManager_OnDeviceChangedCallback callback)
{
    OHAudioRoutingManager* ohAudioRoutingManager = convertManager(audioRoutingManager);
    CHECK_AND_RETURN_RET_LOG(ohAudioRoutingManager != nullptr,
        AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM, "audioRoutingManager is nullptr");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM, "callback is nullptr");
    DeviceFlag flag = static_cast<DeviceFlag>(AUDIO_DEVICE_FLAG_ALL);
    ohAudioRoutingManager->UnsetDeviceChangeCallback(flag, callback);
    return AUDIOCOMMON_RESULT_SUCCESS;
}

OH_AudioCommon_Result OH_AudioRoutingManager_ReleaseDevices(
    OH_AudioRoutingManager *audioRoutingManager,
    OH_AudioDeviceDescriptorArray *audioDeviceDescriptorArray)
{
    OHAudioRoutingManager* ohAudioRoutingManager = convertManager(audioRoutingManager);
    CHECK_AND_RETURN_RET_LOG(ohAudioRoutingManager != nullptr,
        AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM, "audioRoutingManager is nullptr");
    CHECK_AND_RETURN_RET_LOG(audioDeviceDescriptorArray != nullptr,
        AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM, "audioDeviceDescriptorArray is nullptr");
    if (audioDeviceDescriptorArray == nullptr) {
        return AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM;
    }
    for (uint32_t index = 0; index < audioDeviceDescriptorArray->size; index++) {
        OHAudioDeviceDescriptor* ohAudioDeviceDescriptor =
            (OHAudioDeviceDescriptor*)audioDeviceDescriptorArray->descriptors[index];
        delete ohAudioDeviceDescriptor;
        audioDeviceDescriptorArray->descriptors[index] = nullptr;
    }
    free(audioDeviceDescriptorArray->descriptors);
    audioDeviceDescriptorArray->descriptors = nullptr;
    free(audioDeviceDescriptorArray);
    audioDeviceDescriptorArray = nullptr;
    return AUDIOCOMMON_RESULT_SUCCESS;
}

namespace OHOS {
namespace AudioStandard {

void DestroyAudioDeviceDescriptor(OH_AudioDeviceDescriptorArray *array)
{
    if (array) {
        for (uint32_t index = 0; index < array->size; index++) {
            OHAudioDeviceDescriptor* ohAudioDeviceDescriptor = (OHAudioDeviceDescriptor*)array->descriptors[index];
            delete ohAudioDeviceDescriptor;
            array->descriptors[index] = nullptr;
        }
        free(array->descriptors);
        free(array);
    }
}

OHAudioRoutingManager::OHAudioRoutingManager()
{
    AUDIO_INFO_LOG("OHAudioRoutingManager created!");
}

OHAudioRoutingManager::~OHAudioRoutingManager()
{
    AUDIO_INFO_LOG("OHAudioRoutingManager destroyed!");
}

OH_AudioDeviceDescriptorArray* OHAudioRoutingManager::GetDevices(DeviceFlag deviceFlag)
{
    CHECK_AND_RETURN_RET_LOG(audioSystemManager_ != nullptr,
        nullptr, "failed, audioSystemManager is null");
    std::vector<sptr<AudioDeviceDescriptor>> audioDeviceDescriptors = audioSystemManager_->GetDevices(deviceFlag);
    uint32_t size = audioDeviceDescriptors.size();
    if (size <= 0) {
        AUDIO_ERR_LOG("audioDeviceDescriptors is null");
        return nullptr;
    }
    OH_AudioDeviceDescriptorArray *audioDeviceDescriptorArray =
        (OH_AudioDeviceDescriptorArray *)malloc(sizeof(OH_AudioDeviceDescriptorArray));
    
    if (audioDeviceDescriptorArray) {
        audioDeviceDescriptorArray->descriptors =
            (OH_AudioDeviceDescriptor**)malloc(sizeof(OH_AudioDeviceDescriptor*) * size);
        if (audioDeviceDescriptorArray->descriptors == nullptr) {
            return nullptr;
        }
        audioDeviceDescriptorArray->size = size;
        uint32_t index = 0;
        for (auto deviceDescriptor : audioDeviceDescriptors) {
            audioDeviceDescriptorArray->descriptors[index] =
                (OH_AudioDeviceDescriptor *)(new OHAudioDeviceDescriptor(deviceDescriptor));
            if (audioDeviceDescriptorArray->descriptors[index] == nullptr) {
                DestroyAudioDeviceDescriptor(audioDeviceDescriptorArray);
                return nullptr;
            }
            index++;
        }
        return audioDeviceDescriptorArray;
    }
    return nullptr;
}

OH_AudioCommon_Result OHAudioRoutingManager::SetDeviceChangeCallback(const DeviceFlag deviceFlag,
    OH_AudioRoutingManager_OnDeviceChangedCallback callback)
{
    CHECK_AND_RETURN_RET_LOG(audioSystemManager_ != nullptr,
        AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM, "failed, audioSystemManager is null");
    std::shared_ptr<OHAudioDeviceChangedCallback> ohAudioOnDeviceChangedCallback =
        std::make_shared<OHAudioDeviceChangedCallback>(callback);
    if (ohAudioOnDeviceChangedCallback) {
        audioSystemManager_->SetDeviceChangeCallback(deviceFlag, ohAudioOnDeviceChangedCallback);
        ohAudioOnDeviceChangedCallbackArray_.push_back(ohAudioOnDeviceChangedCallback);
        return AUDIOCOMMON_RESULT_SUCCESS;
    }
    return AUDIOCOMMON_RESULT_ERROR_NO_MEMORY;
}

OH_AudioCommon_Result OHAudioRoutingManager::UnsetDeviceChangeCallback(DeviceFlag deviceFlag,
    OH_AudioRoutingManager_OnDeviceChangedCallback ohOnDeviceChangedcallback)
{
    CHECK_AND_RETURN_RET_LOG(audioSystemManager_ != nullptr,
        AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM, "failed, audioSystemManager is null");
    auto iter = std::find_if(ohAudioOnDeviceChangedCallbackArray_.begin(), ohAudioOnDeviceChangedCallbackArray_.end(),
        [&](const std::shared_ptr<OHAudioDeviceChangedCallback> &item) {
        return item->GetCallback() == ohOnDeviceChangedcallback;
    });
    if (iter == ohAudioOnDeviceChangedCallbackArray_.end()) {
        return AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM;
    }
    audioSystemManager_->UnsetDeviceChangeCallback(deviceFlag);
    ohAudioOnDeviceChangedCallbackArray_.erase(iter);
    return AUDIOCOMMON_RESULT_SUCCESS;
}

void OHAudioDeviceChangedCallback::OnDeviceChange(const DeviceChangeAction &deviceChangeAction)
{
    CHECK_AND_RETURN_LOG(callback_ != nullptr, "failed, pointer to the fuction is nullptr");
    OH_AudioDevice_ChangeType type = static_cast<OH_AudioDevice_ChangeType>(deviceChangeAction.type);
    uint32_t size = deviceChangeAction.deviceDescriptors.size();
    if (size <= 0) {
        AUDIO_ERR_LOG("audioDeviceDescriptors is null");
        return;
    }
    
    OH_AudioDeviceDescriptorArray *audioDeviceDescriptorArray =
        (OH_AudioDeviceDescriptorArray *)malloc(sizeof(OH_AudioDeviceDescriptorArray));
    if (audioDeviceDescriptorArray) {
        audioDeviceDescriptorArray->descriptors =
            (OH_AudioDeviceDescriptor**)malloc(sizeof(OH_AudioDeviceDescriptor*) * size);
        CHECK_AND_RETURN_LOG(audioDeviceDescriptorArray->descriptors != nullptr, "failed to malloc descriptors.");
        audioDeviceDescriptorArray->size = size;
        uint32_t index = 0;
        for (auto deviceDescriptor : deviceChangeAction.deviceDescriptors) {
            audioDeviceDescriptorArray->descriptors[index] =
                (OH_AudioDeviceDescriptor *)(new OHAudioDeviceDescriptor(deviceDescriptor));
            if (audioDeviceDescriptorArray->descriptors[index] == nullptr) {
                DestroyAudioDeviceDescriptor(audioDeviceDescriptorArray);
                return;
            }
            index++;
        }
    }
    callback_(type, audioDeviceDescriptorArray);
}
}  // namespace AudioStandard
}  // namespace OHOS