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

#include "audio_sound_manager_napi.h"
#include "hilog/log.h"

using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace {
    /* Constants for array index */
    const int32_t PARAM0 = 0;
    const int32_t PARAM1 = 1;
    const int32_t PARAM2 = 2;
    const int32_t PARAM3 = 3;

    /* Constants for array size */
    const int32_t ARGS_TWO = 2;
    const int32_t ARGS_THREE = 3;
    const int32_t ARGS_FOUR = 4;
    const int32_t SIZE = 100;
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioSoundManagerNapi"};
}

namespace OHOS {
namespace AudioStandard {
napi_ref AudioSoundManagerNapi::sConstructor_ = nullptr;
napi_ref AudioSoundManagerNapi::sRingtoneTypeEnumRef_ = nullptr;

AudioSoundManagerNapi::AudioSoundManagerNapi() : env_(nullptr), wrapper_(nullptr), audioSndMngrClient_(nullptr)
{}

AudioSoundManagerNapi::~AudioSoundManagerNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

napi_value AudioSoundManagerNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor audiosndmngr_prop[] {
        DECLARE_NAPI_FUNCTION("setSystemRingtoneUri", JSSetSystemRingtoneUri),
        DECLARE_NAPI_FUNCTION("getSystemRingtoneUri", JSGetSystemRingtoneUri)
    };

    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("getSystemSoundManager", GetAudioSndMngrInstance),
        DECLARE_NAPI_PROPERTY("RingtoneType", CreateRingtoneTypeEnum(env)),
    };

    status = napi_define_class(env, AUDIO_SND_MNGR_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH,
                               AudioSndMngrNapiConstructor, nullptr, sizeof(audiosndmngr_prop) /
                               sizeof(audiosndmngr_prop[0]), audiosndmngr_prop, &ctorObj);
    if (status == napi_ok) {
        if (napi_create_reference(env, ctorObj, refCount, &sConstructor_) == napi_ok) {
            if (napi_set_named_property(env, exports, AUDIO_SND_MNGR_NAPI_CLASS_NAME.c_str(), ctorObj) == napi_ok &&
                napi_define_properties(env, exports, sizeof(static_prop) /
                sizeof(static_prop[0]), static_prop) == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

napi_value AudioSoundManagerNapi::AudioSndMngrNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    status = napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);
    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<AudioSoundManagerNapi> obj = std::make_unique<AudioSoundManagerNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->audioSndMngrClient_ = AudioStandard::IAudioSoundManager::CreateAudioSoundManager();
            if (obj->audioSndMngrClient_ == nullptr) {
                HiLog::Error(LABEL, "Failed to create audioSndMngrClient_ instance");
                return result;
            }

            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
            AudioSoundManagerNapi::AudioSndMngrNapiDestructor, nullptr, &(obj->wrapper_));
            if (status == napi_ok) {
                obj.release();
                return thisVar;
            } else {
                HiLog::Error(LABEL, "Failed to wrap the native audioSndMngrClient_ object with JS");
            }
        }
    }

    return result;
}

void AudioSoundManagerNapi::AudioSndMngrNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint)
{
    AudioSoundManagerNapi *audioSndMngrHelper = reinterpret_cast<AudioSoundManagerNapi*>(nativeObject);
    if (audioSndMngrHelper != nullptr) {
        audioSndMngrHelper->~AudioSoundManagerNapi();
    }
}

