/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#include "bluetooth_audio_manager.h"
#include "bluetooth_device_manager.h"

namespace OHOS {
namespace Bluetooth {
using namespace AudioStandard;

const int DEFAULT_COD = -1;
const int DEFAULT_MAJOR_CLASS = -1;
const int DEFAULT_MAJOR_MINOR_CLASS = -1;
const int32_t WEAR_ENABLED = 1;
const int A2DP_DEFAULT_SELECTION = 0;
const int HFP_DEFAULT_SELECTION = 0;
const int USER_SELECTION = 1;
const std::map<std::pair<int, int>, DeviceCategory> bluetoothDeviceCategoryMap_ = {
    {std::make_pair(BluetoothDevice::MAJOR_AUDIO_VIDEO, BluetoothDevice::AUDIO_VIDEO_HEADPHONES), BT_HEADPHONE},
    {std::make_pair(BluetoothDevice::MAJOR_AUDIO_VIDEO, BluetoothDevice::AUDIO_VIDEO_WEARABLE_HEADSET), BT_HEADPHONE},
    {std::make_pair(BluetoothDevice::MAJOR_AUDIO_VIDEO, BluetoothDevice::AUDIO_VIDEO_LOUDSPEAKER), BT_SOUNDBOX},
    {std::make_pair(BluetoothDevice::MAJOR_AUDIO_VIDEO, BluetoothDevice::AUDIO_VIDEO_CAR_AUDIO), BT_CAR},
    {std::make_pair(BluetoothDevice::MAJOR_WEARABLE, BluetoothDevice::WEARABLE_GLASSES), BT_GLASSES},
    {std::make_pair(BluetoothDevice::MAJOR_WEARABLE, BluetoothDevice::WEARABLE_WRIST_WATCH), BT_WATCH},
};
IDeviceStatusObserver *g_deviceObserver = nullptr;
std::mutex g_observerLock;
std::mutex g_a2dpDeviceLock;
std::mutex g_a2dpDeviceMapLock;
std::mutex g_a2dpWearStateMapLock;
std::map<std::string, BluetoothRemoteDevice> MediaBluetoothDeviceManager::a2dpBluetoothDeviceMap_;
std::map<std::string, BluetoothDeviceAction> MediaBluetoothDeviceManager::wearDetectionStateMap_;
std::vector<BluetoothRemoteDevice> MediaBluetoothDeviceManager::privacyDevices_;
std::vector<BluetoothRemoteDevice> MediaBluetoothDeviceManager::commonDevices_;
std::vector<BluetoothRemoteDevice> MediaBluetoothDeviceManager::negativeDevices_;

int32_t RegisterDeviceObserver(IDeviceStatusObserver &observer)
{
    std::lock_guard<std::mutex> deviceLock(g_observerLock);
    g_deviceObserver = &observer;
    return SUCCESS;
}

void UnregisterDeviceObserver()
{
    std::lock_guard<std::mutex> deviceLock(g_observerLock);
    g_deviceObserver = nullptr;
}

void SendUserSelectionEvent(DeviceType devType, const std::string &macAddress, int32_t eventType)
{
    AUDIO_INFO_LOG("SendUserSelectionEvent devType is %{public}d, eventType is%{public}d.", devType, eventType);
    BluetoothRemoteDevice device;
    if (devType == DEVICE_TYPE_BLUETOOTH_A2DP) {
        if (MediaBluetoothDeviceManager::GetConnectedA2dpBluetoothDevice(macAddress, device) != SUCCESS) {
            AUDIO_ERR_LOG("SendUserSelectionEvent failed for the device is not connected.");
            return;
        }
        BluetoothAudioManager::GetInstance().SendDeviceSelection(device, eventType,
            HFP_DEFAULT_SELECTION, USER_SELECTION);
    } else if (devType == DEVICE_TYPE_BLUETOOTH_SCO) {
        AUDIO_INFO_LOG("SCO type is not currently supported.");
        BluetoothAudioManager::GetInstance().SendDeviceSelection(device, A2DP_DEFAULT_SELECTION,
            eventType, USER_SELECTION);
    } else {
        AUDIO_ERR_LOG("SendUserSelectionEvent failed for the devType is not Bluetooth type.");
    }
}

void MediaBluetoothDeviceManager::SetMediaStack(const BluetoothRemoteDevice &device, int action)
{
    switch (action) {
        case BluetoothDeviceAction::CONNECT_ACTION:
            HandleConnectDevice(device);
            break;
        case BluetoothDeviceAction::DISCONNECT_ACTION:
            HandleDisconnectDevice(device);
            break;
        case BluetoothDeviceAction::WEAR_ACTION:
            HandleWearDevice(device);
            break;
        case BluetoothDeviceAction::UNWEAR_ACTION:
            HandleUnwearDevice(device);
            break;
        case BluetoothDeviceAction::ENABLEFROMREMOTE_ACTION:
            HandleEnableDevice(device);
            break;
        case BluetoothDeviceAction::DISABLEFROMREMOTE_ACTION:
            HandleDisableDevice();
            break;
        case BluetoothDeviceAction::ENABLE_WEAR_DETECTION_ACTION:
            HandleWearEnable(device);
            break;
        case BluetoothDeviceAction::DISABLE_WEAR_DETECTION_ACTION:
            HandleWearDisable(device);
            break;
        default:
            AUDIO_ERR_LOG("SetMediaStack failed due to the unknow action: %{public}d", action);
            break;
    }
}

void MediaBluetoothDeviceManager::HandleConnectDevice(const BluetoothRemoteDevice &device)
{
    if (IsA2dpBluetoothDeviceExist(device.GetDeviceAddr())) {
        AUDIO_INFO_LOG("The device is already connected, ignore connect action.");
        return;
    }
    int cod = DEFAULT_COD;
    int majorClass = DEFAULT_MAJOR_CLASS;
    int majorMinorClass = DEFAULT_MAJOR_MINOR_CLASS;
    if (device.GetDeviceProductType(cod, majorClass, majorMinorClass) != SUCCESS) {
        AUDIO_ERR_LOG("HandleConnectDevice failed due to the product type fails to be obtained.");
        return;
    }
    DeviceCategory bluetoothCategory = CATEGORY_DEFAULT;
    auto pos = bluetoothDeviceCategoryMap_.find(std::make_pair(majorClass, majorMinorClass));
    if (pos != bluetoothDeviceCategoryMap_.end()) {
        bluetoothCategory = pos->second;
    }
    int32_t wearEnabledAbility = 0;
    bool isWearSupported = false;
    switch (bluetoothCategory) {
        case BT_HEADPHONE:
            BluetoothAudioManager::GetInstance().IsWearDetectionEnabled(device.GetDeviceAddr(), wearEnabledAbility);
            BluetoothAudioManager::GetInstance().IsWearDetectionSupported(device.GetDeviceAddr(), isWearSupported);
            if (wearEnabledAbility == WEAR_ENABLED && isWearSupported) {
                AddDeviceInConfigVector(device, negativeDevices_);
                NotifyToUpdateAudioDevice(device, BT_UNWEAR_HEADPHONE, DeviceStatus::ADD);
            } else {
                AddDeviceInConfigVector(device, privacyDevices_);
                NotifyToUpdateAudioDevice(device, BT_HEADPHONE, DeviceStatus::ADD);
            }
            break;
        case BT_GLASSES:
            AddDeviceInConfigVector(device, privacyDevices_);
            NotifyToUpdateAudioDevice(device, bluetoothCategory, DeviceStatus::ADD);
            break;
        case BT_SOUNDBOX:
        case BT_CAR:
            AddDeviceInConfigVector(device, commonDevices_);
            NotifyToUpdateAudioDevice(device, bluetoothCategory, DeviceStatus::ADD);
            break;
        case BT_WATCH:
            AddDeviceInConfigVector(device, negativeDevices_);
            NotifyToUpdateAudioDevice(device, bluetoothCategory, DeviceStatus::ADD);
            break;
        default:
            break;
    }
}

void MediaBluetoothDeviceManager::HandleDisconnectDevice(const BluetoothRemoteDevice &device)
{
    if (!IsA2dpBluetoothDeviceExist(device.GetDeviceAddr())) {
        AUDIO_INFO_LOG("The device is already disconnected, ignore disconnect action.");
        return;
    }
    std::lock_guard<std::mutex> wearStateMapLock(g_a2dpWearStateMapLock);
    RemoveDeviceInConfigVector(device, privacyDevices_);
    RemoveDeviceInConfigVector(device, commonDevices_);
    RemoveDeviceInConfigVector(device, negativeDevices_);
    wearDetectionStateMap_.erase(device.GetDeviceAddr());
    NotifyToUpdateAudioDevice(device, CATEGORY_DEFAULT, DeviceStatus::REMOVE);
}

void MediaBluetoothDeviceManager::HandleWearDevice(const BluetoothRemoteDevice &device)
{
    if (!IsA2dpBluetoothDeviceExist(device.GetDeviceAddr())) {
        AUDIO_ERR_LOG("HandleWearDevice failed for the device has not be reported the connected action.");
        return;
    }
    std::lock_guard<std::mutex> wearStateMapLock(g_a2dpWearStateMapLock);
    RemoveDeviceInConfigVector(device, negativeDevices_);
    RemoveDeviceInConfigVector(device, privacyDevices_);
    AddDeviceInConfigVector(device, privacyDevices_);
    wearDetectionStateMap_[device.GetDeviceAddr()] = BluetoothDeviceAction::WEAR_ACTION;
    NotifyToUpdateAudioDevice(device, BT_HEADPHONE, DeviceStatus::ADD);
}

void MediaBluetoothDeviceManager::HandleUnwearDevice(const BluetoothRemoteDevice &device)
{
    if (!IsA2dpBluetoothDeviceExist(device.GetDeviceAddr())) {
        AUDIO_ERR_LOG("HandleWearDevice failed for the device has not worn.");
        return;
    }
    std::lock_guard<std::mutex> wearStateMapLock(g_a2dpWearStateMapLock);
    RemoveDeviceInConfigVector(device, privacyDevices_);
    RemoveDeviceInConfigVector(device, negativeDevices_);
    AddDeviceInConfigVector(device, negativeDevices_);
    wearDetectionStateMap_[device.GetDeviceAddr()] = BluetoothDeviceAction::UNWEAR_ACTION;
    NotifyToUpdateAudioDevice(device, BT_UNWEAR_HEADPHONE, DeviceStatus::ADD);
}

void MediaBluetoothDeviceManager::HandleEnableDevice(const BluetoothRemoteDevice &device)
{
    std::lock_guard<std::mutex> observerLock(g_observerLock);
    if (g_deviceObserver != nullptr) {
        g_deviceObserver->OnForcedDeviceSelected(DEVICE_TYPE_BLUETOOTH_A2DP, device.GetDeviceAddr());
    }
}

void MediaBluetoothDeviceManager::HandleDisableDevice()
{
    std::lock_guard<std::mutex> observerLock(g_observerLock);
    if (g_deviceObserver != nullptr) {
        g_deviceObserver->OnForcedDeviceSelected(DEVICE_TYPE_BLUETOOTH_A2DP, "");
    }
}

void MediaBluetoothDeviceManager::HandleWearEnable(const BluetoothRemoteDevice &device)
{
    if (!IsA2dpBluetoothDeviceExist(device.GetDeviceAddr())) {
        AUDIO_ERR_LOG("HandleWearEnable failed for the device has not connected.");
        return;
    }
    RemoveDeviceInConfigVector(device, negativeDevices_);
    RemoveDeviceInConfigVector(device, privacyDevices_);
    std::lock_guard<std::mutex> wearStateMapLock(g_a2dpWearStateMapLock);
    auto wearStateIter = wearDetectionStateMap_.find(device.GetDeviceAddr());
    if (wearStateIter != wearDetectionStateMap_.end() &&
        wearStateIter->second == BluetoothDeviceAction::WEAR_ACTION) {
        AddDeviceInConfigVector(device, privacyDevices_);
        NotifyToUpdateAudioDevice(device, BT_HEADPHONE, DeviceStatus::ADD);
    } else {
        AddDeviceInConfigVector(device, negativeDevices_);
        NotifyToUpdateAudioDevice(device, BT_UNWEAR_HEADPHONE, DeviceStatus::ADD);
    }
}

void MediaBluetoothDeviceManager::HandleWearDisable(const BluetoothRemoteDevice &device)
{
    if (!IsA2dpBluetoothDeviceExist(device.GetDeviceAddr())) {
        AUDIO_ERR_LOG("HandleWearDisable failed for the device has not connected.");
        return;
    }
    RemoveDeviceInConfigVector(device, privacyDevices_);
    RemoveDeviceInConfigVector(device, negativeDevices_);
    AddDeviceInConfigVector(device, privacyDevices_);
    NotifyToUpdateAudioDevice(device, BT_HEADPHONE, DeviceStatus::ADD);
}

void MediaBluetoothDeviceManager::AddDeviceInConfigVector(const BluetoothRemoteDevice &device,
    std::vector<BluetoothRemoteDevice> &deviceVector)
{
    std::lock_guard<std::mutex> a2dpDeviceLock(g_a2dpDeviceLock);
    auto isPresent = [&device] (BluetoothRemoteDevice &bluetoothRemoteDevice) {
        return device.GetDeviceAddr() == bluetoothRemoteDevice.GetDeviceAddr();
    };
    auto deviceIter = std::find_if(deviceVector.begin(), deviceVector.end(), isPresent);
    if (deviceIter == deviceVector.end()) {
        deviceVector.push_back(device);
    }
}

void MediaBluetoothDeviceManager::RemoveDeviceInConfigVector(const BluetoothRemoteDevice &device,
    std::vector<BluetoothRemoteDevice> &deviceVector)
{
    std::lock_guard<std::mutex> a2dpDeviceLock(g_a2dpDeviceLock);
    auto isPresent = [&device] (BluetoothRemoteDevice &bluetoothRemoteDevice) {
        return device.GetDeviceAddr() == bluetoothRemoteDevice.GetDeviceAddr();
    };
    auto deviceIter = std::find_if(deviceVector.begin(), deviceVector.end(), isPresent);
    if (deviceIter != deviceVector.end()) {
        deviceVector.erase(deviceIter);
    }
}

void MediaBluetoothDeviceManager::NotifyToUpdateAudioDevice(const BluetoothRemoteDevice &device,
    DeviceCategory category, DeviceStatus deviceStatus)
{
    AudioDeviceDescriptor audioDeviceDescriptor;
    audioDeviceDescriptor.deviceType_ = DEVICE_TYPE_BLUETOOTH_A2DP;
    audioDeviceDescriptor.deviceRole_ = DeviceRole::OUTPUT_DEVICE;
    audioDeviceDescriptor.macAddress_ = device.GetDeviceAddr();
    audioDeviceDescriptor.deviceName_ = device.GetDeviceName();
    audioDeviceDescriptor.deviceCategory_ = category;
    audioDeviceDescriptor.connectState_ = ConnectState::CONNECTED;
    AUDIO_INFO_LOG("a2dpBluetoothDeviceMap_ operation: %{public}d new bluetooth device, device address is %{public}s,\
        category is %{public}d", deviceStatus, device.GetDeviceAddr().c_str(), category);
    {
        std::lock_guard<std::mutex> deviceMapLock(g_a2dpDeviceMapLock);
        if (deviceStatus == DeviceStatus::ADD) {
            a2dpBluetoothDeviceMap_[device.GetDeviceAddr()] = device;
        } else {
            if (a2dpBluetoothDeviceMap_.find(device.GetDeviceAddr()) != a2dpBluetoothDeviceMap_.end()) {
                a2dpBluetoothDeviceMap_.erase(device.GetDeviceAddr());
            }
        }
    }
    std::lock_guard<std::mutex> observerLock(g_observerLock);
    if (g_deviceObserver == nullptr) {
        AUDIO_ERR_LOG("NotifyToUpdateAudioDevice, device observer is null");
        return;
    }
    bool isConnected = deviceStatus == DeviceStatus::ADD ? true : false;
    g_deviceObserver->OnDeviceStatusUpdated(audioDeviceDescriptor, isConnected);
}

bool MediaBluetoothDeviceManager::IsA2dpBluetoothDeviceExist(const std::string& macAddress)
{
    std::lock_guard<std::mutex> deviceMapLock(g_a2dpDeviceMapLock);
    if (a2dpBluetoothDeviceMap_.find(macAddress) != a2dpBluetoothDeviceMap_.end()) {
        return true;
    }
    return false;
}

int32_t MediaBluetoothDeviceManager::GetConnectedA2dpBluetoothDevice(const std::string& macAddress,
    BluetoothRemoteDevice &device)
{
    std::lock_guard<std::mutex> deviceMapLock(g_a2dpDeviceMapLock);
    auto deviceIter = a2dpBluetoothDeviceMap_.find(macAddress);
    if (deviceIter != a2dpBluetoothDeviceMap_.end()) {
        device = deviceIter->second;
        return SUCCESS;
    }
    return ERROR;
}

void MediaBluetoothDeviceManager::UpdateA2dpDeviceConfiguration(const BluetoothRemoteDevice &device,
    const AudioStreamInfo &streamInfo)
{
    std::lock_guard<std::mutex> observerLock(g_observerLock);
    if (g_deviceObserver == nullptr) {
        AUDIO_ERR_LOG("UpdateA2dpDeviceConfiguration, device observer is null");
        return;
    }
    g_deviceObserver->OnDeviceConfigurationChanged(DEVICE_TYPE_BLUETOOTH_A2DP, device.GetDeviceAddr(),
        device.GetDeviceName(), streamInfo);
}

void MediaBluetoothDeviceManager::ClearAllA2dpBluetoothDevice()
{
    AUDIO_INFO_LOG("Bluetooth Service crashed ane enter the ClearAllA2dpBluetoothDevice.");
    std::lock_guard<std::mutex> deviceMapLock(g_a2dpDeviceMapLock);
    std::lock_guard<std::mutex> wearStateMapLock(g_a2dpWearStateMapLock);
    std::lock_guard<std::mutex> a2dpDeviceLock(g_a2dpDeviceLock);
    a2dpBluetoothDeviceMap_.clear();
    wearDetectionStateMap_.clear();
    privacyDevices_.clear();
    commonDevices_.clear();
    negativeDevices_.clear();
}

void HfpBluetoothDeviceManager::OnHfpStackChanged(const BluetoothRemoteDevice &device, bool isConnected)
{
    std::lock_guard<std::mutex> observerLock(g_observerLock);
    if (g_deviceObserver == nullptr) {
        AUDIO_ERR_LOG("OnHfpStackChanged, device observer is null");
        return;
    }
    AudioStreamInfo streamInfo = {};
    g_deviceObserver->OnDeviceStatusUpdated(DEVICE_TYPE_BLUETOOTH_SCO, isConnected, device.GetDeviceAddr(),
        device.GetDeviceName(), streamInfo);
}
} // namespace Bluetooth
} // namespace OHOS