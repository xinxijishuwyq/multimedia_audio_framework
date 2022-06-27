/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "audio_stream_collector.h"

#include "audio_capturer_state_change_listener_proxy.h"
#include "audio_errors.h"
#include "audio_renderer_state_change_listener_proxy.h"
#include "audio_client_tracker_callback_proxy.h"

#include "i_standard_renderer_state_change_listener.h"
#include "i_standard_capturer_state_change_listener.h"
#include "i_standard_client_tracker.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;

AudioStreamCollector::AudioStreamCollector() : mDispatcherService
    (AudioStreamEventDispatcher::GetAudioStreamEventDispatcher())
{
    AUDIO_INFO_LOG("AudioStreamCollector::AudioStreamCollector()");
}

AudioStreamCollector::~AudioStreamCollector()
{
    AUDIO_INFO_LOG("AudioStreamCollector::~AudioStreamCollector()");
}

int32_t AudioStreamCollector::RegisterAudioRendererEventListener(int32_t clientUID, const sptr<IRemoteObject> &object)
{
    AUDIO_INFO_LOG("AudioStreamCollector: RegisterAudioRendererEventListener client id %{public}d done", clientUID);
    std::lock_guard<std::mutex> lock(rendererStateChangeEventMutex_);

    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_PARAM,
        "AudioStreamCollector:set renderer state change event listner object is nullptr");

    sptr<IStandardRendererStateChangeListener> listener = iface_cast<IStandardRendererStateChangeListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, ERR_INVALID_PARAM,
        "AudioStreamCollector: renderer listener obj cast failed");

    std::shared_ptr<AudioRendererStateChangeCallback> callback =
         std::make_shared<AudioRendererStateChangeListenerCallback>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_PARAM, "AudioStreamCollector: failed to  create cb obj");

    mDispatcherService.addRendererListener(clientUID, callback);
    return SUCCESS;
}

int32_t AudioStreamCollector::UnregisterAudioRendererEventListener(int32_t clientUID)
{
    AUDIO_INFO_LOG("AudioStreamCollector::UnregisterAudioRendererEventListener()");
    mDispatcherService.removeRendererListener(clientUID);
    return SUCCESS;
}

int32_t AudioStreamCollector::RegisterAudioCapturerEventListener(int32_t clientUID, const sptr<IRemoteObject> &object)
{
    AUDIO_INFO_LOG("AudioStreamCollector: RegisterAudioCapturerEventListener for client id %{public}d done", clientUID);
    std::lock_guard<std::mutex> lock(capturerStateChangeEventMutex_);

    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_PARAM,
        "AudioStreamCollector:set capturer event listner object is nullptr");

    sptr<IStandardCapturerStateChangeListener> listener = iface_cast<IStandardCapturerStateChangeListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, ERR_INVALID_PARAM, "AudioStreamCollector: capturer obj cast failed");

    std::shared_ptr<AudioCapturerStateChangeCallback> callback =
        std::make_shared<AudioCapturerStateChangeListenerCallback>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_PARAM,
        "AudioStreamCollector: failed to create capturer cb obj");

    mDispatcherService.addCapturerListener(clientUID, callback);
    return SUCCESS;
}

int32_t AudioStreamCollector::UnregisterAudioCapturerEventListener(int32_t clientUID)
{
    AUDIO_INFO_LOG("AudioStreamCollector: UnregisterAudioCapturerEventListener client id %{public}d done", clientUID);
    mDispatcherService.removeCapturerListener(clientUID);
    return SUCCESS;
}

