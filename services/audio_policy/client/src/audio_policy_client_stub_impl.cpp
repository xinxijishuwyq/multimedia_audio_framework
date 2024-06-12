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
#undef LOG_TAG
#define LOG_TAG "AudioPolicyClientStubImpl"

#include "audio_policy_client_stub_impl.h"
#include "audio_errors.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
int32_t AudioPolicyClientStubImpl::AddVolumeKeyEventCallback(const std::shared_ptr<VolumeKeyEventCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(volumeKeyEventMutex_);
    volumeKeyEventCallbackList_.push_back(cb);
    return SUCCESS;
}

int32_t AudioPolicyClientStubImpl::RemoveVolumeKeyEventCallback(const std::shared_ptr<VolumeKeyEventCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(volumeKeyEventMutex_);
    if (cb == nullptr) {
        volumeKeyEventCallbackList_.clear();
        return SUCCESS;
    }
    auto it = find_if(volumeKeyEventCallbackList_.begin(), volumeKeyEventCallbackList_.end(),
        [&cb](const std::weak_ptr<VolumeKeyEventCallback>& elem) {
            return elem.lock() == cb;
        });
    if (it != volumeKeyEventCallbackList_.end()) {
        volumeKeyEventCallbackList_.erase(it);
    }
    return SUCCESS;
}

void AudioPolicyClientStubImpl::OnVolumeKeyEvent(VolumeEvent volumeEvent)
{
    std::lock_guard<std::mutex> lockCbMap(volumeKeyEventMutex_);
    for (auto it = volumeKeyEventCallbackList_.begin(); it != volumeKeyEventCallbackList_.end(); ++it) {
        std::shared_ptr<VolumeKeyEventCallback> volumeKeyEventCallback = (*it).lock();
        if (volumeKeyEventCallback != nullptr) {
            volumeKeyEventCallback->OnVolumeKeyEvent(volumeEvent);
        }
    }
}

int32_t AudioPolicyClientStubImpl::AddFocusInfoChangeCallback(const std::shared_ptr<AudioFocusInfoChangeCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(focusInfoChangeMutex_);
    focusInfoChangeCallbackList_.push_back(cb);
    return SUCCESS;
}

int32_t AudioPolicyClientStubImpl::RemoveFocusInfoChangeCallback()
{
    std::lock_guard<std::mutex> lockCbMap(focusInfoChangeMutex_);
    focusInfoChangeCallbackList_.clear();
    return SUCCESS;
}

void AudioPolicyClientStubImpl::OnAudioFocusInfoChange(
    const std::list<std::pair<AudioInterrupt, AudioFocuState>> &focusInfoList)
{
    std::lock_guard<std::mutex> lockCbMap(focusInfoChangeMutex_);
    for (auto it = focusInfoChangeCallbackList_.begin(); it != focusInfoChangeCallbackList_.end(); ++it) {
        (*it)->OnAudioFocusInfoChange(focusInfoList);
    }
}

void AudioPolicyClientStubImpl::OnAudioFocusRequested(const AudioInterrupt &requestFocus)
{
    std::lock_guard<std::mutex> lockCbMap(focusInfoChangeMutex_);
    for (auto it = focusInfoChangeCallbackList_.begin(); it != focusInfoChangeCallbackList_.end(); ++it) {
        (*it)->OnAudioFocusRequested(requestFocus);
    }
}

void AudioPolicyClientStubImpl::OnAudioFocusAbandoned(const AudioInterrupt &abandonFocus)
{
    std::lock_guard<std::mutex> lockCbMap(focusInfoChangeMutex_);
    for (auto it = focusInfoChangeCallbackList_.begin(); it != focusInfoChangeCallbackList_.end(); ++it) {
        (*it)->OnAudioFocusAbandoned(abandonFocus);
    }
}

size_t AudioPolicyClientStubImpl::GetFocusInfoChangeCallbackSize() const
{
    std::lock_guard<std::mutex> lockCbMap(focusInfoChangeMutex_);
    return focusInfoChangeCallbackList_.size();
}

