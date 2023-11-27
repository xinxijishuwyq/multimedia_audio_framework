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

#include "audio_spatialization_manager_napi.h"
#include "audio_spatialization_manager_callback_napi.h"

#include "audio_common_napi.h"
#include "audio_errors.h"
#include "audio_log.h"
#include "hilog/log.h"
#include "napi_base_context.h"
#include "audio_system_manager.h"

using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace OHOS {
namespace AudioStandard {
static __thread napi_ref g_spatializationManagerConstructor = nullptr;

#define GET_PARAMS(env, info, num) \
    size_t argc = num;             \
    napi_value argv[num] = {0};    \
    napi_value thisVar = nullptr;  \
    void *data;                    \
    napi_get_cb_info(env, info, &argc, argv, &thisVar, &data)

namespace {
    const int ARGS_ONE = 1;
    const int ARGS_TWO = 2;
    const int PARAM0 = 0;
    const int PARAM1 = 1;
    const int PARAM2 = 2;
    const int PARAM3 = 3;
    const int SIZE = 100;
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioSpatializationManagerNapi"};
    const std::string SPATIALIZATION_ENABLED_CHANGE_CALLBACK_NAME = "spatializationEnabledChange";
    const std::string HEAD_TRACKING_ENABLED_CHANGE_CALLBACK_NAME = "headTrackingEnabledChange";
}

struct AudioSpatializationManagerAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef = nullptr;
    bool spatializationEnable;
    bool headTrackingEnable;
    int32_t status = SUCCESS;
    AudioSpatializationManagerNapi *objectInfo;
    AudioSpatialDeviceState spatialDeviceState;
};

AudioSpatializationManagerNapi::AudioSpatializationManagerNapi()
    : audioSpatializationMngr_(nullptr), env_(nullptr) {}

AudioSpatializationManagerNapi::~AudioSpatializationManagerNapi() = default;

void AudioSpatializationManagerNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    if (nativeObject != nullptr) {
        auto obj = static_cast<AudioSpatializationManagerNapi *>(nativeObject);
        delete obj;
        obj = nullptr;
    }
}

napi_value AudioSpatializationManagerNapi::Construct(napi_env env, napi_callback_info info)
{
    AUDIO_INFO_LOG("Construct");
    napi_status status;
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    void *data = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, &data);
    unique_ptr<AudioSpatializationManagerNapi> audioSpatializationManagerNapi =
        make_unique<AudioSpatializationManagerNapi>();
    CHECK_AND_RETURN_RET_LOG(audioSpatializationManagerNapi != nullptr, result, "No memory");

    audioSpatializationManagerNapi->audioSpatializationMngr_ = AudioSpatializationManager::GetInstance();
    audioSpatializationManagerNapi->cachedClientId_ = getpid();
    audioSpatializationManagerNapi->env_ = env;

    status = napi_wrap(env, thisVar, static_cast<void*>(audioSpatializationManagerNapi.get()),
        AudioSpatializationManagerNapi::Destructor, nullptr, nullptr);
    if (status == napi_ok) {
        audioSpatializationManagerNapi.release();
        return thisVar;
    }

    HiLog::Error(LABEL, "Failed in AudioSpatializationManagerNapi::Construct()!");
    return result;
}

