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

#include "audio_effect_chain_adapter.h"
#include "audio_effect_rotation.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
#ifdef WINDOW_MANAGER_ENABLE
void AudioEffectRotation::AudioRotationListener::OnCreate(Rosen::DisplayId displayId)
{
    std::shared_ptr<AudioEffectRotation> audioEffectRotation = GetInstance();
    if (audioEffectRotation != nullptr) {
        audioEffectRotation->OnCreate(displayId);
    }
}
void AudioEffectRotation::AudioRotationListener::OnDestroy(Rosen::DisplayId displayId)
{
    std::shared_ptr<AudioEffectRotation> audioEffectRotation = GetInstance();
    if (audioEffectRotation != nullptr) {
        audioEffectRotation->OnDestroy(displayId);
    }
}
void AudioEffectRotation::AudioRotationListener::OnChange(Rosen::DisplayId displayId)
{
    std::shared_ptr<AudioEffectRotation> audioEffectRotation = GetInstance();
    if (audioEffectRotation != nullptr) {
        audioEffectRotation->OnChange(displayId);
    }
}

AudioEffectRotation::AudioEffectRotation()
{
    AUDIO_DEBUG_LOG("created!");
    rotationState_ = 0;
}

AudioEffectRotation::~AudioEffectRotation()
{
    AUDIO_DEBUG_LOG("destroyed!");
}

std::shared_ptr<AudioEffectRotation> AudioEffectRotation::GetInstance()
{
    static std::shared_ptr<AudioEffectRotation> effectRotation = std::make_shared<AudioEffectRotation>();
    return effectRotation;
}

void AudioEffectRotation::Init()
{
    AUDIO_DEBUG_LOG("Call RegisterDisplayListener.");
}

void AudioEffectRotation::SetRotation(uint32_t rotationState)
{
    rotationState_ = rotationState;
}

uint32_t AudioEffectRotation::GetRotation()
{
    return rotationState_;
}

void AudioEffectRotation::OnCreate(Rosen::DisplayId displayId)
{
    AUDIO_DEBUG_LOG("Onchange displayId: %{public}d.", static_cast<int32_t>(displayId));
}

void AudioEffectRotation::OnDestroy(Rosen::DisplayId displayId)
{
    AUDIO_DEBUG_LOG("Onchange displayId: %{public}d.", static_cast<int32_t>(displayId));
}

void AudioEffectRotation::OnChange(Rosen::DisplayId displayId)
{
    // get rotation
    int32_t newRotationState = 0;
    EffectChainManagerRotationUpdate(static_cast<uint32_t>(newRotationState));
}
#endif
}  // namespace AudioStandard
}  // namespace OHOS