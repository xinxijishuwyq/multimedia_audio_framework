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

#include "audio_manager_napi.h"
#include "audio_device_descriptor_napi.h"
#include "hilog/log.h"

using namespace OHOS;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

napi_ref AudioManagerNapi::sConstructor_ = nullptr;
napi_ref AudioManagerNapi::audioVolumeTypeRef_ = nullptr;
napi_ref AudioManagerNapi::deviceFlagRef_ = nullptr;
napi_ref AudioManagerNapi::deviceRoleRef_ = nullptr;
napi_ref AudioManagerNapi::deviceTypeRef_ = nullptr;

#define GET_PARAMS(env, info, num) \
    size_t argc = num;             \
    napi_value argv[num] = {0};    \
    napi_value thisVar = nullptr;  \
    void* data;                    \
    napi_get_cb_info(env, info, &argc, argv, &thisVar, &data)

struct AudioManagerAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef = nullptr;
    int32_t volType;
    int32_t volLevel;
    int32_t deviceFlag;
    int status;
    AudioManagerNapi* objectInfo;
    std::vector<sptr<AudioDeviceDescriptor>> deviceDescriptors;
};

namespace {
    const int ARGS_ONE = 1;
    const int ARGS_TWO = 2;
    const int ARGS_THREE = 3;
    const int PARAM0 = 0;
    const int PARAM1 = 1;
    const int PARAM2 = 2;
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioManagerNapi"};
}

AudioManagerNapi::AudioManagerNapi()
    : audioMngr_(nullptr), env_(nullptr), wrapper_(nullptr) {}

AudioManagerNapi::~AudioManagerNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

void AudioManagerNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    if (nativeObject != nullptr) {
        reinterpret_cast<AudioManagerNapi*>(nativeObject)->~AudioManagerNapi();
    }
}

static AudioSvcManager::AudioVolumeType GetNativeAudioVolumeType(int32_t volumeType)
{
    AudioSvcManager::AudioVolumeType result;

    switch (volumeType) {
        case AudioManagerNapi::MEDIA:
            result = AudioSvcManager::STREAM_MUSIC;
            break;

        case AudioManagerNapi::RINGTONE:
            result = AudioSvcManager::STREAM_RING;
            break;

        default:
            result = AudioSvcManager::STREAM_MUSIC;
            HiLog::Error(LABEL, "Unknown volume type!, %{public}d", volumeType);
            break;
    }
    return result;
}

static AudioDeviceDescriptor::DeviceFlag GetNativeDeviceFlag(int32_t deviceFlag)
{
    AudioDeviceDescriptor::DeviceFlag result;

    switch (deviceFlag) {
        case AudioManagerNapi::OUTPUT_DEVICES_FLAG:
            result = AudioDeviceDescriptor::OUTPUT_DEVICES_FLAG;
            break;

        case AudioManagerNapi::INPUT_DEVICES_FLAG:
            result = AudioDeviceDescriptor::INPUT_DEVICES_FLAG;
            break;

        case AudioManagerNapi::ALL_DEVICES_FLAG:
            result = AudioDeviceDescriptor::ALL_DEVICES_FLAG;
            break;

        default:
            result = AudioDeviceDescriptor::ALL_DEVICES_FLAG;
            HiLog::Error(LABEL, "Unknown device flag!, %{public}d", deviceFlag);
            break;
    }
    return result;
}

napi_status AudioManagerNapi::AddNamedProperty(napi_env env, napi_value object, const char *name, int32_t enumValue)
{
    napi_status status;
    napi_value enumNapiValue;

    status = napi_create_int32(env, enumValue, &enumNapiValue);
    if (status == napi_ok) {
        status = napi_set_named_property(env, object, name, enumNapiValue);
    }
    return status;
}

napi_value AudioManagerNapi::CreateAudioVolumeTypeObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    int32_t refCount = 1;

    HiLog::Debug(LABEL, "CreateAudioVolumeTypeObject is called!");
    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        status = AddNamedProperty(env, result, "MEDIA", AudioManagerNapi::MEDIA);
        if (status == napi_ok) {
            status = AddNamedProperty(env, result, "RINGTONE", AudioManagerNapi::RINGTONE);
            if (status == napi_ok) {
                status = napi_create_reference(env, result, refCount, &audioVolumeTypeRef_);
                if (status == napi_ok) {
                    return result;
                }
            }
        }
    }
    HiLog::Error(LABEL, "CreateAudioVolumeTypeObject is Failed!");
    napi_get_undefined(env, &result);
    return result;
}

