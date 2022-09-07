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

#include "audio_stream_mgr_napi.h"

#include "audio_common_napi.h"
#include "audio_errors.h"
#include "audio_log.h"
#include "hilog/log.h"
#include "napi_base_context.h"
#include "audio_capturer_state_callback_napi.h"
#include "audio_renderer_state_callback_napi.h"

using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace OHOS {
namespace AudioStandard {
static __thread napi_ref g_streamMgrConstructor = nullptr;

namespace {
    const std::string RENDERERCHANGE_CALLBACK_NAME = "audioRendererChange";
    const std::string CAPTURERCHANGE_CALLBACK_NAME = "audioCapturerChange";

    const int ARGS_ONE = 1;
    const int ARGS_TWO = 2;

    const int PARAM0 = 0;
    const int PARAM1 = 1;
    const int PARAM2 = 2;
    const int PARAM3 = 3;

    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioStreamManagerNapi"};

#define GET_PARAMS(env, info, num) \
    size_t argc = num;             \
    napi_value argv[num] = {0};    \
    napi_value thisVar = nullptr;  \
    void *data;                    \
    napi_get_cb_info(env, info, &argc, argv, &thisVar, &data)
}

struct AudioStreamMgrAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef = nullptr;
    int32_t status;
    AudioStreamMgrNapi *objectInfo;
    vector<unique_ptr<AudioRendererChangeInfo>> audioRendererChangeInfos;
    vector<unique_ptr<AudioCapturerChangeInfo>> audioCapturerChangeInfos;
};

AudioStreamMgrNapi::AudioStreamMgrNapi()
    : env_(nullptr), wrapper_(nullptr), audioStreamMngr_(nullptr) {}

AudioStreamMgrNapi::~AudioStreamMgrNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

void AudioStreamMgrNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    if (nativeObject != nullptr) {
        auto obj = static_cast<AudioStreamMgrNapi *>(nativeObject);
        delete obj;
        obj = nullptr;
    }
}

static void SetValueInt32(const napi_env& env, const std::string& fieldStr, const int intValue, napi_value &result)
{
    napi_value value = nullptr;
    napi_create_int32(env, intValue, &value);
    napi_set_named_property(env, result, fieldStr.c_str(), value);
}

static void SetValueString(const napi_env &env, const std::string &fieldStr, const std::string stringValue,
    napi_value &result)
{
    napi_value value = nullptr;
    napi_create_string_utf8(env, stringValue.c_str(), NAPI_AUTO_LENGTH, &value);
    napi_set_named_property(env, result, fieldStr.c_str(), value);
}

static void SetDeviceDescriptors(const napi_env& env, napi_value &jsChangeInfoObj, const DeviceInfo &deviceInfo)
{
    napi_value jsDeviceDescriptorsObj = nullptr;
    napi_value valueParam = nullptr;
    napi_create_array_with_length(env, 1, &jsDeviceDescriptorsObj);

    (void)napi_create_object(env, &valueParam);
    SetValueInt32(env, "deviceRole", static_cast<int32_t>(deviceInfo.deviceRole), valueParam);
    SetValueInt32(env, "deviceType", static_cast<int32_t>(deviceInfo.deviceType), valueParam);
    SetValueInt32(env, "id", static_cast<int32_t>(deviceInfo.deviceId), valueParam);
    SetValueString(env, "name", deviceInfo.deviceName, valueParam);
    SetValueString(env, "address", deviceInfo.macAddress, valueParam);
    SetValueString(env, "networkId", deviceInfo.networkId, valueParam);

    napi_value value = nullptr;
    napi_value sampleRates;
    napi_create_array_with_length(env, 1, &sampleRates);
    napi_create_int32(env, deviceInfo.audioStreamInfo.samplingRate, &value);
    napi_set_element(env, sampleRates, 0, value);
    napi_set_named_property(env, valueParam, "sampleRates", sampleRates);

    napi_value channelCounts;
    napi_create_array_with_length(env, 1, &channelCounts);
    napi_create_int32(env, deviceInfo.audioStreamInfo.channels, &value);
    napi_set_element(env, channelCounts, 0, value);
    napi_set_named_property(env, valueParam, "channelCounts", channelCounts);

    napi_value channelMasks;
    napi_create_array_with_length(env, 1, &channelMasks);
    napi_create_int32(env, deviceInfo.channelMasks, &value);
    napi_set_element(env, channelMasks, 0, value);
    napi_set_named_property(env, valueParam, "channelMasks", channelMasks);

    napi_set_element(env, jsDeviceDescriptorsObj, 0, valueParam);
    napi_set_named_property(env, jsChangeInfoObj, "deviceDescriptors", jsDeviceDescriptorsObj);
}

static void GetCurrentRendererChangeInfosCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioStreamMgrAsyncContext*>(data);
    napi_value result[ARGS_TWO] = {0};
    napi_value jsChangeInfoObj = nullptr;
    napi_value jsRenInfoObj = nullptr;
    napi_value retVal;

    size_t size = asyncContext->audioRendererChangeInfos.size();
    int32_t position = 0;

    napi_create_array_with_length(env, size, &result[PARAM1]);
    for (const unique_ptr<AudioRendererChangeInfo> &changeInfo: asyncContext->audioRendererChangeInfos) {
        if (!changeInfo) {
            AUDIO_ERR_LOG("AudioManagerNapi:AudioRendererChangeInfo Null, something wrong!!");
            continue;
        }

        napi_create_object(env, &jsChangeInfoObj);
        SetValueInt32(env, "streamId", changeInfo->sessionId, jsChangeInfoObj);
        SetValueInt32(env, "rendererState", static_cast<int32_t>(changeInfo->rendererState), jsChangeInfoObj);
        SetValueInt32(env, "clientUid", changeInfo->clientUID, jsChangeInfoObj);

        napi_create_object(env, &jsRenInfoObj);
        SetValueInt32(env, "content", static_cast<int32_t>(changeInfo->rendererInfo.contentType), jsRenInfoObj);
        SetValueInt32(env, "usage", static_cast<int32_t>(changeInfo->rendererInfo.streamUsage), jsRenInfoObj);
        SetValueInt32(env, "rendererFlags", changeInfo->rendererInfo.rendererFlags, jsRenInfoObj);
        napi_set_named_property(env, jsChangeInfoObj, "rendererInfo", jsRenInfoObj);
        SetDeviceDescriptors(env, jsChangeInfoObj, changeInfo->outputDeviceInfo);

        napi_set_element(env, result[PARAM1], position, jsChangeInfoObj);
        position++;
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

static void GetCurrentCapturerChangeInfosCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioStreamMgrAsyncContext*>(data);
    napi_value result[ARGS_TWO] = {0};
    napi_value jsChangeInfoObj = nullptr;
    napi_value jsCapInfoObj = nullptr;
    napi_value retVal;

    size_t size = asyncContext->audioCapturerChangeInfos.size();
    int32_t position = 0;

    napi_create_array_with_length(env, size, &result[PARAM1]);
    for (const unique_ptr<AudioCapturerChangeInfo> &changeInfo: asyncContext->audioCapturerChangeInfos) {
        if (!changeInfo) {
            AUDIO_ERR_LOG("AudioManagerNapi:AudioCapturerChangeInfo Null, something wrong!!");
            continue;
        }

        napi_create_object(env, &jsChangeInfoObj);
        SetValueInt32(env, "streamId", changeInfo->sessionId, jsChangeInfoObj);
        SetValueInt32(env, "capturerState", static_cast<int32_t>(changeInfo->capturerState), jsChangeInfoObj);
        SetValueInt32(env, "clientUid", changeInfo->clientUID, jsChangeInfoObj);

        napi_create_object(env, &jsCapInfoObj);
        SetValueInt32(env, "source", static_cast<int32_t>(changeInfo->capturerInfo.sourceType), jsCapInfoObj);
        SetValueInt32(env, "capturerFlags", changeInfo->capturerInfo.capturerFlags, jsCapInfoObj);
        napi_set_named_property(env, jsChangeInfoObj, "capturerInfo", jsCapInfoObj);
        SetDeviceDescriptors(env, jsChangeInfoObj, changeInfo->inputDeviceInfo);

        napi_set_element(env, result[PARAM1], position, jsChangeInfoObj);
        position++;
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

napi_value AudioStreamMgrNapi::Init(napi_env env, napi_value exports)
{
    AUDIO_INFO_LOG("AudioStreamMgrNapi::Init");
    napi_status status;
    napi_value constructor;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_get_undefined(env, &result);

    napi_property_descriptor audio_stream_mgr_properties[] = {
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("off", Off),
        DECLARE_NAPI_FUNCTION("getCurrentAudioRendererInfoArray", GetCurrentAudioRendererInfos),
        DECLARE_NAPI_FUNCTION("getCurrentAudioCapturerInfoArray", GetCurrentAudioCapturerInfos)
    };

    status = napi_define_class(env, AUDIO_STREAM_MGR_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Construct, nullptr,
        sizeof(audio_stream_mgr_properties) / sizeof(audio_stream_mgr_properties[PARAM0]),
        audio_stream_mgr_properties, &constructor);
    if (status != napi_ok) {
        return result;
    }

    status = napi_create_reference(env, constructor, refCount, &g_streamMgrConstructor);
    if (status == napi_ok) {
        status = napi_set_named_property(env, exports, AUDIO_STREAM_MGR_NAPI_CLASS_NAME.c_str(), constructor);
        if (status == napi_ok) {
            return exports;
        }
    }

    HiLog::Error(LABEL, "Failure in AudioStreamMgrNapi::Init()");
    return result;
}

napi_value AudioStreamMgrNapi::CreateStreamManagerWrapper(napi_env env)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, g_streamMgrConstructor, &constructor);
    if (status == napi_ok) {
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        if (status == napi_ok) {
            return result;
        }
    }
    HiLog::Error(LABEL, "Failed in AudioStreamManagerNapi::CreateStreamMngrWrapper!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value AudioStreamMgrNapi::Construct(napi_env env, napi_callback_info info)
{
    AUDIO_INFO_LOG("AudioStreamMgrNapi::Construct");
    napi_status status;
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    GET_PARAMS(env, info, ARGS_TWO);

    unique_ptr<AudioStreamMgrNapi> streamMgrNapi = make_unique<AudioStreamMgrNapi>();
    CHECK_AND_RETURN_RET_LOG(streamMgrNapi != nullptr, result, "No memory");

    streamMgrNapi->env_ = env;
    streamMgrNapi->audioStreamMngr_ = AudioStreamManager::GetInstance();
    streamMgrNapi->cachedClientId_ = getpid();

    status = napi_wrap(env, thisVar, static_cast<void*>(streamMgrNapi.get()),
                       AudioStreamMgrNapi::Destructor, nullptr, &(streamMgrNapi->wrapper_));
    if (status == napi_ok) {
        streamMgrNapi.release();
        return thisVar;
    }

    HiLog::Error(LABEL, "Failed in AudioStreamManager::Construct()!");
    return result;
}

void AudioStreamMgrNapi::RegisterRendererStateChangeCallback(napi_env env, napi_value* args,
    const std::string& cbName, AudioStreamMgrNapi *streamMgrNapi)
{
    if (!streamMgrNapi->rendererStateChangeCallbackNapi_) {
        streamMgrNapi->rendererStateChangeCallbackNapi_ = std::make_shared<AudioRendererStateCallbackNapi>(env);
        if (!streamMgrNapi->rendererStateChangeCallbackNapi_) {
            AUDIO_ERR_LOG("AudioStreamMgrNapi: Memory Allocation Failed !!");
            return;
        }

        int32_t ret =
            streamMgrNapi->audioStreamMngr_->RegisterAudioRendererEventListener(streamMgrNapi->cachedClientId_,
            streamMgrNapi->rendererStateChangeCallbackNapi_);
        if (ret) {
            AUDIO_ERR_LOG("AudioStreamMgrNapi: Registering of Renderer State Change Callback Failed");
            return;
        }
    }

    std::shared_ptr<AudioRendererStateCallbackNapi> cb =
    std::static_pointer_cast<AudioRendererStateCallbackNapi>(streamMgrNapi->rendererStateChangeCallbackNapi_);
    cb->SaveCallbackReference(args[PARAM1]);

    AUDIO_INFO_LOG("AudioManagerNapi::OnRendererStateChangeCallback is successful");
}

void AudioStreamMgrNapi::RegisterCapturerStateChangeCallback(napi_env env, napi_value* args,
    const std::string& cbName, AudioStreamMgrNapi *streamMgrNapi)
{
    if (!streamMgrNapi->capturerStateChangeCallbackNapi_) {
        streamMgrNapi->capturerStateChangeCallbackNapi_ = std::make_shared<AudioCapturerStateCallbackNapi>(env);
        if (!streamMgrNapi->capturerStateChangeCallbackNapi_) {
            AUDIO_ERR_LOG("AudioStreamMgrNapi: Memory Allocation Failed !!");
            return;
        }

        int32_t ret =
            streamMgrNapi->audioStreamMngr_->RegisterAudioCapturerEventListener(streamMgrNapi->cachedClientId_,
            streamMgrNapi->capturerStateChangeCallbackNapi_);
        if (ret) {
            AUDIO_ERR_LOG("AudioStreamMgrNapi: Registering of Capturer State Change Callback Failed");
            return;
        }
    }

    std::shared_ptr<AudioCapturerStateCallbackNapi> cb =
    std::static_pointer_cast<AudioCapturerStateCallbackNapi>(streamMgrNapi->capturerStateChangeCallbackNapi_);
    cb->SaveCallbackReference(args[PARAM1]);

    AUDIO_INFO_LOG("AudioManagerNapi::OnCapturerStateChangeCallback is successful");
}

void AudioStreamMgrNapi::RegisterCallback(napi_env env, napi_value jsThis,
    napi_value* args, const std::string& cbName)
{
    AudioStreamMgrNapi *streamMgrNapi = nullptr;
    napi_status status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&streamMgrNapi));
    if ((status != napi_ok) || (streamMgrNapi == nullptr) || (streamMgrNapi->audioStreamMngr_ == nullptr)) {
        AUDIO_ERR_LOG("AudioStreamMgrNapi::Failed to retrieve stream mgr napi instance.");
        return;
    }

    if (!cbName.compare(RENDERERCHANGE_CALLBACK_NAME)) {
        RegisterRendererStateChangeCallback(env, args, cbName, streamMgrNapi);
    } else if (!cbName.compare(CAPTURERCHANGE_CALLBACK_NAME)) {
        RegisterCapturerStateChangeCallback(env, args, cbName, streamMgrNapi);
    } else {
        AUDIO_ERR_LOG("AudioStreamMgrNapi::No such callback supported");
    }
}

