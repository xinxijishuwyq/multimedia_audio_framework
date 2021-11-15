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

#include "audio_capturer_napi.h"
#include "audio_manager_napi.h"
#include "audio_parameters_napi.h"
#include "hilog/log.h"

#include "securec.h"

using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace OHOS {
namespace AudioStandard {
static __thread napi_ref capturerConstructor_ = nullptr;
std::unique_ptr<AudioParameters> AudioCapturerNapi::sAudioParameters_ = nullptr;

namespace {
    const int ARGS_ONE = 1;
    const int ARGS_TWO = 2;
    const int ARGS_THREE = 3;

    const int PARAM0 = 0;
    const int PARAM1 = 1;
    const int PARAM2 = 2;

    const int ERROR = -1;
    const int SUCCESS = 0;

    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioCapturerNapi"};

#define GET_PARAMS(env, info, num) \
    size_t argc = num;             \
    napi_value argv[num] = {0};    \
    napi_value thisVar = nullptr;  \
    void *data;                    \
    napi_get_cb_info(env, info, &argc, argv, &thisVar, &data)
}

AudioCapturerNapi::AudioCapturerNapi()
    : audioCapturer_(nullptr), contentType_(CONTENT_TYPE_MUSIC), streamUsage_(STREAM_USAGE_MEDIA),
      deviceRole_(INPUT_DEVICE), deviceType_(DEVICE_TYPE_MIC), env_(nullptr), wrapper_(nullptr) {}

AudioCapturerNapi::~AudioCapturerNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

void AudioCapturerNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    if (nativeObject != nullptr) {
        auto obj = static_cast<AudioCapturerNapi *>(nativeObject);
        delete obj;
    }
}

napi_value AudioCapturerNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value constructor;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_get_undefined(env, &result);

    napi_property_descriptor audio_capturer_properties[] = {
        DECLARE_NAPI_FUNCTION("setParams", SetParams),
        DECLARE_NAPI_FUNCTION("getParams", GetParams),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("read", Read),
        DECLARE_NAPI_FUNCTION("getAudioTime", GetAudioTime),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("getBufferSize", GetBufferSize)
    };

    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAudioCapturer", CreateAudioCapturer)
    };

    status = napi_define_class(env, AUDIO_CAPTURER_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Construct, nullptr,
        sizeof(audio_capturer_properties) / sizeof(audio_capturer_properties[PARAM0]),
        audio_capturer_properties, &constructor);
    if (status != napi_ok) {
        return result;
    }

    status = napi_create_reference(env, constructor, refCount, &capturerConstructor_);
    if (status == napi_ok) {
        status = napi_set_named_property(env, exports, AUDIO_CAPTURER_NAPI_CLASS_NAME.c_str(), constructor);
        if (status == napi_ok) {
            status = napi_define_properties(env, exports,
                                            sizeof(static_prop) / sizeof(static_prop[PARAM0]), static_prop);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    HiLog::Error(LABEL, "Failure in AudioCapturerNapi::Init()");

    return result;
}

static int32_t GetAudioStreamType(napi_env env, napi_value value)
{
    napi_status status;
    int32_t streamType = AudioStreamType::STREAM_DEFAULT;

    status = napi_get_value_int32(env, value, &streamType);
    if (status == napi_ok) {
        switch (streamType) {
            case AudioManagerNapi::RINGTONE:
                streamType = AudioStreamType::STREAM_RING;
                break;
            case AudioManagerNapi::MEDIA:
                streamType = AudioStreamType::STREAM_MUSIC;
                break;
            case AudioManagerNapi::VOICE_ASSISTANT:
                streamType = AudioStreamType::STREAM_VOICE_ASSISTANT;
                break;
            case AudioManagerNapi::VOICE_CALL:
                streamType = AudioStreamType::STREAM_VOICE_CALL;
                break;
            default:
                streamType = AudioStreamType::STREAM_MUSIC;
                break;
        }
    }

    return streamType;
}

napi_value AudioCapturerNapi::Construct(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsThis = nullptr;
    napi_value result = nullptr;
    size_t argCount = 1;
    napi_value args[1] = {0};

    napi_get_undefined(env, &result);
    status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok) {
        return result;
    }

    int32_t streamType = GetAudioStreamType(env, args[0]);
    HiLog::Info(LABEL, "AudioCapturerNapi: Audio stream type: %{public}d", streamType);
    if (streamType != AudioStreamType::STREAM_DEFAULT) {
        unique_ptr<AudioCapturerNapi> obj = make_unique<AudioCapturerNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->audioCapturer_
                = AudioCapturer::Create(static_cast<AudioStreamType>(streamType));
            status = napi_wrap(env, jsThis, static_cast<void*>(obj.get()),
                               AudioCapturerNapi::Destructor, nullptr, &(obj->wrapper_));
            if (status == napi_ok) {
                obj.release();
                return jsThis;
            }
        }
    }

    HiLog::Error(LABEL, "Failed in AudioCapturerNapi::Construct()!");

    return result;
}