std::vector<sptr<AudioDeviceDescriptor>> AudioPolicyClientStubImpl::DeviceFilterByFlag(DeviceFlag flag,
    const std::vector<sptr<AudioDeviceDescriptor>>& desc)
{
    std::vector<sptr<AudioDeviceDescriptor>> descRet;
    DeviceRole role = DEVICE_ROLE_NONE;
    switch (flag) {
        case DeviceFlag::ALL_DEVICES_FLAG:
            for (sptr<AudioDeviceDescriptor> var : desc) {
                if (var->networkId_ == LOCAL_NETWORK_ID) {
                    descRet.insert(descRet.end(), var);
                }
            }
            break;
        case DeviceFlag::ALL_DISTRIBUTED_DEVICES_FLAG:
            for (sptr<AudioDeviceDescriptor> var : desc) {
                if (var->networkId_ != LOCAL_NETWORK_ID) {
                    descRet.insert(descRet.end(), var);
                }
            }
            break;
        case DeviceFlag::ALL_L_D_DEVICES_FLAG:
            descRet = desc;
            break;
        case DeviceFlag::OUTPUT_DEVICES_FLAG:
        case DeviceFlag::INPUT_DEVICES_FLAG:
            role = flag == INPUT_DEVICES_FLAG ? INPUT_DEVICE : OUTPUT_DEVICE;
            for (sptr<AudioDeviceDescriptor> var : desc) {
                if (var->networkId_ == LOCAL_NETWORK_ID && var->deviceRole_ == role) {
                    descRet.insert(descRet.end(), var);
                }
            }
            break;
        case DeviceFlag::DISTRIBUTED_OUTPUT_DEVICES_FLAG:
        case DeviceFlag::DISTRIBUTED_INPUT_DEVICES_FLAG:
            role = flag == DISTRIBUTED_INPUT_DEVICES_FLAG ? INPUT_DEVICE : OUTPUT_DEVICE;
            for (sptr<AudioDeviceDescriptor> var : desc) {
                if (var->networkId_ != LOCAL_NETWORK_ID && var->deviceRole_ == role) {
                    descRet.insert(descRet.end(), var);
                }
            }
            break;
        default:
            break;
    }
    return descRet;
}

int32_t AudioPolicyClientStubImpl::AddDeviceChangeCallback(const DeviceFlag &flag,
    const std::shared_ptr<AudioManagerDeviceChangeCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(deviceChangeMutex_);
    deviceChangeCallbackList_.push_back(std::make_pair(flag, cb));
    return SUCCESS;
}

int32_t AudioPolicyClientStubImpl::RemoveDeviceChangeCallback()
{
    std::lock_guard<std::mutex> lockCbMap(deviceChangeMutex_);
    deviceChangeCallbackList_.clear();
    return SUCCESS;
}

void AudioPolicyClientStubImpl::OnDeviceChange(const DeviceChangeAction &dca)
{
    std::lock_guard<std::mutex> lockCbMap(deviceChangeMutex_);
    DeviceChangeAction deviceChangeAction;
    deviceChangeAction.type = dca.type;
    for (auto it = deviceChangeCallbackList_.begin(); it != deviceChangeCallbackList_.end(); ++it) {
        deviceChangeAction.flag = it->first;
        deviceChangeAction.deviceDescriptors = DeviceFilterByFlag(it->first, dca.deviceDescriptors);
        if (it->second && deviceChangeAction.deviceDescriptors.size() > 0) {
            it->second->OnDeviceChange(deviceChangeAction);
        }
    }
}

int32_t AudioPolicyClientStubImpl::AddRingerModeCallback(const std::shared_ptr<AudioRingerModeCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(ringerModeMutex_);
    ringerModeCallbackList_.push_back(cb);
    return SUCCESS;
}

int32_t AudioPolicyClientStubImpl::RemoveRingerModeCallback()
{
    std::lock_guard<std::mutex> lockCbMap(ringerModeMutex_);
    ringerModeCallbackList_.clear();
    return SUCCESS;
}

int32_t AudioPolicyClientStubImpl::RemoveRingerModeCallback(const std::shared_ptr<AudioRingerModeCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(ringerModeMutex_);
    auto iter = ringerModeCallbackList_.begin();
    while (iter != ringerModeCallbackList_.end()) {
        if (*iter == cb) {
            iter = ringerModeCallbackList_.erase(iter);
        } else {
            iter++;
        }
    }
    return SUCCESS;
}

void AudioPolicyClientStubImpl::OnRingerModeUpdated(const AudioRingerMode &ringerMode)
{
    std::lock_guard<std::mutex> lockCbMap(ringerModeMutex_);
    for (auto it = ringerModeCallbackList_.begin(); it != ringerModeCallbackList_.end(); ++it) {
        (*it)->OnRingerModeUpdated(ringerMode);
    }
}

