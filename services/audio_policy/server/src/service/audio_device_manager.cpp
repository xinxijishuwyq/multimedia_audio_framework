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

void AudioDeviceManager::AddRemoteRenderDev(const AudioDeviceDescriptor &devDesc)
{
    unique_ptr<AudioDeviceDescriptor> newDevice = make_unique<AudioDeviceDescriptor>(devDesc);
    if (!newDevice) {
        AUDIO_ERR_LOG("Memory allocation failed");
        return;
    }

    if (devDesc.networkId_ != LOCAL_NETWORK_ID && devDesc.deviceRole_ == DeviceRole::OUTPUT_DEVICE) {
        remoteRenderDevices_.push_back(move(newDevice));
    }
}

void AudioDeviceManager::AddRemoteCaptureDev(const AudioDeviceDescriptor &devDesc)
{
    unique_ptr<AudioDeviceDescriptor> newDevice = make_unique<AudioDeviceDescriptor>(devDesc);
    if (!newDevice) {
        AUDIO_ERR_LOG("Memory allocation failed");
        return;
    }

    if (devDesc.networkId_ != LOCAL_NETWORK_ID && devDesc.deviceRole_ == DeviceRole::INPUT_DEVICE) {
        remoteCaptureDevices_.push_back(move(newDevice));
    }
}

bool AudioDeviceManager::DeviceAttrMatch(const AudioDeviceDescriptor &devDesc, AudioDevicePrivacyType &privacyType,
    DeviceRole &devRole, DeviceUsage &devUsage)
{
    list<DevicePrivacyInfo> deviceList;
    if (privacyType == TYPE_PRIVACY) {
        deviceList = privacyDeviceList_;
    } else if (privacyType == TYPE_PUBLIC) {
        deviceList = publicDeviceList_;
    } else {
        return false;
    }

    for (auto &devInfo : deviceList) {
        if ((devInfo.deviceType == devDesc.deviceType_) &&
            ((devInfo.deviceRole & devRole) != 0) &&
            ((devInfo.deviceUsage & devUsage) != 0) &&
            ((devInfo.deviceCategory == devDesc.deviceCategory_) ||
            ((devInfo.deviceCategory & devDesc.deviceCategory_) != 0))) {
            return true;
        }
    }

    return false;
}

void AudioDeviceManager::FillArrayWhenDeviceAttrMatch(const AudioDeviceDescriptor &devDesc,
    AudioDevicePrivacyType privacyType, DeviceRole devRole, DeviceUsage devUsage, string logName,
    vector<unique_ptr<AudioDeviceDescriptor>> &descArray)
{
    bool result = DeviceAttrMatch(devDesc, privacyType, devRole, devUsage);
    if (result) {
        unique_ptr<AudioDeviceDescriptor> newDevice = make_unique<AudioDeviceDescriptor>(devDesc);
        if (!newDevice) {
            AUDIO_ERR_LOG("Memory allocation failed");
            return;
        }
        AUDIO_INFO_LOG("Add to %{public}s list.", logName.c_str());
        descArray.push_back(move(newDevice));
    }
}

void AudioDeviceManager::AddNewDevice(AudioDeviceDescriptor &devDesc)
{
    if (devDesc.connectTimeStamp_ == 0) {
        devDesc.connectTimeStamp_ = GetCurrentTimeMS();
    }

    AddRemoteRenderDev(devDesc);
    AddRemoteCaptureDev(devDesc);

    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PRIVACY, OUTPUT_DEVICE, VOICE, "communication render privacy device",
        commRenderPrivacyDevices_);
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PUBLIC, OUTPUT_DEVICE, VOICE, "communication render public device",
        commRenderPublicDevices_);
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PRIVACY, INPUT_DEVICE, VOICE, "communication capture privacy device",
        commCapturePrivacyDevices_);
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PUBLIC, INPUT_DEVICE, VOICE, "communication capture public device",
        commCapturePublicDevices_);

    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PRIVACY, OUTPUT_DEVICE, MEDIA, "media render privacy device",
        mediaRenderPrivacyDevices_);
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PUBLIC, OUTPUT_DEVICE, MEDIA, "media render public device",
        mediaRenderPublicDevices_);

    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PRIVACY, INPUT_DEVICE, MEDIA, "media capture privacy device",
        mediaCapturePrivacyDevices_);
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PUBLIC, INPUT_DEVICE, MEDIA, "media capture public device",
        mediaCapturePublicDevices_);

    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PRIVACY, INPUT_DEVICE, ALL_USAGE, "capture privacy device",
        capturePrivacyDevices_);
    FillArrayWhenDeviceAttrMatch(devDesc, TYPE_PUBLIC, INPUT_DEVICE, ALL_USAGE, "capture public device",
        capturePublicDevices_);
}

void AudioDeviceManager::RemoveMatchDeviceInArray(const AudioDeviceDescriptor &devDesc, string logName,
    vector<unique_ptr<AudioDeviceDescriptor>> &descArray)
{
    auto isPresent = [&devDesc] (const unique_ptr<AudioDeviceDescriptor> &desc) {
        CHECK_AND_RETURN_RET_LOG(desc != nullptr, false, "Invalid device descriptor");
        return devDesc.deviceType_ == desc->deviceType_ && devDesc.macAddress_ == desc->macAddress_;
    };
    
    auto itr = find_if(descArray.begin(), descArray.end(), isPresent);
    if (itr != descArray.end()) {
        AUDIO_ERR_LOG("Remove from %{public}s list.", logName.c_str());
        descArray.erase(itr);
    }
}

void AudioDeviceManager::RemoveNewDevice(const AudioDeviceDescriptor &devDesc)
{
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

void AudioDeviceManager::ParseDeviceXml()
{
    unique_ptr<AudioDeviceParser> audioDeviceParser = make_unique<AudioDeviceParser>(this);
    if (audioDeviceParser->LoadConfiguration()) {
        AUDIO_INFO_LOG("Audio device manager load configuration successfully.");
        audioDeviceParser->Parse();
    }
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetRemoteRenderDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (auto &desc : remoteRenderDevices_) {
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}


vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetRemoteCaptureDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (auto &desc : remoteCaptureDevices_) {
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetCommRenderPrivacyDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (auto &desc : commRenderPrivacyDevices_) {
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetCommRenderPublicDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (auto &desc : commRenderPublicDevices_) {
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetCommCapturePrivacyDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (auto &desc : commCapturePrivacyDevices_) {
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetCommCapturePublicDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (auto &desc : commCapturePublicDevices_) {
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetMediaRenderPrivacyDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (auto &desc : mediaRenderPrivacyDevices_) {
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetMediaRenderPublicDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (auto &desc : mediaRenderPublicDevices_) {
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetMediaCapturePrivacyDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (auto &desc : mediaCapturePrivacyDevices_) {
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetMediaCapturePublicDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (auto &desc : mediaCapturePublicDevices_) {
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetCapturePrivacyDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (auto &desc : capturePrivacyDevices_) {
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}

vector<unique_ptr<AudioDeviceDescriptor>> AudioDeviceManager::GetCapturePublicDevices()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    for (auto &desc : capturePublicDevices_) {
        descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    }
    return descs;
}
}
}