napi_value AudioManagerNapi::CreateDeviceFlagObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    int32_t refCount = 1;

    HiLog::Debug(LABEL, "CreateDeviceFlagObject is called!");
    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        status = AddNamedProperty(env, result, "OUTPUT_DEVICES_FLAG", AudioManagerNapi::OUTPUT_DEVICES_FLAG);
        if (status == napi_ok) {
            status = AddNamedProperty(env, result, "INPUT_DEVICES_FLAG", AudioManagerNapi::INPUT_DEVICES_FLAG);
            if (status == napi_ok) {
                status = AddNamedProperty(env, result, "ALL_DEVICES_FLAG", AudioManagerNapi::ALL_DEVICES_FLAG);
                if (status == napi_ok) {
                    status = napi_create_reference(env, result, refCount, &deviceFlagRef_);
                    if (status == napi_ok) {
                        return result;
                    }
                }
            }
        }
    }
    HiLog::Error(LABEL, "CreateDeviceFlagObject is Failed!");
    napi_get_undefined(env, &result);
    return result;
}

napi_value AudioManagerNapi::CreateDeviceRoleObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    int32_t refCount = 1;

    HiLog::Debug(LABEL, "CreateDeviceRoleObject is called!");
    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        status = AddNamedProperty(env, result, "INPUT_DEVICE", AudioDeviceDescriptorNapi::INPUT_DEVICE);
        if (status == napi_ok) {
            status = AddNamedProperty(env, result, "OUTPUT_DEVICE", AudioDeviceDescriptorNapi::OUTPUT_DEVICE);
            if (status == napi_ok) {
                status = napi_create_reference(env, result, refCount, &deviceRoleRef_);
                if (status == napi_ok) {
                    return result;
                }
            }
        }
    }
    HiLog::Error(LABEL, "CreateDeviceRoleObject is Failed!");
    napi_get_undefined(env, &result);
    return result;
}

napi_value AudioManagerNapi::CreateDeviceTypeObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    int32_t refCount = 1;
    const char *propName = nullptr;

    HiLog::Debug(LABEL, "CreateDeviceTypeObject is called!");
    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (int i = AudioDeviceDescriptorNapi::INVALID; i <= AudioDeviceDescriptorNapi::MIC; i++) {
            switch (i) {
                case AudioDeviceDescriptorNapi::INVALID:
                    propName = "INVALID";
                    break;

                case AudioDeviceDescriptorNapi::SPEAKER:
                    propName = "SPEAKER";
                    break;

                case AudioDeviceDescriptorNapi::WIRED_HEADSET:
                    propName = "WIRED_HEADSET";
                    break;

                case AudioDeviceDescriptorNapi::BLUETOOTH_SCO:
                    propName = "BLUETOOTH_SCO";
                    break;

                case AudioDeviceDescriptorNapi::BLUETOOTH_A2DP:
                    propName = "BLUETOOTH_A2DP";
                    break;

                case AudioDeviceDescriptorNapi::MIC:
                    propName = "MIC";
                    break;

                default:
                    HiLog::Error(LABEL, "Invalid prop!");
                    continue;
            }
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                HiLog::Error(LABEL, "Failed to add named prop!");
                break;
            }
            propName = nullptr;
        }
        if (status == napi_ok) {
            status = napi_create_reference(env, result, refCount, &deviceTypeRef_);
            if (status == napi_ok) {
                return result;
            }
        }
    }
    HiLog::Error(LABEL, "CreateDeviceTypeObject is Failed!");
    napi_get_undefined(env, &result);
    return result;
}

napi_value AudioManagerNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value constructor;
    napi_value result = nullptr;
    const int32_t refCount = 1;

    HiLog::Debug(LABEL, "AudioManagerNapi::Init() is called!");
    napi_property_descriptor audio_svc_mngr_properties[] = {
        DECLARE_NAPI_FUNCTION("setVolume", SetVolume),
        DECLARE_NAPI_FUNCTION("getVolume", GetVolume),
        DECLARE_NAPI_FUNCTION("getMaxVolume", GetMaxVolume),
        DECLARE_NAPI_FUNCTION("getMinVolume", GetMinVolume),
        DECLARE_NAPI_FUNCTION("getDevices", GetDevices)
    };

    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("getAudioManager", GetAudioManager),
        DECLARE_NAPI_PROPERTY("AudioVolumeType", CreateAudioVolumeTypeObject(env)),
        DECLARE_NAPI_PROPERTY("DeviceFlag", CreateDeviceFlagObject(env)),
        DECLARE_NAPI_PROPERTY("DeviceRole", CreateDeviceRoleObject(env)),
        DECLARE_NAPI_PROPERTY("DeviceType", CreateDeviceTypeObject(env))
    };

    status = napi_define_class(env, AUDIO_MNGR_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Construct, nullptr,
        sizeof(audio_svc_mngr_properties) / sizeof(audio_svc_mngr_properties[PARAM0]),
        audio_svc_mngr_properties, &constructor);
    if (status == napi_ok) {
        status = napi_create_reference(env, constructor, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, AUDIO_MNGR_NAPI_CLASS_NAME.c_str(), constructor);
            if (status == napi_ok) {
                status = napi_define_properties(env, exports,
                                                sizeof(static_prop) / sizeof(static_prop[PARAM0]), static_prop);
                if (status == napi_ok) {
                    HiLog::Info(LABEL, "All props and functions are configured..");
                    return exports;
                }
            }
        }
    }
    HiLog::Error(LABEL, "Failure in AudioManagerNapi::Init()");
    napi_get_undefined(env, &result);
    return result;
}