int32_t AudioPolicyClientStubImpl::AddMicStateChangeCallback(
    const std::shared_ptr<AudioManagerMicStateChangeCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(micStateChangeMutex_);
    micStateChangeCallbackList_.push_back(cb);
    return SUCCESS;
}
int32_t AudioPolicyClientStubImpl::RemoveMicStateChangeCallback()
{
    std::lock_guard<std::mutex> lockCbMap(micStateChangeMutex_);
    micStateChangeCallbackList_.clear();
    return SUCCESS;
}

bool AudioPolicyClientStubImpl::HasMicStateChangeCallback()
{
    std::lock_guard<std::mutex> lockCbMap(micStateChangeMutex_);
    if (micStateChangeCallbackList_.empty()) {
        AUDIO_INFO_LOG("MicStateChangeCallback list is empty.");
        return false;
    }
    return true;
}

void AudioPolicyClientStubImpl::OnMicStateUpdated(const MicStateChangeEvent &micStateChangeEvent)
{
    std::lock_guard<std::mutex> lockCbMap(micStateChangeMutex_);
    for (auto it = micStateChangeCallbackList_.begin(); it != micStateChangeCallbackList_.end(); ++it) {
        (*it)->OnMicStateUpdated(micStateChangeEvent);
    }
}

int32_t AudioPolicyClientStubImpl::AddPreferredOutputDeviceChangeCallback(
    const std::shared_ptr<AudioPreferredOutputDeviceChangeCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(pOutputDeviceChangeMutex_);
    preferredOutputDeviceCallbackList_.push_back(cb);
    return SUCCESS;
}

int32_t AudioPolicyClientStubImpl::RemovePreferredOutputDeviceChangeCallback()
{
    std::lock_guard<std::mutex> lockCbMap(pOutputDeviceChangeMutex_);
    preferredOutputDeviceCallbackList_.clear();
    return SUCCESS;
}

void AudioPolicyClientStubImpl::OnPreferredOutputDeviceUpdated(const std::vector<sptr<AudioDeviceDescriptor>> &desc)
{
    std::lock_guard<std::mutex> lockCbMap(pOutputDeviceChangeMutex_);
    for (auto it = preferredOutputDeviceCallbackList_.begin(); it != preferredOutputDeviceCallbackList_.end(); ++it) {
        (*it)->OnPreferredOutputDeviceUpdated(desc);
    }
}

int32_t AudioPolicyClientStubImpl::AddPreferredInputDeviceChangeCallback(
    const std::shared_ptr<AudioPreferredInputDeviceChangeCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(pInputDeviceChangeMutex_);
    preferredInputDeviceCallbackList_.push_back(cb);
    return SUCCESS;
}

int32_t AudioPolicyClientStubImpl::RemovePreferredInputDeviceChangeCallback()
{
    std::lock_guard<std::mutex> lockCbMap(pInputDeviceChangeMutex_);
    preferredInputDeviceCallbackList_.clear();
    return SUCCESS;
}

void AudioPolicyClientStubImpl::OnPreferredInputDeviceUpdated(const std::vector<sptr<AudioDeviceDescriptor>> &desc)
{
    std::lock_guard<std::mutex> lockCbMap(pInputDeviceChangeMutex_);
    for (auto it = preferredInputDeviceCallbackList_.begin(); it != preferredInputDeviceCallbackList_.end(); ++it) {
        (*it)->OnPreferredInputDeviceUpdated(desc);
    }
}

int32_t AudioPolicyClientStubImpl::AddRendererStateChangeCallback(
    const std::shared_ptr<AudioRendererStateChangeCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(rendererStateChangeMutex_);
    rendererStateChangeCallbackList_.push_back(cb);
    return SUCCESS;
}

int32_t AudioPolicyClientStubImpl::RemoveRendererStateChangeCallback()
{
    std::lock_guard<std::mutex> lockCbMap(rendererStateChangeMutex_);
    rendererStateChangeCallbackList_.clear();
    return SUCCESS;
}

size_t AudioPolicyClientStubImpl::GetRendererStateChangeCallbackSize() const
{
    std::lock_guard<std::mutex> lockCbMap(rendererStateChangeMutex_);
    return rendererStateChangeCallbackList_.size();
}

int32_t AudioPolicyClientStubImpl::AddDeviceChangeWithInfoCallback(
    const uint32_t sessionId, const std::shared_ptr<DeviceChangeWithInfoCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(deviceChangeWithInfoCallbackMutex_);
    deviceChangeWithInfoCallbackMap_[sessionId] = cb;
    return SUCCESS;
}