napi_value AudioCapturerNapi::CreateAudioCapturer(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;
    size_t argCount = 1;
    napi_value args[1] = {0};

    status = napi_get_cb_info(env, info, &argCount, args, nullptr, nullptr);
    if (status != napi_ok || argCount != 1) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return nullptr;
    }

    status = napi_get_reference_value(env, capturerConstructor_, &constructor);
    if (status == napi_ok) {
        status = napi_new_instance(env, constructor, argCount, args, &result);
        if (status == napi_ok) {
            return result;
        }
    }

    HiLog::Error(LABEL, "Create audio capturer failed");
    napi_get_undefined(env, &result);

    return result;
}

void AudioCapturerNapi::CommonCallbackRoutine(napi_env env, AudioCapturerAsyncContext* &asyncContext,
                                              const napi_value &valueParam)
{
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;

    if (!asyncContext->status) {
        napi_get_undefined(env, &result[PARAM0]);
        result[PARAM1] = valueParam;
    } else {
        napi_value message = nullptr;
        napi_create_string_utf8(env, "Error, Operation not supported or Failed", NAPI_AUTO_LENGTH, &message);
        napi_create_error(env, nullptr, message, &result[PARAM0]);
        napi_get_undefined(env, &result[PARAM1]);
    }

    if (asyncContext->deferred) {
        if (!asyncContext->status) {
            napi_resolve_deferred(env, asyncContext->deferred, result[PARAM1]);
        } else {
            napi_reject_deferred(env, asyncContext->deferred, result[PARAM0]);
        }
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
    asyncContext = nullptr;
}

void AudioCapturerNapi::SetFunctionAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioCapturerAsyncContext*>(data);
    napi_value valueParam = nullptr;

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            napi_get_undefined(env, &valueParam);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        HiLog::Error(LABEL, "ERROR: AudioCapturerAsyncContext* is Null!");
    }
}

void AudioCapturerNapi::AudioParamsAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioCapturerAsyncContext *>(data);
    napi_value valueParam = nullptr;

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            unique_ptr<AudioParameters> audioParams = make_unique<AudioParameters>();
            audioParams->format = asyncContext->audioSampleFormat;
            audioParams->samplingRate = asyncContext->samplingRate;
            audioParams->channels = asyncContext->audioChannel;
            audioParams->encoding = asyncContext->audioEncoding;
            audioParams->contentType = asyncContext->contentType;
            audioParams->usage = asyncContext->usage;
            audioParams->deviceRole = asyncContext->deviceRole;
            audioParams->deviceType = asyncContext->deviceType;

            valueParam = AudioParametersNapi::CreateAudioParametersWrapper(env, audioParams);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        HiLog::Error(LABEL, "ERROR: AudioCapturerAsyncContext* is Null!");
    }
}

