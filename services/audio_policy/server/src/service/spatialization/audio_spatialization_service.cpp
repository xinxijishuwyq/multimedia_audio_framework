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


#include "audio_spatialization_service.h"

#include "ipc_skeleton.h"
#include "hisysevent.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "parameter.h"

#include "audio_errors.h"
#include "audio_log.h"
#include "audio_utils.h"

#include "audio_spatialization_state_change_listener_proxy.h"
#include "i_standard_spatialization_state_change_listener.h"

#include "audio_policy_service.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;

static const int32_t SPATIALIZATION_SERVICE_OK = 0;
static sptr<IStandardAudioService> g_adProxy = nullptr;
mutex g_adSpatializationProxyMutex;

static bool IsAudioSpatialDeviceStateEqual(const AudioSpatialDeviceState &a, const AudioSpatialDeviceState &b)
{
    return ((a.address == b.address) && (a.isSpatializationSupported == b.isSpatializationSupported) &&
        (a.isHeadTrackingSupported == b.isHeadTrackingSupported) && (a.spatialDeviceType == b.spatialDeviceType));
}

AudioSpatializationService::~AudioSpatializationService()
{
    AUDIO_ERR_LOG("~AudioSpatializationService()");
}

bool AudioSpatializationService::Init(void)
{
    return true;
}

void AudioSpatializationService::Deinit(void)
{
    return;
}

const sptr<IStandardAudioService> AudioSpatializationService::GetAudioServerProxy()
{
    AUDIO_DEBUG_LOG("[Spatialization Service] Start get audio spatialization service proxy.");
    lock_guard<mutex> lock(g_adSpatializationProxyMutex);

    if (g_adProxy == nullptr) {
        auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgr == nullptr) {
            AUDIO_ERR_LOG("[Spatialization Service] Get samgr failed.");
            return nullptr;
        }

        sptr<IRemoteObject> object = samgr->GetSystemAbility(AUDIO_DISTRIBUTED_SERVICE_ID);
        if (object == nullptr) {
            AUDIO_ERR_LOG("[Spatialization Service] audio service remote object is NULL.");
            return nullptr;
        }

        g_adProxy = iface_cast<IStandardAudioService>(object);
        if (g_adProxy == nullptr) {
            AUDIO_ERR_LOG("[Spatialization Service] init g_adProxy is NULL.");
            return nullptr;
        }
    }
    const sptr<IStandardAudioService> gsp = g_adProxy;
    return gsp;
}

bool AudioSpatializationService::IsSpatializationEnabled()
{
    std::lock_guard<std::mutex> lock(spatializationServiceMutex_);
    return spatializationEnabledFlag_;
}

int32_t AudioSpatializationService::SetSpatializationEnabled(const bool enable)
{
    std::lock_guard<std::mutex> lock(spatializationServiceMutex_);
    if (spatializationEnabledFlag_ == enable) {
        return SPATIALIZATION_SERVICE_OK;
    }
    spatializationEnabledFlag_ = enable;
    HandleSpatializationEnabledChange(enable);
    if (UpdateSpatializationStateReal() != 0) {
        return ERROR;
    }
    return SPATIALIZATION_SERVICE_OK;
}

bool AudioSpatializationService::IsHeadTrackingEnabled()
{
    std::lock_guard<std::mutex> lock(spatializationServiceMutex_);
    return headTrackingEnabledFlag_;
}

int32_t AudioSpatializationService::SetHeadTrackingEnabled(const bool enable)
{
    std::lock_guard<std::mutex> lock(spatializationServiceMutex_);
    if (headTrackingEnabledFlag_ == enable) {
        return SPATIALIZATION_SERVICE_OK;
    }
    headTrackingEnabledFlag_ = enable;
    HandleHeadTrackingEnabledChange(enable);
    if (UpdateSpatializationStateReal() != 0) {
        return ERROR;
    }
    return SPATIALIZATION_SERVICE_OK;
}

int32_t AudioSpatializationService::RegisterSpatializationEnabledEventListener(int32_t clientPid,
    const sptr<IRemoteObject> &object, bool hasSystemPermission)
{
    std::lock_guard<std::mutex> lock(spatializationEnabledChangeListnerMutex_);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_PARAM,
        "set spatialization enabled event listener object is nullptr");
    sptr<IStandardSpatializationEnabledChangeListener> listener =
        iface_cast<IStandardSpatializationEnabledChangeListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, ERR_INVALID_PARAM, "spatialization enabled obj cast failed");

    std::shared_ptr<AudioSpatializationEnabledChangeCallback> callback =
        std::make_shared<AudioSpatializationEnabledChangeListenerCallback>(listener, hasSystemPermission);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_PARAM,
        "failed to create spatialization enabled cb obj");

    spatializationEnabledCBMap_[clientPid] = callback;
    return SUCCESS;
}

