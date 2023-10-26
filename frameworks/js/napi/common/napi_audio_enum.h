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
#ifndef NAPI_AUDIO_ENUM_H_
#define NAPI_AUDIO_ENUM_H_

#include "napi/native_api.h"
#include "napi/native_common.h"
#include "napi/native_node_api.h"
#include "napi_param_utils.h"
#include "audio_renderer.h"
#include "audio_errors.h"
#include "audio_stream_manager.h"
#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {
const int32_t REFERENCE_CREATION_COUNT = 1;

class NapiAudioEnum {
public:
    NapiAudioEnum();
    ~NapiAudioEnum();

    static napi_value Init(napi_env env, napi_value exports);

private:
    static void Destructor(napi_env env, void *nativeObject, void *finalizeHint);
    static napi_value Construct(napi_env env, napi_callback_info info);
    static NapiAudioEnum* SetValue(napi_env env, napi_callback_info info, napi_value *args, napi_value &result);
    static napi_value GetAudioSampleFormat(napi_env env, napi_callback_info info);
    static napi_value SetAudioSampleFormat(napi_env env, napi_callback_info info);
    static napi_value GetAudioChannel(napi_env env, napi_callback_info info);
    static napi_value SetAudioChannel(napi_env env, napi_callback_info info);
    static napi_value GetAudioSamplingRate(napi_env env, napi_callback_info info);
    static napi_value SetAudioSamplingRate(napi_env env, napi_callback_info info);
    static napi_value GetAudioEncodingType(napi_env env, napi_callback_info info);
    static napi_value SetAudioEncodingType(napi_env env, napi_callback_info info);
    static napi_value GetContentType(napi_env env, napi_callback_info info);
    static napi_value SetContentType(napi_env env, napi_callback_info info);
    static napi_value GetStreamUsage(napi_env env, napi_callback_info info);
    static napi_value SetStreamUsage(napi_env env, napi_callback_info info);
    static napi_value GetDeviceRole(napi_env env, napi_callback_info info);
    static napi_value SetDeviceRole(napi_env env, napi_callback_info info);
    static napi_value GetDeviceType(napi_env env, napi_callback_info info);
    static napi_value SetDeviceType(napi_env env, napi_callback_info info);

    static napi_status CreateEnumObject(const napi_env &env, const std::map<std::string, int32_t> &map,
        napi_value &result, napi_ref &ref);
    static napi_value CreateAudioChannelObject(napi_env env);
    static napi_value CreateSamplingRateObject(napi_env env);
    static napi_value CreateEncodingTypeObject(napi_env env);
    static napi_value CreateContentTypeObject(napi_env env);
    static napi_value CreateStreamUsageObject(napi_env env);
    static napi_value CreateDeviceRoleObject(napi_env env);
    static napi_value CreateDeviceTypeObject(napi_env env);
    static napi_value CreateSourceTypeObject(napi_env env);
    static napi_value CreateVolumeAdjustTypeObject(napi_env env);
    static napi_value CreateChannelBlendModeObject(napi_env env);

    static napi_ref sConstructor_;
    static napi_ref audioChannel_;
    static napi_ref samplingRate_;
    static napi_ref encodingType_;
    static napi_ref contentType_;
    static napi_ref streamUsage_;
    static napi_ref deviceRole_;
    static napi_ref deviceType_;
    static napi_ref sourceType_;

    static const std::map<std::string, int32_t> audioChannelMap;
    static const std::map<std::string, int32_t> samplingRateMap;
    static const std::map<std::string, int32_t> encodingTypeMap;
    static const std::map<std::string, int32_t> contentTypeMap;
    static const std::map<std::string, int32_t> streamUsageMap;
    static const std::map<std::string, int32_t> deviceRoleMap;
    static const std::map<std::string, int32_t> deviceTypeMap;
    static const std::map<std::string, int32_t> sourceTypeMap;
    static const std::map<std::string, int32_t> volumeAdjustTypeMap;
    static const std::map<std::string, int32_t> channelBlendModeMap;

    static std::unique_ptr<AudioParameters> sAudioParameters_;

    std::unique_ptr<AudioParameters> audioParameters_;
    napi_env env_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // OHOS_NAPI_AUDIO_ERROR_H_