void AudioCapturerNapi::ReadAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioCapturerAsyncContext *>(data);
    napi_value valueParam = nullptr;

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            uint8_t *native = nullptr;
            napi_create_arraybuffer(env, asyncContext->bytesRead, reinterpret_cast<void **>(&native), &valueParam);
            if (memcpy_s(native, asyncContext->bytesRead, asyncContext->buffer, asyncContext->bytesRead)) {
                valueParam = nullptr;
            }

            free(asyncContext->buffer);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        HiLog::Error(LABEL, "ERROR: AudioCapturerAsyncContext* is Null!");
    }
}

void AudioCapturerNapi::IsTrueAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioCapturerAsyncContext*>(data);
    napi_value valueParam = nullptr;

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            napi_get_boolean(env, asyncContext->isTrue, &valueParam);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        HiLog::Error(LABEL, "ERROR: AudioCapturerAsyncContext* is Null!");
    }
}

void AudioCapturerNapi::GetIntValueAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioCapturerAsyncContext*>(data);
    napi_value valueParam = nullptr;

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            napi_create_int32(env, asyncContext->intValue, &valueParam);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        HiLog::Error(LABEL, "ERROR: AudioCapturerAsyncContext* is Null!");
    }
}

void AudioCapturerNapi::GetInt64ValueAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioCapturerAsyncContext*>(data);
    napi_value valueParam = nullptr;

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            napi_create_int64(env, asyncContext->time, &valueParam);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        HiLog::Error(LABEL, "ERROR: AudioCapturerAsyncContext* is Null!");
    }
}

int32_t AudioCapturerNapi::SetAudioParameters(napi_env env, napi_value arg)
{
    int32_t format;
    int32_t channels;
    int32_t samplingRate;
    int32_t encoding;
    int32_t contentType = static_cast<int32_t>(CONTENT_TYPE_MUSIC);
    int32_t usage = static_cast<int32_t>(STREAM_USAGE_MEDIA);
    int32_t deviceRole = static_cast<int32_t>(INPUT_DEVICE);
    int32_t deviceType = static_cast<int32_t>(DEVICE_TYPE_MIC);

    napi_value property;

    if ((napi_get_named_property(env, arg, "format", &property) != napi_ok)
        || napi_get_value_int32(env, property, &format) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the format argument!");
        return ERROR;
    }

    if ((napi_get_named_property(env, arg, "channels", &property) != napi_ok)
        || napi_get_value_int32(env, property, &channels) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the channels argument!");
        return ERROR;
    }

    if ((napi_get_named_property(env, arg, "samplingRate", &property) != napi_ok)
        || napi_get_value_int32(env, property, &samplingRate) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the samplingRate argument!");
        return ERROR;
    }

    if ((napi_get_named_property(env, arg, "encoding", &property) != napi_ok)
        || napi_get_value_int32(env, property, &encoding) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the encoding argument!");
        return ERROR;
    }

    if ((napi_get_named_property(env, arg, "contentType", &property) != napi_ok)
        || napi_get_value_int32(env, property, &contentType) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the contentType argument, set existing values");
        contentType = this->contentType_;
    }

    if ((napi_get_named_property(env, arg, "usage", &property) != napi_ok)
        || napi_get_value_int32(env, property, &usage) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the usage argument, set existing value");
        usage = this->streamUsage_;
    }

    if ((napi_get_named_property(env, arg, "deviceRole", &property) != napi_ok)
        || napi_get_value_int32(env, property, &deviceRole) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the device role argument, set existing value");
        deviceRole = this->deviceRole_;
    }

    if ((napi_get_named_property(env, arg, "deviceType", &property) != napi_ok)
        || napi_get_value_int32(env, property, &deviceType) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the device type argument, set existing value");
        deviceType = this->deviceType_;
    }

    if (sAudioParameters_) {
        sAudioParameters_.reset();
    }

    sAudioParameters_ = std::make_unique<AudioParameters>();
    sAudioParameters_->format = static_cast<AudioSampleFormat>(format);
    sAudioParameters_->channels = static_cast<AudioChannel>(channels);
    sAudioParameters_->samplingRate = static_cast<AudioSamplingRate>(samplingRate);
    sAudioParameters_->encoding = static_cast<AudioEncodingType>(encoding);
    sAudioParameters_->contentType = static_cast<ContentType>(contentType);
    sAudioParameters_->usage = static_cast<StreamUsage>(usage);
    sAudioParameters_->deviceRole = static_cast<DeviceRole>(deviceRole);
    sAudioParameters_->deviceType = static_cast<DeviceType>(deviceType);

    return SUCCESS;
}

