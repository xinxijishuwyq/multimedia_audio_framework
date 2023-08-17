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

#ifndef AUDIO_CAPTURER_INFO_CHANGE_CALLBACK_NAPI_H_
#define AUDIO_CAPTURER_INFO_CHANGE_CALLBACK_NAPI_H_

#include <uv.h>
#include <list>
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "audio_common_napi.h"
#include "audio_capturer.h"

namespace OHOS {
namespace AudioStandard {
class AudioCapturerInfoChangeCallbackNapi : public AudioCapturerInfoChangeCallback {
public:
    explicit AudioCapturerInfoChangeCallbackNapi(napi_env env);
    virtual ~AudioCapturerInfoChangeCallbackNapi();
    void SaveCallbackReference(napi_value callback);
    void OnStateChange(const AudioCapturerChangeInfo &deviceInfo) override;
    bool ContainSameJsCallback(napi_value args);
private:
    struct AudioCapturerChangeInfoJsCallback {
        napi_ref callback_;
        napi_env env_;
        AudioCapturerChangeInfo capturerChangeInfo_;
    };

    void OnJsCallbackCapturerChangeInfo(napi_ref method, const AudioCapturerChangeInfo &capturerChangeInfo);
    static void WorkCallbackCompleted(uv_work_t* work, int aStatus);

    std::mutex mutex_;
    napi_env env_ = nullptr;
    napi_ref callback_;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif // AUDIO_CAPTURER_INFO_CHANGE_CALLBACK_NAPI_H_