napi_value AudioSpatializationManagerNapi::CreateSpatializationManagerWrapper(napi_env env)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, g_spatializationManagerConstructor, &constructor);
    if (status == napi_ok) {
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        if (status == napi_ok) {
            return result;
        }
    }
    HiLog::Error(LABEL, "Failed in AudioSpatializationManagerNapi::CreateSpatializationManagerWrapper!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value AudioSpatializationManagerNapi::Init(napi_env env, napi_value exports)
{
    AUDIO_INFO_LOG("Init");
    
    napi_status status;
    napi_value constructor;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_get_undefined(env, &result);

    napi_property_descriptor audio_spatialization_manager_properties[] = {
        DECLARE_NAPI_FUNCTION("isSpatializationEnabled", IsSpatializationEnabled),
        DECLARE_NAPI_FUNCTION("setSpatializationEnabled", SetSpatializationEnabled),
        DECLARE_NAPI_FUNCTION("isHeadTrackingEnabled", IsHeadTrackingEnabled),
        DECLARE_NAPI_FUNCTION("setHeadTrackingEnabled", SetHeadTrackingEnabled),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("off", Off),
        DECLARE_NAPI_FUNCTION("isSpatializationSupported", IsSpatializationSupported),
        DECLARE_NAPI_FUNCTION("isSpatializationSupportedForDevice", IsSpatializationSupportedForDevice),
        DECLARE_NAPI_FUNCTION("isHeadTrackingSupported", IsHeadTrackingSupported),
        DECLARE_NAPI_FUNCTION("isHeadTrackingSupportedForDevice", IsHeadTrackingSupportedForDevice),
        DECLARE_NAPI_FUNCTION("updateSpatialDeviceState", UpdateSpatialDeviceState),
    };

    status = napi_define_class(env, AUDIO_SPATIALIZATION_MANAGER_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Construct,
        nullptr,
        sizeof(audio_spatialization_manager_properties) / sizeof(audio_spatialization_manager_properties[PARAM0]),
        audio_spatialization_manager_properties, &constructor);
    if (status != napi_ok) {
        return result;
    }
    status = napi_create_reference(env, constructor, refCount, &g_spatializationManagerConstructor);
    if (status == napi_ok) {
        status = napi_set_named_property(env, exports, AUDIO_SPATIALIZATION_MANAGER_NAPI_CLASS_NAME.c_str(),
            constructor);
        if (status == napi_ok) {
            return exports;
        }
    }

    HiLog::Error(LABEL, "Failure in AudioSpatializationManagerNapi::Init()");
    return result;
}

static void CommonCallbackRoutine(napi_env env, AudioSpatializationManagerAsyncContext* &asyncContext,
    const napi_value &valueParam)
{
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;

    if (!asyncContext->status) {
        napi_get_undefined(env, &result[PARAM0]);
        result[PARAM1] = valueParam;
    } else {
        napi_value message = nullptr;
        std::string messageValue = AudioCommonNapi::getMessageByCode(asyncContext->status);
        napi_create_string_utf8(env, messageValue.c_str(), NAPI_AUTO_LENGTH, &message);

        napi_value code = nullptr;
        napi_create_string_utf8(env, (std::to_string(asyncContext->status)).c_str(), NAPI_AUTO_LENGTH, &code);

        napi_create_error(env, code, message, &result[PARAM0]);
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

static void ParseAudioDeviceDescriptor(napi_env env, napi_value root, sptr<AudioDeviceDescriptor> &selectedAudioDevice,
    bool &argTransFlag)
{
    napi_value tempValue = nullptr;
    int32_t intValue = {0};
    char buffer[SIZE];
    size_t res = 0;
    argTransFlag = true;
    bool hasDeviceRole = true;
    bool hasNetworkId = true;
    napi_has_named_property(env, root, "deviceRole", &hasDeviceRole);
    napi_has_named_property(env, root, "networkId", &hasNetworkId);

    if ((!hasDeviceRole) || (!hasNetworkId)) {
        argTransFlag = false;
        return;
    }

    if (napi_get_named_property(env, root, "deviceRole", &tempValue) == napi_ok) {
        napi_get_value_int32(env, tempValue, &intValue);
        selectedAudioDevice->deviceRole_ = static_cast<DeviceRole>(intValue);
    }

    if (napi_get_named_property(env, root, "deviceType", &tempValue) == napi_ok) {
        napi_get_value_int32(env, tempValue, &intValue);
        selectedAudioDevice->deviceType_ = static_cast<DeviceType>(intValue);
    }

    if (napi_get_named_property(env, root, "networkId", &tempValue) == napi_ok) {
        napi_get_value_string_utf8(env, tempValue, buffer, SIZE, &res);
        selectedAudioDevice->networkId_ = std::string(buffer);
    }

    if (napi_get_named_property(env, root, "displayName", &tempValue) == napi_ok) {
        napi_get_value_string_utf8(env, tempValue, buffer, SIZE, &res);
        selectedAudioDevice->displayName_ = std::string(buffer);
    }

    if (napi_get_named_property(env, root, "interruptGroupId", &tempValue) == napi_ok) {
        napi_get_value_int32(env, tempValue, &intValue);
        selectedAudioDevice->interruptGroupId_ = intValue;
    }

    if (napi_get_named_property(env, root, "volumeGroupId", &tempValue) == napi_ok) {
        napi_get_value_int32(env, tempValue, &intValue);
        selectedAudioDevice->volumeGroupId_ = intValue;
    }

    if (napi_get_named_property(env, root, "address", &tempValue) == napi_ok) {
        napi_get_value_string_utf8(env, tempValue, buffer, SIZE, &res);
        selectedAudioDevice->macAddress_ = std::string(buffer);
    }
}

bool AudioSpatializationManagerNapi::ParseSpatialDeviceState(napi_env env, napi_value root,
    AudioSpatialDeviceState *spatialDeviceState)
{
    napi_value res = nullptr;
    int32_t intValue = {0};
    if (napi_get_named_property(env, root, "address", &res) == napi_ok) {
        spatialDeviceState->address = AudioCommonNapi::GetStringArgument(env, res);
    }

    if (napi_get_named_property(env, root, "isSpatializationSupported", &res) == napi_ok) {
        napi_get_value_bool(env, res, &(spatialDeviceState->isSpatializationSupported));
    }

    if (napi_get_named_property(env, root, "isHeadTrackingSupported", &res) == napi_ok) {
        napi_get_value_bool(env, res, &(spatialDeviceState->isHeadTrackingSupported));
    }

    if (napi_get_named_property(env, root, "spatialDeviceType", &res) == napi_ok) {
        napi_get_value_int32(env, res, &intValue);
        spatialDeviceState->spatialDeviceType = static_cast<AudioSpatialDeviceType>(intValue);
    }

    return true;
}

napi_value AudioSpatializationManagerNapi::IsSpatializationEnabled(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value thisVar = nullptr;
    napi_value result = nullptr;
    size_t argCount = 0;
    void *native = nullptr;

    status = napi_get_cb_info(env, info, &argCount, nullptr, &thisVar, nullptr);
    if (status != napi_ok) {
        AUDIO_ERR_LOG("Invalid parameters!");
        return result;
    }

    status = napi_unwrap(env, thisVar, &native);
    auto *audioSpatializationManagerNapi = reinterpret_cast<AudioSpatializationManagerNapi *>(native);
    if (status != napi_ok || audioSpatializationManagerNapi == nullptr) {
        AUDIO_ERR_LOG("IsSpatializationEnabled unwrap failure!");
        return result;
    }

    bool isSpatializationEnabled = audioSpatializationManagerNapi->audioSpatializationMngr_->IsSpatializationEnabled();
    napi_get_boolean(env, isSpatializationEnabled, &result);

    return result;
}

bool GetArgvForSetSpatializationEnabled(napi_env env, size_t argc, napi_value* argv,
    unique_ptr<AudioSpatializationManagerAsyncContext> &asyncContext)
{
    const int32_t refCount = 1;

    if (argv == nullptr) {
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
        return false;
    }
    if (argc < ARGS_ONE) {
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
        return false;
    }
    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (i == PARAM0 && valueType == napi_boolean) {
            napi_get_value_bool(env, argv[i], &asyncContext->spatializationEnable);
        } else if (i == PARAM1) {
            if (valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
            }
            break;
        } else {
            AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
            return false;
        }
    }

    return true;
}

static void SetSpatializationEnabledAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioSpatializationManagerAsyncContext*>(data);
    napi_value valueParam = nullptr;

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            napi_get_undefined(env, &valueParam);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        HiLog::Error(LABEL, "ERROR: AudioSpatializationManagerAsyncContext* is Null!");
    }
}

