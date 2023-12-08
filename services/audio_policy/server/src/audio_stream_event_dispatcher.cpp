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


#include "audio_stream_event_dispatcher.h"

namespace OHOS {
namespace AudioStandard {
AudioStreamEventDispatcher::AudioStreamEventDispatcher()
{
    AUDIO_DEBUG_LOG("AudioStreamEventDispatcher::AudioStreamEventDispatcher()");
}

AudioStreamEventDispatcher::~AudioStreamEventDispatcher()
{
    AUDIO_DEBUG_LOG("AudioStreamEventDispatcher::~AudioStreamEventDispatcher()");
}

void AudioStreamEventDispatcher::AddAudioPolicyClientProxyMap(int32_t clientPid, const sptr<IAudioPolicyClient>& cb)
{
    std::lock_guard<std::mutex> lock(updatePolicyPorxyMapMutex_);
    audioPolicyClientRCProxyCBMap_.emplace(clientPid, cb);
}

void AudioStreamEventDispatcher::ReduceAudioPolicyClientProxyMap(pid_t clientPid)
{
    std::lock_guard<std::mutex> lock(updatePolicyPorxyMapMutex_);
    audioPolicyClientRCProxyCBMap_.erase(clientPid);
}

void AudioStreamEventDispatcher::SendRendererInfoEventToDispatcher(AudioMode mode,
    std::vector<std::unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    AUDIO_DEBUG_LOG("AudioStreamEventDispatcher::SendRendererInfoEventToDispatcher:mode %{public}d ", mode);

    std::lock_guard<std::mutex> lock(streamStateChangeQueueMutex_);

    std::vector<std::unique_ptr<AudioRendererChangeInfo>> rendererChangeInfos;
    for (const auto &changeInfo : audioRendererChangeInfos) {
        rendererChangeInfos.push_back(std::make_unique<AudioRendererChangeInfo>(*changeInfo));
    }

    unique_ptr<StreamStateChangeRequest> streamStateChangeRequest = make_unique<StreamStateChangeRequest>();
    if (!streamStateChangeRequest) {
        AUDIO_ERR_LOG("AudioStreamEventDispatcher::SendRendererInfoEventToDispatcher:Memory alloc failed!!");
        return;
    }
    streamStateChangeRequest->mode = mode;
    streamStateChangeRequest->audioRendererChangeInfos = move(rendererChangeInfos);
    streamStateChangeQueue_.push(move(streamStateChangeRequest));
    DispatcherEvent();
}

void AudioStreamEventDispatcher::SendCapturerInfoEventToDispatcher(AudioMode mode,
    std::vector<std::unique_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos)
{
    AUDIO_DEBUG_LOG("AudioStreamEventDispatcher::SendCapturerInfoEventToDispatcher:mode %{public}d ", mode);

    std::lock_guard<std::mutex> lock(streamStateChangeQueueMutex_);

    std::vector<std::unique_ptr<AudioCapturerChangeInfo>> capturerChangeInfos;
    for (const auto &changeInfo : audioCapturerChangeInfos) {
        capturerChangeInfos.push_back(std::make_unique<AudioCapturerChangeInfo>(*changeInfo));
    }

    unique_ptr<StreamStateChangeRequest> streamStateChangeRequest = make_unique<StreamStateChangeRequest>();
    if (!streamStateChangeRequest) {
        AUDIO_ERR_LOG("AudioStreamEventDispatcher::Memory alloc failed!!");
        return;
    }
    streamStateChangeRequest->mode = mode;
    streamStateChangeRequest->audioCapturerChangeInfos = move(capturerChangeInfos);
    streamStateChangeQueue_.push(move(streamStateChangeRequest));
    DispatcherEvent();
}

void AudioStreamEventDispatcher::HandleRendererStreamStateChange(
    const unique_ptr<StreamStateChangeRequest> &streamStateChangeRequest)
{
    std::lock_guard<std::mutex> lock(rendererStateChangeListnerMutex_);
    for (auto it = audioPolicyClientRCProxyCBMap_.begin(); it != audioPolicyClientRCProxyCBMap_.end(); ++it) {
        sptr<IAudioPolicyClient> rendererStateChangeCb = it->second;
        if (rendererStateChangeCb == nullptr) {
            AUDIO_ERR_LOG("rendererStateChangeCb : nullptr for client : %{public}d", it->first);
            it = audioPolicyClientRCProxyCBMap_.erase(it);
            continue;
        }
        rendererStateChangeCb->OnRendererStateChange(streamStateChangeRequest->audioRendererChangeInfos);
    }
}

void AudioStreamEventDispatcher::HandleCapturerStreamStateChange(
    const unique_ptr<StreamStateChangeRequest> &streamStateChangeRequest)
{
    std::lock_guard<std::mutex> lock(capturerStateChangeListnerMutex_);
    for (auto it = audioPolicyClientRCProxyCBMap_.begin(); it != audioPolicyClientRCProxyCBMap_.end(); ++it) {
        sptr<IAudioPolicyClient> capturerStateChangeCb = it->second;
        if (capturerStateChangeCb == nullptr) {
            AUDIO_ERR_LOG("capturerStateChangeCb : nullptr for client : %{public}d", it->first);
            it = audioPolicyClientRCProxyCBMap_.erase(it);
            continue;
        }
        capturerStateChangeCb->OnCapturerStateChange(streamStateChangeRequest->audioCapturerChangeInfos);
    }
}

void AudioStreamEventDispatcher::DispatcherEvent()
{
    AUDIO_DEBUG_LOG("DispatcherEvent entered");
    while (!streamStateChangeQueue_.empty()) {
        std::unique_ptr<StreamStateChangeRequest> streamStateChangeRequest =
            std::move(streamStateChangeQueue_.front());
        if (streamStateChangeRequest != nullptr) {
            if (streamStateChangeRequest->mode == AUDIO_MODE_PLAYBACK) {
                HandleRendererStreamStateChange(streamStateChangeRequest);
            } else {
                HandleCapturerStreamStateChange(streamStateChangeRequest);
            }
        }
        streamStateChangeQueue_.pop();
    }
}
} // namespace AudioStandard
} // namespace OHOS