int32_t AudioSpatializationService::RegisterHeadTrackingEnabledEventListener(int32_t clientPid,
    const sptr<IRemoteObject> &object, bool hasSystemPermission)
{
    std::lock_guard<std::mutex> lock(headTrackingEnabledChangeListnerMutex_);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_PARAM,
        "set head tracking enabled event listener object is nullptr");
    sptr<IStandardHeadTrackingEnabledChangeListener> listener =
        iface_cast<IStandardHeadTrackingEnabledChangeListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, ERR_INVALID_PARAM, "head tracking enabled obj cast failed");

    std::shared_ptr<AudioHeadTrackingEnabledChangeCallback> callback =
        std::make_shared<AudioHeadTrackingEnabledChangeListenerCallback>(listener, hasSystemPermission);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_PARAM,
        "failed to create head tracking enabled cb obj");

    headTrackingEnabledCBMap_[clientPid] = callback;
    return SUCCESS;
}

int32_t AudioSpatializationService::UnregisterSpatializationEnabledEventListener(int32_t clientPid)
{
    std::lock_guard<std::mutex> lock(spatializationEnabledChangeListnerMutex_);
    spatializationEnabledCBMap_.erase(clientPid);
    return SUCCESS;
}

int32_t AudioSpatializationService::UnregisterHeadTrackingEnabledEventListener(int32_t clientPid)
{
    std::lock_guard<std::mutex> lock(headTrackingEnabledChangeListnerMutex_);
    headTrackingEnabledCBMap_.erase(clientPid);
    return SUCCESS;
}

void AudioSpatializationService::HandleSpatializationEnabledChange(const bool &enabled)
{
    std::lock_guard<std::mutex> lock(spatializationEnabledChangeListnerMutex_);
    for (auto it = spatializationEnabledCBMap_.begin(); it != spatializationEnabledCBMap_.end(); ++it) {
        std::shared_ptr<AudioSpatializationEnabledChangeCallback> spatializationEnabledChangeCb = it->second;
        if (spatializationEnabledChangeCb == nullptr) {
            AUDIO_ERR_LOG("spatializationEnabledChangeCb : nullptr for client : %{public}d", it->first);
            it = spatializationEnabledCBMap_.erase(it);
            continue;
        }
        spatializationEnabledChangeCb->OnSpatializationEnabledChange(enabled);
    }
}

void AudioSpatializationService::HandleHeadTrackingEnabledChange(const bool &enabled)
{
    std::lock_guard<std::mutex> lock(headTrackingEnabledChangeListnerMutex_);
    for (auto it = headTrackingEnabledCBMap_.begin(); it != headTrackingEnabledCBMap_.end(); ++it) {
        std::shared_ptr<AudioHeadTrackingEnabledChangeCallback> headTrackingEnabledChangeCb = it->second;
        if (headTrackingEnabledChangeCb == nullptr) {
            AUDIO_ERR_LOG("headTrackingEnabledChangeCb : nullptr for client : %{public}d", it->first);
            it = headTrackingEnabledCBMap_.erase(it);
            continue;
        }
        headTrackingEnabledChangeCb->OnHeadTrackingEnabledChange(enabled);
    }
}

AudioSpatializationState AudioSpatializationService::GetSpatializationState(const StreamUsage streamUsage)
{
    std::lock_guard<std::mutex> lock(spatializationServiceMutex_);
    AudioSpatializationState spatializationState = {false, false};
    if (streamUsage != STREAM_USAGE_GAME) {
        spatializationState.spatializationEnabled = spatializationEnabledReal_;
        spatializationState.headTrackingEnabled = headTrackingEnabledReal_;
    }
    return spatializationState;
}

bool AudioSpatializationService::IsSpatializationSupported()
{
    std::lock_guard<std::mutex> lock(spatializationSupportedMutex_);
    return true;
}

bool AudioSpatializationService::IsSpatializationSupportedForDevice(const std::string address)
{
    std::lock_guard<std::mutex> lock(spatializationSupportedMutex_);
    if (!address.empty()) {
        return true;
    }
    if (!addressToSpatialDeviceStateMap_.count(address)) {
        return false;
    }
    return addressToSpatialDeviceStateMap_[address].isSpatializationSupported;
}

bool AudioSpatializationService::IsHeadTrackingSupported()
{
    std::lock_guard<std::mutex> lock(spatializationSupportedMutex_);
    return true;
}

bool AudioSpatializationService::IsHeadTrackingSupportedForDevice(const std::string address)
{
    std::lock_guard<std::mutex> lock(spatializationSupportedMutex_);
    if (!address.empty()) {
        return true;
    }
    if (!addressToSpatialDeviceStateMap_.count(address)) {
        return false;
    }
    return addressToSpatialDeviceStateMap_[address].isHeadTrackingSupported;
}

