/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef LOG_TAG
#define LOG_TAG "AudioEffectVolume"
#endif

#include "audio_effect_volume.h"
#include "audio_effect_log.h"

namespace OHOS {
namespace AudioStandard {
AudioEffectVolume::AudioEffectVolume()
{
    AUDIO_DEBUG_LOG("created!");
    SceneTypeToVolumeMap_.clear();
    dspVolume_ = 0;
}

AudioEffectVolume::~AudioEffectVolume()
{
    AUDIO_DEBUG_LOG("destructor!");
}

std::shared_ptr<AudioEffectVolume> AudioEffectVolume::GetInstance()
{
    static std::shared_ptr<AudioEffectVolume> effectVolume = std::make_shared<AudioEffectVolume>();
    return effectVolume;
}

void AudioEffectVolume::SetApVolume(std::string sceneType, uint32_t volume)
{
    if (!SceneTypeToVolumeMap_.count(sceneType)) {
        SceneTypeToVolumeMap_.insert(std::make_pair(sceneType, volume));
    } else {
        SceneTypeToVolumeMap_[sceneType] = volume;
    }
}

uint32_t AudioEffectVolume::GetApVolume(std::string sceneType)
{
    if (!SceneTypeToVolumeMap_.count(sceneType)) {
        return 0;
    } else {
        return SceneTypeToVolumeMap_[sceneType];
    }
}

void AudioEffectVolume::SetDspVolume(uint32_t volume)
{
    AUDIO_DEBUG_LOG("setDspVolume: %{public}u", volume);
    dspVolume_ = volume;
}

uint32_t AudioEffectVolume::GetDspVolume()
{
    return dspVolume_;
}
}  // namespace AudioStandard
}  // namespace OHOS