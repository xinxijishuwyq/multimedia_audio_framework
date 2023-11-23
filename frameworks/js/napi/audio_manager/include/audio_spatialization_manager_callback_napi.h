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

#ifndef AUDIO_SPATIALIZATION_MANAGER_CALLBACK_NAPI_H
#define AUDIO_SPATIALIZATION_MANAGER_CALLBACK_NAPI_H

#include "audio_common_napi.h"
#include "audio_spatialization_manager.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include <algorithm>

namespace OHOS {
namespace AudioStandard {
class AudioSpatializationEnabledChangeCallbackNapi : public AudioSpatializationEnabledChangeCallback {
public:
    explicit AudioSpatializationEnabledChangeCallbackNapi(napi_env env);
    virtual ~AudioSpatializationEnabledChangeCallbackNapi();
    void SaveSpatializationEnabledChangeCallbackReference(napi_value args);
    void RemoveSpatializationEnabledChangeCallbackReference(napi_env env, napi_value args);
    void RemoveAllSpatializationEnabledChangeCallbackReference();
    int32_t GetSpatializationEnabledChangeCbListSize();
    void OnSpatializationEnabledChange(const bool &enabled) override;

private:
    struct AudioSpatializationEnabledJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
        bool enabled;
    };

    void OnJsCallbackSpatializationEnabled(std::unique_ptr<AudioSpatializationEnabledJsCallback> &jsCb);

    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::list<std::shared_ptr<AutoRef>> spatializationEnabledChangeCbList_;
};

class AudioHeadTrackingEnabledChangeCallbackNapi : public AudioHeadTrackingEnabledChangeCallback {
public:
    explicit AudioHeadTrackingEnabledChangeCallbackNapi(napi_env env);
    virtual ~AudioHeadTrackingEnabledChangeCallbackNapi();
    void SaveHeadTrackingEnabledChangeCallbackReference(napi_value args);
    void RemoveHeadTrackingEnabledChangeCallbackReference(napi_env env, napi_value args);
    void RemoveAllHeadTrackingEnabledChangeCallbackReference();
    int32_t GetHeadTrackingEnabledChangeCbListSize();
    void OnHeadTrackingEnabledChange(const bool &enabled) override;

private:
    struct AudioHeadTrackingEnabledJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
        bool enabled;
    };

    void OnJsCallbackHeadTrackingEnabled(std::unique_ptr<AudioHeadTrackingEnabledJsCallback> &jsCb);

    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::list<std::shared_ptr<AutoRef>> headTrackingEnabledChangeCbList_;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif // AUDIO_SPATIALIZATION_MANAGER_CALLBACK_NAPI_H
