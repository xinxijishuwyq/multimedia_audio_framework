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
#include "audio_device_manager.h"

#include "audio_errors.h"
#include "audio_log.h"
#include "audio_utils.h"
#include "audio_device_parser.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;
constexpr int32_t MS_PER_S = 1000;
constexpr int32_t NS_PER_MS = 1000000;

static int64_t GetCurrentTimeMS()
{
    timespec tm {};
    clock_gettime(CLOCK_MONOTONIC, &tm);
    return tm.tv_sec * MS_PER_S + (tm.tv_nsec / NS_PER_MS);
}

void AudioDeviceManager::ParseDeviceXml()
{
    unique_ptr<AudioDeviceParser> audioDeviceParser = make_unique<AudioDeviceParser>(this);
    if (audioDeviceParser->LoadConfiguration()) {
        AUDIO_INFO_LOG("Audio device manager load configuration successfully.");
        audioDeviceParser->Parse();
    }
}

void AudioDeviceManager::OnXmlParsingCompleted(
    const unordered_map<AudioDevicePrivacyType, list<DevicePrivacyInfo>> &xmlData)
{
    if (xmlData.empty()) {
        AUDIO_ERR_LOG("Failed to parse xml file.");
        return;
    }

    devicePrivacyMaps_ = xmlData;

    auto privacyDevices = devicePrivacyMaps_.find(AudioDevicePrivacyType::TYPE_PRIVACY);
    if (privacyDevices != devicePrivacyMaps_.end()) {
        privacyDeviceList_ = privacyDevices->second;
    }

    auto publicDevices = devicePrivacyMaps_.find(AudioDevicePrivacyType::TYPE_PUBLIC);
    if (publicDevices != devicePrivacyMaps_.end()) {
        publicDeviceList_ = publicDevices->second;
    }
}

bool AudioDeviceManager::DeviceAttrMatch(const shared_ptr<AudioDeviceDescriptor> &devDesc,
    AudioDevicePrivacyType &privacyType, DeviceRole &devRole, DeviceUsage &devUsage)
{
    list<DevicePrivacyInfo> deviceList;
    if (privacyType == TYPE_PRIVACY) {
        deviceList = privacyDeviceList_;
    } else if (privacyType == TYPE_PUBLIC) {
        deviceList = publicDeviceList_;
    } else {
        return false;
    }

    if (devDesc->connectState_ == VIRTUAL_CONNECTED) {
        return false;
    }

    for (auto &devInfo : deviceList) {
        if ((devInfo.deviceType == devDesc->deviceType_) &&
            ((devRole == devDesc->deviceRole_) && ((devInfo.deviceRole & devRole) != 0)) &&
            ((devInfo.deviceUsage & devUsage) != 0) &&
            ((devInfo.deviceCategory == devDesc->deviceCategory_) ||
            ((devInfo.deviceCategory & devDesc->deviceCategory_) != 0))) {
            return true;
        }
    }

    return false;
}

void AudioDeviceManager::FillArrayWhenDeviceAttrMatch(const shared_ptr<AudioDeviceDescriptor> &devDesc,
    AudioDevicePrivacyType privacyType, DeviceRole devRole, DeviceUsage devUsage, string logName,
    vector<shared_ptr<AudioDeviceDescriptor>> &descArray)
{
    bool result = DeviceAttrMatch(devDesc, privacyType, devRole, devUsage);
    if (result) {
        AUDIO_INFO_LOG("Add to %{public}s list.", logName.c_str());
        descArray.push_back(devDesc);
    }
}

void AudioDeviceManager::AddRemoteRenderDev(const shared_ptr<AudioDeviceDescriptor> &devDesc)
{
    if (devDesc->networkId_ != LOCAL_NETWORK_ID && devDesc->deviceRole_ == DeviceRole::OUTPUT_DEVICE) {
        remoteRenderDevices_.push_back(devDesc);
    }
}

void AudioDeviceManager::AddRemoteCaptureDev(const shared_ptr<AudioDeviceDescriptor> &devDesc)
{
    if (devDesc->networkId_ != LOCAL_NETWORK_ID && devDesc->deviceRole_ == DeviceRole::INPUT_DEVICE) {
        remoteCaptureDevices_.push_back(devDesc);
    }
}

