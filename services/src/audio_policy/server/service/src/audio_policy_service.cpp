/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "audio_errors.h"
#include "audio_focus_parser.h"
#include "audio_manager_base.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "system_ability_definition.h"

#include "audio_policy_service.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;
static sptr<IStandardAudioService> g_sProxy = nullptr;

bool AudioPolicyService::Init(void)
{
    mAudioPolicyManager.Init();
    if (!mConfigParser.LoadConfiguration()) {
        MEDIA_ERR_LOG("Audio Config Load Configuration failed");
        return false;
    }
    if (!mConfigParser.Parse()) {
        MEDIA_ERR_LOG("Audio Config Parse failed");
        return false;
    }

    std::unique_ptr<AudioFocusParser> audioFocusParser;
    audioFocusParser = make_unique<AudioFocusParser>();
    std::string AUDIO_FOCUS_CONFIG_FILE = "/etc/audio/audio_interrupt_policy_config.xml";

    if (audioFocusParser->LoadConfig(focusTable_[0][0])) {
        MEDIA_ERR_LOG("Audio Interrupt Load Configuration failed");
        return false;
    }

    if (mDeviceStatusListener->RegisterDeviceStatusListener(nullptr)) {
        MEDIA_ERR_LOG("[Policy Service] Register for device status events failed");
        return false;
    }

    return true;
}

void AudioPolicyService::InitKVStore()
{
    mAudioPolicyManager.InitKVStore();
}

bool AudioPolicyService::ConnectServiceAdapter()
{
    if (!mAudioPolicyManager.ConnectServiceAdapter()) {
        MEDIA_ERR_LOG("AudioPolicyService::ConnectServiceAdapter  Error in connecting to audio service adapter");
        return false;
    }

    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        MEDIA_ERR_LOG("[Policy Service] Get samgr failed");
        return false;
    }

    sptr<IRemoteObject> object = samgr->GetSystemAbility(AUDIO_DISTRIBUTED_SERVICE_ID);
    if (object == nullptr) {
        MEDIA_DEBUG_LOG("[Policy Service] audio service remote object is NULL.");
        return false;
    }

    g_sProxy = iface_cast<IStandardAudioService>(object);
    if (g_sProxy == nullptr) {
        MEDIA_DEBUG_LOG("[Policy Service] init g_sProxy is NULL.");
        return false;
    }

    return true;
}

void AudioPolicyService::Deinit(void)
{
    mAudioPolicyManager.CloseAudioPort(mIOHandles[PRIMARY_SPEAKER]);
    mAudioPolicyManager.CloseAudioPort(mIOHandles[PRIMARY_MIC]);

    mDeviceStatusListener->UnRegisterDeviceStatusListener();
    return;
}

int32_t AudioPolicyService::SetAudioSessionCallback(AudioSessionCallback *callback)
{
    return mAudioPolicyManager.SetAudioSessionCallback(callback);
}

int32_t AudioPolicyService::SetStreamVolume(AudioStreamType streamType, float volume) const
{
    return mAudioPolicyManager.SetStreamVolume(streamType, volume);
}

float AudioPolicyService::GetStreamVolume(AudioStreamType streamType) const
{
    return mAudioPolicyManager.GetStreamVolume(streamType);
}

int32_t AudioPolicyService::SetStreamMute(AudioStreamType streamType, bool mute) const
{
    return mAudioPolicyManager.SetStreamMute(streamType, mute);
}

bool AudioPolicyService::GetStreamMute(AudioStreamType streamType) const
{
    return mAudioPolicyManager.GetStreamMute(streamType);
}

bool AudioPolicyService::IsStreamActive(AudioStreamType streamType) const
{
    return mAudioPolicyManager.IsStreamActive(streamType);
}

std::string AudioPolicyService::GetPortName(InternalDeviceType deviceType)
{
    std::string portName = PORT_NONE;
    switch (deviceType) {
        case InternalDeviceType::DEVICE_TYPE_BLUETOOTH_SCO:
            portName = PIPE_SINK;
            break;
        case InternalDeviceType::DEVICE_TYPE_SPEAKER:
            portName = PRIMARY_SPEAKER;
            break;
        case InternalDeviceType::DEVICE_TYPE_MIC:
            portName = PRIMARY_MIC;
            break;
        default:
            portName = PORT_NONE;
            break;
    }
    return portName;
}

