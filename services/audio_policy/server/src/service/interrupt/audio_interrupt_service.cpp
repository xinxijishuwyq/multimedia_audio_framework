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

#include "audio_interrupt_service.h"

#include "audio_log.h"
#include "audio_errors.h"

namespace OHOS {
namespace AudioStandard {

AudioInterruptService::AudioInterruptService()
{
}

AudioInterruptService::~AudioInterruptService()
{
    AUDIO_ERR_LOG("should not happen");
}

int32_t AudioInterruptService::SetAudioManagerInterruptCallback(const sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALId_PARAM,
        "object is nullptr");

    sptr<IStandardAudioPolicyManagerListener> listener = iface_cast<IStandardAudioPolicyManagerListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, ERR_INVALId_PARAM,
        "obj cast failed");

    std::shared_ptr<AudioInterruptCallback> callback = std::make_shared<AudioPolicyManagerListenerCallback>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALId_PARAM,
        "create cb failed");

    int32_t callerPid = IPCSkeleton::GetCallingPid();

    if (handler_ != nullptr) {
        handler_->AddExternInterruptCbsMap(callerPid, callback);
    }

    AUDIO_DEBUG_LOG("for client id %{public}d done", callerPid);

    return SUCCESS;
}

int32_t AudioInterruptService::UnsetAudioManagerInterruptCallback()
{
    int32_t callerPid = IPCSkeleton::GetCallingPid();
    if (handler_ != nullptr) {
        return handler_->RemoveExternInterruptCbsMap(callerPid);
    }

    return SUCCESS;
}

int32_t AudioInterruptService::RequestAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt)
{
    AUDIO_INFO_LOG("in");
    std::lock_guard<std::mutex> lock(mutex_);

    if (clientOnFocus_ == clientId) {
        AUDIO_INFO_LOG("client already has focus");
        NotifyFocusGranted(clientId, audioInterrupt);
        return SUCCESS;
    }

    if (focussedAudioInterruptInfo_ != nullptr) {
        AUDIO_INFO_LOG("Existing stream: %{public}d, incoming stream: %{public}d",
            (focussedAudioInterruptInfo_->audioFocusType).streamType, audioInterrupt.audioFocusType.streamType);
        NotifyFocusAbandoned(clientOnFocus_, *focussedAudioInterruptInfo_);
        AbandonAudioFocusInternal(clientOnFocus_, *focussedAudioInterruptInfo_);
    }

    NotifyFocusGranted(clientId, audioInterrupt);

    return SUCCESS;
}

int32_t AudioInterruptService::AbandonAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt)
{
    AUDIO_INFO_LOG("in");
    std::lock_guard<std::mutex> lock(mutex_);

    return AbandonAudioFocusInternal(clientId, audioInterrupt);
}

int32_t AudioInterruptService::SetAudioInterruptCallback(const int32_t zoneId, const uint32_t sessionId,
    const sptr<IRemoteObject> &object)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto callerUid = IPCSkeleton::GetCallingUid();
    bool ret = audioPolicyService_.IsSessionIdValid(callerUid, sessionId);
    CHECK_AND_RETURN_RET_LOG(ret, ERR_INVALId_PARAM,
        "for sessionId %{public}d, id is invalid", sessionId);

    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALId_PARAM, "object is nullptr");

    sptr<IStandardAudioPolicyManagerListener> listener = iface_cast<IStandardAudioPolicyManagerListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, ERR_INVALId_PARAM, "obj cast failed");

    std::shared_ptr<AudioInterruptCallback> callback = std::make_shared<AudioPolicyManagerListenerCallback>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALId_PARAM, "create cb failed");

    if (handler_ != nullptr) {
        handler_->AddInterruptCbsMap(sessionId, callback);
        auto it = audioInterruptZonesMap_.find(zoneId);
        if (it != audioInterruptZonesMap_.end() && it->second != nullptr) {
            it->second->interruptCbsMap[sessionId] = callback;
            audioInterruptZonesMap_[zoneId] = it->second;
        }
    }

    return SUCCESS;
}