// Constructor callback
napi_value AudioManagerNapi::Construct(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsThis;
    napi_value  result = nullptr;
    size_t argCount = 0;

    HiLog::Debug(LABEL, "AudioManagerNapi::Construct() is called!");
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status == napi_ok) {
        AudioManagerNapi* obj = new AudioManagerNapi();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->audioMngr_ = AudioSvcManager::GetInstance();
            status = napi_wrap(env, jsThis, reinterpret_cast<void*>(obj),
                               AudioManagerNapi::Destructor, nullptr, &(obj->wrapper_));
            if (status == napi_ok) {
                return jsThis;
            }
        }
    }
    HiLog::Error(LABEL, "Failed in AudioManagerNapi::Construct()!");
    napi_get_undefined(env, &result);
    return result;
}

napi_value AudioManagerNapi::CreateAudioManagerWrapper(napi_env env)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    HiLog::Debug(LABEL, "AudioManagerNapi::CreateAudioManagerWrapper() is called!");
    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        if (status == napi_ok) {
            return result;
        }
    }
    HiLog::Error(LABEL, "Failed in AudioManagerNapi::CreateaudioMngrWrapper!");
    napi_get_undefined(env, &result);
    return result;
}

napi_value AudioManagerNapi::GetAudioManager(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;

    HiLog::Debug(LABEL, "AudioManagerNapi::GetAudioManager() is called!");
    status = napi_get_cb_info(env, info, &argCount, nullptr, nullptr, nullptr);
    if (status != napi_ok || argCount != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return nullptr;
    }
    return AudioManagerNapi::CreateAudioManagerWrapper(env);
}

static void SetVolumeAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    AudioManagerAsyncContext* asyncContext = (AudioManagerAsyncContext*) data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[PARAM0]);
    napi_get_undefined(env, &result[PARAM1]);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[PARAM1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext;
}

napi_value AudioManagerNapi::SetVolume(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_THREE);
    NAPI_ASSERT(env, argc >= ARGS_TWO, "requires 2 parameters minimum");

    HiLog::Debug(LABEL, "AudioManagerNapi::SetVolume() is called!");
    AudioManagerAsyncContext* asyncContext = new AudioManagerAsyncContext();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->volType);
            } else if (i == PARAM1 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->volLevel);
            } else if (i == PARAM2 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                delete asyncContext;
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
            [](napi_env env, void* data) {
                AudioManagerAsyncContext* context = (AudioManagerAsyncContext*) data;
                context->objectInfo->audioMngr_->SetVolume(GetNativeAudioVolumeType(context->volType),
                                                           context->volLevel);
                context->status = 0;
            },
            SetVolumeAsyncCallbackComplete, (void*)asyncContext, &asyncContext->work);
        if (status != napi_ok) {
            delete asyncContext;
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
        }
    } else {
        delete asyncContext;
    }
    return result;
}

static void GetVolumeAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    AudioManagerAsyncContext* asyncContext = (AudioManagerAsyncContext*) data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;

    napi_get_undefined(env, &result[PARAM0]);
    napi_create_int32(env, asyncContext->volLevel, &result[PARAM1]);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[PARAM1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext;
}

napi_value AudioManagerNapi::GetVolume(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);
    NAPI_ASSERT(env, argc >= ARGS_ONE, "requires 1 parameters minimum");

    HiLog::Debug(LABEL, "AudioManagerNapi::GetVolume() is called!");
    AudioManagerAsyncContext* asyncContext = new AudioManagerAsyncContext();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->volType);
            } else if (i == PARAM1 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                delete asyncContext;
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
            [](napi_env env, void* data) {
                AudioManagerAsyncContext* context = (AudioManagerAsyncContext*) data;
                context->volLevel =
                    context->objectInfo->audioMngr_->GetVolume(GetNativeAudioVolumeType(context->volType));
                context->status = 0;
            },
            GetVolumeAsyncCallbackComplete, (void*)asyncContext, &asyncContext->work);
        if (status != napi_ok) {
            delete asyncContext;
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
        }
    } else {
        delete asyncContext;
    }
    return result;
}

static void GetMaxVolumeAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    AudioManagerAsyncContext* asyncContext = (AudioManagerAsyncContext*) data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;

    napi_get_undefined(env, &result[PARAM0]);
    napi_create_int32(env, asyncContext->volLevel, &result[PARAM1]);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[PARAM1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext;
}