int32_t AudioStreamCollector::AddRendererStream(AudioStreamChangeInfo &streamChangeInfo)
{
    AUDIO_INFO_LOG("AudioStreamCollector: AddRendererStream playback client id %{public}d session %{public}d",
        streamChangeInfo.audioRendererChangeInfo.clientUID, streamChangeInfo.audioRendererChangeInfo.sessionId);

    rendererStatequeue_.insert({{streamChangeInfo.audioRendererChangeInfo.clientUID,
        streamChangeInfo.audioRendererChangeInfo.sessionId},
        streamChangeInfo.audioRendererChangeInfo.rendererState});

    unique_ptr<AudioRendererChangeInfo> rendererChangeInfo = make_unique<AudioRendererChangeInfo>();
    if (!rendererChangeInfo) {
        AUDIO_ERR_LOG("AudioStreamCollector::AddRendererStream Memory Allocation Failed");
        return ERR_MEMORY_ALLOC_FAILED;
    }
    rendererChangeInfo->clientUID = streamChangeInfo.audioRendererChangeInfo.clientUID;
    rendererChangeInfo->sessionId = streamChangeInfo.audioRendererChangeInfo.sessionId;
    rendererChangeInfo->rendererState = streamChangeInfo.audioRendererChangeInfo.rendererState;
    rendererChangeInfo->rendererInfo = streamChangeInfo.audioRendererChangeInfo.rendererInfo;
    audioRendererChangeInfos_.push_back(move(rendererChangeInfo));

    AUDIO_DEBUG_LOG("AudioStreamCollector: audioRendererChangeInfos_: Added for client %{public}d session %{public}d",
        streamChangeInfo.audioRendererChangeInfo.clientUID, streamChangeInfo.audioRendererChangeInfo.sessionId);

    mDispatcherService.SendRendererInfoEventToDispatcher(AudioMode::AUDIO_MODE_PLAYBACK, audioRendererChangeInfos_);
    return SUCCESS;
}

int32_t AudioStreamCollector::AddCapturerStream(AudioStreamChangeInfo &streamChangeInfo)
{
    AUDIO_INFO_LOG("AudioStreamCollector: AddCapturerStream recording client id %{public}d session %{public}d",
        streamChangeInfo.audioCapturerChangeInfo.clientUID, streamChangeInfo.audioCapturerChangeInfo.sessionId);

    capturerStatequeue_.insert({{streamChangeInfo.audioCapturerChangeInfo.clientUID,
        streamChangeInfo.audioCapturerChangeInfo.sessionId},
        streamChangeInfo.audioCapturerChangeInfo.capturerState});

    unique_ptr<AudioCapturerChangeInfo> capturerChangeInfo = make_unique<AudioCapturerChangeInfo>();
    if (!capturerChangeInfo) {
        AUDIO_ERR_LOG("AudioStreamCollector::AddCapturerStream Memory Allocation Failed");
        return ERR_MEMORY_ALLOC_FAILED;
    }
    capturerChangeInfo->clientUID = streamChangeInfo.audioCapturerChangeInfo.clientUID;
    capturerChangeInfo->sessionId = streamChangeInfo.audioCapturerChangeInfo.sessionId;
    capturerChangeInfo->capturerState = streamChangeInfo.audioCapturerChangeInfo.capturerState;
    capturerChangeInfo->capturerInfo = streamChangeInfo.audioCapturerChangeInfo.capturerInfo;
    audioCapturerChangeInfos_.push_back(move(capturerChangeInfo));

    AUDIO_DEBUG_LOG("AudioStreamCollector: audioCapturerChangeInfos_: Added for client %{public}d session %{public}d",
        streamChangeInfo.audioCapturerChangeInfo.clientUID, streamChangeInfo.audioCapturerChangeInfo.sessionId);

    mDispatcherService.SendCapturerInfoEventToDispatcher(AudioMode::AUDIO_MODE_RECORD, audioCapturerChangeInfos_);
    return SUCCESS;
}