std::vector<sptr<AudioDeviceDescriptor>> AudioPolicyService::GetDevices(DeviceFlag deviceFlag)
{
    MEDIA_INFO_LOG("Entered AudioPolicyService::%{public}s", __func__);
    std::vector<sptr<AudioDeviceDescriptor>> deviceList = {};

    if (deviceFlag < DeviceFlag::OUTPUT_DEVICES_FLAG || deviceFlag > DeviceFlag::ALL_DEVICES_FLAG) {
        MEDIA_ERR_LOG("Invalid flag provided %{public}d", deviceFlag);
        return deviceList;
    }

    if (deviceFlag == DeviceFlag::ALL_DEVICES_FLAG) {
        deviceList = mActiveDevices;
        return deviceList;
    }

    DeviceRole role = DeviceRole::OUTPUT_DEVICE;
    role = (deviceFlag == DeviceFlag::OUTPUT_DEVICES_FLAG) ? DeviceRole::OUTPUT_DEVICE : DeviceRole::INPUT_DEVICE;

    MEDIA_INFO_LOG("GetDevices mActiveDevices size = [%{public}zu]", mActiveDevices.size());
    for (auto &device : mActiveDevices) {
        if (device->deviceRole_ == role) {
            sptr<AudioDeviceDescriptor> devDesc = new(std::nothrow) AudioDeviceDescriptor(device->deviceType_,
                                                                                          device->deviceRole_);
            deviceList.push_back(devDesc);
        }
    }

    MEDIA_INFO_LOG("GetDevices list size = [%{public}zu]", deviceList.size());
    return deviceList;
}

int32_t AudioPolicyService::SetDeviceActive(InternalDeviceType deviceType, bool active)
{
    MEDIA_DEBUG_LOG("[Policy Service] Device type[%{public}d] status[%{public}d]", deviceType, active);

    if (deviceType == InternalDeviceType::DEVICE_TYPE_NONE) {
        return ERR_DEVICE_NOT_SUPPORTED;
    }

    bool updateActiveDevices = true;
    auto isPresent = [&deviceType] (const sptr<AudioDeviceDescriptor> &descriptor) {
        return descriptor->deviceType_ == deviceType;
    };

    DeviceRole role = GetDeviceRole(deviceType);
    auto deviceRole = [&role] (const sptr<AudioDeviceDescriptor> &descriptor) {
        return descriptor->deviceRole_ == role;
    };

    if (!active) {
        if (count_if(mActiveDevices.begin(), mActiveDevices.end(), deviceRole) <= 1) {
            MEDIA_ERR_LOG("[Policy Service] Only one Active device. So cannot deactivate!");
            return ERROR;
        }

        // If device is disconnected, remove the device info from active device list
        auto itr = std::find_if(mActiveDevices.begin(), mActiveDevices.end(), isPresent);
        if (itr != mActiveDevices.end()) {
            mActiveDevices.erase(itr);
        }

        deviceType = mActiveDevices.front()->deviceType_;
        updateActiveDevices = false;
    }

    int32_t result = 0;
    AudioIOHandle ioHandle = GetAudioIOHandle(deviceType);
    std::string portName = GetPortName(deviceType);
    if (portName.compare(PORT_NONE)) {
        result = mAudioPolicyManager.SetDeviceActive(ioHandle, deviceType, portName, active);
    } else {
        result = ERR_DEVICE_NOT_SUPPORTED;
    }

    if (result) {
        MEDIA_ERR_LOG("SetDeviceActive - Policy Service: returned:%{public}d", result);
        return ERROR;
    }

    sptr<AudioDeviceDescriptor> audioDescriptor = new(std::nothrow) AudioDeviceDescriptor(deviceType, role);

    // If a device is made active, bring it to the top
    if (updateActiveDevices) {
        auto itr = std::find_if(mActiveDevices.begin(), mActiveDevices.end(), isPresent);
        if (itr != mActiveDevices.end()) {
            mActiveDevices.erase(itr);
        }

        mActiveDevices.insert(mActiveDevices.begin(), audioDescriptor);
    }

    // fill device change action for callback
    std::vector<sptr<AudioDeviceDescriptor>> deviceChangeDescriptor = {};
    deviceChangeDescriptor.push_back(audioDescriptor);

    return SUCCESS;
}