napi_value AudioSpatializationManagerNapi::SetSpatializationEnabled(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);

    unique_ptr<AudioSpatializationManagerAsyncContext> asyncContext =
        make_unique<AudioSpatializationManagerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status != napi_ok || asyncContext->objectInfo == nullptr) {
        AUDIO_ERR_LOG("SetSpatializationEnabled unwrap failure!");
        return nullptr;
    }

    if (!GetArgvForSetSpatializationEnabled(env, argc, argv, asyncContext)) {
        return nullptr;
    }

    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    } else {
        napi_get_undefined(env, &result);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "SetSpatializationEnabled", NAPI_AUTO_LENGTH, &resource);

    status = napi_create_async_work(
        env, nullptr, resource,
        [](napi_env env, void *data) {
            auto context = static_cast<AudioSpatializationManagerAsyncContext*>(data);
            if (context->status == SUCCESS) {
                context->status = context->objectInfo->audioSpatializationMngr_->SetSpatializationEnabled(
                    context->spatializationEnable);
            }
        },
        SetSpatializationEnabledAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        result = nullptr;
    } else {
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_default));
        asyncContext.release();
    }

    return result;
}

napi_value AudioSpatializationManagerNapi::IsHeadTrackingEnabled(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value thisVar = nullptr;
    napi_value result = nullptr;
    size_t argCount = 0;
    void *native = nullptr;

    status = napi_get_cb_info(env, info, &argCount, nullptr, &thisVar, nullptr);
    if (status != napi_ok) {
        AUDIO_ERR_LOG("Invalid parameters!");
        return result;
    }

    status = napi_unwrap(env, thisVar, &native);
    auto *audioSpatializationManagerNapi = reinterpret_cast<AudioSpatializationManagerNapi *>(native);
    if (status != napi_ok || audioSpatializationManagerNapi == nullptr) {
        AUDIO_ERR_LOG("IsHeadTrackingEnabled unwrap failure!");
        return result;
    }

    bool isHeadTrackingEnabled = audioSpatializationManagerNapi->audioSpatializationMngr_->IsHeadTrackingEnabled();
    napi_get_boolean(env, isHeadTrackingEnabled, &result);

    return result;
}

