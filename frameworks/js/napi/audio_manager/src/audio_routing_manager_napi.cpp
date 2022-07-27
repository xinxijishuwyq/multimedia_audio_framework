/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "audio_routing_manager_napi.h"

#include "audio_common_napi.h"
#include "audio_errors.h"
#include "audio_log.h"
#include "hilog/log.h"
#include "napi_base_context.h"

using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace OHOS {
namespace AudioStandard {
static __thread napi_ref g_routingManagerConstructor = nullptr;

namespace {

    const int ARGS_ONE = 1;
    const int ARGS_TWO = 2;
    const int ARGS_THREE = 3;
    const int SIZE = 100;
    const int PARAM0 = 0;
    const int PARAM1 = 1;
    const int PARAM2 = 2;

    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioRoutingManagerNapi"};

#define GET_PARAMS(env, info, num) \
    size_t argc = num;             \
    napi_value argv[num] = {0};    \
    napi_value thisVar = nullptr;  \
    void *data;                    \
    napi_get_cb_info(env, info, &argc, argv, &thisVar, &data)
}


struct AudioRoutingManagerAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef = nullptr;
    int32_t intValue;
    int32_t status;
    AudioRoutingManagerNapi *objectInfo;
    sptr<AudioRendererFilter> audioRendererFilter;
    sptr<AudioCapturerFilter> audioCapturerFilter;
    vector<sptr<AudioDeviceDescriptor>> deviceDescriptors;
};

AudioRoutingManagerNapi::AudioRoutingManagerNapi()
    : audioMngr_(nullptr) , env_(nullptr), wrapper_(nullptr){}

AudioRoutingManagerNapi::~AudioRoutingManagerNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

void AudioRoutingManagerNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    if (nativeObject != nullptr) {
        auto obj = static_cast<AudioRoutingManagerNapi *>(nativeObject);
        delete obj;
        obj = nullptr;
    }
}


napi_value AudioRoutingManagerNapi::Init(napi_env env, napi_value exports)
{
    AUDIO_INFO_LOG("AudioRoutingManagerNapi::Init");
    napi_status status;
    napi_value constructor;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_get_undefined(env, &result);

    napi_property_descriptor audio_routing_manager_properties[] = {
        DECLARE_NAPI_FUNCTION("selectOutputDevice", SelectOutputDevice),
        DECLARE_NAPI_FUNCTION("selectOutputDeviceByFilter", SelectOutputDeviceByFilter),
        DECLARE_NAPI_FUNCTION("selectInputDevice", SelectInputDevice),
        DECLARE_NAPI_FUNCTION("selectInputDeviceByFilter", SelectInputDeviceByFilter),
    };

    status = napi_define_class(env, AUDIO_ROUTING_MANAGER_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Construct, nullptr,
        sizeof(audio_routing_manager_properties) / sizeof(audio_routing_manager_properties[PARAM0]),
        audio_routing_manager_properties, &constructor);
    if (status != napi_ok) {
        return result;
    }

    status = napi_create_reference(env, constructor, refCount, &g_routingManagerConstructor);
    if (status == napi_ok) {
        status = napi_set_named_property(env, exports, AUDIO_ROUTING_MANAGER_NAPI_CLASS_NAME.c_str(), constructor);
        if (status == napi_ok) {
            return exports;
        }
    }

    HiLog::Error(LABEL, "Failure in AudioRoutingManagerNapi::Init()");
    return result;
}

