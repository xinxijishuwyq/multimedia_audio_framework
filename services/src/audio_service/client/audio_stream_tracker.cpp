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

#include "audio_stream_tracker.h"

namespace OHOS {
namespace AudioStandard {
AudioStreamTracker::AudioStreamTracker(AudioMode mode, int32_t clientUID)
{
    AUDIO_DEBUG_LOG("AudioStreamtracker:CTOR");
    eMode_ = mode;
    clientUID_ = clientUID;
    state_ = INVALID;
}

AudioStreamTracker::~AudioStreamTracker() {}

void AudioStreamTracker::RegisterTracker(const int32_t sessionId, const State state,
    const AudioRendererInfo &rendererInfo, const AudioCapturerInfo &capturerInfo,
    const std::shared_ptr<AudioClientTracker> &clientTrackerObj)
{
    AUDIO_DEBUG_LOG("AudioStreamtracker:Register tracker entered");
    AudioStreamChangeInfo streamChangeInfo;

    state_ = state;

    if (eMode_ == AUDIO_MODE_PLAYBACK) {
        streamChangeInfo.audioRendererChangeInfo.clientUID = clientUID_;
        streamChangeInfo.audioRendererChangeInfo.sessionId = sessionId;
        streamChangeInfo.audioRendererChangeInfo.rendererState = static_cast<RendererState>(state);
        streamChangeInfo.audioRendererChangeInfo.rendererInfo = rendererInfo;
    } else {
        streamChangeInfo.audioCapturerChangeInfo.clientUID = clientUID_;
        streamChangeInfo.audioCapturerChangeInfo.sessionId = sessionId;
        streamChangeInfo.audioCapturerChangeInfo.capturerState = static_cast<CapturerState>(state);
        streamChangeInfo.audioCapturerChangeInfo.capturerInfo = capturerInfo;
    }
    AudioPolicyManager::GetInstance().RegisterTracker(eMode_, streamChangeInfo, clientTrackerObj);
}

void AudioStreamTracker::UpdateTracker(const int32_t sessionId, const State state,
    const AudioRendererInfo &rendererInfo, const AudioCapturerInfo &capturerInfo)
{
    AUDIO_DEBUG_LOG("AudioStreamtracker:Update tracker entered");
    AudioStreamChangeInfo streamChangeInfo;

    if (state_ == INVALID || state_ == state) {
        AUDIO_DEBUG_LOG("AudioStreamtracker:Update tracker is called in wrong state/same state");
        return;
    }

    state_ = state;
    if (eMode_ == AUDIO_MODE_PLAYBACK) {
        streamChangeInfo.audioRendererChangeInfo.clientUID = clientUID_;
        streamChangeInfo.audioRendererChangeInfo.sessionId = sessionId;
        streamChangeInfo.audioRendererChangeInfo.rendererState = static_cast<RendererState>(state);
        streamChangeInfo.audioRendererChangeInfo.rendererInfo = rendererInfo;
    } else {
        streamChangeInfo.audioCapturerChangeInfo.clientUID = clientUID_;
        streamChangeInfo.audioCapturerChangeInfo.sessionId = sessionId;
        streamChangeInfo.audioCapturerChangeInfo.capturerState = static_cast<CapturerState>(state);
        streamChangeInfo.audioCapturerChangeInfo.capturerInfo = capturerInfo;
    }
    AudioPolicyManager::GetInstance().UpdateTracker(eMode_, streamChangeInfo);
}
} // namespace AudioStandard
} // namespace OHOS