int32_t AudioStreamCollector::RegisterTracker(AudioMode &mode, AudioStreamChangeInfo &streamChangeInfo,
    const sptr<IRemoteObject> &object)
{
    AUDIO_INFO_LOG("AudioStreamCollector: RegisterTracker mode %{public}d", mode);

    DisplayInternalStreamInfo();
    int32_t clientUID;
    if (mode == AUDIO_MODE_PLAYBACK) {
        AddRendererStream(streamChangeInfo);
        clientUID = streamChangeInfo.audioRendererChangeInfo.clientUID;
    } else {
        // mode = AUDIO_MODE_RECORD
        AddCapturerStream(streamChangeInfo);
        clientUID = streamChangeInfo.audioCapturerChangeInfo.clientUID;
    }
    DisplayInternalStreamInfo();
	
    sptr<IStandardClientTracker> listener = iface_cast<IStandardClientTracker>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr,
        ERR_INVALID_PARAM, "AudioStreamCollector: client tracker obj cast failed");
    std::shared_ptr<AudioClientTracker> callback = std::make_shared<ClientTrackerCallbackListener>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr,
        ERR_INVALID_PARAM, "AudioStreamCollector: failed to create tracker cb obj");
    clientTracker_[clientUID] = callback;

    return SUCCESS;
}

int32_t AudioStreamCollector::UpdateRendererStream(AudioStreamChangeInfo &streamChangeInfo)
{
    AUDIO_INFO_LOG("AudioStreamCollector: UpdateRendererStream client %{public}d state %{public}d session %{public}d",
        streamChangeInfo.audioRendererChangeInfo.clientUID, streamChangeInfo.audioRendererChangeInfo.rendererState,
        streamChangeInfo.audioRendererChangeInfo.sessionId);

    if (rendererStatequeue_.find(make_pair(streamChangeInfo.audioRendererChangeInfo.clientUID,
        streamChangeInfo.audioRendererChangeInfo.sessionId)) != rendererStatequeue_.end()) {
        if (streamChangeInfo.audioRendererChangeInfo.rendererState ==
            rendererStatequeue_[make_pair(streamChangeInfo.audioRendererChangeInfo.clientUID,
                streamChangeInfo.audioRendererChangeInfo.sessionId)]) {
            // Renderer state not changed
            return SUCCESS;
        }
    } else {
        AUDIO_INFO_LOG("UpdateRendererStream client %{public}d not found in rendererStatequeue_",
            streamChangeInfo.audioRendererChangeInfo.clientUID);
    }

    // Update the renderer info in audioRendererChangeInfos_
    for (auto it = audioRendererChangeInfos_.begin(); it != audioRendererChangeInfos_.end(); it++) {
        AudioRendererChangeInfo audioRendererChangeInfo = **it;
        if (audioRendererChangeInfo.clientUID == streamChangeInfo.audioRendererChangeInfo.clientUID &&
            audioRendererChangeInfo.sessionId == streamChangeInfo.audioRendererChangeInfo.sessionId) {
            rendererStatequeue_[make_pair(audioRendererChangeInfo.clientUID, audioRendererChangeInfo.sessionId)] =
                streamChangeInfo.audioRendererChangeInfo.rendererState;
            AUDIO_DEBUG_LOG("AudioStreamCollector: UpdateRendererStream: update client %{public}d session %{public}d",
                audioRendererChangeInfo.clientUID, audioRendererChangeInfo.sessionId);

            unique_ptr<AudioRendererChangeInfo> RendererChangeInfo = make_unique<AudioRendererChangeInfo>();
            RendererChangeInfo->clientUID = streamChangeInfo.audioRendererChangeInfo.clientUID;
            RendererChangeInfo->sessionId = streamChangeInfo.audioRendererChangeInfo.sessionId;
            RendererChangeInfo->rendererState = streamChangeInfo.audioRendererChangeInfo.rendererState;
            RendererChangeInfo->rendererInfo = streamChangeInfo.audioRendererChangeInfo.rendererInfo;
            *it = move(RendererChangeInfo);
            AUDIO_DEBUG_LOG("AudioStreamCollector: Playback details updated, to be dispatched");

            mDispatcherService.SendRendererInfoEventToDispatcher(AudioMode::AUDIO_MODE_PLAYBACK,
                audioRendererChangeInfos_);

            if (streamChangeInfo.audioRendererChangeInfo.rendererState == RENDERER_RELEASED) {
                audioRendererChangeInfos_.erase(it);
                AUDIO_DEBUG_LOG("AudioStreamCollector: Session removed for client %{public}d session %{public}d",
                    streamChangeInfo.audioRendererChangeInfo.clientUID,
                    streamChangeInfo.audioRendererChangeInfo.sessionId);
                rendererStatequeue_.erase(make_pair(audioRendererChangeInfo.clientUID,
                    audioRendererChangeInfo.sessionId));
                clientTracker_.erase(audioRendererChangeInfo.clientUID);
            }
            return SUCCESS;
        }
    }
    AUDIO_INFO_LOG("UpdateRendererStream: Not found clientUid:%{public}d sessionId:%{public}d",
        streamChangeInfo.audioRendererChangeInfo.clientUID, streamChangeInfo.audioRendererChangeInfo.clientUID);
    return SUCCESS;
}

