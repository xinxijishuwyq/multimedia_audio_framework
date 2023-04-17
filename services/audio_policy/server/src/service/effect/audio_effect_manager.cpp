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

#include "audio_effect_manager.h"

namespace OHOS {
namespace AudioStandard {
AudioEffectManager::AudioEffectManager()
{
    AUDIO_INFO_LOG("AudioEffectManager ctor");
}

AudioEffectManager::~AudioEffectManager()
{
}

void AudioEffectManager::EffectManagerInit()
{
    // load XML
    std::unique_ptr<AudioEffectConfigParser> effectConfigParser = std::make_unique<AudioEffectConfigParser>();
    int32_t ret = effectConfigParser->LoadEffectConfig(oriEffectConfig_);
    if (ret != 0) {
        AUDIO_ERR_LOG("AudioEffectManager->effectConfigParser failed: %{public}d", ret);
        return;
    }
}

void AudioEffectManager::GetAvailableEffects(std::vector<Effect> &availableEffects)
{
    availableEffects = availableEffects_;
}

void AudioEffectManager::GetOriginalEffectConfig(OriginalEffectConfig &oriEffectConfig)
{
    oriEffectConfig = oriEffectConfig_;
}

void AudioEffectManager::UpdateAvailableEffects(std::vector<Effect> &newAvailableEffects)
{
    availableEffects_ = newAvailableEffects;
}
} // namespce AudioStandard
} // namespace OHOS
  