napi_value AudioManagerNapi::GetMaxVolume(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);
    NAPI_ASSERT(env, argc >= ARGS_ONE, "requires 1 parameters minimum");

    HiLog::Debug(LABEL, "AudioManagerNapi::GetMaxVolume() is called!");
    AudioManagerAsyncContext* asyncContext = new AudioManagerAsyncContext();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->volType);
            } else if (i == PARAM1 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                delete asyncContext;
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
            [](napi_env env, void* data) {
                AudioManagerAsyncContext* context = (AudioManagerAsyncContext*) data;
                context->volLevel =
                    context->objectInfo->audioMngr_->GetMaxVolume(GetNativeAudioVolumeType(context->volType));
                context->status = 0;
            },
            GetMaxVolumeAsyncCallbackComplete, (void*)asyncContext, &asyncContext->work);
        if (status != napi_ok) {
            delete asyncContext;
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
        }
    } else {
        delete asyncContext;
    }
    return result;
}

static void GetMinVolumeAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    AudioManagerAsyncContext* asyncContext = (AudioManagerAsyncContext*) data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;

    napi_get_undefined(env, &result[PARAM0]);
    napi_create_int32(env, asyncContext->volLevel, &result[PARAM1]);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[PARAM1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext;
}

napi_value AudioManagerNapi::GetMinVolume(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);
    NAPI_ASSERT(env, argc >= ARGS_ONE, "requires 1 parameters minimum");

    HiLog::Debug(LABEL, "AudioManagerNapi::GetMinVolume() is called!");
    AudioManagerAsyncContext* asyncContext = new AudioManagerAsyncContext();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->volType);
            } else if (i == PARAM1 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                delete asyncContext;
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
            [](napi_env env, void* data) {
                AudioManagerAsyncContext* context = (AudioManagerAsyncContext*) data;
                context->volLevel =
                    context->objectInfo->audioMngr_->GetMinVolume(GetNativeAudioVolumeType(context->volType));
                context->status = 0;
            },
            GetMinVolumeAsyncCallbackComplete, (void*)asyncContext, &asyncContext->work);
        if (status != napi_ok) {
            delete asyncContext;
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
        }
    } else {
        delete asyncContext;
    }
    return result;
}

static void GetDevicesAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    AudioManagerAsyncContext* asyncContext = (AudioManagerAsyncContext*) data;
    napi_value result[ARGS_TWO] = {0};
    napi_value ddWrapper = nullptr;
    napi_value retVal;
    int32_t size = asyncContext->deviceDescriptors.size();
    HiLog::Debug(LABEL, "Size of Device Descriptors array, %{public}d!, deviceflag: %{public}d",
                 size, asyncContext->deviceFlag);
    napi_create_array_with_length(env, size, &result[PARAM1]);

    for (int i = 0; i < size; i += 1) {
        AudioDeviceDescriptor *curDeviceDescriptor = asyncContext->deviceDescriptors[i];
        HiLog::Debug(LABEL, "Device role, %{public}d, Device type: %{public}d",
                     curDeviceDescriptor->deviceRole_, curDeviceDescriptor->deviceType_);
        ddWrapper = AudioDeviceDescriptorNapi::CreateAudioDeviceDescriptorWrapper(env, curDeviceDescriptor);
        napi_set_element(env, result[PARAM1], i, ddWrapper);
    }

    napi_get_undefined(env, &result[PARAM0]);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[PARAM1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext;
}

napi_value AudioManagerNapi::GetDevices(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);
    NAPI_ASSERT(env, argc >= ARGS_ONE, "requires 1 parameters minimum");

    HiLog::Debug(LABEL, "AudioManagerNapi::GetDevices() is called!");
    AudioManagerAsyncContext* asyncContext = new AudioManagerAsyncContext();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->deviceFlag);
            } else if (i == PARAM1 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                delete asyncContext;
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "GetDevices", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                AudioManagerAsyncContext* context = (AudioManagerAsyncContext*) data;
                context->deviceDescriptors =
                    context->objectInfo->audioMngr_->GetDevices(GetNativeDeviceFlag(context->deviceFlag));
                context->status = 0;
            },
            GetDevicesAsyncCallbackComplete, (void*)asyncContext, &asyncContext->work);
        if (status != napi_ok) {
            delete asyncContext;
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
        }
    } else {
        delete asyncContext;
    }
    return result;
}

static napi_value Init(napi_env env, napi_value exports)
{
    AudioManagerNapi::Init(env, exports);
    AudioDeviceDescriptorNapi::Init(env, exports);
    return exports;
}

static napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "multimedia.audio",
    .nm_priv = ((void *)0),
    .reserved = {0}
};

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    HiLog::Debug(LABEL, "RegisterModule() is called!");
    napi_module_register(&g_module);
}