int32_t AudioInterruptService::UnsetAudioInterruptCallback(const int32_t zoneId, const uint32_t sessionId)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (handler_ != nullptr) {
        auto it = audioInterruptZonesMap_.find(zoneId);
        if (it != audioInterruptZonesMap_.end() && it->second != nullptr &&
            it->second->interruptCbsMap.find(sessionId) != it->second->interruptCbsMap.end()) {
            it->second->interruptCbsMap.erase(it->second->interruptCbsMap.find(sessionId));
            audioInterruptZonesMap_[zoneId] = it->second;
        }
        return handler_->RemoveInterruptCbsMap(sessionId);
    }
    return SUCCESS;
}

int32_t AudioInterruptService::ActivateAudioInterrupt(const AudioInterrupt &audioInterrupt, const int32_t zoneId)
{
    std::lock_guard<std::mutex> lock(mutex_);

    AudioStreamType streamType = audioInterrupt.audioFocusType.streamType;
    uint32_t incomingSessionId = audioInterrupt.sessionId;
    AUDIO_INFO_LOG("sessionId: %{public}u pid: %{public}d streamType: %{public}d "\
        "usage: %{public}d source: %{public}d",
        incomingSessionId, audioInterrupt.pid, streamType,
        audioInterrupt.streamUsage, (audioInterrupt.audioFocusType).sourceType);

    if (audioInterrupt.parallelPlayFlag) {
        AUDIO_INFO_LOG("allow parallel play");
        return SUCCESS;
    }

    OffloadStreamCheck(incomingSessionId, streamType, OFFLOAD_NO_SESSION_Id);

    bool shouldReturnSuccess = false;
    ProcessAudioScene(audioInterrupt, incomingSessionId, zoneId, shouldReturnSuccess);
    if (shouldReturnSuccess) {
        return SUCCESS;
    }
    // Process ProcessFocusEntryTable for current audioFocusInfoList
    int32_t ret = ProcessFocusEntry(audioInterrupt, zoneId);
    CHECK_AND_RETURN_RET_LOG(!ret, ERR_FOCUS_DENIED, "request rejected");

    AudioScene targetAudioScene = GetHighestPriorityAudioSceneFromAudioFocusInfoList(zoneId);
    UpdateAudioScene(targetAudioScene, ACTIVATE_AUDIO_INTERRUPT);
    return SUCCESS;
}

