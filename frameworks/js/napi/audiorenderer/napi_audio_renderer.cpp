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
#include "napi_audio_renderer.h"
#include "audio_utils.h"
#include "xpower_event_js.h"
#include "napi_param_utils.h"
#include "napi_audio_error.h"
#include "napi_audio_renderer_callback.h"

namespace OHOS {
namespace AudioStandard {
static __thread napi_ref g_rendererConstructor = nullptr;
mutex NapiAudioRenderer::createMutex_;
int32_t NapiAudioRenderer::isConstructSuccess_ = SUCCESS;
std::unique_ptr<AudioRendererOptions> NapiAudioRenderer::sRendererOptions_ = nullptr;

static napi_value ThrowErrorAndReturn(napi_env env, int32_t errCode)
{
    NapiAudioError::ThrowError(env, errCode);
    return nullptr;
}

NapiAudioRenderer::NapiAudioRenderer()
    : audioRenderer_(nullptr), contentType_(CONTENT_TYPE_MUSIC), streamUsage_(STREAM_USAGE_MEDIA), env_(nullptr) {}

void NapiAudioRenderer::Destructor(napi_env env, void *nativeObject, void *finalizeHint)
{
    if (nativeObject != nullptr) {
        auto obj = static_cast<NapiAudioRenderer *>(nativeObject);
        ObjectRefMap<NapiAudioRenderer>::DecreaseRef(obj);
    }
    AUDIO_INFO_LOG("Destructor is successful");
}

napi_status NapiAudioRenderer::InitNapiAudioRenderer(napi_env env, napi_value &constructor)
{
    napi_property_descriptor audio_renderer_properties[] = {
        DECLARE_NAPI_FUNCTION("setRenderRate", SetRenderRate),
        DECLARE_NAPI_FUNCTION("getRenderRate", GetRenderRate),
        DECLARE_NAPI_FUNCTION("getRenderRateSync", GetRenderRateSync),
        DECLARE_NAPI_FUNCTION("setRendererSamplingRate", SetRendererSamplingRate),
        DECLARE_NAPI_FUNCTION("getRendererSamplingRate", GetRendererSamplingRate),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("write", Write),
        DECLARE_NAPI_FUNCTION("getAudioTime", GetAudioTime),
        DECLARE_NAPI_FUNCTION("getAudioTimeSync", GetAudioTimeSync),
        DECLARE_NAPI_FUNCTION("drain", Drain),
        DECLARE_NAPI_FUNCTION("pause", Pause),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("getBufferSize", GetBufferSize),
        DECLARE_NAPI_FUNCTION("getBufferSizeSync", GetBufferSizeSync),
        DECLARE_NAPI_FUNCTION("getAudioStreamId", GetAudioStreamId),
        DECLARE_NAPI_FUNCTION("getAudioStreamIdSync", GetAudioStreamIdSync),
        DECLARE_NAPI_FUNCTION("setVolume", SetVolume),
    };

    napi_status status = napi_define_class(env, NAPI_AUDIO_RENDERER_CLASS_NAME.c_str(),
        NAPI_AUTO_LENGTH, Construct, nullptr,
        sizeof(audio_renderer_properties) / sizeof(audio_renderer_properties[PARAM0]),
        audio_renderer_properties, &constructor);
    return status;
}

napi_value NapiAudioRenderer::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value constructor;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_get_undefined(env, &result);

    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAudioRenderer", CreateAudioRenderer),
        DECLARE_NAPI_STATIC_FUNCTION("createAudioRendererSync", CreateAudioRendererSync),
    };

    status = InitNapiAudioRenderer(env, constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "InitNapiAudioRenderer fail");

    status = napi_create_reference(env, constructor, refCount, &g_rendererConstructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "napi_create_reference fail");
    status = napi_set_named_property(env, exports, NAPI_AUDIO_RENDERER_CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "napi_set_named_property fail");
    status = napi_define_properties(env, exports,
        sizeof(static_prop) / sizeof(static_prop[PARAM0]), static_prop);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "napi_define_properties fail");
    return exports;
}

void NapiAudioRenderer::CreateRendererFailed()
{
    AUDIO_ERR_LOG("Renderer Create failed");
    NapiAudioRenderer::isConstructSuccess_ = NAPI_ERR_SYSTEM;
    if (AudioRenderer::CheckMaxRendererInstances() == ERR_OVERFLOW) {
        NapiAudioRenderer::isConstructSuccess_ = NAPI_ERR_STREAM_LIMIT;
    }
}