void AudioDeviceManager::MakePairedDeviceDescriptor(const shared_ptr<AudioDeviceDescriptor> &devDesc,
    DeviceRole devRole)
{
    auto isPresent = [&devDesc, &devRole] (const shared_ptr<AudioDeviceDescriptor> &desc) {
        if (desc->networkId_ != devDesc->networkId_ || desc->deviceRole_ != devRole) {
            return false;
        }
        if (devDesc->macAddress_ != "" && devDesc->macAddress_ == desc->macAddress_) {
            return true;
        } else {
            return (desc->deviceType_ == devDesc->deviceType_);
        }
    };

    auto it = find_if(connectedDevices_.begin(), connectedDevices_.end(), isPresent);
    if (it != connectedDevices_.end()) {
        devDesc->pairDeviceDescriptor_ = *it;
        (*it)->pairDeviceDescriptor_ = devDesc;
    }
}

void AudioDeviceManager::MakePairedDeviceDescriptor(const shared_ptr<AudioDeviceDescriptor> &devDesc)
{
    if (devDesc->deviceRole_ == INPUT_DEVICE) {
        MakePairedDeviceDescriptor(devDesc, OUTPUT_DEVICE);
    } else if (devDesc->deviceRole_ == OUTPUT_DEVICE) {
        MakePairedDeviceDescriptor(devDesc, INPUT_DEVICE);
    }
}

void AudioDeviceManager::AddConnectedDevices(const shared_ptr<AudioDeviceDescriptor> &devDesc)
{
    connectedDevices_.insert(connectedDevices_.begin(), devDesc);
}

void AudioDeviceManager::RemoveConnectedDevices(const shared_ptr<AudioDeviceDescriptor> &devDesc)
{
    auto isPresent = [&devDesc](const shared_ptr<AudioDeviceDescriptor> &descriptor) {
        if (descriptor->deviceType_ == devDesc->deviceType_ &&
            descriptor->networkId_ == devDesc->networkId_) {
            if (descriptor->deviceType_ != DEVICE_TYPE_BLUETOOTH_A2DP) {
                return true;
            } else {
                // if the disconnecting device is A2DP, need to compare mac address in addition.
                return descriptor->macAddress_ == devDesc->macAddress_;
            }
        }
        return false;
    };

    for (auto it = connectedDevices_.begin(); it != connectedDevices_.end();) {
        it = find_if(it, connectedDevices_.end(), isPresent);
        if (it != connectedDevices_.end()) {
            if ((*it)->pairDeviceDescriptor_ != nullptr) {
                (*it)->pairDeviceDescriptor_->pairDeviceDescriptor_ = nullptr;
            }
            it = connectedDevices_.erase(it);
        }
    }
}

void AudioDeviceManager::AddDefaultDevices(const sptr<AudioDeviceDescriptor> &devDesc)
{
    DeviceType devType = devDesc->deviceType_;
    if (devType == DEVICE_TYPE_EARPIECE) {
        earpiece_ = devDesc;
    } else if (devType == DEVICE_TYPE_SPEAKER) {
        speaker_ = devDesc;
    } else if (devType == DEVICE_TYPE_MIC) {
        defalutMic_ = devDesc;
    }
}

void AudioDeviceManager::UpdateDeviceInfo(shared_ptr<AudioDeviceDescriptor> &deviceDesc)
{
    if (deviceDesc->connectTimeStamp_ == 0) {
        deviceDesc->connectTimeStamp_ = GetCurrentTimeMS();
    }
    MakePairedDeviceDescriptor(deviceDesc);
}

void AudioDeviceManager::AddCommunicationDevices(const shared_ptr<AudioDeviceDescriptor> &devDesc)
{
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PRIVACY, OUTPUT_DEVICE, VOICE, "communication render privacy device",
        commRenderPrivacyDevices_);
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PUBLIC, OUTPUT_DEVICE, VOICE, "communication render public device",
        commRenderPublicDevices_);
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PRIVACY, INPUT_DEVICE, VOICE, "communication capture privacy device",
        commCapturePrivacyDevices_);
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PUBLIC, INPUT_DEVICE, VOICE, "communication capture public device",
        commCapturePublicDevices_);
}

void AudioDeviceManager::AddMediaDevices(const shared_ptr<AudioDeviceDescriptor> &devDesc)
{
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PRIVACY, OUTPUT_DEVICE, MEDIA, "media render privacy device",
        mediaRenderPrivacyDevices_);
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PUBLIC, OUTPUT_DEVICE, MEDIA, "media render public device",
        mediaRenderPublicDevices_);
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PRIVACY, INPUT_DEVICE, MEDIA, "media capture privacy device",
        mediaCapturePrivacyDevices_);
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PUBLIC, INPUT_DEVICE, MEDIA, "media capture public device",
        mediaCapturePublicDevices_);
}