napi_value AudioRoutingManagerNapi::CreateRoutingManagerWrapper(napi_env env)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, g_routingManagerConstructor, &constructor);
    if (status == napi_ok) {
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        if (status == napi_ok) {
            return result;
        }
    }
    HiLog::Error(LABEL, "Failed in AudioRoutingManagerNapi::CreateRoutingManagerWrapper!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value AudioRoutingManagerNapi::Construct(napi_env env, napi_callback_info info)
{
    AUDIO_INFO_LOG("AudioRoutingManagerNapi::Construct");
    napi_status status;
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    GET_PARAMS(env, info, ARGS_TWO);

    unique_ptr<AudioRoutingManagerNapi> audioRoutingManagerNapi = make_unique<AudioRoutingManagerNapi>();
    CHECK_AND_RETURN_RET_LOG(audioRoutingManagerNapi != nullptr, result, "No memory");

    audioRoutingManagerNapi->audioMngr_ = AudioSystemManager::GetInstance();

    audioRoutingManagerNapi->env_ = env;

    status = napi_wrap(env, thisVar, static_cast<void*>(audioRoutingManagerNapi.get()),
                       AudioRoutingManagerNapi::Destructor, nullptr, &(audioRoutingManagerNapi->wrapper_));
    if (status == napi_ok) {
        audioRoutingManagerNapi.release();
        return thisVar;
    }

    HiLog::Error(LABEL, "Failed in AudioRoutingManager::Construct()!");
    return result;
}


static void CommonCallbackRoutine(napi_env env, AudioRoutingManagerAsyncContext* &asyncContext, const napi_value &valueParam)
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


static void ParseAudioRendererInfo(napi_env env, napi_value root, AudioRendererInfo *rendererInfo)
{
    napi_value tempValue = nullptr;
    int32_t intValue = {0};

    if (napi_get_named_property(env, root, "contentType", &tempValue) == napi_ok)
    {
        napi_get_value_int32(env, tempValue, &intValue);
        rendererInfo->contentType = static_cast<ContentType>(intValue);
    }

    if (napi_get_named_property(env, root, "streamUsage", &tempValue) == napi_ok)
    {
        napi_get_value_int32(env, tempValue, &intValue);
        rendererInfo->streamUsage = static_cast<StreamUsage>(intValue);
    }

    if (napi_get_named_property(env, root, "rendererFlags", &tempValue) == napi_ok)
    {
        napi_get_value_int32(env, tempValue, &(rendererInfo->rendererFlags));
    }
}


static void ParseAudioRendererFilter(napi_env env, napi_value root, sptr<AudioRendererFilter> &audioRendererFilter)
{
    napi_value tempValue = nullptr;
    int32_t intValue = {0};

    audioRendererFilter = new(std::nothrow) AudioRendererFilter();
    if (napi_get_named_property(env, root, "uid", &tempValue) == napi_ok)
    {
        napi_get_value_int32(env, tempValue, &intValue);
        audioRendererFilter->uid = intValue;
    }

    if (napi_get_named_property(env, root, "rendererInfo", &tempValue) == napi_ok)
    {
        ParseAudioRendererInfo(env, tempValue, &(audioRendererFilter->rendererInfo));
    }

    if (napi_get_named_property(env, root, "streamId", &tempValue) == napi_ok)
    {
        napi_get_value_int32(env, tempValue, &intValue);
        audioRendererFilter->streamId = intValue;
    }
}


static void ParseAudioDeviceDescriptor(napi_env env, napi_value root,sptr<AudioDeviceDescriptor> &selectedAudioDevice)
{
    napi_value tempValue = nullptr;
    int32_t intValue = {0};
    char buffer[SIZE];
    size_t res = 0;

    if (napi_get_named_property(env, root, "deviceRole", &tempValue) == napi_ok)
    {
        napi_get_value_int32(env, tempValue, &intValue);
        selectedAudioDevice->deviceRole_ = static_cast<DeviceRole>(intValue);
    }

    if (napi_get_named_property(env, root, "deviceType", &tempValue) == napi_ok)
    {
        napi_get_value_int32(env, tempValue, &intValue);
        selectedAudioDevice->deviceType_ = static_cast<DeviceType>(intValue);
    }

    if (napi_get_named_property(env, root, "networkId", &tempValue) == napi_ok)
    {
        napi_get_value_string_utf8(env, tempValue, buffer, SIZE, &res);
        selectedAudioDevice->networkId_ = std::string(buffer);
    }

    if (napi_get_named_property(env, root, "interruptGroupId", &tempValue) == napi_ok)
    {
        napi_get_value_int32(env, tempValue, &intValue);
        selectedAudioDevice->interruptGroupId_ = intValue;
    }

    if (napi_get_named_property(env, root, "volumeGroupId", &tempValue) == napi_ok)
    {
        napi_get_value_int32(env, tempValue, &intValue);
        selectedAudioDevice->volumeGroupId_ = intValue;
    }
}


static void ParseAudioDeviceDescriptorVector(napi_env env, napi_value root, vector<sptr<AudioDeviceDescriptor>> &deviceDescriptorsVector)
{
    uint32_t arrayLen = 0;
    napi_get_array_length(env,root,&arrayLen);
    if (arrayLen == 0)
    {
        deviceDescriptorsVector = {};
        AUDIO_INFO_LOG("Error: AudioDeviceDescriptor vector is NULL!");
    }

    for (size_t i = 0; i < arrayLen; i++)
    {
        napi_value element;
        napi_get_element(env,root,i,&element);
        sptr<AudioDeviceDescriptor> selectedAudioDevice = new(std::nothrow) AudioDeviceDescriptor();
        ParseAudioDeviceDescriptor(env,element,selectedAudioDevice);
        deviceDescriptorsVector.push_back(selectedAudioDevice);
    }
}


static void SelectOutputDeviceAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioRoutingManagerAsyncContext*>(data);
    napi_value valueParam = nullptr;

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            napi_create_int32(env, asyncContext->intValue, &valueParam);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        HiLog::Error(LABEL, "ERROR: AudioRoutingManagerAsyncContext* is Null!");
    }
}