int32_t AudioPolicyClientStubImpl::RemoveDeviceChangeWithInfoCallback(const uint32_t sessionId)
{
    std::lock_guard<std::mutex> lockCbMap(deviceChangeWithInfoCallbackMutex_);
    deviceChangeWithInfoCallbackMap_.erase(sessionId);
    return SUCCESS;
}

void AudioPolicyClientStubImpl::OnRendererDeviceChange(const uint32_t sessionId,
    const DeviceInfo &deviceInfo, const AudioStreamDeviceChangeReason reason)
{
    std::shared_ptr<DeviceChangeWithInfoCallback> callback = nullptr;
    {
        std::lock_guard<std::mutex> lockCbMap(deviceChangeWithInfoCallbackMutex_);
        if (deviceChangeWithInfoCallbackMap_.count(sessionId) == 0) {
            return;
        }
        callback = deviceChangeWithInfoCallbackMap_.at(sessionId);
    }
    if (callback != nullptr) {
        callback->OnDeviceChangeWithInfo(sessionId, deviceInfo, reason);
    }
}

void AudioPolicyClientStubImpl::OnRendererStateChange(
    std::vector<std::unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    std::lock_guard<std::mutex> lockCbMap(rendererStateChangeMutex_);
    for (auto it = rendererStateChangeCallbackList_.begin(); it != rendererStateChangeCallbackList_.end(); ++it) {
        (*it)->OnRendererStateChange(audioRendererChangeInfos);
    }
}

void AudioPolicyClientStubImpl::OnRecreateRendererStreamEvent(const uint32_t sessionId, const int32_t streamFlag)
{
    AUDIO_INFO_LOG("Enter");
    std::shared_ptr<DeviceChangeWithInfoCallback> callback = nullptr;
    {
        std::lock_guard<std::mutex> lockCbMap(deviceChangeWithInfoCallbackMutex_);
        if (deviceChangeWithInfoCallbackMap_.count(sessionId) == 0) {
            AUDIO_ERR_LOG("No session id %{public}d", sessionId);
            return;
        }
        callback = deviceChangeWithInfoCallbackMap_.at(sessionId);
    }
    if (callback != nullptr) {
        callback->OnRecreateStreamEvent(sessionId, streamFlag);
    }
}

void AudioPolicyClientStubImpl::OnRecreateCapturerStreamEvent(const uint32_t sessionId, const int32_t streamFlag)
{
    AUDIO_INFO_LOG("Enter");
    std::shared_ptr<DeviceChangeWithInfoCallback> callback = nullptr;
    {
        std::lock_guard<std::mutex> lockCbMap(deviceChangeWithInfoCallbackMutex_);
        if (deviceChangeWithInfoCallbackMap_.count(sessionId) == 0) {
            AUDIO_ERR_LOG("No session id %{public}d", sessionId);
            return;
        }
        callback = deviceChangeWithInfoCallbackMap_.at(sessionId);
    }
    if (callback != nullptr) {
        callback->OnRecreateStreamEvent(sessionId, streamFlag);
    }
}

int32_t AudioPolicyClientStubImpl::AddCapturerStateChangeCallback(
    const std::shared_ptr<AudioCapturerStateChangeCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(capturerStateChangeMutex_);
    capturerStateChangeCallbackList_.push_back(cb);
    return SUCCESS;
}

int32_t AudioPolicyClientStubImpl::RemoveCapturerStateChangeCallback()
{
    std::lock_guard<std::mutex> lockCbMap(capturerStateChangeMutex_);
    capturerStateChangeCallbackList_.clear();
    return SUCCESS;
}

size_t AudioPolicyClientStubImpl::GetCapturerStateChangeCallbackSize() const
{
    std::lock_guard<std::mutex> lockCbMap(capturerStateChangeMutex_);
    return capturerStateChangeCallbackList_.size();
}

void AudioPolicyClientStubImpl::OnCapturerStateChange(
    std::vector<std::unique_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos)
{
    std::lock_guard<std::mutex> lockCbMap(capturerStateChangeMutex_);
    for (auto it = capturerStateChangeCallbackList_.begin(); it != capturerStateChangeCallbackList_.end(); ++it) {
        std::shared_ptr<AudioCapturerStateChangeCallback> capturerStateChangeCallback = (*it).lock();
        if (capturerStateChangeCallback != nullptr) {
            capturerStateChangeCallback->OnCapturerStateChange(audioCapturerChangeInfos);
        }
    }
}