unique_ptr<NapiAudioRenderer> NapiAudioRenderer::CreateAudioRendererNativeObject(napi_env env)
{
    unique_ptr<NapiAudioRenderer> rendererNapi = make_unique<NapiAudioRenderer>();
    CHECK_AND_RETURN_RET_LOG(rendererNapi != nullptr, nullptr, "No memory");

    rendererNapi->env_ = env;
    rendererNapi->contentType_ = sRendererOptions_->rendererInfo.contentType;
    rendererNapi->streamUsage_ = sRendererOptions_->rendererInfo.streamUsage;

    AudioRendererOptions rendererOptions = *sRendererOptions_;
    std::string cacheDir = "";
    rendererNapi->audioRenderer_ = AudioRenderer::Create(cacheDir, rendererOptions);

    if (rendererNapi->audioRenderer_ == nullptr) {
        CreateRendererFailed();
        rendererNapi.release();
        return nullptr;
    }

    if (rendererNapi->audioRenderer_ != nullptr && rendererNapi->callbackNapi_ == nullptr) {
        rendererNapi->callbackNapi_ = std::make_shared<NapiAudioRendererCallback>(env);
        CHECK_AND_RETURN_RET_LOG(rendererNapi->callbackNapi_ != nullptr, nullptr, "No memory");
        int32_t ret = rendererNapi->audioRenderer_->SetRendererCallback(rendererNapi->callbackNapi_);
        CHECK_AND_RETURN_RET_LOG(!ret, rendererNapi, "Construct SetRendererCallback failed");
    }
    return rendererNapi;
}

napi_value NapiAudioRenderer::Construct(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = ARGS_TWO;
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &thisVar, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to napi_get_cb_info");

    unique_ptr<NapiAudioRenderer> rendererNapi = CreateAudioRendererNativeObject(env);
    CHECK_AND_RETURN_RET_LOG(rendererNapi != nullptr, result, "failed to CreateAudioRendererNativeObject");

    status = napi_wrap(env, thisVar, static_cast<void*>(rendererNapi.get()),
        NapiAudioRenderer::Destructor, nullptr, nullptr);
    if (status != napi_ok) {
        ObjectRefMap<NapiAudioRenderer>::Erase(rendererNapi.get());
        return result;
    }
    rendererNapi.release();
    return thisVar;
}

bool NapiAudioRenderer::CheckContextStatus(std::shared_ptr<AudioRendererAsyncContext> context)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, false, "context object is nullptr.");
    if (context->native == nullptr) {
        context->SignError(NAPI_ERR_SYSTEM);
        return false;
    }
    return true;
}

bool NapiAudioRenderer::CheckAudioRendererStatus(NapiAudioRenderer *napi,
    std::shared_ptr<AudioRendererAsyncContext> context)
{
    CHECK_AND_RETURN_RET_LOG(napi != nullptr, false, "napi object is nullptr.");
    if (napi->audioRenderer_ == nullptr) {
        context->SignError(NAPI_ERR_SYSTEM);
        return false;
    }
    return true;
}

NapiAudioRenderer* NapiAudioRenderer::GetParamWithSync(const napi_env &env, napi_callback_info info,
    size_t &argc, napi_value *args)
{
    NapiAudioRenderer *napiAudioRenderer = nullptr;
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argc, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr,
        "GetParamWithSync fail to napi_get_cb_info");

    status = napi_unwrap(env, jsThis, (void **)&napiAudioRenderer);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "napi_unwrap failed");
    CHECK_AND_RETURN_RET_LOG(napiAudioRenderer != nullptr && napiAudioRenderer->audioRenderer_ != nullptr,
        napiAudioRenderer, "GetParamWithSync fail to napi_unwrap");
    return napiAudioRenderer;
}