int32_t AudioStreamCollector::UpdateCapturerStream(AudioStreamChangeInfo &streamChangeInfo)
{
    AUDIO_INFO_LOG("AudioStreamCollector: UpdateCapturerStream client %{public}d state %{public}d session %{public}d",
        streamChangeInfo.audioCapturerChangeInfo.clientUID, streamChangeInfo.audioCapturerChangeInfo.capturerState,
        streamChangeInfo.audioCapturerChangeInfo.sessionId);

    if (capturerStatequeue_.find(make_pair(streamChangeInfo.audioCapturerChangeInfo.clientUID,
        streamChangeInfo.audioCapturerChangeInfo.sessionId)) != capturerStatequeue_.end()) {
        if (streamChangeInfo.audioCapturerChangeInfo.capturerState ==
            capturerStatequeue_[make_pair(streamChangeInfo.audioCapturerChangeInfo.clientUID,
                streamChangeInfo.audioCapturerChangeInfo.sessionId)]) {
            // Capturer state not changed
            return SUCCESS;
        }
    } else {
        AUDIO_INFO_LOG("AudioStreamCollector: UpdateCapturerStream client %{public}d not found in capturerStatequeue_",
            streamChangeInfo.audioCapturerChangeInfo.clientUID);
    }

    // Update the capturer info in audioCapturerChangeInfos_
    for (auto it = audioCapturerChangeInfos_.begin(); it != audioCapturerChangeInfos_.end(); it++) {
        AudioCapturerChangeInfo audioCapturerChangeInfo = **it;
        if (audioCapturerChangeInfo.clientUID == streamChangeInfo.audioCapturerChangeInfo.clientUID &&
            audioCapturerChangeInfo.sessionId == streamChangeInfo.audioCapturerChangeInfo.sessionId) {
            capturerStatequeue_[make_pair(audioCapturerChangeInfo.clientUID, audioCapturerChangeInfo.sessionId)] =
                streamChangeInfo.audioCapturerChangeInfo.capturerState;

            AUDIO_DEBUG_LOG("AudioStreamCollector: Session is updated for client %{public}d session %{public}d",
                streamChangeInfo.audioCapturerChangeInfo.clientUID,
                streamChangeInfo.audioCapturerChangeInfo.sessionId);

            unique_ptr<AudioCapturerChangeInfo> CapturerChangeInfo = make_unique<AudioCapturerChangeInfo>();
            CapturerChangeInfo->clientUID = streamChangeInfo.audioCapturerChangeInfo.clientUID;
            CapturerChangeInfo->sessionId = streamChangeInfo.audioCapturerChangeInfo.sessionId;
            CapturerChangeInfo->capturerState = streamChangeInfo.audioCapturerChangeInfo.capturerState;
            CapturerChangeInfo->capturerInfo = streamChangeInfo.audioCapturerChangeInfo.capturerInfo;
            *it = move(CapturerChangeInfo);

            mDispatcherService.SendCapturerInfoEventToDispatcher(AudioMode::AUDIO_MODE_RECORD,
                audioCapturerChangeInfos_);
            if (streamChangeInfo.audioCapturerChangeInfo.capturerState ==  CAPTURER_RELEASED) {
                audioCapturerChangeInfos_.erase(it);
                AUDIO_DEBUG_LOG("UpdateCapturerStream::Session is removed for client %{public}d session %{public}d",
                    streamChangeInfo.audioCapturerChangeInfo.clientUID,
                    streamChangeInfo.audioCapturerChangeInfo.sessionId);
                capturerStatequeue_.erase(make_pair(audioCapturerChangeInfo.clientUID,
                    audioCapturerChangeInfo.sessionId));
                clientTracker_.erase(audioCapturerChangeInfo.clientUID);
            }
        return SUCCESS;
        }
    }
    AUDIO_INFO_LOG("AudioStreamCollector:UpdateCapturerStream: clientUI not in audioCapturerChangeInfos_::%{public}d",
        streamChangeInfo.audioCapturerChangeInfo.clientUID);
    return SUCCESS;
}