int32_t AudioPolicyClientStubImpl::AddHeadTrackingDataRequestedChangeCallback(const std::string &macAddress,
    const std::shared_ptr<HeadTrackingDataRequestedChangeCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(headTrackingDataRequestedChangeMutex_);
    if (!headTrackingDataRequestedChangeCallbackMap_.count(macAddress)) {
        AUDIO_INFO_LOG("First registeration for the specified device");
        headTrackingDataRequestedChangeCallbackMap_.insert(std::make_pair(macAddress, cb));
    } else {
        AUDIO_INFO_LOG("Repeated registeration for the specified device, replaced by the new one");
        headTrackingDataRequestedChangeCallbackMap_[macAddress] = cb;
    }
    return SUCCESS;
}

int32_t AudioPolicyClientStubImpl::RemoveHeadTrackingDataRequestedChangeCallback(const std::string &macAddress)
{
    std::lock_guard<std::mutex> lockCbMap(headTrackingDataRequestedChangeMutex_);
    headTrackingDataRequestedChangeCallbackMap_.erase(macAddress);
    return SUCCESS;
}

void AudioPolicyClientStubImpl::OnHeadTrackingDeviceChange(const std::unordered_map<std::string, bool> &changeInfo)
{
    std::lock_guard<std::mutex> lockCbMap(headTrackingDataRequestedChangeMutex_);
    if (headTrackingDataRequestedChangeCallbackMap_.size() == 0) {
        return;
    }
    for (const auto &pair : changeInfo) {
        if (!headTrackingDataRequestedChangeCallbackMap_.count(pair.first)) {
            AUDIO_WARNING_LOG("the specified device has not been registered");
            continue;
        }
        std::shared_ptr<HeadTrackingDataRequestedChangeCallback> headTrackingDataRequestedChangeCallback =
            headTrackingDataRequestedChangeCallbackMap_[pair.first];
        if (headTrackingDataRequestedChangeCallback != nullptr) {
            AUDIO_DEBUG_LOG("head tracking data requested change event of the specified device has been notified");
            headTrackingDataRequestedChangeCallback->OnHeadTrackingDataRequestedChange(pair.second);
        }
    }
}

int32_t AudioPolicyClientStubImpl::AddSpatializationEnabledChangeCallback(
    const std::shared_ptr<AudioSpatializationEnabledChangeCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(spatializationEnabledChangeMutex_);
    spatializationEnabledChangeCallbackList_.push_back(cb);
    return SUCCESS;
}

int32_t AudioPolicyClientStubImpl::RemoveSpatializationEnabledChangeCallback()
{
    std::lock_guard<std::mutex> lockCbMap(spatializationEnabledChangeMutex_);
    spatializationEnabledChangeCallbackList_.clear();
    return SUCCESS;
}

void AudioPolicyClientStubImpl::OnSpatializationEnabledChange(const bool &enabled)
{
    std::lock_guard<std::mutex> lockCbMap(spatializationEnabledChangeMutex_);
    for (const auto &callback : spatializationEnabledChangeCallbackList_) {
        callback->OnSpatializationEnabledChange(enabled);
    }
}

int32_t AudioPolicyClientStubImpl::AddHeadTrackingEnabledChangeCallback(
    const std::shared_ptr<AudioHeadTrackingEnabledChangeCallback> &cb)
{
    std::lock_guard<std::mutex> lockCbMap(headTrackingEnabledChangeMutex_);
    headTrackingEnabledChangeCallbackList_.push_back(cb);
    return SUCCESS;
}

int32_t AudioPolicyClientStubImpl::RemoveHeadTrackingEnabledChangeCallback()
{
    std::lock_guard<std::mutex> lockCbMap(headTrackingEnabledChangeMutex_);
    headTrackingEnabledChangeCallbackList_.clear();
    return SUCCESS;
}

void AudioPolicyClientStubImpl::OnHeadTrackingEnabledChange(const bool &enabled)
{
    std::lock_guard<std::mutex> lockCbMap(headTrackingEnabledChangeMutex_);
    for (const auto &callback : headTrackingEnabledChangeCallbackList_) {
        callback->OnHeadTrackingEnabledChange(enabled);
    }
}
} // namespace AudioStandard
} // namespace OHOS