napi_value NapiAudioRenderer::CreateAudioRendererWrapper(napi_env env, const AudioRendererOptions rendererOptions)
{
    lock_guard<mutex> lock(createMutex_);
    napi_value result = nullptr;
    napi_value constructor;

    napi_status status = napi_get_reference_value(env, g_rendererConstructor, &constructor);
    if (status != napi_ok) {
        AUDIO_ERR_LOG("Failed in CreateAudioRendererWrapper, %{public}d", status);
        goto fail;
    }
    if (sRendererOptions_ != nullptr) {
        sRendererOptions_.release();
    }
    sRendererOptions_ = make_unique<AudioRendererOptions>();
    CHECK_AND_RETURN_RET_LOG(sRendererOptions_ != nullptr, result, "sRendererOptions_ create failed");
    *sRendererOptions_ = rendererOptions;
    status = napi_new_instance(env, constructor, 0, nullptr, &result);
    if (status != napi_ok) {
        AUDIO_ERR_LOG("napi_new_instance failed, sttaus:%{public}d", status);
        goto fail;
    }
    NapiAudioRenderer::isConstructSuccess_ = SUCCESS;
    return result;

fail:
    napi_get_undefined(env, &result);
    return result;
}

napi_value NapiAudioRenderer::CreateAudioRenderer(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("CreateAudioRenderer failed : no memory");
        NapiAudioError::ThrowError(env, "CreateAudioRenderer failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    auto inputParser = [env, context](size_t argc, napi_value *argv) {
        NAPI_CHECK_ARGS_RETURN_VOID(context, argc >= ARGS_ONE, "invalid arguments", NAPI_ERR_INVALID_PARAM);
        context->status = NapiParamUtils::GetRendererOptions(env, &context->rendererOptions, argv[PARAM0]);
        NAPI_CHECK_ARGS_RETURN_VOID(context, context->status == napi_ok, "get audioRendererRate failed",
            NAPI_ERR_INVALID_PARAM);
    };
    context->GetCbInfo(env, info, inputParser);

    auto complete = [env, context](napi_value &output) {
        output = CreateAudioRendererWrapper(env, context->rendererOptions);
    };

    return NapiAsyncWork::Enqueue(env, context, "CreateAudioRenderer", nullptr, complete);
}

napi_value NapiAudioRenderer::CreateAudioRendererSync(napi_env env, napi_callback_info info)
{
    AUDIO_INFO_LOG("CreateAudioRendererSync");
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {};
    napi_status status = NapiParamUtils::GetParam(env, info, argc, argv);
    CHECK_AND_RETURN_RET_LOG((argc == ARGS_ONE) && (status == napi_ok),
        ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID), "GetParam failed");

    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, argv[PARAM0], &valueType);
    CHECK_AND_RETURN_RET_LOG(valueType == napi_object,
        ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID), "valueType invaild");

    AudioRendererOptions rendererOptions;
    CHECK_AND_RETURN_RET_LOG(NapiParamUtils::GetRendererOptions(env, &rendererOptions, argv[PARAM0]) == napi_ok,
        ThrowErrorAndReturn(env, NAPI_ERR_INVALID_PARAM), "GetRendererOptions failed");

    return NapiAudioRenderer::CreateAudioRendererWrapper(env, rendererOptions);
}

napi_value NapiAudioRenderer::SetRenderRate(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("SetRenderRate failed : no memory");
        NapiAudioError::ThrowError(env, "SetRenderRate failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    auto inputParser = [env, context](size_t argc, napi_value *argv) {
        NAPI_CHECK_ARGS_RETURN_VOID(context, argc >= ARGS_ONE, "invalid arguments", NAPI_ERR_INVALID_PARAM);
        context->status = NapiParamUtils::GetValueInt32(env, context->audioRendererRate, argv[PARAM0]);
        NAPI_CHECK_ARGS_RETURN_VOID(context, context->status == napi_ok, "get audioRendererRate failed",
            NAPI_ERR_INVALID_PARAM);
    };
    context->GetCbInfo(env, info, inputParser);

    auto executor = [context]() {
        CHECK_AND_RETURN_LOG(CheckContextStatus(context), "context object state is error.");
        auto *napiAudioRenderer = reinterpret_cast<NapiAudioRenderer*>(context->native);
        AudioRendererRate audioRenderRate = static_cast<AudioRendererRate>(context->audioRendererRate);
        CHECK_AND_RETURN_LOG(CheckAudioRendererStatus(napiAudioRenderer, context),
            "context object state is error.");
        int32_t audioClientInvalidParamsErr = -2;
        context->intValue = napiAudioRenderer->audioRenderer_->SetRenderRate(audioRenderRate);
        if (context->intValue != SUCCESS) {
            if (context->intValue == audioClientInvalidParamsErr) {
                context->SignError(NAPI_ERR_UNSUPPORTED);
            } else {
                context->SignError(NAPI_ERR_SYSTEM);
            }
        }
    };
    auto complete = [env](napi_value &output) {
        output = NapiParamUtils::GetUndefinedValue(env);
    };
    return NapiAsyncWork::Enqueue(env, context, "SetRenderRate", executor, complete);
}