napi_value AudioStreamMgrNapi::On(napi_env env, napi_callback_info info)
{
    const size_t requireArgc = PARAM2;
    size_t argc = PARAM3;

    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_value args[requireArgc + 1] = {nullptr, nullptr, nullptr};
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &jsThis, nullptr);
    NAPI_ASSERT(env, status == napi_ok && argc == requireArgc, "AudioStreamMgrNapi: On: requires 2 parameters");

    napi_valuetype eventType = napi_undefined;
    napi_typeof(env, args[0], &eventType);
    NAPI_ASSERT(env, eventType == napi_string, "AudioStreamMgrNapi:On: type mismatch for event name, parameter 1");

    std::string callbackName = AudioCommonNapi::GetStringArgument(env, args[0]);
    AUDIO_DEBUG_LOG("AudioStreamMgrNapi: On callbackName: %{public}s", callbackName.c_str());

    napi_valuetype handler = napi_undefined;
 
    napi_typeof(env, args[1], &handler);
    NAPI_ASSERT(env, handler == napi_function, "type mismatch for parameter 2");
  
    RegisterCallback(env, jsThis, args, callbackName);

    return undefinedResult;
}

void AudioStreamMgrNapi::UnregisterCallback(napi_env env, napi_value jsThis, const std::string& cbName)
{
    AUDIO_INFO_LOG("AudioStreamMgrNapi::UnregisterCallback");
    AudioStreamMgrNapi *streamMgrNapi = nullptr;
    napi_status status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&streamMgrNapi));
    if ((status != napi_ok) || (streamMgrNapi == nullptr) || (streamMgrNapi->audioStreamMngr_ == nullptr)) {
        AUDIO_ERR_LOG("AudioStreamMgrNapi::Failed to retrieve stream mgr napi instance.");
        return;
    }

    if (!cbName.compare(RENDERERCHANGE_CALLBACK_NAME)) {
        int32_t ret = streamMgrNapi->audioStreamMngr_->
            UnregisterAudioRendererEventListener(streamMgrNapi->cachedClientId_);
        if (ret) {
            AUDIO_ERR_LOG("AudioStreamMgrNapi:UnRegistering of Renderer State Change Callback Failed");
            return;
        }
        if (streamMgrNapi->rendererStateChangeCallbackNapi_ != nullptr) {
            streamMgrNapi->rendererStateChangeCallbackNapi_.reset();
            streamMgrNapi->rendererStateChangeCallbackNapi_ = nullptr;
        }
        AUDIO_INFO_LOG("AudioStreamMgrNapi:UnRegistering of renderer State Change Callback successful");
    } else if (!cbName.compare(CAPTURERCHANGE_CALLBACK_NAME)) {
        int32_t ret = streamMgrNapi->audioStreamMngr_->
            UnregisterAudioCapturerEventListener(streamMgrNapi->cachedClientId_);
        if (ret) {
            AUDIO_ERR_LOG("AudioStreamMgrNapi:UnRegistering of capturer State Change Callback Failed");
            return;
        }
        if (streamMgrNapi->capturerStateChangeCallbackNapi_ != nullptr) {
            streamMgrNapi->capturerStateChangeCallbackNapi_.reset();
            streamMgrNapi->capturerStateChangeCallbackNapi_ = nullptr;
        }
        AUDIO_INFO_LOG("AudioStreamMgrNapi:UnRegistering of capturer State Change Callback successful");
    } else {
        AUDIO_ERR_LOG("AudioStreamMgrNapi::No such callback supported");
    }
}

