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
#ifndef NAPI_PARAM_UTILS_H
#define NAPI_PARAM_UTILS_H

#include <cstdint>
#include <map>
#include <list>
#include "ability.h"
#include "napi/native_api.h"
#include "napi/native_common.h"
#include "napi/native_node_api.h"
#include "napi_base_context.h"
#include "audio_log.h"
#include "audio_info.h"
#include "audio_system_manager.h"

namespace OHOS {
namespace AudioStandard {
const int ARGS_ONE = 1;
const int ARGS_TWO = 2;
const int ARGS_THREE = 3;

const int PARAM0 = 0;
const int PARAM1 = 1;
const int PARAM2 = 2;

/* check condition related to argc/argv, return and logging. */
#define NAPI_CHECK_ARGS_RETURN_VOID(context, condition, message, code)               \
    do {                                                               \
        if (!(condition)) {                                            \
            (context)->status = napi_invalid_arg;                         \
            (context)->errMessage = std::string(message);                      \
            (context)->errCode = code;                      \
            AUDIO_ERR_LOG("test (" #condition ") failed: " message);           \
            return;                                                    \
        }                                                              \
    } while (0)

#define NAPI_CHECK_STATUS_RETURN_VOID(context, message, code)                        \
    do {                                                               \
        if ((context)->status != napi_ok) {                               \
            (context)->errMessage = std::string(message);                      \
            (context)->errCode = code;                      \
            AUDIO_ERR_LOG("test (context->status == napi_ok) failed: " message);  \
            return;                                                    \
        }                                                              \
    } while (0)

class NapiParamUtils {
public:
    static napi_status GetParam(const napi_env &env, napi_callback_info info, size_t &argc, napi_value *args);
    static napi_status GetValueInt32(const napi_env &env, int32_t &value, napi_value in);
    static napi_status SetValueInt32(const napi_env &env, const int32_t &value, napi_value &result);
    static napi_status GetValueInt32(const napi_env &env, const std::string &fieldStr, int32_t &value, napi_value in);
    static napi_status SetValueInt32(const napi_env &env, const std::string &fieldStr,
        const int32_t value, napi_value &result);

    static napi_status GetValueUInt32(const napi_env &env, uint32_t &value, napi_value in);
    static napi_status SetValueUInt32(const napi_env &env, const uint32_t &value, napi_value &result);

    static napi_status GetValueDouble(const napi_env &env, double &value, napi_value in);
    static napi_status SetValueDouble(const napi_env &env, const double &value, napi_value &result);
    static napi_status GetValueDouble(const napi_env &env, const std::string &fieldStr, double &value, napi_value in);
    static napi_status SetValueDouble(const napi_env &env, const std::string &fieldStr,
        const double value, napi_value &result);

    static std::string GetStringArgument(napi_env env, napi_value value);
    static std::string GetPropertyString(napi_env env, napi_value value, const std::string &fieldStr);
    static napi_status SetValueString(const napi_env &env, const std::string stringValue, napi_value &result);
    static napi_status SetValueString(const napi_env &env, const std::string &fieldStr, const std::string stringValue,
        napi_value &result);

    static napi_status GetValueBoolean(const napi_env &env, bool &boolValue, napi_value in);
    static napi_status SetValueBoolean(const napi_env &env, const bool boolValue, napi_value &result);
    static napi_status GetValueBoolean(const napi_env &env, const std::string &fieldStr,
        bool &boolValue, napi_value in);
    static napi_status SetValueBoolean(const napi_env &env, const std::string &fieldStr,
        const bool boolValue, napi_value &result);

    static napi_status GetValueInt64(const napi_env &env, int64_t &value, napi_value in);
    static napi_status SetValueInt64(const napi_env &env, const int64_t &value, napi_value &result);
    static napi_status GetValueInt64(const napi_env &env, const std::string &fieldStr, int64_t &value, napi_value in);
    static napi_status SetValueInt64(const napi_env &env, const std::string &fieldStr,
        const int64_t value, napi_value &result);
    static napi_status GetArrayBuffer(const napi_env &env, void* &data, size_t &length, napi_value in);
    static napi_status CreateArrayBuffer(const napi_env &env, const std::string &fieldStr, size_t bufferLen,
        uint8_t *bufferData, napi_value &result);

    static napi_value GetUndefinedValue(napi_env env);

    /* NapiAudioRenderer Get&&Set object */
    static void ConvertDeviceInfoToAudioDeviceDescriptor(sptr<AudioDeviceDescriptor> audioDeviceDescriptor,
        const DeviceInfo &deviceInfo);
    static napi_status GetRendererOptions(const napi_env &env, AudioRendererOptions *opts, napi_value in);
    static napi_status GetRendererInfo(const napi_env &env, AudioRendererInfo *rendererInfo, napi_value in);
    static napi_status SetRendererInfo(const napi_env &env, const AudioRendererInfo &rendererInfo, napi_value &result);
    static napi_status GetStreamInfo(const napi_env &env, AudioStreamInfo *streamInfo, napi_value in);
    static napi_status SetStreamInfo(const napi_env &env, const AudioStreamInfo &streamInfo, napi_value &result);
    static napi_status SetValueInt32Element(const napi_env &env, const std::string &fieldStr,
        const std::vector<int32_t> &values, napi_value &result);
    static napi_status SetDeviceDescriptor(const napi_env &env, const AudioDeviceDescriptor &deviceInfo,
        napi_value &result);
    static napi_status SetDeviceDescriptors(const napi_env &env,
        const std::vector<sptr<AudioDeviceDescriptor>> &deviceDescriptors, napi_value &result);
    static napi_status SetValueDeviceInfo(const napi_env &env, const DeviceInfo &deviceInfo, napi_value &result);
    static napi_status SetInterruptEvent(const napi_env &env, const InterruptEvent &interruptEvent,
        napi_value &result);
    static napi_status SetNativeAudioRendererDataInfo(const napi_env &env,
        const AudioRendererDataInfo &audioRendererDataInfo, napi_value &result);
};
} // namespace AudioStandard
} // namespace OHOS
#endif // NAPI_PARAM_UTILS_H
