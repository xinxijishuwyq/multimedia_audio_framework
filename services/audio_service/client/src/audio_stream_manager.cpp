/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#include "audio_policy_manager.h"
#include "audio_log.h"
#include "audio_stream_manager.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;
AudioStreamManager *AudioStreamManager::GetInstance()
{
    static AudioStreamManager audioStreamManager;
    return &audioStreamManager;
}

int32_t AudioStreamManager::RegisterAudioRendererEventListener(const int32_t clientUID,
    const std::shared_ptr<AudioRendererStateChangeCallback> &callback)
{
    AUDIO_INFO_LOG("AudioStreamManager:: RegisterAudioRendererEventListener client id: %{public}d", clientUID);
    if (callback == nullptr) {
        AUDIO_ERR_LOG("AudioStreamManager::callback is null");
        return ERR_INVALID_PARAM;
    }
    return AudioPolicyManager::GetInstance().RegisterAudioRendererEventListener(clientUID, callback);
}

int32_t AudioStreamManager::UnregisterAudioRendererEventListener(const int32_t clientUID)
{
    AUDIO_INFO_LOG("AudioStreamManager:: UnregisterAudioRendererEventListener client id: %{public}d", clientUID);
    return AudioPolicyManager::GetInstance().UnregisterAudioRendererEventListener(clientUID);
}

int32_t AudioStreamManager::RegisterAudioCapturerEventListener(const int32_t clientUID,
    const std::shared_ptr<AudioCapturerStateChangeCallback> &callback)
{
    AUDIO_INFO_LOG("AudioStreamManager:: RegisterAudioCapturerEventListener client id: %{public}d", clientUID);
    if (callback == nullptr) {
        AUDIO_ERR_LOG("AudioStreamManager::callback is null");
        return ERR_INVALID_PARAM;
    }
    return AudioPolicyManager::GetInstance().RegisterAudioCapturerEventListener(clientUID, callback);
}

int32_t AudioStreamManager::UnregisterAudioCapturerEventListener(const int32_t clientUID)
{
    AUDIO_INFO_LOG("AudioStreamManager:: UnregisterAudioCapturerEventListener client id: %{public}d", clientUID);
    return AudioPolicyManager::GetInstance().UnregisterAudioCapturerEventListener(clientUID);
}

int32_t AudioStreamManager::GetCurrentRendererChangeInfos(
    vector<unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    AUDIO_INFO_LOG("AudioStreamManager:: GetCurrentRendererChangeInfos");
    return AudioPolicyManager::GetInstance().GetCurrentRendererChangeInfos(audioRendererChangeInfos);
}

int32_t AudioStreamManager::GetCurrentCapturerChangeInfos(
    vector<unique_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos)
{
    AUDIO_INFO_LOG("AudioStreamManager:: GetCurrentCapturerChangeInfos");
    return AudioPolicyManager::GetInstance().GetCurrentCapturerChangeInfos(audioCapturerChangeInfos);
}

bool AudioStreamManager::IsAudioRendererLowLatencySupported(const AudioStreamInfo &audioStreamInfo)
{
    AUDIO_INFO_LOG("AudioStreamManager::IsAudioRendererLowLatencySupported");
    return AudioPolicyManager::GetInstance().IsAudioRendererLowLatencySupported(audioStreamInfo);
}
} // namespace AudioStandard
} // namespace OHOS