void AudioDeviceManager::AddCaptureDevices(const shared_ptr<AudioDeviceDescriptor> &devDesc)
{
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PRIVACY, INPUT_DEVICE, ALL_USAGE, "capture privacy device",
        capturePrivacyDevices_);
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PUBLIC, INPUT_DEVICE, ALL_USAGE, "capture public device",
        capturePublicDevices_);
}

void AudioDeviceManager::HandleScoWithDefaultCategory(const shared_ptr<AudioDeviceDescriptor> &devDesc)
{
    if (devDesc->deviceType_ == DEVICE_TYPE_BLUETOOTH_SCO && devDesc->deviceCategory_ == CATEGORY_DEFAULT) {
        if (devDesc->deviceRole_ == INPUT_DEVICE) {
            commCapturePrivacyDevices_.push_back(devDesc);
        } else if (devDesc->deviceRole_ == OUTPUT_DEVICE) {
            commRenderPrivacyDevices_.push_back(devDesc);
        }
    }
}

bool AudioDeviceManager::UpdateExistDeviceDescriptor(const sptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    auto isPresent = [&deviceDescriptor](const shared_ptr<AudioDeviceDescriptor> &descriptor) {
        if (descriptor->deviceType_ == deviceDescriptor->deviceType_ &&
            descriptor->networkId_ == deviceDescriptor->networkId_ &&
            descriptor->deviceRole_ == deviceDescriptor->deviceRole_) {
            if (descriptor->deviceType_ != DEVICE_TYPE_BLUETOOTH_A2DP &&
                descriptor->deviceType_ != DEVICE_TYPE_BLUETOOTH_SCO) {
                return true;
            } else {
                // if the disconnecting device is A2DP, need to compare mac address in addition.
                return descriptor->macAddress_ == deviceDescriptor->macAddress_;
            }
        }
        return false;
    };

    auto iter = std::find_if(connectedDevices_.begin(), connectedDevices_.end(), isPresent);
    if (iter != connectedDevices_.end()) {
        if ((deviceDescriptor->deviceType_ == DEVICE_TYPE_BLUETOOTH_A2DP ||
            deviceDescriptor->deviceType_ == DEVICE_TYPE_BLUETOOTH_SCO) &&
            (*iter)->deviceCategory_ != deviceDescriptor->deviceCategory_) {
            AUDIO_INFO_LOG("A2DP device category changed,RemoveConnectedDevices");
            RemoveNewDevice(deviceDescriptor);
            return false;
        }
        **iter = deviceDescriptor;
        UpdateDeviceInfo(*iter);
        return true;
    }
    return false;
}

void AudioDeviceManager::AddNewDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    shared_ptr<AudioDeviceDescriptor> devDesc = make_shared<AudioDeviceDescriptor>(deviceDescriptor);
    CHECK_AND_RETURN_LOG(devDesc != nullptr, "Memory allocation failed");

    if (UpdateExistDeviceDescriptor(deviceDescriptor)) {
        AUDIO_INFO_LOG("The device has been added and will not be added again.");
        return;
    }

    UpdateDeviceInfo(devDesc);
    AddConnectedDevices(devDesc);

    if (devDesc->networkId_ != LOCAL_NETWORK_ID) {
        AddRemoteRenderDev(devDesc);
        AddRemoteCaptureDev(devDesc);
    } else {
        HandleScoWithDefaultCategory(devDesc);
        AddDefaultDevices(deviceDescriptor);
        AddCommunicationDevices(devDesc);
        AddMediaDevices(devDesc);
        AddCaptureDevices(devDesc);
    }
}

void AudioDeviceManager::RemoveMatchDeviceInArray(const AudioDeviceDescriptor &devDesc, string logName,
    vector<shared_ptr<AudioDeviceDescriptor>> &descArray)
{
    auto isPresent = [&devDesc] (const shared_ptr<AudioDeviceDescriptor> &desc) {
        CHECK_AND_RETURN_RET_LOG(desc != nullptr, false, "Invalid device descriptor");
        return devDesc.deviceType_ == desc->deviceType_ && devDesc.macAddress_ == desc->macAddress_ &&
            devDesc.networkId_ == desc->networkId_;
    };

    auto itr = find_if(descArray.begin(), descArray.end(), isPresent);
    if (itr != descArray.end()) {
        AUDIO_ERR_LOG("Remove from %{public}s list.", logName.c_str());
        descArray.erase(itr);
    }
}