napi_value AudioRoutingManagerNapi::SelectOutputDevice(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;
    GET_PARAMS(env, info, ARGS_TWO);
    NAPI_ASSERT(env, argc >= ARGS_ONE, "requires 1 parameters minimum");

    unique_ptr<AudioRoutingManagerAsyncContext> asyncContext = make_unique<AudioRoutingManagerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_object) {
                ParseAudioDeviceDescriptorVector(env,argv[i],asyncContext->deviceDescriptors);
            }else if (i == PARAM1 && valueType == napi_function){
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
        napi_create_string_utf8(env, "SelectOutputDevice", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioRoutingManagerAsyncContext*>(data);
                context->intValue = context->objectInfo->audioMngr_->SelectOutputDevice(context->deviceDescriptors);
                context->status = SUCCESS;
            },
            SelectOutputDeviceAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
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

napi_value AudioRoutingManagerNapi::SelectOutputDeviceByFilter(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;
    GET_PARAMS(env, info, ARGS_THREE);
    NAPI_ASSERT(env, argc >= ARGS_TWO, "requires 2 parameters minimum");

    unique_ptr<AudioRoutingManagerAsyncContext> asyncContext = make_unique<AudioRoutingManagerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_object) {
                ParseAudioRendererFilter(env,argv[i],asyncContext->audioRendererFilter);
            } else if(i == PARAM1 && valueType == napi_object){
                ParseAudioDeviceDescriptorVector(env,argv[i],asyncContext->deviceDescriptors);
            } else if (i == PARAM2 && valueType == napi_function){
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
        napi_create_string_utf8(env, "SelectOutputDeviceByFilter", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioRoutingManagerAsyncContext*>(data);
                context->intValue = context->objectInfo->audioMngr_->SelectOutputDevice(
                    context->audioRendererFilter,context->deviceDescriptors);
                context->status = SUCCESS;
            },
            SelectOutputDeviceAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
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


static void ParseAudioCapturerFilter(napi_env env, napi_value root, sptr<AudioCapturerFilter> &audioCapturerFilter)
{
    napi_value tempValue = nullptr;
    int32_t intValue = {0};

    audioCapturerFilter = new(std::nothrow) AudioCapturerFilter();
    if (napi_get_named_property(env, root, "uid", &tempValue) == napi_ok)
    {
        napi_get_value_int32(env, tempValue, &intValue);
        audioCapturerFilter->uid = intValue;
    }
}

static void SelectInputDeviceAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioRoutingManagerAsyncContext*>(data);
    napi_value valueParam = nullptr;

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            napi_create_int32(env, asyncContext->intValue, &valueParam);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        HiLog::Error(LABEL, "ERROR: AudioRoutingManagerAsyncContext* is Null!");
    }
}

napi_value AudioRoutingManagerNapi::SelectInputDevice(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;
    GET_PARAMS(env, info, ARGS_TWO);
    NAPI_ASSERT(env, argc >= ARGS_ONE, "requires 1 parameters minimum");

    unique_ptr<AudioRoutingManagerAsyncContext> asyncContext = make_unique<AudioRoutingManagerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == PARAM0 && valueType == napi_object) {
               ParseAudioDeviceDescriptorVector(env,argv[i],asyncContext->deviceDescriptors);
            }else if (i == PARAM1 && valueType == napi_function){
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
        napi_create_string_utf8(env, "SelectInputDevice", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioRoutingManagerAsyncContext*>(data);
                context->intValue = context->objectInfo->audioMngr_->SelectInputDevice(context->deviceDescriptors);
                context->status = SUCCESS;
            },
            SelectInputDeviceAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
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


napi_value AudioRoutingManagerNapi::SelectInputDeviceByFilter(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;
    GET_PARAMS(env, info, ARGS_THREE);
    NAPI_ASSERT(env, argc >= ARGS_TWO, "requires 2 parameters minimum");

    unique_ptr<AudioRoutingManagerAsyncContext> asyncContext = make_unique<AudioRoutingManagerAsyncContext>();

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == PARAM0 && valueType == napi_object) {
                ParseAudioCapturerFilter(env,argv[i],asyncContext->audioCapturerFilter);
            } else if(i == PARAM1 && valueType == napi_object){
                ParseAudioDeviceDescriptorVector(env,argv[i],asyncContext->deviceDescriptors);
            } else if (i == PARAM2 && valueType == napi_function){
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
        napi_create_string_utf8(env, "SelectInputDeviceByFilter", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioRoutingManagerAsyncContext*>(data);
                context->intValue = context->objectInfo->audioMngr_->SelectInputDevice(
                    context->audioCapturerFilter,context->deviceDescriptors);
                context->status = SUCCESS;
            },
            SelectInputDeviceAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
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