bool GetArgvForSetHeadTrackingEnabled(napi_env env, size_t argc, napi_value* argv,
    unique_ptr<AudioSpatializationManagerAsyncContext> &asyncContext)
{
    const int32_t refCount = 1;

    if (argv == nullptr) {
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
        return false;
    }
    if (argc < ARGS_ONE) {
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
        return false;
    }
    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (i == PARAM0 && valueType == napi_boolean) {
            napi_get_value_bool(env, argv[i], &asyncContext->headTrackingEnable);
        } else if (i == PARAM1) {
            if (valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
            }
            break;
        } else {
            AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
            return false;
        }
    }

    return true;
}

static void SetHeadTrackingEnabledAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioSpatializationManagerAsyncContext*>(data);
    napi_value valueParam = nullptr;

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            napi_get_undefined(env, &valueParam);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        HiLog::Error(LABEL, "ERROR: AudioSpatializationManagerAsyncContext* is Null!");
    }
}

napi_value AudioSpatializationManagerNapi::SetHeadTrackingEnabled(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);

    unique_ptr<AudioSpatializationManagerAsyncContext> asyncContext =
        make_unique<AudioSpatializationManagerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status != napi_ok || asyncContext->objectInfo == nullptr) {
        AUDIO_ERR_LOG("SetHeadTrackingEnabled unwrap failure!");
        return nullptr;
    }

    if (!GetArgvForSetHeadTrackingEnabled(env, argc, argv, asyncContext)) {
        return nullptr;
    }

    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    } else {
        napi_get_undefined(env, &result);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "SetHeadTrackingEnabled", NAPI_AUTO_LENGTH, &resource);

    status = napi_create_async_work(
        env, nullptr, resource,
        [](napi_env env, void *data) {
            auto context = static_cast<AudioSpatializationManagerAsyncContext*>(data);
            if (context->status == SUCCESS) {
                context->status = context->objectInfo->audioSpatializationMngr_->SetHeadTrackingEnabled(
                    context->headTrackingEnable);
            }
        },
        SetHeadTrackingEnabledAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        result = nullptr;
    } else {
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_default));
        asyncContext.release();
    }

    return result;
}

