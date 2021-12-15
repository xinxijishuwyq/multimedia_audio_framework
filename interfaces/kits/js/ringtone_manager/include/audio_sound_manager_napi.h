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

#ifndef AUDIO_SOUND_MANAGER_NAPI_H_
#define AUDIO_SOUND_MANAGER_NAPI_H_

#include "iaudio_sound_manager.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "ability.h"

namespace OHOS {
namespace AudioStandard {
static const std::string AUDIO_SND_MNGR_NAPI_CLASS_NAME = "AudioSoundManager";

const std::vector<std::string> ringtoneTypesEnum {
    "RINGTONE_TYPE_DEFAULT", "RINGTONE_TYPE_MULTISIM"
};

class AudioSoundManagerNapi {
public:
    enum RingtoneType {
        DEFAULT = 0,
        MULTISIM = 1
    };

    static napi_value Init(napi_env env, napi_value exports);

    AudioSoundManagerNapi();
    ~AudioSoundManagerNapi();

private:
    static void AudioSndMngrNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value AudioSndMngrNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value GetAudioSndMngrInstance(napi_env env, napi_callback_info info);
    static napi_value JSSetSystemRingtoneUri(napi_env env, napi_callback_info info);
    static napi_value JSGetSystemRingtoneUri(napi_env env, napi_callback_info info);
    static napi_value CreateRingtoneTypeEnum(napi_env env);

    napi_env env_;
    napi_ref wrapper_;
    std::unique_ptr<AudioStandard::IAudioSoundManager> audioSndMngrClient_ = nullptr;

    static napi_ref sConstructor_;
    static napi_ref sRingtoneTypeEnumRef_;
};

struct AudioSndMngrAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    bool status;
    AudioSoundManagerNapi *objectInfo;
    std::string uri;
    int32_t ringtoneType;
};
} // namespace AudioStandard
} // namespace OHOS
#endif /* AUDIO_SOUND_MANAGER_NAPI_H_ */