napi_value AudioCapturerNapi::SetParams(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);
    NAPI_ASSERT(env, argc >= ARGS_ONE, "requires 1 parameters minimum");

    unique_ptr<AudioCapturerAsyncContext> asyncContext = make_unique<AudioCapturerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_object) {
                int32_t ret = asyncContext->objectInfo->SetAudioParameters(env, argv[PARAM0]);
                NAPI_ASSERT(env, ret == SUCCESS, "missing properties");
                asyncContext->audioSampleFormat = sAudioParameters_->format;
                asyncContext->samplingRate =  sAudioParameters_->samplingRate;
                asyncContext->audioChannel = sAudioParameters_->channels;
                asyncContext->audioEncoding = sAudioParameters_->encoding;
                asyncContext->objectInfo->contentType_ = sAudioParameters_->contentType;
                asyncContext->objectInfo->streamUsage_ = sAudioParameters_->usage;
                asyncContext->objectInfo->deviceRole_ = sAudioParameters_->deviceRole;
                asyncContext->objectInfo->deviceType_ = sAudioParameters_->deviceType;
            } else if (i == PARAM1 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "SetParams", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioCapturerAsyncContext *>(data);
                AudioCapturerParams capturerParams;
                capturerParams.audioSampleFormat = context->audioSampleFormat;
                capturerParams.samplingRate =  context->samplingRate;
                capturerParams.audioChannel = context->audioChannel;
                capturerParams.audioEncoding = context->audioEncoding;
                context->status = context->objectInfo->audioCapturer_->SetParams(capturerParams);
            },
            SetFunctionAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            status = napi_queue_async_work(env, asyncContext->work);
            if (status == napi_ok) {
                asyncContext.release();
            } else {
                result = nullptr;
            }
        }
    }

    return result;
}

napi_value AudioCapturerNapi::GetParams(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_ONE);

    unique_ptr<AudioCapturerAsyncContext> asyncContext = make_unique<AudioCapturerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "GetParams", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioCapturerAsyncContext *>(data);
                AudioCapturerParams capturerParams;
                context->status = context->objectInfo->audioCapturer_->GetParams(capturerParams);
                if (context->status == SUCCESS) {
                    context->audioSampleFormat = capturerParams.audioSampleFormat;
                    context->samplingRate = capturerParams.samplingRate;
                    context->audioChannel = capturerParams.audioChannel;
                    context->audioEncoding = capturerParams.audioEncoding;
                    context->contentType = context->objectInfo->contentType_;
                    context->usage = context->objectInfo->streamUsage_;
                    context->deviceRole = context->objectInfo->deviceRole_;
                    context->deviceType = context->objectInfo->deviceType_;
                }
            },
            AudioParamsAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            status = napi_queue_async_work(env, asyncContext->work);
            if (status == napi_ok) {
                asyncContext.release();
            } else {
                result = nullptr;
            }
        }
    }

    return result;
}

napi_value AudioCapturerNapi::Start(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_ONE);

    unique_ptr<AudioCapturerAsyncContext> asyncContext = make_unique<AudioCapturerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "Start", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioCapturerAsyncContext *>(data);
                context->isTrue = context->objectInfo->audioCapturer_->Start();
                context->status = SUCCESS;
            },
            IsTrueAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            status = napi_queue_async_work(env, asyncContext->work);
            if (status == napi_ok) {
                asyncContext.release();
            } else {
                result = nullptr;
            }
        }
    }

    return result;
}