void AudioSpatializationManagerNapi::RegisterSpatializationEnabledChangeCallback(napi_env env, napi_value* args,
    const std::string& cbName, AudioSpatializationManagerNapi *spatializationManagerNapi)
{
    if (!spatializationManagerNapi->spatializationEnabledChangeCallbackNapi_) {
        spatializationManagerNapi->spatializationEnabledChangeCallbackNapi_ =
            std::make_shared<AudioSpatializationEnabledChangeCallbackNapi>(env);
        if (!spatializationManagerNapi->spatializationEnabledChangeCallbackNapi_) {
            AUDIO_ERR_LOG("AudioSpatializationManagerNapi: Memory Allocation Failed !!");
            return;
        }

        int32_t ret = spatializationManagerNapi->audioSpatializationMngr_->RegisterSpatializationEnabledEventListener(
            spatializationManagerNapi->cachedClientId_,
            spatializationManagerNapi->spatializationEnabledChangeCallbackNapi_);
        if (ret) {
            AUDIO_ERR_LOG(
                "AudioSpatializationManagerNapi: Registering of Spatialization Enabled Change Callback Failed");
            return;
        }
    }

    std::shared_ptr<AudioSpatializationEnabledChangeCallbackNapi> cb =
        std::static_pointer_cast<AudioSpatializationEnabledChangeCallbackNapi>
        (spatializationManagerNapi->spatializationEnabledChangeCallbackNapi_);
    cb->SaveSpatializationEnabledChangeCallbackReference(args[PARAM1]);

    AUDIO_INFO_LOG("OnSpatializationEnabledChangeCallback is successful");
}

void AudioSpatializationManagerNapi::RegisterHeadTrackingEnabledChangeCallback(napi_env env, napi_value* args,
    const std::string& cbName, AudioSpatializationManagerNapi *spatializationManagerNapi)
{
    if (!spatializationManagerNapi->headTrackingEnabledChangeCallbackNapi_) {
        spatializationManagerNapi->headTrackingEnabledChangeCallbackNapi_ =
            std::make_shared<AudioHeadTrackingEnabledChangeCallbackNapi>(env);
        if (!spatializationManagerNapi->headTrackingEnabledChangeCallbackNapi_) {
            AUDIO_ERR_LOG("AudioSpatializationManagerNapi: Memory Allocation Failed !!");
            return;
        }

        int32_t ret = spatializationManagerNapi->audioSpatializationMngr_->RegisterHeadTrackingEnabledEventListener(
            spatializationManagerNapi->cachedClientId_,
            spatializationManagerNapi->headTrackingEnabledChangeCallbackNapi_);
        if (ret) {
            AUDIO_ERR_LOG(
                "AudioSpatializationManagerNapi: Registering of Head Tracking Enabled Change Callback Failed");
            return;
        }
    }

    std::shared_ptr<AudioHeadTrackingEnabledChangeCallbackNapi> cb =
        std::static_pointer_cast<AudioHeadTrackingEnabledChangeCallbackNapi>
        (spatializationManagerNapi->headTrackingEnabledChangeCallbackNapi_);
    cb->SaveHeadTrackingEnabledChangeCallbackReference(args[PARAM1]);

    AUDIO_INFO_LOG("OnHeadTrackingEnabledChangeCallback is successful");
}

void AudioSpatializationManagerNapi::RegisterCallback(napi_env env, napi_value jsThis,
    napi_value* args, const std::string& cbName)
{
    AudioSpatializationManagerNapi *spatializationManagerNapi = nullptr;
    napi_status status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&spatializationManagerNapi));
    if ((status != napi_ok) || (spatializationManagerNapi == nullptr) ||
        (spatializationManagerNapi->audioSpatializationMngr_ == nullptr)) {
        AUDIO_ERR_LOG("AudioSpatializationManagerNapi::Failed to retrieve audio spatialization manager napi instance.");
        return;
    }

    if (!cbName.compare(SPATIALIZATION_ENABLED_CHANGE_CALLBACK_NAME)) {
        RegisterSpatializationEnabledChangeCallback(env, args, cbName, spatializationManagerNapi);
    } else if (!cbName.compare(HEAD_TRACKING_ENABLED_CHANGE_CALLBACK_NAME)) {
        RegisterHeadTrackingEnabledChangeCallback(env, args, cbName, spatializationManagerNapi);
    } else {
        AUDIO_ERR_LOG("AudioSpatializationManagerNapi::No such callback supported");
        AudioCommonNapi::throwError(env, NAPI_ERR_INVALID_PARAM);
    }
}