napi_value AudioSoundManagerNapi::GetAudioSndMngrInstance(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value ctor;

    status = napi_get_reference_value(env, sConstructor_, &ctor);
    if (status == napi_ok) {
        status = napi_new_instance(env, ctor, 0, nullptr, &result);
        if (status == napi_ok) {
            return result;
        } else {
            HiLog::Error(LABEL, "New instance could not be obtained");
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

std::shared_ptr<AppExecFwk::Context> GetDataAbilityContext(napi_env env)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_global(env, &result));

    napi_value abilityObj = nullptr;
    NAPI_CALL(env, napi_get_named_property(env, result, "ability", &abilityObj));

    AppExecFwk::Ability *ability = nullptr;
    NAPI_CALL(env, napi_get_value_external(env, abilityObj, (void**)&ability));

    shared_ptr<AppExecFwk::Context> context = shared_ptr<AppExecFwk::Context>(ability);
    return context;
}

static napi_status AddIntegerNamedProperty(napi_env env, napi_value object,
    const string &name, int32_t enumValue)
{
    napi_status status;
    napi_value enumNapiValue;

    HiLog::Info(LABEL, "Add int prop");
    status = napi_create_int32(env, enumValue, &enumNapiValue);
    if (status == napi_ok) {
        status = napi_set_named_property(env, object, name.c_str(), enumNapiValue);
    }

    return status;
}

napi_value AudioSoundManagerNapi::CreateRingtoneTypeEnum(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    string propName;
    int refCount = 1;

    HiLog::Info(LABEL, "ringtone type enum");
    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = RINGTONE_TYPE_DEFAULT; i < ringtoneTypesEnum.size(); i++) {
            propName = ringtoneTypesEnum[i];
            status = AddIntegerNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                HiLog::Error(LABEL, "Failed to add named prop!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        // The reference count is for creating Media Type Enum Reference
        status = napi_create_reference(env, result, refCount, &sRingtoneTypeEnumRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    HiLog::Error(LABEL, "Failed to created object for media type enum!");

    napi_get_undefined(env, &result);
    return result;
}

static RingtoneType GetNativeRingtoneType(int32_t ringtoneType)
{
    RingtoneType result = RingtoneType::RINGTONE_TYPE_DEFAULT;
    if (ringtoneType == AudioSoundManagerNapi::MULTISIM) {
        result = RingtoneType::RINGTONE_TYPE_MULTISIM;
    }

    return result;
}

void GetArgsForSetSystemRingtoneUri(napi_env env, size_t argc, const napi_value argv[],
    AudioSndMngrAsyncContext &context)
{
    char buffer[SIZE];
    size_t res = 0;
    auto asyncContext = &context;
    const int32_t refCount = 1;

    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, argv[PARAM1], &valueType);
    if (valueType == napi_string) {
        napi_get_value_string_utf8(env, argv[PARAM1], buffer, SIZE, &res);
        asyncContext->uri = std::string(buffer);
    }

    napi_typeof(env, argv[PARAM2], &valueType);
    if (valueType == napi_number) {
        napi_get_value_int32(env, argv[PARAM2], &asyncContext->ringtoneType);
    }

    if (argc == ARGS_FOUR) {
    napi_typeof(env, argv[PARAM3], &valueType);
        if (valueType == napi_function) {
            napi_create_reference(env, argv[PARAM3], refCount, &asyncContext->callbackRef);
    }
    }
}

static void SetSysRngUriAsyncCallbComp(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<AudioSndMngrAsyncContext*>(data);
    napi_value callback = nullptr;
    napi_value retVal = nullptr;
    napi_value result[2] = {};

    napi_get_undefined(env, &result[PARAM1]);
    if (!context->status) {
        napi_get_undefined(env, &result[PARAM0]);
    } else {
        napi_value message = nullptr;
        napi_create_string_utf8(env, "Error, operation not supported or failed", NAPI_AUTO_LENGTH, &message);
        napi_create_error(env, nullptr, message, &result[PARAM0]);
    }

    if (context->deferred) {
        if (!context->status) {
            napi_resolve_deferred(env, context->deferred, result[PARAM1]);
        } else {
            napi_reject_deferred(env, context->deferred, result[PARAM0]);
        }
    } else {
        napi_get_reference_value(env, context->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, context->callbackRef);
    }

    napi_delete_async_work(env, context->work);

    delete context;
}

napi_value AudioSoundManagerNapi::JSSetSystemRingtoneUri(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_FOUR;
    napi_value argv[ARGS_FOUR] = {0};
    napi_value thisVar = nullptr;

    status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    if (status != napi_ok || thisVar == nullptr) {
        HiLog::Error(LABEL, "Failed to retrieve details about the object");
        return result;
    }

    NAPI_ASSERT(env, (argc == ARGS_THREE || argc == ARGS_FOUR), "required 4 parameters maximum");

    std::unique_ptr<AudioSndMngrAsyncContext> asyncContext = std::make_unique<AudioSndMngrAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        GetArgsForSetSystemRingtoneUri(env, argc, argv, *asyncContext);
        if (argc == ARGS_THREE) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "JSSetSystemRingtoneUri", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void* data) {
                AudioSndMngrAsyncContext* context = static_cast<AudioSndMngrAsyncContext*>(data);
                (void) context->objectInfo->audioSndMngrClient_->SetSystemRingtoneUri(
                    nullptr, context->uri, GetNativeRingtoneType(context->ringtoneType));
                context->status = 0;
            },
            SetSysRngUriAsyncCallbComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            HiLog::Error(LABEL, "Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void GetSysRngUriAsyncCallbComp(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<AudioSndMngrAsyncContext*>(data);
    napi_value callback = nullptr;
    napi_value retVal;
    napi_value result[2] = {};

    if (!context->status) {
        napi_get_undefined(env, &result[PARAM0]);
        napi_create_string_utf8(env, context->uri.c_str(), NAPI_AUTO_LENGTH, &result[PARAM1]);
    } else {
        napi_value message = nullptr;
        napi_create_string_utf8(env, "Error, operation not supported or failed", NAPI_AUTO_LENGTH, &message);
        napi_create_error(env, nullptr, message, &result[PARAM0]);
        napi_get_undefined(env, &result[PARAM1]);
    }

    if (context->deferred) {
        if (!context->status) {
            napi_resolve_deferred(env, context->deferred, result[PARAM1]);
        } else {
            napi_reject_deferred(env, context->deferred, result[PARAM0]);
        }
    } else {
        napi_get_reference_value(env, context->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, context->callbackRef);
    }

    napi_delete_async_work(env, context->work);

    delete context;
}

void GetArgsForGetSystemRingtoneUri(napi_env env, size_t argc, const napi_value argv[],
    AudioSndMngrAsyncContext &context)
{
    auto asyncContext = &context;
    const int32_t refCount = 1;

    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, argv[PARAM1], &valueType);
    if (valueType == napi_number) {
        napi_get_value_int32(env, argv[PARAM1], &asyncContext->ringtoneType);
    }

    if (argc == ARGS_THREE) {
        napi_typeof(env, argv[PARAM2], &valueType);
        if (valueType == napi_function) {
            napi_create_reference(env, argv[PARAM2], refCount, &asyncContext->callbackRef);
        }
    }
}

napi_value AudioSoundManagerNapi::JSGetSystemRingtoneUri(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {0};
    napi_value thisVar = nullptr;

    status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    if (status != napi_ok || thisVar == nullptr) {
        HiLog::Error(LABEL, "Failed to retrieve details about the object");
        return result;
    }

    NAPI_ASSERT(env, (argc == ARGS_TWO || argc == ARGS_THREE), "required 3 parameters maximum");

    std::unique_ptr<AudioSndMngrAsyncContext> asyncContext = std::make_unique<AudioSndMngrAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        GetArgsForGetSystemRingtoneUri(env, argc, argv, *asyncContext);
        if (argc == ARGS_TWO) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "JSGetSystemRingtoneUri", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void* data) {
                AudioSndMngrAsyncContext* context = static_cast<AudioSndMngrAsyncContext*>(data);
                context->uri = context->objectInfo->audioSndMngrClient_->GetSystemRingtoneUri(
                    nullptr, GetNativeRingtoneType(context->ringtoneType));
                context->status = 0;
            },
            GetSysRngUriAsyncCallbComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            HiLog::Error(LABEL, "Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}
} // namespace AudioStandard
} // namespace OHOS