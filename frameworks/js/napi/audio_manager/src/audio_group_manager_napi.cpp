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
#include "audio_group_manager_napi.h"
#include "audio_common_napi.h"
#include "audio_errors.h"

#include "hilog/log.h"
#include "audio_log.h"

using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace OHOS {
namespace AudioStandard {
static __thread napi_ref g_groupmanagerConstructor = nullptr;

#define GET_PARAMS(env, info, num) \
    size_t argc = num;             \
    napi_value argv[num] = {0};    \
    napi_value thisVar = nullptr;  \
    void *data;                    \
    napi_get_cb_info(env, info, &argc, argv, &thisVar, &data)

struct AudioGroupManagerAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef = nullptr;
    int32_t volType;
    int32_t volLevel;
    int32_t deviceType;
    int32_t ringMode;
    int32_t scene;
    int32_t deviceFlag;
    int32_t intValue;
    int32_t status;
    int32_t groupId;
    bool isMute;
    bool isActive;
    bool isTrue;
    string key;
    string valueStr;
    int32_t networkId;
    AudioGroupManagerNapi *objectInfo;
};

namespace {
    const int ARGS_ONE = 1;
    const int ARGS_TWO = 2;
    const int ARGS_THREE = 3;
    const int PARAM0 = 0;
    const int PARAM1 = 1;
    const int PARAM2 = 2;
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioGroupManagerNapi"};
}

static AudioVolumeType GetNativeAudioVolumeType(int32_t volumeType)
{
    AudioVolumeType result = STREAM_MUSIC;

    switch (volumeType) {
        case AudioManagerNapi::RINGTONE:
            result = STREAM_RING;
            break;
        case AudioManagerNapi::MEDIA:
            result = STREAM_MUSIC;
            break;
        case AudioManagerNapi::VOICE_CALL:
            result = STREAM_VOICE_CALL;
            break;
        case AudioManagerNapi::VOICE_ASSISTANT:
            result = STREAM_VOICE_ASSISTANT;
            break;

        default:
            result = STREAM_MUSIC;
            HiLog::Error(LABEL, "Unknown volume type, Set it to default MEDIA!");
            break;
    }
    return result;
}

static void CommonCallbackRoutine(napi_env env, AudioGroupManagerAsyncContext* &asyncContext,
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

static void GetIntValueAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioGroupManagerAsyncContext*>(data);
    napi_value valueParam = nullptr;

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            napi_create_int32(env, asyncContext->intValue, &valueParam);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        HiLog::Error(LABEL, "ERROR: AudioGroupManagerAsyncContext* is Null!");
    }
}


static void SetFunctionAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioGroupManagerAsyncContext*>(data);
    napi_value valueParam = nullptr;

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            napi_get_undefined(env, &valueParam);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        HiLog::Error(LABEL, "ERROR: AudioManagerAsyncContext* is Null!");
    }
}

static void IsTrueAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioGroupManagerAsyncContext*>(data);
    napi_value valueParam = nullptr;

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            napi_get_boolean(env, asyncContext->isTrue, &valueParam);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        HiLog::Error(LABEL, "ERROR: AudioManagerAsyncContext* is Null!");
    }
}

// Constructor callback
napi_value AudioGroupManagerNapi::Construct(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsThis;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    size_t argCount = 1;
    int32_t valueParam = 0;

    napi_value args[1] = { nullptr};
    status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    napi_get_value_int32(env, args[0], &valueParam);
    HiLog::Info(LABEL, "Construct() %{public}d", valueParam);

    if (status == napi_ok) {
        unique_ptr<AudioGroupManagerNapi> groupmanagerNapi = make_unique<AudioGroupManagerNapi>();
        if (groupmanagerNapi != nullptr) {
            groupmanagerNapi->audioGroupMngr_ = AudioSystemManager::GetInstance()-> GetGroupManager(valueParam);
            status = napi_wrap(env, jsThis, static_cast<void*>(groupmanagerNapi.get()),
                AudioGroupManagerNapi::Destructor, nullptr, &(groupmanagerNapi->wrapper_));
            if (status == napi_ok) {
                groupmanagerNapi.release();
                return jsThis;
            }
        }
    }

    HiLog::Error(LABEL, "Failed in AudioGroupManagerNapi::Construct()!");

    return undefinedResult;
}

void AudioGroupManagerNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    if (nativeObject != nullptr) {
        auto obj = static_cast<AudioGroupManagerNapi*>(nativeObject);
        delete obj;
        obj = nullptr;
        AUDIO_DEBUG_LOG("AudioGroupManagerNapi::Destructor delete AudioGroupManagerNapi obj done");
    }
}

napi_value AudioGroupManagerNapi::CreateAudioGroupManagerWrapper(napi_env env, int32_t groupId)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;
    napi_value resul;
    napi_create_int64(env, groupId, &resul);
    napi_value args[1] = {resul};
    HiLog::Info(LABEL, "AudioGroupManagerNapi::CreateAudioGroupManagerWrapper() groupId %{public}d", groupId);
    status = napi_get_reference_value(env, g_groupmanagerConstructor, &constructor);
    if (status == napi_ok) {
        status = napi_new_instance(env, constructor, 1, args, &result);
        if (status == napi_ok) {
            return result;
        }
    }

    HiLog::Error(LABEL, "Failed in AudioGroupManagerNapi::CreateaudioMngrWrapper!");
    napi_get_undefined(env, &result);

    return result;
}

void GetGroupManagerAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    napi_value valueParam = nullptr;
    auto asyncContext = static_cast<AudioGroupManagerAsyncContext *>(data);

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            unique_ptr<AudioGroupManagerAsyncContext> capturerOptions = make_unique<AudioGroupManagerAsyncContext>();
            valueParam = AudioGroupManagerNapi::CreateAudioGroupManagerWrapper(env, asyncContext->groupId);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        HiLog::Error(LABEL, "ERROR: GetGroupManagerAsyncCallbackComplete is Null!");
    }
}

napi_value AudioGroupManagerNapi::GetVolume(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);
    NAPI_ASSERT(env, argc >= ARGS_ONE, "requires 1 parameter minimum");

    unique_ptr<AudioGroupManagerAsyncContext> asyncContext = make_unique<AudioGroupManagerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->volType);
                HiLog::Info(LABEL, " GetVolume volType = %{public}d", asyncContext->volType);
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
        napi_create_string_utf8(env, "GetVolume", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioGroupManagerAsyncContext*>(data);
                context->volLevel = context->objectInfo->audioGroupMngr_->GetVolume(
                    GetNativeAudioVolumeType(context->volType));
                context->intValue = context->volLevel;
                context->status = 0;
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

napi_value AudioGroupManagerNapi::SetVolume(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_THREE);
    NAPI_ASSERT(env, argc >= ARGS_TWO, "requires 2 parameters minimum");

    unique_ptr<AudioGroupManagerAsyncContext> asyncContext = make_unique<AudioGroupManagerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->volType);
                HiLog::Info(LABEL, " SetVolume volType = %{public}d", asyncContext->volType);
            } else if (i == PARAM1 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->volLevel);
                HiLog::Info(LABEL, " SetVolume volLevel = %{public}d", asyncContext->volLevel);
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
        napi_create_string_utf8(env, "SetVolume", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioGroupManagerAsyncContext*>(data);
                context->status = context->objectInfo->audioGroupMngr_->SetVolume(GetNativeAudioVolumeType(
                    context->volType), context->volLevel);
            }, SetFunctionAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
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

napi_value AudioGroupManagerNapi::GetMaxVolume(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);
    NAPI_ASSERT(env, argc >= ARGS_ONE, "requires 1 parameter minimum");

    unique_ptr<AudioGroupManagerAsyncContext> asyncContext = make_unique<AudioGroupManagerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->volType);
                HiLog::Info(LABEL, " GetMaxVolume volType = %{public}d", asyncContext->volType);
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
        napi_create_string_utf8(env, "GetMaxVolume", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioGroupManagerAsyncContext*>(data);
                context->volLevel = context->objectInfo->audioGroupMngr_->GetMaxVolume(
                    GetNativeAudioVolumeType(context->volType));
                context->intValue = context->volLevel;
                context->status = 0;
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

napi_value AudioGroupManagerNapi::GetMinVolume(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);
    NAPI_ASSERT(env, argc >= ARGS_ONE, "requires 1 parameter minimum");

    unique_ptr<AudioGroupManagerAsyncContext> asyncContext = make_unique<AudioGroupManagerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->volType);
                HiLog::Info(LABEL, " GetMinVolume volType = %{public}d", asyncContext->volType);
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
        napi_create_string_utf8(env, "GetMinVolume", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioGroupManagerAsyncContext*>(data);
                context->volLevel = context->objectInfo->audioGroupMngr_->GetMinVolume(
                    GetNativeAudioVolumeType(context->volType));
                context->intValue = context->volLevel;
                context->status = 0;
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

napi_value AudioGroupManagerNapi::SetMute(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_THREE);
    NAPI_ASSERT(env, argc >= ARGS_TWO, "requires 2 parameters minimum");

    unique_ptr<AudioGroupManagerAsyncContext> asyncContext = make_unique<AudioGroupManagerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->volType);
                HiLog::Info(LABEL, " SetMute volType = %{public}d", asyncContext->volType);
            } else if (i == PARAM1 && valueType == napi_boolean) {
                napi_get_value_bool(env, argv[i], &asyncContext->isMute);
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
        napi_create_string_utf8(env, "SetStreamMute", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioGroupManagerAsyncContext*>(data);
                context->status = context->objectInfo->audioGroupMngr_->SetMute(GetNativeAudioVolumeType(
                    context->volType), context->isMute);
            },
            SetFunctionAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
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

napi_value AudioGroupManagerNapi::IsStreamMute(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);
    NAPI_ASSERT(env, argc >= ARGS_ONE, "requires 1 parameter minimum");

    unique_ptr<AudioGroupManagerAsyncContext> asyncContext = make_unique<AudioGroupManagerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->volType);
                HiLog::Info(LABEL, " IsStreamMute volType = %{public}d", asyncContext->volType);
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
        napi_create_string_utf8(env, "IsStreamMute", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioGroupManagerAsyncContext*>(data);
                context->isMute =
                    context->objectInfo->audioGroupMngr_->IsStreamMute(GetNativeAudioVolumeType(context->volType));
                context->isTrue = context->isMute;
                context->status = 0;
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
} // namespace AudioStandard
} // namespace OHOS
