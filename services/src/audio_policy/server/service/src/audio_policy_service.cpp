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
#include "audio_policy_service.h"
#include "media_log.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;
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

    return true;
}

void AudioPolicyService::Deinit(void)
{
    mAudioPolicyManager.CloseAudioPort(mIOHandles[HDI_SINK]);
    mAudioPolicyManager.CloseAudioPort(mIOHandles[HDI_SOURCE]);
    return;
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

std::string AudioPolicyService::GetPortName(DeviceType deviceType)
{
    std::string portName = PORT_NONE;
    switch (deviceType) {
        case BLUETOOTH_A2DP:
            portName = BLUEZ_SINK;
            break;
        case SPEAKER:
            portName = HDI_SINK;
            break;
        case MIC:
            portName = HDI_SOURCE;
            break;
        case BLUETOOTH_SCO:
            portName = BLUEZ_SOURCE;
            break;
        default:
            portName = PORT_NONE;
            break;
    }
    return portName;
}

int32_t AudioPolicyService::SetDeviceActive(DeviceType deviceType, bool active)
{
    MEDIA_DEBUG_LOG("[Policy Service] deviceType %{public}d, activate?: %{public}d", deviceType, active);

    if (deviceType == DEVICE_TYPE_NONE)
        return ERR_DEVICE_NOT_SUPPORTED;

    bool updateActiveDevices = true;
    AudioIOHandle ioHandle = GetAudioIOHandle(deviceType);
    list<DeviceType> &activeDevices = GetActiveDevicesList(deviceType);

    if (!active) {
        if (activeDevices.size() <= 1) {
            MEDIA_ERR_LOG("[Policy Service] Only one Active device. So cannot deactivate!");
            return ERROR;
        }

        list<DeviceType>::const_iterator iter = activeDevices.begin();
        while (iter != activeDevices.end()) {
            if (*iter == deviceType) {
                iter = activeDevices.erase(iter);
            } else {
                ++iter;
            }
        }

        deviceType = activeDevices.front();
        updateActiveDevices = false;
    }

    int32_t result = 0;
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

    if (updateActiveDevices) {
        list<DeviceType>::const_iterator iter = activeDevices.begin();
        while (iter != activeDevices.end()) {
            if (*iter == deviceType) {
                iter = activeDevices.erase(iter);
            } else {
                ++iter;
            }
        }
        activeDevices.push_front(deviceType);
    }

    return SUCCESS;
}

bool AudioPolicyService::IsDeviceActive(DeviceType deviceType) const
{
    bool result = false;

    switch (deviceType) {
        case SPEAKER:
        case BLUETOOTH_A2DP:
            for (list<DeviceType>::const_iterator iter = mActiveOutputDevices.begin();
                iter != mActiveOutputDevices.end(); ++iter) {
                if (*iter == deviceType) {
                    result = true;
                    break;
                }
            }
            break;
        case MIC:
        case BLUETOOTH_SCO:
            for (list<DeviceType>::const_iterator iter = mActiveInputDevices.begin();
                iter != mActiveInputDevices.end(); ++iter) {
                if (*iter == deviceType) {
                    result = true;
                    break;
                }
            }
            break;
        default:
            break;
    }

    return result;
}

int32_t AudioPolicyService::SetRingerMode(AudioRingerMode ringMode)
{
    return mAudioPolicyManager.SetRingerMode(ringMode);
}

AudioRingerMode AudioPolicyService::GetRingerMode() const
{
    return mAudioPolicyManager.GetRingerMode();
}

// Parser callbacks

void AudioPolicyService::OnAudioPortAvailable(shared_ptr<AudioPortInfo> portInfo)
{
    AudioIOHandle ioHandle = mAudioPolicyManager.OpenAudioPort(portInfo);
    mIOHandles[portInfo->name] = ioHandle;
    return;
}

void AudioPolicyService::OnAudioPortPinAvailable(shared_ptr<AudioPortPinInfo> portInfo)
{
    return;
}

void AudioPolicyService::OnDefaultOutputPortPin(DeviceType deviceType)
{
    AudioIOHandle ioHandle = GetAudioIOHandle(deviceType);
    mAudioPolicyManager.SetDeviceActive(ioHandle, deviceType, HDI_SINK, true);
    mActiveOutputDevices.push_front(deviceType);
    MEDIA_DEBUG_LOG("OnDefaultOutputPortPin DeviceType: %{public}d", deviceType);
    return;
}

void AudioPolicyService::OnDefaultInputPortPin(DeviceType deviceType)
{
    MEDIA_DEBUG_LOG("OnDefaultInputPortPin DeviceType: %{public}d", deviceType);
    AudioIOHandle ioHandle = GetAudioIOHandle(deviceType);
    mAudioPolicyManager.SetDeviceActive(ioHandle, deviceType, HDI_SOURCE, true);
    mActiveInputDevices.push_front(deviceType);
    return;
}

// private methods
AudioIOHandle AudioPolicyService::GetAudioIOHandle(DeviceType deviceType)
{
    AudioIOHandle ioHandle;
    switch (deviceType) {
        case SPEAKER:
        case WIRED_HEADSET:
            ioHandle = mIOHandles[HDI_SINK];
            break;
        case BLUETOOTH_A2DP:
            ioHandle = mIOHandles[BLUEZ_SINK];
            break;
        case MIC:
            ioHandle = mIOHandles[HDI_SOURCE];
            break;
        case BLUETOOTH_SCO:
            ioHandle = mIOHandles[BLUEZ_SOURCE];
            break;
        default:
            ioHandle = mIOHandles[HDI_SINK];
            break;
    }
    return ioHandle;
}
}
}