napi_value AudioSpatializationManagerNapi::On(napi_env env, napi_callback_info info)
{
    const size_t requireArgc = PARAM2;
    size_t argc = PARAM3;

    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_value args[requireArgc + 1] = {nullptr, nullptr, nullptr};
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &jsThis, nullptr);
    if (status != napi_ok || argc < requireArgc) {
        AUDIO_ERR_LOG("On fail to napi_get_cb_info/Requires min 2 parameters");
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
    }

    napi_valuetype eventType = napi_undefined;
    if (napi_typeof(env, args[PARAM0], &eventType) != napi_ok || eventType != napi_string) {
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
        return undefinedResult;
    }
    std::string callbackName = AudioCommonNapi::GetStringArgument(env, args[0]);
    AUDIO_DEBUG_LOG("On callbackName: %{public}s", callbackName.c_str());

    napi_valuetype handler = napi_undefined;
    if (napi_typeof(env, args[PARAM1], &handler) != napi_ok || handler != napi_function) {
        AUDIO_ERR_LOG("On type mismatch for parameter 2");
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
        return undefinedResult;
    }

    RegisterCallback(env, jsThis, args, callbackName);

    return undefinedResult;
}

void AudioSpatializationManagerNapi::UnregisterSpatializationEnabledChangeCallback(napi_env env, napi_value callback,
    AudioSpatializationManagerNapi *spatializationManagerNapi)
{
    if (spatializationManagerNapi->spatializationEnabledChangeCallbackNapi_ != nullptr) {
        std::shared_ptr<AudioSpatializationEnabledChangeCallbackNapi> cb =
            std::static_pointer_cast<AudioSpatializationEnabledChangeCallbackNapi>(
            spatializationManagerNapi->spatializationEnabledChangeCallbackNapi_);
        if (callback != nullptr) {
            cb->RemoveSpatializationEnabledChangeCallbackReference(env, callback);
        }
        if (callback == nullptr || cb->GetSpatializationEnabledChangeCbListSize() == 0) {
            int32_t ret = spatializationManagerNapi->audioSpatializationMngr_->
                UnregisterSpatializationEnabledEventListener(spatializationManagerNapi->cachedClientId_);
            CHECK_AND_RETURN_LOG(ret == SUCCESS, "UnregisterSpatializationEnabledEventListener Failed");
            spatializationManagerNapi->spatializationEnabledChangeCallbackNapi_.reset();
            spatializationManagerNapi->spatializationEnabledChangeCallbackNapi_ = nullptr;
            cb->RemoveAllSpatializationEnabledChangeCallbackReference();
        }
    } else {
        AUDIO_ERR_LOG("UnregisterSpatializationEnabledChangeCb: spatializationEnabledChangeCallbackNapi_ is null");
    }
}

void AudioSpatializationManagerNapi::UnregisterHeadTrackingEnabledChangeCallback(napi_env env, napi_value callback,
    AudioSpatializationManagerNapi *spatializationManagerNapi)
{
    if (spatializationManagerNapi->headTrackingEnabledChangeCallbackNapi_ != nullptr) {
        std::shared_ptr<AudioHeadTrackingEnabledChangeCallbackNapi> cb =
            std::static_pointer_cast<AudioHeadTrackingEnabledChangeCallbackNapi>(
            spatializationManagerNapi->headTrackingEnabledChangeCallbackNapi_);
        if (callback != nullptr) {
            cb->RemoveHeadTrackingEnabledChangeCallbackReference(env, callback);
        }
        if (callback == nullptr || cb->GetHeadTrackingEnabledChangeCbListSize() == 0) {
            int32_t ret = spatializationManagerNapi->audioSpatializationMngr_->
                UnregisterHeadTrackingEnabledEventListener(spatializationManagerNapi->cachedClientId_);
            CHECK_AND_RETURN_LOG(ret == SUCCESS, "UnregisterHeadTrackingEnabledEventListener Failed");
            spatializationManagerNapi->headTrackingEnabledChangeCallbackNapi_.reset();
            spatializationManagerNapi->headTrackingEnabledChangeCallbackNapi_ = nullptr;
            cb->RemoveAllHeadTrackingEnabledChangeCallbackReference();
        }
    } else {
        AUDIO_ERR_LOG("UnregisterHeadTrackingEnabledChangeCb: headTrackingEnabledChangeCallbackNapi_ is null");
    }
}

