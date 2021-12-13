/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef AUDIO_SOUND_MANAGER_H
#define AUDIO_SOUND_MANAGER_H

#include "iaudio_sound_manager.h"
#include "audio_errors.h"
#include "media_log.h"

namespace OHOS {
namespace AudioStandard {
class AudioSoundManager : public IAudioSoundManager {
public:
    AudioSoundManager();
    ~AudioSoundManager() = default;

    std::string GetSystemRingtoneUri(const std::shared_ptr<AppExecFwk::Context> &context, RingtoneType type) override;
    int SetSystemRingtoneUri(const std::shared_ptr<AppExecFwk::Context> &context, const std::string &uri,
        RingtoneType type) override;

private:
    std::string uri_;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif  // AUDIO_SOUND_MANAGER_H
