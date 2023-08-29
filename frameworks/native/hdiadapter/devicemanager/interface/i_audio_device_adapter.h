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

#ifndef I_AUDIO_DEVICE_ADAPTER_H
#define I_AUDIO_DEVICE_ADAPTER_H

#include "audio_manager.h"

namespace OHOS {
namespace AudioStandard {
class IAudioDeviceAdapterCallback {
public:
    IAudioDeviceAdapterCallback() = default;
    virtual ~IAudioDeviceAdapterCallback() = default;

    virtual void OnAudioParamChange(const std::string &adapterName, const AudioParamKey key,
        const std::string &condition, const std::string &value) = 0;
};

class IAudioDeviceAdapter {
public:
    IAudioDeviceAdapter() = default;
    virtual ~IAudioDeviceAdapter() = default;

    virtual int32_t Init() = 0;
    virtual int32_t RegExtraParamObserver() = 0;
    virtual int32_t CreateRender(const struct AudioDeviceDescriptor *devDesc,
        const struct AudioSampleAttributes *attr, struct AudioRender **audioRender,
        IAudioDeviceAdapterCallback *renderCb) = 0;
    virtual void DestroyRender(struct AudioRender *audioRender) = 0;
    virtual int32_t CreateCapture(const struct AudioDeviceDescriptor *devDesc,
        const struct AudioSampleAttributes *attr, struct AudioCapture **audioCapture,
        IAudioDeviceAdapterCallback *captureCb) = 0;
    virtual void DestroyCapture(struct AudioCapture *audioCapture) = 0;
    virtual void SetAudioParameter(const AudioParamKey key, const std::string &condition,
        const std::string &value) = 0;
    virtual std::string GetAudioParameter(const AudioParamKey key, const std::string &condition) = 0;
    virtual int32_t UpdateAudioRoute(const struct AudioRoute *route, int32_t *routeHandle_) = 0;
    virtual int32_t Release() = 0;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif // I_AUDIO_DEVICE_ADAPTER_H