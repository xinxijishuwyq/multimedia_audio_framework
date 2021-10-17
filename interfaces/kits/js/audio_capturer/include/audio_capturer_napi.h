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

#ifndef AUDIO_CAPTURER_NAPI_H_
#define AUDIO_CAPTURER_NAPI_H_

#include <iostream>

#include "audio_capturer.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace AudioStandard {
static const std::string AUDIO_CAPTURER_NAPI_CLASS_NAME = "AudioCapturer";

class AudioCapturerNapi {
public:
    AudioCapturerNapi();
    ~AudioCapturerNapi();

    static napi_value Init(napi_env env, napi_value exports);
private:
    struct AudioCapturerAsyncContext {
        napi_env env;
        napi_async_work work;
        napi_deferred deferred;
        napi_ref callbackRef = nullptr;
        uint64_t time;
        int32_t status;
        int32_t intValue;
        uint32_t userSize;
        uint8_t *buffer = nullptr;
        size_t bytesRead;
        bool isBlocking;
        bool isTrue;
        AudioSampleFormat audioSampleFormat;
        AudioSamplingRate samplingRate;
        AudioChannel audioChannel;
        AudioEncodingType audioEncoding;
        ContentType contentType;
        StreamUsage usage;
        DeviceRole deviceRole;
        DeviceType deviceType;
        AudioCapturerNapi *objectInfo;
    };

    static void Destructor(napi_env env, void *nativeObject, void *finalize_hint);
    static napi_value Construct(napi_env env, napi_callback_info info);
    static napi_value CreateAudioCapturer(napi_env env, napi_callback_info info);
    static napi_value SetParams(napi_env env, napi_callback_info info);
    static napi_value GetParams(napi_env env, napi_callback_info info);
    static napi_value Start(napi_env env, napi_callback_info info);
    static napi_value Read(napi_env env, napi_callback_info info);
    static napi_value GetAudioTime(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value GetBufferSize(napi_env env, napi_callback_info info);

    static void CommonCallbackRoutine(napi_env env, AudioCapturerAsyncContext* &asyncContext,
                                      const napi_value &valueParam);
    static void SetFunctionAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void AudioParamsAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void ReadAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void IsTrueAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void GetIntValueAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void GetInt64ValueAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static napi_status CreateReadAsyncWork(const AudioCapturerAsyncContext &asyncContext);

    static std::unique_ptr<AudioParameters> sAudioParameters_;

    int32_t SetAudioParameters(napi_env env, napi_value arg);

    std::unique_ptr<AudioCapturer> audioCapturer_;
    ContentType contentType_;
    StreamUsage streamUsage_;
    DeviceRole deviceRole_;
    DeviceType deviceType_;
    napi_env env_;
    napi_ref wrapper_;
};
}
}
#endif /* AUDIO_CAPTURER_NAPI_H_ */