napi_value NapiAudioRenderer::GetRenderRate(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("GetRenderRate failed : no memory");
        NapiAudioError::ThrowError(env, "GetRenderRate failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    context->GetCbInfo(env, info);

    auto executor = [context]() {
        CHECK_AND_RETURN_LOG(CheckContextStatus(context), "context object state is error.");
        auto *napiAudioRenderer = reinterpret_cast<NapiAudioRenderer*>(context->native);
        CHECK_AND_RETURN_LOG(CheckAudioRendererStatus(napiAudioRenderer, context),
            "context object state is error.");
        context->intValue = napiAudioRenderer->audioRenderer_->GetRenderRate();
    };
    auto complete = [env, context](napi_value &output) {
        NapiParamUtils::SetValueInt32(env, context->intValue, output);
    };
    return NapiAsyncWork::Enqueue(env, context, "GetRenderRate", executor, complete);
}

napi_value NapiAudioRenderer::GetRenderRateSync(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    size_t argc = PARAM0;
    auto *napiAudioRenderer = GetParamWithSync(env, info, argc, nullptr);
    CHECK_AND_RETURN_RET_LOG(argc == PARAM0, ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID), "argcCount invaild");

    CHECK_AND_RETURN_RET_LOG(napiAudioRenderer != nullptr, result, "napiAudioRenderer is nullptr");
    CHECK_AND_RETURN_RET_LOG(napiAudioRenderer->audioRenderer_ != nullptr, result, "audioRenderer_ is nullptr");
    AudioRendererRate rendererRate = napiAudioRenderer->audioRenderer_->GetRenderRate();
    NapiParamUtils::SetValueInt32(env, static_cast<int32_t>(rendererRate), result);
    return result;
}

napi_value NapiAudioRenderer::SetRendererSamplingRate(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("SetRendererSamplingRate failed : no memory");
        NapiAudioError::ThrowError(env, "SetRendererSamplingRate failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    auto inputParser = [env, context](size_t argc, napi_value *argv) {
        NAPI_CHECK_ARGS_RETURN_VOID(context, argc >= ARGS_ONE, "invalid arguments", NAPI_ERR_INVALID_PARAM);
        context->status = NapiParamUtils::GetValueUInt32(env, context->rendererSampleRate, argv[PARAM0]);
        NAPI_CHECK_ARGS_RETURN_VOID(context, context->status == napi_ok, "get RendererSamplingRate failed",
            NAPI_ERR_INVALID_PARAM);
    };

    context->GetCbInfo(env, info, inputParser);

    auto executor = [context]() {
        CHECK_AND_RETURN_LOG(CheckContextStatus(context), "context object state is error.");
        auto *napiAudioRenderer = reinterpret_cast<NapiAudioRenderer*>(context->native);
        CHECK_AND_RETURN_LOG(CheckAudioRendererStatus(napiAudioRenderer, context),
            "context object state is error.");
        if (context->rendererSampleRate <= 0) {
            context->SignError(NAPI_ERR_UNSUPPORTED);
            return;
        }
        context->intValue =
            napiAudioRenderer->audioRenderer_->SetRendererSamplingRate(context->rendererSampleRate);
        if (context->intValue != SUCCESS) {
            context->SignError(NAPI_ERR_SYSTEM);
        }
    };

    auto complete = [env](napi_value &output) {
        output = NapiParamUtils::GetUndefinedValue(env);
    };
    return NapiAsyncWork::Enqueue(env, context, "SetRendererSamplingRate", executor, complete);
}

