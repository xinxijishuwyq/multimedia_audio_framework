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

#ifndef AUDIO_ROUTING_MANAGER_CALLBACK_NAPI_H
#define AUDIO_ROUTING_MANAGER_CALLBACK_NAPI_H

#include "audio_common_napi.h"
#include "audio_routing_manager_napi.h"
#include "audio_routing_manager.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include <algorithm>

namespace {
    const std::string PREFERRED_OUTPUT_DEVICE_CALLBACK_NAME = "preferredOutputDeviceChangeForRendererInfo";
    const std::string PREFER_OUTPUT_DEVICE_CALLBACK_NAME = "preferOutputDeviceChangeForRendererInfo";
    const std::string PREFERRED_INPUT_DEVICE_CALLBACK_NAME  = "preferredInputDeviceChangeForCapturerInfo";
}

namespace OHOS {
namespace AudioStandard {
class AudioPreferredOutputDeviceChangeCallbackNapi : public AudioPreferredOutputDeviceChangeCallback {
public:
    explicit AudioPreferredOutputDeviceChangeCallbackNapi(napi_env env);
    virtual ~AudioPreferredOutputDeviceChangeCallbackNapi();
    void SaveCallbackReference(AudioStreamType streamType, napi_value callback);
    void OnPreferredOutputDeviceUpdated(const std::vector<sptr<AudioDeviceDescriptor>> &desc) override;
    void RemoveCallbackReference(napi_env env, napi_value args);
    void RemoveAllCallbacks();

private:
    struct AudioActiveOutputDeviceChangeJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
        std::string callbackName = "unknown";
        std::vector<sptr<AudioDeviceDescriptor>> desc;
    };

    void OnJsCallbackActiveOutputDeviceChange(std::unique_ptr<AudioActiveOutputDeviceChangeJsCallback> &jsCb);

    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::shared_ptr<AutoRef> preferredOutputDeviceCallback_ = nullptr;
    std::list<std::pair<std::shared_ptr<AutoRef>, AudioStreamType>> preferredOutputDeviceCbList_;
};

class AudioPreferredInputDeviceChangeCallbackNapi : public AudioPreferredInputDeviceChangeCallback {
public:
    explicit AudioPreferredInputDeviceChangeCallbackNapi(napi_env env);
    virtual ~AudioPreferredInputDeviceChangeCallbackNapi();
    void SaveCallbackReference(SourceType sourceType, napi_value callback);
    void OnPreferredInputDeviceUpdated(const std::vector<sptr<AudioDeviceDescriptor>> &desc) override;
    void RemoveCallbackReference(napi_env env, napi_value args);
    void RemoveAllCallbacks();

private:
    struct AudioActiveInputDeviceChangeJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
        std::string callbackName = "unknown";
        std::vector<sptr<AudioDeviceDescriptor>> desc;
    };

    void OnJsCallbackActiveInputDeviceChange(std::unique_ptr<AudioActiveInputDeviceChangeJsCallback> &jsCb);

    std::mutex preferredInputListMutex_;
    napi_env env_ = nullptr;
    std::shared_ptr<AutoRef> preferredInputDeviceCallback_ = nullptr;
    std::list<std::pair<std::shared_ptr<AutoRef>, SourceType>> preferredInputDeviceCbList_;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif // AUDIO_ROUTING_MANAGER_CALLBACK_NAPI_H