bool AudioPolicyService::IsDeviceActive(InternalDeviceType deviceType) const
{
    MEDIA_INFO_LOG("Entered AudioPolicyService::%{public}s", __func__);
    bool isActive = false;

    for (const sptr<AudioDeviceDescriptor> &devDesc : mActiveDevices) {
        if (devDesc->deviceType_ == deviceType) {
            MEDIA_INFO_LOG("Device: %{public}d is present in active list", deviceType);
            isActive = true;
            break;
        }
    }

    return isActive;
}

int32_t AudioPolicyService::SetRingerMode(AudioRingerMode ringMode)
{
    return mAudioPolicyManager.SetRingerMode(ringMode);
}

AudioRingerMode AudioPolicyService::GetRingerMode() const
{
    return mAudioPolicyManager.GetRingerMode();
}

int32_t AudioPolicyService::SetAudioScene(AudioScene audioScene)
{
    if (g_sProxy == nullptr) {
        MEDIA_DEBUG_LOG("AudioPolicyService::SetAudioScene g_sProxy is nullptr");
        return ERR_OPERATION_FAILED;
    }
    list<InternalDeviceType> activeDeviceList;

    for (const sptr<AudioDeviceDescriptor> &devDesc : mActiveDevices) {
        activeDeviceList.push_front(devDesc->deviceType_);
    }

    int32_t result = g_sProxy->SetAudioScene(activeDeviceList, audioScene);
    MEDIA_INFO_LOG("SetAudioScene return value from audio HAL: %{public}d", result);
    // As Audio HAL is stubbed now, we set and return
    mAudioScene = audioScene;
    MEDIA_INFO_LOG("AudioScene is set as: %{public}d", audioScene);

    return SUCCESS;
}

AudioScene AudioPolicyService::GetAudioScene() const
{
    MEDIA_INFO_LOG("GetAudioScene return value: %{public}d", mAudioScene);
    return mAudioScene;
}

void AudioPolicyService::OnAudioPortAvailable(const AudioModuleInfo &moduleInfo)
{
    MEDIA_INFO_LOG("Port detected for [%{public}s]", moduleInfo.name.c_str());
    return;
}

bool AudioPolicyService::IsAudioInterruptEnabled() const
{
    return interruptEnabled_;
}

void AudioPolicyService::OnAudioInterruptEnable(bool enable)
{
    interruptEnabled_ = enable;
}

void AudioPolicyService::OnDeviceStatusUpdated(DeviceType devType, bool isConnected, void *privData)
{
    MEDIA_INFO_LOG("=== DEVICE STATUS CHANGED | TYPE[%{public}d] STATUS[%{public}d] ===", devType, isConnected);
    DeviceRole role = GetDeviceRole(devType);

    // fill device change action for callback
    std::vector<sptr<AudioDeviceDescriptor>> deviceChangeDescriptor = {};
    sptr<AudioDeviceDescriptor> audioDescriptor = new(std::nothrow) AudioDeviceDescriptor(devType, role);
    deviceChangeDescriptor.push_back(audioDescriptor);

    auto isPresent = [&devType, &role] (const sptr<AudioDeviceDescriptor> &descriptor) {
        return descriptor->deviceType_ == devType && descriptor->deviceRole_ == role;
    };

    // If device already in list, remove it else do not modify the list
    auto itr = std::find_if(mActiveDevices.begin(), mActiveDevices.end(), isPresent);
    if (itr != mActiveDevices.end()) {
        MEDIA_INFO_LOG("removing [%{public}d] from active list", devType);
        mActiveDevices.erase(itr);
    }

    // new device found. If connected, add into active device list
    if (isConnected) {
        MEDIA_INFO_LOG("=== DEVICE ACTIVATED === TYPE[%{public}d]|ROLE[%{public}d]", devType, role);
        mActiveDevices.insert(mActiveDevices.begin(), audioDescriptor);
    }

    TriggerDeviceChangedCallback(deviceChangeDescriptor, isConnected);
    MEDIA_INFO_LOG("output device list = [%{public}zu]", mActiveDevices.size());
}

