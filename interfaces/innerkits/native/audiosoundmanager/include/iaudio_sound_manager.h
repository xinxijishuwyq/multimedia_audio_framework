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

#ifndef IAUDIO_SOUND_MANAGER_H
#define IAUDIO_SOUND_MANAGER_H

#include<memory>
#include "context.h"

namespace OHOS {
namespace AudioStandard {
enum RingtoneType {
    RINGTONE_TYPE_DEFAULT = 0,
    RINGTONE_TYPE_MULTISIM
};

/**
 * @brief Provides functions for applications to implement ringtone.
 */
class IAudioSoundManager {
public:
    /**
     * @brief creates audio sound manager instance
     *
     * @return Returns instance of class that extends AudioRingtone
    */
    static std::unique_ptr<IAudioSoundManager> CreateAudioSoundManager();

    /**
     * @brief api for setting the system ringtone uri
     *
     * @param context Indicates the context object in OHOS
     * @param uri indicates which uri to be set for ringtone
     * @param type indicates whether the uri is for default sim or second sim
     * @return Returns {@link SUCCESS} if uri is set successfully; returns an error code
     * defined in {@link audio_errors.h} otherwise.
    */
    virtual int SetSystemRingtoneUri(const std::shared_ptr<AppExecFwk::Context> &context, const std::string &uri,
        RingtoneType type) = 0;

    /**
     * @brief api for getting the system ringtone uri
     *
     * @param context Indicates the context object in OHOS
     * @param type indicates whether the uri queried is for default sim or second sim
     * @return Returns uri if available; returns empty string otherwise
    */
    virtual std::string GetSystemRingtoneUri(const std::shared_ptr<AppExecFwk::Context> &context,
        RingtoneType type) = 0;

    virtual ~IAudioSoundManager() = default;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif  // IAUDIO_SOUND_MANAGER_H