napi_value AudioStreamMgrNapi::Off(napi_env env, napi_callback_info info)
{
    const size_t requireArgc = 1;
    size_t argc = 1;

    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_value args[requireArgc] = {nullptr};
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &jsThis, nullptr);
    NAPI_ASSERT(env, status == napi_ok && argc >= requireArgc, "AudioStreamMgrNapi: Off: requires min 1 parameters");

    napi_valuetype eventType = napi_undefined;
    napi_typeof(env, args[0], &eventType);
    NAPI_ASSERT(env, eventType == napi_string, "AudioStreamMgrNapi:Off: type mismatch for event name, parameter 1");

    std::string callbackName = AudioCommonNapi::GetStringArgument(env, args[0]);
    AUDIO_DEBUG_LOG("AudioStreamMgrNapi: Off callbackName: %{public}s", callbackName.c_str());

    UnregisterCallback(env, jsThis, callbackName);
    return undefinedResult;
}

napi_value AudioStreamMgrNapi::GetCurrentAudioRendererInfos(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_ONE);

    unique_ptr<AudioStreamMgrAsyncContext> asyncContext = make_unique<AudioStreamMgrAsyncContext>();
    if (!asyncContext) {
        AUDIO_ERR_LOG("AudioManagerNapi:Audio manager async context failed");
        return result;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if ((status == napi_ok && asyncContext->objectInfo != nullptr)
        && (asyncContext->objectInfo->audioStreamMngr_ != nullptr)) {
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
        napi_create_string_utf8(env, "getCurrentAudioRendererInfoArray", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioStreamMgrAsyncContext *>(data);
                context->status = context->objectInfo->audioStreamMngr_->
                    GetCurrentRendererChangeInfos(context->audioRendererChangeInfos);
            },
            GetCurrentRendererChangeInfosCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
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

napi_value AudioStreamMgrNapi::GetCurrentAudioCapturerInfos(napi_env env, napi_callback_info info)
{
    napi_status status;
    const int32_t refCount = 1;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_ONE);

    unique_ptr<AudioStreamMgrAsyncContext> asyncContext = make_unique<AudioStreamMgrAsyncContext>();
    if (!asyncContext) {
        AUDIO_ERR_LOG("AudioStreamMgrNapi:async context memory alloc failed");
        return result;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if ((status == napi_ok && asyncContext->objectInfo != nullptr)
        && (asyncContext->objectInfo->audioStreamMngr_ != nullptr)) {
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
        napi_create_string_utf8(env, "getCurrentAudioCapturerInfoArray", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void *data) {
                auto context = static_cast<AudioStreamMgrAsyncContext*>(data);
                context->objectInfo->audioStreamMngr_->GetCurrentCapturerChangeInfos(context->audioCapturerChangeInfos);
                context->status = 0;
            },
            GetCurrentCapturerChangeInfosCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
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
