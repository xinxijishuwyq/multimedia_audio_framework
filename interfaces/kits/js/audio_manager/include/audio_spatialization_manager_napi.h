/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef AUDIO_SPATIALIZATION_MANAGER_NAPI_H
#define AUDIO_SPATIALIZATION_MANAGER_NAPI_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "audio_spatialization_manager.h"

namespace OHOS {
namespace AudioStandard {
static const std::string AUDIO_SPATIALIZATION_MANAGER_NAPI_CLASS_NAME = "AudioSpatializationManager";

struct AudioSpatializationManagerAsyncContext;
class AudioSpatializationManagerNapi {
public:
    AudioSpatializationManagerNapi();
    ~AudioSpatializationManagerNapi();

    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreateSpatializationManagerWrapper(napi_env env);
private:
    static void Destructor(napi_env env, void *nativeObject, void *finalize_hint);
    static napi_value Construct(napi_env env, napi_callback_info info);
    static napi_value IsSpatializationEnabled(napi_env env, napi_callback_info info);
    static napi_value SetSpatializationEnabled(napi_env env, napi_callback_info info);
    static napi_value IsHeadTrackingEnabled(napi_env env, napi_callback_info info);
    static napi_value SetHeadTrackingEnabled(napi_env env, napi_callback_info info);
    static void RegisterSpatializationEnabledChangeCallback(napi_env env, napi_value* args,
        const std::string& cbName, AudioSpatializationManagerNapi *spatializationManagerNapi);
    static void RegisterHeadTrackingEnabledChangeCallback(napi_env env, napi_value* args,
        const std::string& cbName, AudioSpatializationManagerNapi *spatializationManagerNapi);
    static void RegisterCallback(napi_env env, napi_value jsThis, napi_value* args, const std::string& cbName);
    static napi_value On(napi_env env, napi_callback_info info);
    static void UnregisterSpatializationEnabledChangeCallback(napi_env env, napi_value callback,
        AudioSpatializationManagerNapi *spatializationManagerNapi);
    static void UnregisterHeadTrackingEnabledChangeCallback(napi_env env, napi_value callback,
        AudioSpatializationManagerNapi *spatializationManagerNapi);
    static napi_value Off(napi_env env, napi_callback_info info);
    static bool ParseSpatialDeviceState(napi_env env, napi_value root, AudioSpatialDeviceState *spatialDeviceState);
    static napi_value IsSpatializationSupported(napi_env env, napi_callback_info info);
    static napi_value IsSpatializationSupportedForDevice(napi_env env, napi_callback_info info);
    static napi_value IsHeadTrackingSupported(napi_env env, napi_callback_info info);
    static napi_value IsHeadTrackingSupportedForDevice(napi_env env, napi_callback_info info);
    static napi_value UpdateSpatialDeviceState(napi_env env, napi_callback_info info);
    static napi_value CreateAudioSpatialDeviceTypeObject(napi_env env);
    static napi_status AddNamedProperty(napi_env env, napi_value object, const std::string name, int32_t enumValue);

    AudioSpatializationManager *audioSpatializationMngr_;
    static napi_ref audioSpatialDeivceType_;
    std::shared_ptr<AudioSpatializationEnabledChangeCallback> spatializationEnabledChangeCallbackNapi_ = nullptr;
    std::shared_ptr<AudioHeadTrackingEnabledChangeCallback> headTrackingEnabledChangeCallbackNapi_ = nullptr;

    napi_env env_;
};

static const std::map<std::string, AudioSpatialDeviceType> audioSpatialDeivceTypeMap = {
    {"SPATIAL_DEVICE_TYPE_NONE", EARPHONE_TYPE_NONE },
    {"SPATIAL_DEVICE_TYPE_IN_EAR_HEADPHONE", EARPHONE_TYPE_INEAR},
    {"SPATIAL_DEVICE_TYPE_HALF_IN_EAR_HEADPHONE", EARPHONE_TYPE_HALF_INEAR},
    {"SPATIAL_DEVICE_TYPE_OVER_EAR_HEADPHONE", EARPHONE_TYPE_HEADPHONE},
    {"SPATIAL_DEVICE_TYPE_GLASSES", EARPHONE_TYPE_GLASSES},
    {"SPATIAL_DEVICE_TYPE_OTHERS", EARPHONE_TYPE_OTHERS},
};
} // namespace AudioStandard
} // namespace OHOS
#endif /* AUDIO_SPATIALIZATION_MANAGER_NAPI_H */
