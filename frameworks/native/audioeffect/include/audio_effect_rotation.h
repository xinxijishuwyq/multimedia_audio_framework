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

#ifndef AUDIO_EFFECT_ROTATION_H
#define AUDIO_EFFECT_ROTATION_H

#include <cstdint>
#include <mutex>
#include "display_manager.h"
#include "dm_common.h"

namespace OHOS {
namespace AudioStandard {
class AudioEffectRotation {
public:
    AudioEffectRotation();
    ~AudioEffectRotation();
    static AudioEffectRotation *GetInstance();
    void Init();
    void SetRotation(uint32_t rotationState);
    uint32_t GetRotation();
private:
    class AudioRotationListener : public OHOS::Rosen::DisplayManager::IDisplayListener {
    public:
        void OnCreate(Rosen::DisplayId displayId) override
        {
            AudioEffectRotation *audioEffectRotation = GetInstance();
            if (audioEffectRotation != nullptr) {
                audioEffectRotation->OnCreate(displayId);
            }
        }
        void OnDestroy(Rosen::DisplayId displayId) override
        {
            AudioEffectRotation *audioEffectRotation = GetInstance();
            if (audioEffectRotation != nullptr) {
                audioEffectRotation->OnDestroy(displayId);
            }
        }
        void OnChange(Rosen::DisplayId displayId) override
        {
            AudioEffectRotation *audioEffectRotation = GetInstance();
            if (audioEffectRotation != nullptr) {
                audioEffectRotation->OnChange(displayId);
            }
        }
    };

    sptr<AudioRotationListener> audioRotationListener_;
    uint32_t rotationState_;
    void OnCreate(Rosen::DisplayId displayId);
    void OnDestroy(Rosen::DisplayId displayId);
    void OnChange(Rosen::DisplayId displayId);
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif // AUDIO_EFFECT_ROTATION_H