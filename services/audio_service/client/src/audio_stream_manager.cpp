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
#include "audio_stream.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;
AudioStreamManager *AudioStreamManager::GetInstance()
{
    static AudioStreamManager audioStreamManager;
    return &audioStreamManager;
}

int32_t AudioStreamManager::RegisterAudioRendererEventListener(const int32_t clientPid,
    const std::shared_ptr<AudioRendererStateChangeCallback> &callback)
{
    AUDIO_INFO_LOG("RegisterAudioRendererEventListener client id: %{public}d", clientPid);
    if (callback == nullptr) {
        AUDIO_ERR_LOG("callback is null");
        return ERR_INVALID_PARAM;
    }
    return AudioPolicyManager::GetInstance().RegisterAudioRendererEventListener(clientPid, callback);
}

int32_t AudioStreamManager::UnregisterAudioRendererEventListener(const int32_t clientPid)
{
    AUDIO_INFO_LOG("UnregisterAudioRendererEventListener client id: %{public}d", clientPid);
    return AudioPolicyManager::GetInstance().UnregisterAudioRendererEventListener(clientPid);
}

int32_t AudioStreamManager::RegisterAudioCapturerEventListener(const int32_t clientPid,
    const std::shared_ptr<AudioCapturerStateChangeCallback> &callback)
{
    AUDIO_INFO_LOG("RegisterAudioCapturerEventListener client id: %{public}d", clientPid);
    if (callback == nullptr) {
        AUDIO_ERR_LOG("callback is null");
        return ERR_INVALID_PARAM;
    }
    return AudioPolicyManager::GetInstance().RegisterAudioCapturerEventListener(clientPid, callback);
}

int32_t AudioStreamManager::UnregisterAudioCapturerEventListener(const int32_t clientPid)
{
    AUDIO_INFO_LOG("UnregisterAudioCapturerEventListener client id: %{public}d", clientPid);
    return AudioPolicyManager::GetInstance().UnregisterAudioCapturerEventListener(clientPid);
}

int32_t AudioStreamManager::GetCurrentRendererChangeInfos(
    vector<unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    AUDIO_DEBUG_LOG("GetCurrentRendererChangeInfos");
    return AudioPolicyManager::GetInstance().GetCurrentRendererChangeInfos(audioRendererChangeInfos);
}

int32_t AudioStreamManager::GetCurrentCapturerChangeInfos(
    vector<unique_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos)
{
    AUDIO_DEBUG_LOG("GetCurrentCapturerChangeInfos");
    return AudioPolicyManager::GetInstance().GetCurrentCapturerChangeInfos(audioCapturerChangeInfos);
}

bool AudioStreamManager::IsAudioRendererLowLatencySupported(const AudioStreamInfo &audioStreamInfo)
{
    AUDIO_DEBUG_LOG("IsAudioRendererLowLatencySupported");
    return AudioPolicyManager::GetInstance().IsAudioRendererLowLatencySupported(audioStreamInfo);
}

static void UpdateEffectInfoArray(SupportedEffectConfig &supportedEffectConfig,
    AudioSceneEffectInfo &audioSceneEffectInfo, int i)
{
    uint32_t j;
    AudioEffectMode audioEffectMode;
    for (j = 0; j < supportedEffectConfig.postProcessNew.stream[i].streamEffectMode.size(); j++) {
        audioEffectMode = effectModeMap.at(supportedEffectConfig.postProcessNew.stream[i].streamEffectMode[j].mode);
        audioSceneEffectInfo.mode.push_back(audioEffectMode);
    }
    auto index = std::find(audioSceneEffectInfo.mode.begin(), audioSceneEffectInfo.mode.end(), 0);
    if (index == audioSceneEffectInfo.mode.end()) {
        audioEffectMode = effectModeMap.at("EFFECT_NONE");
        audioSceneEffectInfo.mode.push_back(audioEffectMode);
    }
    index = std::find(audioSceneEffectInfo.mode.begin(), audioSceneEffectInfo.mode.end(), 1);
    if (index == audioSceneEffectInfo.mode.end()) {
        audioEffectMode = effectModeMap.at("EFFECT_DEFAULT");
        audioSceneEffectInfo.mode.push_back(audioEffectMode);
    }
    std::sort(audioSceneEffectInfo.mode.begin(), audioSceneEffectInfo.mode.end());
}

int32_t AudioStreamManager::GetEffectInfoArray(AudioSceneEffectInfo &audioSceneEffectInfo,
    ContentType contentType, StreamUsage streamUsage)
{
    int i;
    AudioStreamType streamType =  AudioStream::GetStreamType(contentType, streamUsage);
    std::string effectScene = AudioServiceClient::GetEffectSceneName(streamType);
    SupportedEffectConfig supportedEffectConfig;
    int ret = AudioPolicyManager::GetInstance().QueryEffectSceneMode(supportedEffectConfig);
    int streamNum = supportedEffectConfig.postProcessNew.stream.size();
    if (streamNum > 0) {
        for (i = 0; i < streamNum; i++) {
            if (effectScene == supportedEffectConfig.postProcessNew.stream[i].scene) {
                UpdateEffectInfoArray(supportedEffectConfig, audioSceneEffectInfo, i);
                break;
            }
        }
    }
    return ret;
}
} // namespace AudioStandard
} // namespace OHOS