int32_t AudioInterruptService::DeactivateAudioInterrupt(const AudioInterrupt &audioInterrupt, const int32_t zoneID)
{
    std::lock_guard<std::mutex> lock(mutex_);

    AudioScene highestPriorityAudioScene = AUDIO_SCENE_DEFAULT;

    std::list<std::pair<AudioInterrupt, AudioFocuState>> audioFocusInfoList {};
    auto itZone = audioInterruptZonesMap_.find(zoneID);
    if (itZone != audioInterruptZonesMap_.end() && itZone->second != nullptr) {
        audioFocusInfoList = itZone->second->audioFocusInfoList;
    }

    if (!audioPolicyService_.IsAudioInterruptEnabled()) {
        AUDIO_WARNING_LOG("interrupt is not enabled, no need to deactivate");
        uint32_t exitSessionID = audioInterrupt.sessionID;
        int32_t exitPid = audioInterrupt.pid;
        audioFocusInfoList.remove_if([&](std::pair<AudioInterrupt, AudioFocuState> &audioFocusInfo) {
            if ((audioFocusInfo.first).sessionID != exitSessionID) {
                AudioScene targetAudioScene = GetAudioSceneFromAudioInterrupt(audioFocusInfo.first);
                if (GetAudioScenePriority(targetAudioScene) > GetAudioScenePriority(highestPriorityAudioScene)) {
                    highestPriorityAudioScene = targetAudioScene;
                }
                return false;
            }
            OnAudioFocusInfoChange(AudioPolicyServerHandler::ABANDON_CALLBACK_CATEGORY, audioInterrupt, zoneID);
            return true;
        });

        if (itZone->second != nullptr) {
            itZone->second->zoneID = zoneID;
            if (itZone->second->pids.find(exitPid) != itZone->second->pids.end()) {
                itZone->second->pids.erase(itZone->second->pids.find(exitPid));
            }
            itZone->second->audioFocusInfoList = audioFocusInfoList;
            audioInterruptZonesMap_[zoneID] = itZone->second;
        }

        UpdateAudioScene(highestPriorityAudioScene, DEACTIVATE_AUDIO_INTERRUPT);
        return SUCCESS;
    }

    AUDIO_INFO_LOG("sessionID: %{public}u pid: %{public}d streamType: %{public}d "\
        "streamUsage: %{public}d sourceType: %{public}d",
        audioInterrupt.sessionID, audioInterrupt.pid, (audioInterrupt.audioFocusType).streamType,
        audioInterrupt.streamUsage, (audioInterrupt.audioFocusType).sourceType);

    if (audioInterrupt.parallelPlayFlag) {
        AUDIO_INFO_LOG("allow parallel play");
        return SUCCESS;
    }
    return DeactivateAudioInterruptEnable(audioInterrupt, zoneID);
}

void AudioInterruptService::NotifyFocusGranted(const int32_t clientId, const AudioInterrupt &audioInterrupt)
{
    AUDIO_INFO_LOG("Notify focus granted in: %{public}d", clientId);

    InterruptEventInternal interruptEvent = {};
    interruptEvent.eventType = INTERRUPT_TYPE_END;
    interruptEvent.forceType = INTERRUPT_SHARE;
    interruptEvent.hintType = INTERRUPT_HINT_NONE;
    interruptEvent.duckVolume = 0;

    if (handler_ != nullptr) {
        handler_->SendInterruptEventWithClientIdCallBack(interruptEvent, clientId);
        unique_ptr<AudioInterrupt> tempAudioInterruptInfo = make_unique<AudioInterrupt>();
        tempAudioInterruptInfo->streamUsage = audioInterrupt.streamUsage;
        tempAudioInterruptInfo->contentType = audioInterrupt.contentType;
        (tempAudioInterruptInfo->audioFocusType).streamType = audioInterrupt.audioFocusType.streamType;
        tempAudioInterruptInfo->pauseWhenDucked = audioInterrupt.pauseWhenDucked;
        focussedAudioInterruptInfo_ = move(tempAudioInterruptInfo);
        clientOnFocus_ = clientId;
    }
}

int32_t AudioInterruptService::NotifyFocusAbandoned(const int32_t clientId, const AudioInterrupt &audioInterrupt)
{
    AUDIO_INFO_LOG("Notify focus abandoned in: %{public}d", clientId);

    InterruptEventInternal interruptEvent = {};
    interruptEvent.eventType = INTERRUPT_TYPE_BEGIN;
    interruptEvent.forceType = INTERRUPT_SHARE;
    interruptEvent.hintType = INTERRUPT_HINT_STOP;
    interruptEvent.duckVolume = 0;
    if (handler_ != nullptr) {
        handler_->SendInterruptEventWithClientIdCallBack(interruptEvent, clientId);
    }

    return SUCCESS;
}

int32_t AudioInterruptService::AbandonAudioFocusInternal(const int32_t clientId, const AudioInterrupt &audioInterrupt)
{
    if (clientId == clientOnFocus_) {
        AUDIO_INFO_LOG("remove app focus");
        focussedAudioInterruptInfo_.reset();
        focussedAudioInterruptInfo_ = nullptr;
        clientOnFocus_ = 0;
    }

    return SUCCESS;
}

} // namespace AudioStandard
} // namespace OHOS