void AudioDeviceManager::RemoveNewDevice(const sptr<AudioDeviceDescriptor> &devDesc)
{
    RemoveConnectedDevices(make_shared<AudioDeviceDescriptor>(devDesc));
    RemoveMatchDeviceInArray(devDesc, "remote render device", remoteRenderDevices_);
    RemoveMatchDeviceInArray(devDesc, "remote capture device", remoteCaptureDevices_);

    RemoveMatchDeviceInArray(devDesc, "communication render privacy device", commRenderPrivacyDevices_);
    RemoveMatchDeviceInArray(devDesc, "communication render public device", commRenderPublicDevices_);
    RemoveMatchDeviceInArray(devDesc, "communication capture privacy device", commCapturePrivacyDevices_);
    RemoveMatchDeviceInArray(devDesc, "communication capture public device", commCapturePublicDevices_);

    RemoveMatchDeviceInArray(devDesc, "media render privacy device", mediaRenderPrivacyDevices_);
    RemoveMatchDeviceInArray(devDesc, "media render public device", mediaRenderPublicDevices_);
    RemoveMatchDeviceInArray(devDesc, "media capture privacy device", mediaCapturePrivacyDevices_);
    RemoveMatchDeviceInArray(devDesc, "media capture public device", mediaCapturePublicDevices_);

    RemoveMatchDeviceInArray(devDesc, "capture privacy device", capturePrivacyDevices_);
    RemoveMatchDeviceInArray(devDesc, "capture public device", capturePublicDevices_);
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetRemoteRenderDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (const auto &desc : remoteRenderDevices_) {
        if (desc == nullptr) {
            continue;
        }
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetRemoteCaptureDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (const auto &desc : remoteCaptureDevices_) {
        if (desc == nullptr) {
            continue;
        }
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetCommRenderPrivacyDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (const auto &desc : commRenderPrivacyDevices_) {
        if (desc == nullptr) {
            continue;
        }
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetCommRenderPublicDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (const auto &desc : commRenderPublicDevices_) {
        if (desc == nullptr) {
            continue;
        }
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetCommCapturePrivacyDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (const auto &desc : commCapturePrivacyDevices_) {
        if (desc == nullptr) {
            continue;
        }
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetCommCapturePublicDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (const auto &desc : commCapturePublicDevices_) {
        if (desc == nullptr) {
            continue;
        }
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetMediaRenderPrivacyDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (const auto &desc : mediaRenderPrivacyDevices_) {
        if (desc == nullptr) {
            continue;
        }
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetMediaRenderPublicDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (const auto &desc : mediaRenderPublicDevices_) {
        if (desc == nullptr) {
            continue;
        }
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetMediaCapturePrivacyDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (const auto &desc : mediaCapturePrivacyDevices_) {
        if (desc == nullptr) {
            continue;
        }
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetMediaCapturePublicDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (const auto &desc : mediaCapturePublicDevices_) {
        if (desc == nullptr) {
            continue;
        }
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetCapturePrivacyDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (const auto &desc : capturePrivacyDevices_) {
        if (desc == nullptr) {
            continue;
        }
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetCapturePublicDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (const auto &desc : capturePublicDevices_) {
        if (desc == nullptr) {
            continue;
        }
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

unique_ptr<AudioDeviceDescriptor> AudioDeviceManager::GetCommRenderDefaultDevice()
{
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(earpiece_);
    return devDesc;
}

unique_ptr<AudioDeviceDescriptor> AudioDeviceManager::GetRenderDefaultDevice()
{
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(speaker_);
    return devDesc;
}

unique_ptr<AudioDeviceDescriptor> AudioDeviceManager::GetCaptureDefaultDevice()
{
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(defalutMic_);
    return devDesc;
}

void AudioDeviceManager::AddAvailableDevicesByUsage(const AudioDeviceUsage usage,
    const DevicePrivacyInfo &deviceInfo, const sptr<AudioDeviceDescriptor> &dev,
    std::vector<unique_ptr<AudioDeviceDescriptor>> &audioDeviceDescriptors)
{
    switch (usage) {
        case MEDIA_OUTPUT_DEVICES:
            if ((dev->deviceRole_ & OUTPUT_DEVICE) && (deviceInfo.deviceUsage & MEDIA)) {
                audioDeviceDescriptors.push_back(make_unique<AudioDeviceDescriptor>(dev));
            }
            break;
        case MEDIA_INPUT_DEVICES:
            if ((dev->deviceRole_ & INPUT_DEVICE) && (deviceInfo.deviceUsage & MEDIA)) {
                audioDeviceDescriptors.push_back(make_unique<AudioDeviceDescriptor>(dev));
            }
            break;
        case ALL_MEDIA_DEVICES:
            if (deviceInfo.deviceUsage & MEDIA) {
                audioDeviceDescriptors.push_back(make_unique<AudioDeviceDescriptor>(dev));
            }
            break;
        case CALL_OUTPUT_DEVICES:
            if ((dev->deviceRole_ & OUTPUT_DEVICE) && (deviceInfo.deviceUsage & VOICE)) {
                audioDeviceDescriptors.push_back(make_unique<AudioDeviceDescriptor>(dev));
            }
            break;
        case CALL_INPUT_DEVICES:
            if ((dev->deviceRole_ & INPUT_DEVICE) && (deviceInfo.deviceUsage & VOICE)) {
                audioDeviceDescriptors.push_back(make_unique<AudioDeviceDescriptor>(dev));
            }
            break;
        case ALL_CALL_DEVICES:
            if (deviceInfo.deviceUsage & VOICE) {
                audioDeviceDescriptors.push_back(make_unique<AudioDeviceDescriptor>(dev));
            }
            break;
        default:
            break;
    }
}

bool AudioDeviceManager::IsExistedDevice(const sptr<AudioDeviceDescriptor> &device,
    const vector<unique_ptr<AudioDeviceDescriptor>> &audioDeviceDescriptors)
{
    bool isExistedDev = false;
    for (const auto &dev : audioDeviceDescriptors) {
        if (device->deviceType_ == dev->deviceType_ &&
            device->networkId_ == dev->networkId_ &&
            device->deviceRole_ == dev->deviceRole_ &&
            device->macAddress_ == dev->macAddress_) {
            isExistedDev = true;
        }
    }
    return isExistedDev;
}

void AudioDeviceManager::GetAvailableDevicesWithUsage(const AudioDeviceUsage usage,
    const list<DevicePrivacyInfo> &deviceInfos, const sptr<AudioDeviceDescriptor> &dev,
    vector<unique_ptr<AudioDeviceDescriptor>> &audioDeviceDescriptors)
{
    for (auto &deviceInfo : deviceInfos) {
        if (dev->deviceType_ != deviceInfo.deviceType ||
            IsExistedDevice(dev, audioDeviceDescriptors)) {
            continue;
        }
        AddAvailableDevicesByUsage(usage, deviceInfo, dev, audioDeviceDescriptors);
    }
}

void AudioDeviceManager::GetDefaultAvailableDevicesByUsage(AudioDeviceUsage usage,
    vector<unique_ptr<AudioDeviceDescriptor>> &audioDeviceDescriptors)
{
    if (((usage & MEDIA_OUTPUT_DEVICES) != 0) || ((usage & CALL_OUTPUT_DEVICES) != 0)) {
        if (speaker_ != nullptr) {
            audioDeviceDescriptors.push_back(make_unique<AudioDeviceDescriptor>(speaker_));
        }
    }

    if (((usage & MEDIA_INPUT_DEVICES) != 0) || ((usage & CALL_INPUT_DEVICES) != 0)) {
        if (defalutMic_ != nullptr) {
            audioDeviceDescriptors.push_back(make_unique<AudioDeviceDescriptor>(defalutMic_));
        }
    }

    if ((usage & CALL_OUTPUT_DEVICES) != 0) {
        if (earpiece_ != nullptr) {
            audioDeviceDescriptors.push_back(make_unique<AudioDeviceDescriptor>(earpiece_));
        }
    }
}

std::vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetAvailableDevicesByUsage(AudioDeviceUsage usage)
{
    std::vector<unique_ptr<AudioDeviceDescriptor>> audioDeviceDescriptors;

    GetDefaultAvailableDevicesByUsage(usage, audioDeviceDescriptors);
    for (const auto &dev : connectedDevices_) {
        for (const auto &devicePrivacy : devicePrivacyMaps_) {
            list<DevicePrivacyInfo> deviceInfos = devicePrivacy.second;
            sptr<AudioDeviceDescriptor> desc = new (std::nothrow) AudioDeviceDescriptor(*dev);
            GetAvailableDevicesWithUsage(usage, deviceInfos, desc, audioDeviceDescriptors);
        }
    }
    return audioDeviceDescriptors;
}

unordered_map<AudioDevicePrivacyType, list<DevicePrivacyInfo>> AudioDeviceManager::GetDevicePrivacyMaps()
{
    return devicePrivacyMaps_;
}

std::vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetAvailableBluetoothDevice(DeviceType devType,
    const std::string &macAddress)
{
    std::vector<unique_ptr<AudioDeviceDescriptor>> audioDeviceDescriptors;
    for (auto &desc : connectedDevices_) {
        if (desc->deviceType_ == devType && desc->macAddress_ == macAddress) {
            audioDeviceDescriptors.push_back(make_unique<AudioDeviceDescriptor>(*desc));
        }
    }
    return audioDeviceDescriptors;
}
}
}
