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

#ifndef AUDIO_RENDERER_NAPI_H_
#define AUDIO_RENDERER_NAPI_H_

#include <iostream>
#include <queue>

#include "audio_renderer.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace AudioStandard {
static const std::string AUDIO_RENDERER_NAPI_CLASS_NAME = "AudioRenderer";

class AudioRendererNapi {
public:
    AudioRendererNapi();
    ~AudioRendererNapi();

    static napi_value Init(napi_env env, napi_value exports);
private:
    struct AudioRendererAsyncContext {
        napi_env env;
        napi_async_work work;
        napi_deferred deferred;
        napi_ref callbackRef = nullptr;
        int32_t status;
        int32_t intValue;
        bool isTrue;
        uint64_t time;
        size_t bufferLen;
        void *data;
        AudioSampleFormat sampleFormat;
        AudioSamplingRate samplingRate;
        AudioChannel channelCount;
        AudioEncodingType encodingType;
        ContentType contentType;
        StreamUsage usage;
        DeviceRole deviceRole;
        DeviceType deviceType;
        AudioRendererNapi *objectInfo;
    };

    static void Destructor(napi_env env, void *nativeObject, void *finalize_hint);
    static napi_value Construct(napi_env env, napi_callback_info info);
    static napi_value CreateAudioRenderer(napi_env env, napi_callback_info info);
    static napi_value SetParams(napi_env env, napi_callback_info info);
    static napi_value GetParams(napi_env env, napi_callback_info info);
    static napi_value Start(napi_env env, napi_callback_info info);
    static napi_value Write(napi_env env, napi_callback_info info);
    static napi_value GetAudioTime(napi_env env, napi_callback_info info);
    static napi_value Drain(napi_env env, napi_callback_info info);
    static napi_value Pause(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value GetBufferSize(napi_env env, napi_callback_info info);

    static void CommonCallbackRoutine(napi_env env, AudioRendererAsyncContext* &asyncContext,
                                      const napi_value &valueParam);
    static void SetFunctionAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void AudioParamsAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void IsTrueAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void GetIntValueAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void GetInt64ValueAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void WriteAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void PauseAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void StartAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void StopAsyncCallbackComplete(napi_env env, napi_status status, void *data);

    static std::unique_ptr<AudioParameters> sAudioParameters_;

    int32_t SetAudioParameters(napi_env env, napi_value arg);

    std::unique_ptr<AudioRenderer> audioRenderer_;
    ContentType contentType_;
    StreamUsage streamUsage_;
    DeviceRole deviceRole_;
    DeviceType deviceType_;
    napi_env env_;
    napi_ref wrapper_;
    std::queue<napi_async_work> writeRequestQ_;
    std::atomic<bool> scheduleFromApiCall_;
    std::atomic<bool> doNotScheduleWrite_;
    std::atomic<bool> isDrainWriteQInProgress_;
};
}
}
#endif /* AUDIO_RENDERER_NAPI_H_ */