napi_value AudioSpatializationManagerNapi::Off(napi_env env, napi_callback_info info)
{
    const size_t requireArgc = 1;
    size_t argc = PARAM2;

    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_value args[requireArgc + 1] = {nullptr, nullptr};
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &jsThis, nullptr);
    if (status != napi_ok || argc < requireArgc) {
        AUDIO_ERR_LOG("Off fail to napi_get_cb_info/Requires min 1 parameters");
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
        return undefinedResult;
    }

    napi_valuetype eventType = napi_undefined;
    if (napi_typeof(env, args[PARAM0], &eventType) != napi_ok || eventType != napi_string) {
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
        return undefinedResult;
    }

    napi_valuetype secondArgsType = napi_undefined;
    if (argc > requireArgc &&
        (napi_typeof(env, args[PARAM1], &secondArgsType) != napi_ok || secondArgsType != napi_function)) {
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
        return undefinedResult;
    }
    std::string callbackName = AudioCommonNapi::GetStringArgument(env, args[0]);

    if (argc == requireArgc) {
        args[PARAM1] = nullptr;
    }
    AUDIO_DEBUG_LOG("AudioSpatializationManagerNapi: Off callbackName: %{public}s", callbackName.c_str());

    AudioSpatializationManagerNapi *spatializationManagerNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&spatializationManagerNapi));
    NAPI_ASSERT(env, status == napi_ok && spatializationManagerNapi != nullptr, "Failed to retrieve napi instance.");
    NAPI_ASSERT(env, spatializationManagerNapi->audioSpatializationMngr_ != nullptr, "spatialization instance null.");

    if (!callbackName.compare(SPATIALIZATION_ENABLED_CHANGE_CALLBACK_NAME)) {
        UnregisterSpatializationEnabledChangeCallback(env, args[PARAM1], spatializationManagerNapi);
    } else if (!callbackName.compare(HEAD_TRACKING_ENABLED_CHANGE_CALLBACK_NAME)) {
        UnregisterHeadTrackingEnabledChangeCallback(env, args[PARAM1], spatializationManagerNapi);
    } else {
        AudioCommonNapi::throwError(env, NAPI_ERR_INVALID_PARAM);
    }
    return undefinedResult;
}

napi_value AudioSpatializationManagerNapi::IsSpatializationSupported(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value thisVar = nullptr;
    napi_value result = nullptr;
    size_t argCount = 0;
    void *native = nullptr;

    status = napi_get_cb_info(env, info, &argCount, nullptr, &thisVar, nullptr);
    if (status != napi_ok) {
        AUDIO_ERR_LOG("Invalid parameters!");
        return result;
    }

    status = napi_unwrap(env, thisVar, &native);
    auto *audioSpatializationManagerNapi = reinterpret_cast<AudioSpatializationManagerNapi *>(native);
    if (status != napi_ok || audioSpatializationManagerNapi == nullptr) {
        AUDIO_ERR_LOG("IsSpatializationSupported unwrap failure!");
        return result;
    }

    bool isSpatializationSupported =
        audioSpatializationManagerNapi->audioSpatializationMngr_->IsSpatializationSupported();
    napi_get_boolean(env, isSpatializationSupported, &result);
    return result;
}

