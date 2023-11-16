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

napi_value NapiAudioRenderer::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value constructor;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_get_undefined(env, &result);

    napi_property_descriptor audio_renderer_properties[] = {
        DECLARE_NAPI_FUNCTION("setRenderRate", SetRenderRate),
        DECLARE_NAPI_FUNCTION("getRenderRate", GetRenderRate)
    };

    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAudioRenderer", CreateAudioRenderer),
        DECLARE_NAPI_STATIC_FUNCTION("createAudioRendererSync", CreateAudioRendererSync),
    };

    status = napi_define_class(env, NAPI_AUDIO_RENDERER_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Construct, nullptr,
        sizeof(audio_renderer_properties) / sizeof(audio_renderer_properties[PARAM0]),
        audio_renderer_properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "napi_define_class fail");

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
} // namespace AudioStandard
} // namespace OHOS