napi_value AudioCapturerNapi::Read(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_THREE);
    NAPI_ASSERT(env, argc >= ARGS_TWO, "requires 2 parameters minimum");

    unique_ptr<AudioCapturerAsyncContext> asyncContext = make_unique<AudioCapturerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if ((i == PARAM0) && (valueType == napi_number)) {
                napi_get_value_uint32(env, argv[i], &asyncContext->userSize);
            } else if ((i == PARAM1) && (valueType == napi_boolean)) {
                napi_get_value_bool(env, argv[i], &asyncContext->isBlocking);
            } else if (i == PARAM2 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "Read", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioCapturerAsyncContext *>(data);
                context->status = ERROR;
                uint32_t userSize = context->userSize;
                auto buffer = std::make_unique<uint8_t[]>(userSize);
                if (!buffer) {
                    return;
                }

                size_t bytesRead = 0;
                while (bytesRead < context->userSize) {
                    int32_t len = context->objectInfo->audioCapturer_->Read(*(buffer.get() + bytesRead),
                                                                            userSize - bytesRead,
                                                                            context->isBlocking);
                    if (len >= 0) {
                        bytesRead += len;
                    } else {
                        bytesRead = len;
                        break;
                    }
                }

                if (bytesRead > 0) {
                    context->bytesRead = bytesRead;
                    context->buffer = buffer.get();
                    buffer.release();
                    context->status = SUCCESS;
                }
            },
            ReadAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            status = napi_queue_async_work(env, asyncContext->work);
            if (status == napi_ok) {
                asyncContext.release();
            } else {
                result = nullptr;
            }
        }
    }

    return result;
}

napi_value AudioCapturerNapi::GetAudioTime(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_ONE);

    unique_ptr<AudioCapturerAsyncContext> asyncContext = make_unique<AudioCapturerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "GetAudioTime", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioCapturerAsyncContext *>(data);
                context->status = ERROR;
                Timestamp timestamp;
                if (context->objectInfo->audioCapturer_->GetAudioTime(timestamp, Timestamp::Timestampbase::MONOTONIC)) {
                    const uint64_t secToNanosecond = 1000000000;
                    context->time = timestamp.time.tv_nsec + timestamp.time.tv_sec * secToNanosecond;
                    context->status = SUCCESS;
                }
            },
            GetInt64ValueAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            status = napi_queue_async_work(env, asyncContext->work);
            if (status == napi_ok) {
                asyncContext.release();
            } else {
                result = nullptr;
            }
        }
    }

    return result;
}

napi_value AudioCapturerNapi::Stop(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_ONE);

    unique_ptr<AudioCapturerAsyncContext> asyncContext = make_unique<AudioCapturerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "Stop", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioCapturerAsyncContext *>(data);
                context->isTrue = context->objectInfo->audioCapturer_->Stop();
                context->status = SUCCESS;
            },
            IsTrueAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            status = napi_queue_async_work(env, asyncContext->work);
            if (status == napi_ok) {
                asyncContext.release();
            } else {
                result = nullptr;
            }
        }
    }

    return result;
}

napi_value AudioCapturerNapi::Release(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_ONE);

    unique_ptr<AudioCapturerAsyncContext> asyncContext = make_unique<AudioCapturerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "Release", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioCapturerAsyncContext *>(data);
                context->isTrue = context->objectInfo->audioCapturer_->Release();
                context->status = SUCCESS;
            },
            IsTrueAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            status = napi_queue_async_work(env, asyncContext->work);
            if (status == napi_ok) {
                asyncContext.release();
            } else {
                result = nullptr;
            }
        }
    }

    return result;
}

napi_value AudioCapturerNapi::GetBufferSize(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_ONE);

    unique_ptr<AudioCapturerAsyncContext> asyncContext = make_unique<AudioCapturerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "GetBufferSize", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioCapturerAsyncContext *>(data);
                size_t bufferSize;
                context->status = context->objectInfo->audioCapturer_->GetBufferSize(bufferSize);
                if (context->status == SUCCESS) {
                    context->intValue = bufferSize;
                }
            },
            GetIntValueAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            status = napi_queue_async_work(env, asyncContext->work);
            if (status == napi_ok) {
                asyncContext.release();
            } else {
                result = nullptr;
            }
        }
    }

    return result;
}
}
}