napi_value AudioSpatializationManagerNapi::IsSpatializationSupportedForDevice(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    void *native = nullptr;
    bool argTransFlag = true;
    GET_PARAMS(env, info, ARGS_ONE);

    if (argc < ARGS_ONE) {
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }

    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, argv[PARAM0], &valueType);
    if (valueType != napi_object) {
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }

    status = napi_unwrap(env, thisVar, &native);
    auto *audioSpatializationManagerNapi = reinterpret_cast<AudioSpatializationManagerNapi *>(native);
    if (status != napi_ok || audioSpatializationManagerNapi == nullptr) {
        AUDIO_ERR_LOG("IsSpatializationSupportedForDevice unwrap failure!");
        return result;
    }
    sptr<AudioDeviceDescriptor> selectedAudioDevice = new (std::nothrow) AudioDeviceDescriptor();
    ParseAudioDeviceDescriptor(env, argv[PARAM0], selectedAudioDevice, argTransFlag);
    bool isSpatializationSupportedForDevice = audioSpatializationManagerNapi
        ->audioSpatializationMngr_->IsSpatializationSupportedForDevice(selectedAudioDevice);
    napi_get_boolean(env, isSpatializationSupportedForDevice, &result);
    return result;
}

napi_value AudioSpatializationManagerNapi::IsHeadTrackingSupported(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value thisVar = nullptr;
    napi_value result = nullptr;
    size_t argCount = 0;
    void *native = nullptr;

    status = napi_get_cb_info(env, info, &argCount, nullptr, &thisVar, nullptr);
    if (status != napi_ok) {
        AUDIO_ERR_LOG("Invalid parameters!");
        return result;
    }

    status = napi_unwrap(env, thisVar, &native);
    auto *audioSpatializationManagerNapi = reinterpret_cast<AudioSpatializationManagerNapi *>(native);
    if (status != napi_ok || audioSpatializationManagerNapi == nullptr) {
        AUDIO_ERR_LOG("IsHeadTrackingSupported unwrap failure!");
        return result;
    }

    bool isHeadTrackingSupported =
        audioSpatializationManagerNapi->audioSpatializationMngr_->IsHeadTrackingSupported();
    napi_get_boolean(env, isHeadTrackingSupported, &result);
    return result;
}

napi_value AudioSpatializationManagerNapi::IsHeadTrackingSupportedForDevice(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    void *native = nullptr;
    bool argTransFlag = true;

    GET_PARAMS(env, info, ARGS_ONE);

    if (argc < ARGS_ONE) {
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }

    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, argv[PARAM0], &valueType);
    if (valueType != napi_object) {
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }

    status = napi_unwrap(env, thisVar, &native);
    auto *audioSpatializationManagerNapi = reinterpret_cast<AudioSpatializationManagerNapi *>(native);
    if (status != napi_ok || audioSpatializationManagerNapi == nullptr) {
        AUDIO_ERR_LOG("IsHeadTrackingSupportedForDevice unwrap failure!");
        return result;
    }
    sptr<AudioDeviceDescriptor> selectedAudioDevice = new (std::nothrow) AudioDeviceDescriptor();
    ParseAudioDeviceDescriptor(env, argv[PARAM0], selectedAudioDevice, argTransFlag);
    bool isHeadTrackingSupportedForDevice = audioSpatializationManagerNapi
        ->audioSpatializationMngr_->IsHeadTrackingSupportedForDevice(selectedAudioDevice);
    napi_get_boolean(env, isHeadTrackingSupportedForDevice, &result);
    return result;
}

napi_value AudioSpatializationManagerNapi::UpdateSpatialDeviceState(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    void *native = nullptr;

    GET_PARAMS(env, info, ARGS_ONE);

    if (argc < ARGS_ONE) {
        AudioCommonNapi::throwError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }

    status = napi_unwrap(env, thisVar, &native);
    auto *audioSpatializationManagerNapi = reinterpret_cast<AudioSpatializationManagerNapi *>(native);
    if (status != napi_ok || audioSpatializationManagerNapi == nullptr) {
        AUDIO_ERR_LOG("UpdateSpatialDeviceState unwrap failure!");
        return result;
    }

    AudioSpatialDeviceState audioSpatialDeviceState;
    if (!ParseSpatialDeviceState(env, argv[PARAM0], &audioSpatialDeviceState)) {
        AudioCommonNapi::throwError(env, NAPI_ERR_INVALID_PARAM);
        return result;
    }
    audioSpatializationManagerNapi->audioSpatializationMngr_->UpdateSpatialDeviceState(audioSpatialDeviceState);
    return result;
}
} // namespace AudioStandard
} // namespace OHOS
