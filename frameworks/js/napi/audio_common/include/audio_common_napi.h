/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef AUDIO_COMMON_NAPI_H_
#define AUDIO_COMMON_NAPI_H_

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "audio_info.h"


#define THROW_ERROR_ASSERT(env, assertion, code)        \
    do {                                                \
        if (!(assertion)) {                             \
            AudioCommonNapi::throwError( env, code);    \
            return nullptr;                             \
        }                                               \
    } while (0)

#define GET_PARAMS(env, info, num) \
    size_t argc = num;             \
    napi_value argv[num] = {0};    \
    napi_value thisVar = nullptr;  \
    void *data;                    \
    napi_get_cb_info(env, info, &argc, argv, &thisVar, &data)

namespace OHOS {
namespace AudioStandard {
namespace {
    const std::string INTERRUPT_CALLBACK_NAME = "interrupt";
    const std::string AUDIO_INTERRUPT_CALLBACK_NAME = "audioInterrupt";
    const std::string INDEPENDENTINTERRUPT_CALLBACK_NAME = "independentInterrupt";
    const std::string STATE_CHANGE_CALLBACK_NAME = "stateChange";
}

class AudioCommonNapi {
public:
    enum AudioVolumeType {
        VOLUMETYPE_DEFAULT = -1,
        VOICE_CALL = 0,
        RINGTONE = 2,
        MEDIA = 3,
        VOICE_ASSISTANT = 9,
        VOLUMETYPE_MAX,
        ALL = 100
    };
    AudioCommonNapi() = delete;
    ~AudioCommonNapi() = delete;
    static std::string GetStringArgument(napi_env env, napi_value value);
    static std::string getMessageByCode(int32_t &code);
    static void throwError (napi_env env, int32_t code);
    static bool IsLegalInputArgumentVolLevel(int32_t volLevel);
    static bool IsLegalInputArgumentVolType(int32_t inputType);
    static bool IsLegalInputArgumentDeviceFlag(int32_t inputType);
    static bool IsLegalInputArgumentActiveDeviceType(int32_t deviceType);
    static bool IsLegalInputArgumentCommunicationDeviceType(int32_t deviceType);
    static bool IsLegalInputArgumentRingMode(int32_t ringerMode);
    static AudioVolumeType GetNativeAudioVolumeType(int32_t volumeType);
};

struct AutoRef {
    AutoRef(napi_env env, napi_ref cb)
        : env_(env), cb_(cb)
    {
    }
    ~AutoRef()
    {
        if (env_ != nullptr && cb_ != nullptr) {
            (void)napi_delete_reference(env_, cb_);
        }
    }
    napi_env env_;
    napi_ref cb_;
};

const int32_t  ERR_NUMBER_101 = 6800101;
const int32_t  ERR_NUMBER_401 = 401;
const int32_t  ERR_NUMBER101 = 6800101;
const int32_t  ERR_NUMBER102 = 6800102;
const int32_t  ERR_NUMBER103 = 6800103;
const int32_t  ERR_NUMBER104 = 6800104;
const int32_t  ERR_NUMBER105 = 6800105;
const int32_t  ERR_NUMBER201 = 6800201;
const int32_t  ERR_NUMBER301 = 6800301;

const std::string ERR_MESSAGE_101 = "input parameter value error";
const std::string ERR_MESSAGE_401 = "input parameter type or number mismatch";
const std::string ERR_MESSAGE101 = "invalid parameter";
const std::string ERR_MESSAGE102 = "allocate memory failed";
const std::string ERR_MESSAGE103 = "Operation not permit at current state";
const std::string ERR_MESSAGE104 = "unsupported option";
const std::string ERR_MESSAGE105 = "time out";
const std::string ERR_MESSAGE201 = "stream number limited";
const std::string ERR_MESSAGE301 = "system error";
}  // namespace AudioStandard
}  // namespace OHOS
#endif // AUDIO_COMMON_NAPI_H_