int32_t AudioSpatializationService::UpdateSpatialDeviceState(const AudioSpatialDeviceState audioSpatialDeviceState)
{
    {
        std::lock_guard<std::mutex> lock(spatializationSupportedMutex_);
        if (addressToSpatialDeviceStateMap_.count(audioSpatialDeviceState.address) > 0 &&
            IsAudioSpatialDeviceStateEqual(addressToSpatialDeviceStateMap_[audioSpatialDeviceState.address],
            audioSpatialDeviceState)) {
            return SPATIALIZATION_SERVICE_OK;
        }
        addressToSpatialDeviceStateMap_[audioSpatialDeviceState.address] = audioSpatialDeviceState;
    }

    std::lock_guard<std::mutex> lock(spatializationServiceMutex_);
    if (UpdateSpatializationStateReal() != 0) {
        return ERROR;
    }
    return SPATIALIZATION_SERVICE_OK;
}

int32_t AudioSpatializationService::RegisterSpatializationStateEventListener(const uint32_t sessionID,
    const StreamUsage streamUsage, const sptr<IRemoteObject> &object)
{
    std::lock_guard<std::mutex> lock(spatializationStateChangeListnerMutex_);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_PARAM,
        "set spatialization state event listener object is nullptr");
    sptr<IStandardSpatializationStateChangeListener> listener =
        iface_cast<IStandardSpatializationStateChangeListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, ERR_INVALID_PARAM, "spatialization state obj cast failed");

    std::shared_ptr<AudioSpatializationStateChangeCallback> callback =
        std::make_shared<AudioSpatializationStateChangeListenerCallback>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_PARAM,
        "failed to create spatialization state cb obj");

    spatializationStateCBMap_[sessionID] = std::make_pair(callback, streamUsage);
    return SUCCESS;
}

int32_t AudioSpatializationService::UnregisterSpatializationStateEventListener(const uint32_t sessionID)
{
    std::lock_guard<std::mutex> lock(spatializationStateChangeListnerMutex_);
    spatializationStateCBMap_.erase(sessionID);
    return SUCCESS;
}

void AudioSpatializationService::UpdateCurrentDevice(const std::string macAddress)
{
    std::lock_guard<std::mutex> lock(spatializationServiceMutex_);
    currentDeviceAddress_ = macAddress;
    if (UpdateSpatializationStateReal() != 0) {
        AUDIO_WARNING_LOG("UpdateSpatializationStateReal fail");
    }
}

int32_t AudioSpatializationService::UpdateSpatializationStateReal()
{
    bool spatializationEnabled = spatializationEnabledFlag_ && IsSpatializationSupported() &&
        IsSpatializationSupportedForDevice(currentDeviceAddress_);
    bool headTrackingEnabled = headTrackingEnabledFlag_ && IsHeadTrackingSupported() &&
        IsHeadTrackingSupportedForDevice(currentDeviceAddress_) && spatializationEnabled;
    if ((spatializationEnabledReal_ == spatializationEnabled) && (headTrackingEnabledReal_ == headTrackingEnabled)) {
        return SUCCESS;
    }
    spatializationEnabledReal_ = spatializationEnabled;
    headTrackingEnabledReal_ = headTrackingEnabled;
    if (UpdateSpatializationState() != 0) {
        return ERROR;
    }
    HandleSpatializationStateChange();
    return SPATIALIZATION_SERVICE_OK;
}

int32_t AudioSpatializationService::UpdateSpatializationState()
{
    const sptr<IStandardAudioService> gsp = GetAudioServerProxy();
    if (gsp == nullptr) {
        AUDIO_ERR_LOG("Service proxy unavailable: g_adProxy null");
        return -1;
    }
    AudioSpatializationState spatializationState = {spatializationEnabledReal_, headTrackingEnabledReal_};
    int32_t ret = gsp->UpdateSpatializationState(spatializationState);
    if (ret != 0) {
        AUDIO_WARNING_LOG("UpdateSpatializationState fail");
    }
    return SPATIALIZATION_SERVICE_OK;
}

void AudioSpatializationService::HandleSpatializationStateChange()
{
    std::lock_guard<std::mutex> lock(spatializationStateChangeListnerMutex_);

    AudioSpatializationState spatializationState = {spatializationEnabledReal_, headTrackingEnabledReal_};
    AudioSpatializationState spatializationNotSupported = {false, false};

    AudioPolicyService::GetAudioPolicyService().UpdateA2dpOffloadFlagBySpatialService(currentDeviceAddress_);

    for (auto it = spatializationStateCBMap_.begin(); it != spatializationStateCBMap_.end(); ++it) {
        std::shared_ptr<AudioSpatializationStateChangeCallback> spatializationStateChangeCb = (it->second).first;
        if (spatializationStateChangeCb == nullptr) {
            AUDIO_ERR_LOG("spatializationStateChangeCb : nullptr for sessionID : %{public}d",
                static_cast<int32_t>(it->first));
            it = spatializationStateCBMap_.erase(it);
            continue;
        }
        if ((it->second).second == STREAM_USAGE_GAME) {
            spatializationStateChangeCb->OnSpatializationStateChange(spatializationNotSupported);
        } else {
            spatializationStateChangeCb->OnSpatializationStateChange(spatializationState);
        }
    }
}
} // namespace AudioStandard
} // namespace OHOS