int32_t AudioStreamCollector::UpdateTracker(AudioMode &mode, AudioStreamChangeInfo &streamChangeInfo)
{
    DisplayInternalStreamInfo();
    // update the stream change info
    if (mode == AUDIO_MODE_PLAYBACK) {
        UpdateRendererStream(streamChangeInfo);
    } else {
    // mode = AUDIO_MODE_RECORD
        UpdateCapturerStream(streamChangeInfo);
    }
    DisplayInternalStreamInfo();
    return SUCCESS;
}

void AudioStreamCollector::DisplayInternalStreamInfo()
{
    AUDIO_DEBUG_LOG("AudioStreamCollector: Display audioRendererChangeInfos_ Number of entries %{public}u",
        static_cast<uint32_t>(audioRendererChangeInfos_.size()));
    int index = 0;
    for (auto it = audioRendererChangeInfos_.begin(); it != audioRendererChangeInfos_.end(); it++) {
        AudioRendererChangeInfo audioRendererChangeInfo = **it;
        AUDIO_DEBUG_LOG("audioRendererChangeInfos_[%{public}d]", index++);
        AUDIO_DEBUG_LOG("clientUID = %{public}d", audioRendererChangeInfo.clientUID);
        AUDIO_DEBUG_LOG("sessionId = %{public}d", audioRendererChangeInfo.sessionId);
        AUDIO_DEBUG_LOG("rendererState = %{public}d", audioRendererChangeInfo.rendererState);
    }

    AUDIO_DEBUG_LOG("rendererStatequeue_");
    for (auto const &pair: rendererStatequeue_) {
        AUDIO_DEBUG_LOG("clientUID: %{public}d:sessionId %{public}d :: RendererState : %{public}d",
            pair.first.first, pair.first.second, pair.second);
    }

    AUDIO_DEBUG_LOG("AudioStreamCollector: Display audioCapturerChangeInfos_ Number of entries %{public}u",
        static_cast<uint32_t>(audioCapturerChangeInfos_.size()));
    index = 0;
    for (auto it = audioCapturerChangeInfos_.begin(); it != audioCapturerChangeInfos_.end(); it++) {
        AudioCapturerChangeInfo audioCapturerChangeInfo = **it;
        AUDIO_DEBUG_LOG("audioCapturerChangeInfos_[%{public}d]", index++);
        AUDIO_DEBUG_LOG("clientUID = %{public}d", audioCapturerChangeInfo.clientUID);
        AUDIO_DEBUG_LOG("sessionId = %{public}d", audioCapturerChangeInfo.sessionId);
        AUDIO_DEBUG_LOG("capturerState = %{public}d", audioCapturerChangeInfo.capturerState);
    }

    AUDIO_DEBUG_LOG("capturerStatequeue_");
    for (auto const &pair: capturerStatequeue_) {
        AUDIO_DEBUG_LOG("clientUID: %{public}d sessionId %{public}d :: CapturerState : %{public}d",
            pair.first.first, pair.first.second, pair.second);
    }
}