void AudioPolicyService::OnServiceConnected()
{
    MEDIA_INFO_LOG("HDI service started: load modules");
    auto primaryModulesPos = deviceClassInfo_.find(ClassType::TYPE_PRIMARY);
    if (primaryModulesPos != deviceClassInfo_.end()) {
        auto moduleInfoList = primaryModulesPos->second;
        for (auto &moduleInfo : moduleInfoList) {
            MEDIA_INFO_LOG("Load modules: %{public}s", moduleInfo.name.c_str());
            AudioIOHandle ioHandle = mAudioPolicyManager.OpenAudioPort(moduleInfo);

            auto devType = GetDeviceType(moduleInfo.name);
            if (devType == DeviceType::DEVICE_TYPE_SPEAKER || devType == DeviceType::DEVICE_TYPE_MIC) {
                mAudioPolicyManager.SetDeviceActive(ioHandle, devType, moduleInfo.name, true);

                // add new device into active device list
                sptr<AudioDeviceDescriptor> audioDescriptor = new(std::nothrow) AudioDeviceDescriptor(devType,
                    GetDeviceRole(moduleInfo.role));
                mActiveDevices.insert(mActiveDevices.begin(), audioDescriptor);
            }

            mIOHandles[moduleInfo.name] = ioHandle;
        }
    }
}

// Parser callbacks
void AudioPolicyService::OnXmlParsingCompleted(const std::unordered_map<ClassType, std::list<AudioModuleInfo>> &xmlData)
{
    MEDIA_INFO_LOG("AudioPolicyService::%{public}s, device class num [%{public}zu]", __func__, xmlData.size());
    if (xmlData.empty()) {
        MEDIA_ERR_LOG("failed to parse xml file. Received data is empty");
        return;
    }

    deviceClassInfo_ = xmlData;
}

int32_t AudioPolicyService::SetDeviceChangeCallback(const sptr<IRemoteObject> &object)
{
    MEDIA_INFO_LOG("Entered AudioPolicyService::%{public}s", __func__);

    auto callback = iface_cast<IStandardAudioPolicyManagerListener>(object);
    if (callback != nullptr) {
        deviceChangeCallbackList_.push_back(callback);
    }

    return SUCCESS;
}

// private methods
AudioIOHandle AudioPolicyService::GetAudioIOHandle(InternalDeviceType deviceType)
{
    AudioIOHandle ioHandle;
    switch (deviceType) {
        case InternalDeviceType::DEVICE_TYPE_SPEAKER:
            ioHandle = mIOHandles[PRIMARY_SPEAKER];
            break;
        case InternalDeviceType::DEVICE_TYPE_BLUETOOTH_SCO:
            ioHandle = mIOHandles[PIPE_SINK];
            break;
        case InternalDeviceType::DEVICE_TYPE_MIC:
            ioHandle = mIOHandles[PRIMARY_MIC];
            break;
        default:
            ioHandle = mIOHandles[PRIMARY_MIC];
            break;
    }
    return ioHandle;
}

InternalDeviceType AudioPolicyService::GetDeviceType(const std::string &deviceName)
{
    if (deviceName == "Speaker")
        return InternalDeviceType::DEVICE_TYPE_SPEAKER;
    if (deviceName == "Built_in_mic")
        return InternalDeviceType::DEVICE_TYPE_MIC;

    return InternalDeviceType::DEVICE_TYPE_NONE;
}

void AudioPolicyService::TriggerDeviceChangedCallback(const vector<sptr<AudioDeviceDescriptor>> &desc, bool isConnected)
{
    DeviceChangeAction deviceChangeAction;
    deviceChangeAction.deviceDescriptors = desc;
    deviceChangeAction.type = isConnected ? DeviceChangeType::CONNECT : DeviceChangeType::DISCONNECT;

    for (auto const &deviceChangedCallback : deviceChangeCallbackList_) {
        if (deviceChangedCallback) {
            deviceChangedCallback->OnDeviceChange(deviceChangeAction);
        }
    }
}
}
}