napi_value NapiAudioRenderer::GetRendererSamplingRate(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("GetRendererSamplingRate failed : no memory");
        NapiAudioError::ThrowError(env, "GetRendererSamplingRate failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    context->GetCbInfo(env, info);

    auto executor = [context]() {
        CHECK_AND_RETURN_LOG(CheckContextStatus(context), "context object state is error.");
        auto *napiAudioRenderer = reinterpret_cast<NapiAudioRenderer*>(context->native);
        CHECK_AND_RETURN_LOG(CheckAudioRendererStatus(napiAudioRenderer, context),
            "context object state is error.");
        context->rendererSampleRate = napiAudioRenderer->audioRenderer_->GetRendererSamplingRate();
    };

    auto complete = [env, context](napi_value &output) {
        NapiParamUtils::SetValueUInt32(env, context->rendererSampleRate, output);
    };
    return NapiAsyncWork::Enqueue(env, context, "GetRendererSamplingRate", executor, complete);
}

napi_value NapiAudioRenderer::Start(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("Start failed : no memory");
        NapiAudioError::ThrowError(env, "Start failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    context->GetCbInfo(env, info);
    HiviewDFX::ReportXPowerJsStackSysEvent(env, "STREAM_CHANGE", "SRC=Audio");

    auto executor = [context]() {
        CHECK_AND_RETURN_LOG(CheckContextStatus(context), "context object state is error.");
        auto *napiAudioRenderer = reinterpret_cast<NapiAudioRenderer*>(context->native);
        CHECK_AND_RETURN_LOG(CheckAudioRendererStatus(napiAudioRenderer, context),
            "context object state is error.");
        context->isTrue = napiAudioRenderer->audioRenderer_->Start();
        context->status = context->isTrue ? napi_ok : napi_generic_failure;
        if (context->status != napi_ok) {
            context->SignError(NAPI_ERR_SYSTEM);
        }
    };

    auto complete = [env](napi_value &output) {
        output = NapiParamUtils::GetUndefinedValue(env);
    };
    return NapiAsyncWork::Enqueue(env, context, "Start", executor, complete);
}

napi_value NapiAudioRenderer::Write(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("Write failed : no memory");
        NapiAudioError::ThrowError(env, "Write failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    auto inputParser = [env, context](size_t argc, napi_value *argv) {
        NAPI_CHECK_ARGS_RETURN_VOID(context, argc >= ARGS_ONE, "invalid arguments", NAPI_ERR_INVALID_PARAM);
        context->status = NapiParamUtils::GetArrayBuffer(env, context->data, context->bufferLen, argv[PARAM0]);
        NAPI_CHECK_ARGS_RETURN_VOID(context, context->status == napi_ok, "get buffer failed",
            NAPI_ERR_INVALID_PARAM);
    };

    context->GetCbInfo(env, info, inputParser);

    auto executor = [context]() {
        context->status = WriteArrayBufferToNative(context);
        if (context->status != napi_ok) {
            context->SignError(NAPI_ERR_SYSTEM);
        }
    };

    auto complete = [env, context](napi_value &output) {
        NapiParamUtils::SetValueUInt32(env, context->totalBytesWritten, output);
    };
    return NapiAsyncWork::Enqueue(env, context, "Write", executor, complete);
}

napi_status NapiAudioRenderer::WriteArrayBufferToNative(std::shared_ptr<AudioRendererAsyncContext> context)
{
    CHECK_AND_RETURN_RET_LOG(CheckContextStatus(context), napi_generic_failure, "context object state is error.");
    auto *napiAudioRenderer = reinterpret_cast<NapiAudioRenderer*>(context->native);

    CHECK_AND_RETURN_RET_LOG(CheckAudioRendererStatus(napiAudioRenderer, context),
        napi_generic_failure, "context object state is error.");
    size_t bufferLen = context->bufferLen;
    context->status = napi_generic_failure;
    auto buffer = std::make_unique<uint8_t[]>(bufferLen);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, napi_generic_failure, "Renderer write buffer allocation failed");
    if (memcpy_s(buffer.get(), bufferLen, context->data, bufferLen)) {
        AUDIO_ERR_LOG("Renderer mem copy failed");
        return napi_generic_failure;
    }
    int32_t bytesWritten = 0;
    size_t totalBytesWritten = 0;
    size_t minBytes = 4;
    while ((totalBytesWritten < bufferLen) && ((bufferLen - totalBytesWritten) > minBytes)) {
        bytesWritten = napiAudioRenderer->audioRenderer_->Write(buffer.get() + totalBytesWritten,
        bufferLen - totalBytesWritten);
        CHECK_AND_BREAK_LOG(bytesWritten >= 0, "Write length < 0,break.");
        totalBytesWritten += bytesWritten;
    }
    context->status = napi_ok;
    context->totalBytesWritten = totalBytesWritten;
    return context->status;
}

napi_value NapiAudioRenderer::GetAudioTime(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("GetAudioTime failed : no memory");
        NapiAudioError::ThrowError(env, "GetAudioTime failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    context->GetCbInfo(env, info);

    auto executor = [context]() {
        CHECK_AND_RETURN_LOG(CheckContextStatus(context), "context object state is error.");
        auto *napiAudioRenderer = reinterpret_cast<NapiAudioRenderer*>(context->native);
        CHECK_AND_RETURN_LOG(CheckAudioRendererStatus(napiAudioRenderer, context),
            "context object state is error.");
        Timestamp timestamp;
        if (napiAudioRenderer->audioRenderer_->GetAudioTime(timestamp, Timestamp::Timestampbase::MONOTONIC)) {
            const uint64_t secToNanosecond = 1000000000;
            context->time = timestamp.time.tv_nsec + timestamp.time.tv_sec * secToNanosecond;
            context->status = napi_ok;
        } else {
            context->SignError(NAPI_ERR_SYSTEM);
        }
    };

    auto complete = [env, context](napi_value &output) {
        NapiParamUtils::SetValueInt64(env, context->time, output);
    };
    return NapiAsyncWork::Enqueue(env, context, "GetAudioTime", executor, complete);
}

napi_value NapiAudioRenderer::GetAudioTimeSync(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    size_t argc = PARAM0;
    auto *napiAudioRenderer = GetParamWithSync(env, info, argc, nullptr);
    CHECK_AND_RETURN_RET_LOG(argc == PARAM0, ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID), "argcCount invaild");

    CHECK_AND_RETURN_RET_LOG(napiAudioRenderer != nullptr, result, "napiAudioRenderer is nullptr");
    CHECK_AND_RETURN_RET_LOG(napiAudioRenderer->audioRenderer_ != nullptr, result, "audioRenderer_ is nullptr");
    Timestamp timestamp;
    bool ret = napiAudioRenderer->audioRenderer_->GetAudioTime(timestamp, Timestamp::Timestampbase::MONOTONIC);
    CHECK_AND_RETURN_RET_LOG(ret, result, "GetAudioTime failure!");

    const uint64_t secToNanosecond = 1000000000;
    uint64_t time = timestamp.time.tv_nsec + timestamp.time.tv_sec * secToNanosecond;

    NapiParamUtils::SetValueInt64(env, time, result);
    return result;
}

napi_value NapiAudioRenderer::Drain(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("Drain failed : no memory");
        NapiAudioError::ThrowError(env, "Drain failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    context->GetCbInfo(env, info);

    auto executor = [context]() {
        CHECK_AND_RETURN_LOG(CheckContextStatus(context), "context object state is error.");
        auto *napiAudioRenderer = reinterpret_cast<NapiAudioRenderer*>(context->native);
        CHECK_AND_RETURN_LOG(CheckAudioRendererStatus(napiAudioRenderer, context),
            "context object state is error.");
        context->isTrue = napiAudioRenderer->audioRenderer_->Drain();
        if (!context->isTrue) {
            context->SignError(NAPI_ERR_SYSTEM);
        }
    };
    auto complete = [env](napi_value &output) {
        output = NapiParamUtils::GetUndefinedValue(env);
    };
    return NapiAsyncWork::Enqueue(env, context, "Drain", executor, complete);
}

napi_value NapiAudioRenderer::Pause(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("Pause failed : no memory");
        NapiAudioError::ThrowError(env, "Pause failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    context->GetCbInfo(env, info);

    auto executor = [context]() {
        CHECK_AND_RETURN_LOG(CheckContextStatus(context), "context object state is error.");
        auto *napiAudioRenderer = reinterpret_cast<NapiAudioRenderer*>(context->native);
        CHECK_AND_RETURN_LOG(CheckAudioRendererStatus(napiAudioRenderer, context),
            "context object state is error.");
        context->isTrue = napiAudioRenderer->audioRenderer_->Pause();
        if (!context->isTrue) {
            context->SignError(NAPI_ERR_SYSTEM);
        }
    };
    auto complete = [env](napi_value &output) {
        output = NapiParamUtils::GetUndefinedValue(env);
    };
    return NapiAsyncWork::Enqueue(env, context, "Pause", executor, complete);
}

napi_value NapiAudioRenderer::Stop(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("Stop failed : no memory");
        NapiAudioError::ThrowError(env, "Stop failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    context->GetCbInfo(env, info);

    auto executor = [context]() {
        CHECK_AND_RETURN_LOG(CheckContextStatus(context), "context object state is error.");
        auto *napiAudioRenderer = reinterpret_cast<NapiAudioRenderer*>(context->native);
        CHECK_AND_RETURN_LOG(CheckAudioRendererStatus(napiAudioRenderer, context),
            "context object state is error.");
        context->isTrue = napiAudioRenderer->audioRenderer_->Stop();
        if (!context->isTrue) {
            context->SignError(NAPI_ERR_SYSTEM);
        }
    };
    auto complete = [env](napi_value &output) {
        output = NapiParamUtils::GetUndefinedValue(env);
    };
    return NapiAsyncWork::Enqueue(env, context, "Stop", executor, complete);
}

napi_value NapiAudioRenderer::Release(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("Release failed : no memory");
        NapiAudioError::ThrowError(env, "Release failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    context->GetCbInfo(env, info);

    auto executor = [context]() {
        CHECK_AND_RETURN_LOG(CheckContextStatus(context), "context object state is error.");
        auto *napiAudioRenderer = reinterpret_cast<NapiAudioRenderer*>(context->native);
        CHECK_AND_RETURN_LOG(CheckAudioRendererStatus(napiAudioRenderer, context),
            "context object state is error.");
        context->isTrue = napiAudioRenderer->audioRenderer_->Release();
        if (!context->isTrue) {
            context->SignError(NAPI_ERR_SYSTEM);
        }
    };
    auto complete = [env](napi_value &output) {
        output = NapiParamUtils::GetUndefinedValue(env);
    };
    return NapiAsyncWork::Enqueue(env, context, "Release", executor, complete);
}

napi_value NapiAudioRenderer::GetBufferSize(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("GetBufferSize failed : no memory");
        NapiAudioError::ThrowError(env, "GetBufferSize failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    context->GetCbInfo(env, info);

    auto executor = [context]() {
        CHECK_AND_RETURN_LOG(CheckContextStatus(context), "context object state is error.");
        auto *napiAudioRenderer = reinterpret_cast<NapiAudioRenderer*>(context->native);
        CHECK_AND_RETURN_LOG(CheckAudioRendererStatus(napiAudioRenderer, context),
            "context object state is error.");
        size_t bufferSize;
        context->intValue = napiAudioRenderer->audioRenderer_->GetBufferSize(bufferSize);
        if (context->intValue != SUCCESS) {
            context->SignError(NAPI_ERR_SYSTEM);
        } else {
            context->bufferSize = bufferSize;
        }
    };
    auto complete = [env, context](napi_value &output) {
        NapiParamUtils::SetValueUInt32(env, context->bufferSize, output);
    };
    return NapiAsyncWork::Enqueue(env, context, "GetBufferSize", executor, complete);
}

napi_value NapiAudioRenderer::GetBufferSizeSync(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    size_t argc = PARAM0;
    auto *napiAudioRenderer = GetParamWithSync(env, info, argc, nullptr);
    CHECK_AND_RETURN_RET_LOG(argc == PARAM0, ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID), "argcCount invaild");

    CHECK_AND_RETURN_RET_LOG(napiAudioRenderer != nullptr, result, "napiAudioRenderer is nullptr");
    CHECK_AND_RETURN_RET_LOG(napiAudioRenderer->audioRenderer_ != nullptr, result, "audioRenderer_ is nullptr");
    size_t bufferSize;
    int32_t ret = napiAudioRenderer->audioRenderer_->GetBufferSize(bufferSize);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, result, "GetBufferSize failure!");

    NapiParamUtils::SetValueUInt32(env, bufferSize, result);
    return result;
}

napi_value NapiAudioRenderer::GetAudioStreamId(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("GetAudioStreamId failed : no memory");
        NapiAudioError::ThrowError(env, "GetAudioStreamId failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    context->GetCbInfo(env, info);

    auto executor = [context]() {
        CHECK_AND_RETURN_LOG(CheckContextStatus(context), "context object state is error.");
        auto *napiAudioRenderer = reinterpret_cast<NapiAudioRenderer*>(context->native);
        CHECK_AND_RETURN_LOG(CheckAudioRendererStatus(napiAudioRenderer, context),
            "context object state is error.");
        context->intValue = napiAudioRenderer->audioRenderer_->GetAudioStreamId(context->audioStreamId);
        if (context->intValue == ERR_INVALID_INDEX) {
            context->SignError(NAPI_ERR_SYSTEM);
        } else if (context->intValue == ERR_ILLEGAL_STATE) {
            context->SignError(NAPI_ERR_ILLEGAL_STATE);
        }
    };
    auto complete = [env, context](napi_value &output) {
        NapiParamUtils::SetValueUInt32(env, context->audioStreamId, output);
    };
    return NapiAsyncWork::Enqueue(env, context, "GetAudioStreamId", executor, complete);
}

napi_value NapiAudioRenderer::GetAudioStreamIdSync(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    size_t argc = PARAM0;
    auto *napiAudioRenderer = GetParamWithSync(env, info, argc, nullptr);
    CHECK_AND_RETURN_RET_LOG(argc == PARAM0, ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID), "argcCount invaild");

    CHECK_AND_RETURN_RET_LOG(napiAudioRenderer != nullptr, result, "napiAudioRenderer is nullptr");
    CHECK_AND_RETURN_RET_LOG(napiAudioRenderer->audioRenderer_ != nullptr, result, "audioRenderer_ is nullptr");
    uint32_t audioStreamId;
    int32_t streamIdStatus = napiAudioRenderer->audioRenderer_->GetAudioStreamId(audioStreamId);
    CHECK_AND_RETURN_RET_LOG(streamIdStatus == SUCCESS, result, "GetAudioStreamId failure!");

    NapiParamUtils::SetValueUInt32(env, audioStreamId, result);
    return result;
}

napi_value NapiAudioRenderer::SetVolume(napi_env env, napi_callback_info info)
{
    auto context = std::make_shared<AudioRendererAsyncContext>();
    if (context == nullptr) {
        AUDIO_ERR_LOG("SetVolume failed : no memory");
        NapiAudioError::ThrowError(env, "SetVolume failed : no memory", NAPI_ERR_NO_MEMORY);
        return NapiParamUtils::GetUndefinedValue(env);
    }

    auto inputParser = [env, context](size_t argc, napi_value *argv) {
        NAPI_CHECK_ARGS_RETURN_VOID(context, argc >= ARGS_ONE, "invalid arguments", NAPI_ERR_INVALID_PARAM);
        context->status = NapiParamUtils::GetValueDouble(env, context->volLevel, argv[PARAM0]);
        NAPI_CHECK_ARGS_RETURN_VOID(context, context->status == napi_ok, "get RendererSamplingRate failed",
            NAPI_ERR_INVALID_PARAM);
    };

    context->GetCbInfo(env, info, inputParser);

    auto executor = [context]() {
        CHECK_AND_RETURN_LOG(CheckContextStatus(context), "context object state is error.");
        auto *napiAudioRenderer = reinterpret_cast<NapiAudioRenderer*>(context->native);
        CHECK_AND_RETURN_LOG(CheckAudioRendererStatus(napiAudioRenderer, context),
            "context object state is error.");
        if (context->volLevel < MIN_VOLUME_IN_DOUBLE || context->volLevel > MAX_VOLUME_IN_DOUBLE) {
            context->SignError(NAPI_ERR_UNSUPPORTED);
            return;
        }
        context->intValue = napiAudioRenderer->audioRenderer_->SetVolume(static_cast<float>(context->volLevel));
        if (context->intValue != SUCCESS) {
            context->SignError(NAPI_ERR_SYSTEM);
        }
    };

    auto complete = [env](napi_value &output) {
        output = NapiParamUtils::GetUndefinedValue(env);
    };
    return NapiAsyncWork::Enqueue(env, context, "SetVolume", executor, complete);
}
} // namespace AudioStandard
} // namespace OHOS