int32_t AudioStreamCollector::GetCurrentRendererChangeInfos(
    vector<unique_ptr<AudioRendererChangeInfo>> &rendererChangeInfos)
{
    DisplayInternalStreamInfo();
    for (const auto &changeInfo : audioRendererChangeInfos_) {
        rendererChangeInfos.push_back(make_unique<AudioRendererChangeInfo>(*changeInfo));
    }
    AUDIO_DEBUG_LOG("AudioStreamCollector::GetCurrentRendererChangeInfos returned");

    return SUCCESS;
}

int32_t AudioStreamCollector::GetCurrentCapturerChangeInfos(
    vector<unique_ptr<AudioCapturerChangeInfo>> &capturerChangeInfos)
{
    AUDIO_DEBUG_LOG("AudioStreamCollector::GetCurrentCapturerChangeInfos");
    DisplayInternalStreamInfo();
    for (const auto &changeInfo : audioCapturerChangeInfos_) {
        capturerChangeInfos.push_back(make_unique<AudioCapturerChangeInfo>(*changeInfo));
    AUDIO_DEBUG_LOG("AudioStreamCollector::GetCurrentCapturerChangeInfos returned");
    }

    return SUCCESS;
}

void AudioStreamCollector::RegisteredTrackerClientDied(int32_t uid)
{
    AUDIO_DEBUG_LOG("TrackerClientDied:client %{public}d Died", uid);

    // Send the release state event notification for all stream of died client to registered app
    for (auto it = audioRendererChangeInfos_.begin(); it != audioRendererChangeInfos_.end(); it++) {
        AudioRendererChangeInfo audioRendererChangeInfo = **it;
        if (audioRendererChangeInfo.clientUID == uid) {
            AUDIO_DEBUG_LOG("TrackerClientDied:client %{public}d session %{public}d Renderer Release",
                uid, audioRendererChangeInfo.sessionId);
            audioRendererChangeInfo.rendererState = RENDERER_RELEASED;
            mDispatcherService.SendRendererInfoEventToDispatcher(AudioMode::AUDIO_MODE_PLAYBACK,
                audioRendererChangeInfos_);
            rendererStatequeue_.erase(make_pair(audioRendererChangeInfo.clientUID, audioRendererChangeInfo.sessionId));
            audioRendererChangeInfos_.erase(it);
        }
    }

    for (auto it = audioCapturerChangeInfos_.begin(); it != audioCapturerChangeInfos_.end(); it++) {
        AudioCapturerChangeInfo audioCapturerChangeInfo = **it;
        if (audioCapturerChangeInfo.clientUID == uid) {
            AUDIO_DEBUG_LOG("TrackerClientDied:client %{public}d session %{public}d Capturer Release",
                uid, audioCapturerChangeInfo.sessionId);
            audioCapturerChangeInfo.capturerState = CAPTURER_RELEASED;
            mDispatcherService.SendCapturerInfoEventToDispatcher(AudioMode::AUDIO_MODE_RECORD,
                audioCapturerChangeInfos_);
            capturerStatequeue_.erase(make_pair(audioCapturerChangeInfo.clientUID, audioCapturerChangeInfo.sessionId));
            audioCapturerChangeInfos_.erase(it);
        }
    }
    if (clientTracker_.erase(uid)) {
        AUDIO_DEBUG_LOG("AudioStreamCollector::TrackerClientDied:client %{public}d cleared", uid);
        return;
    }
    AUDIO_INFO_LOG("AudioStreamCollector::TrackerClientDied:client %{public}d not present", uid);
}

void AudioStreamCollector::RegisteredStreamListenerClientDied(int32_t uid)
{
    AUDIO_INFO_LOG("AudioStreamCollector::StreamListenerClientDied:client %{public}d", uid);
    mDispatcherService.removeRendererListener(uid);
    mDispatcherService.removeCapturerListener(uid);
}
} // namespace AudioStandard
} // namespace OHOS
