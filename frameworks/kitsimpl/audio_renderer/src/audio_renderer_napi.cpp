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

#include "audio_renderer_napi.h"
#include "audio_manager_napi.h"
#include "audio_parameters_napi.h"
#include "hilog/log.h"

#include "securec.h"

using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace OHOS {
namespace AudioStandard {
napi_ref AudioRendererNapi::sConstructor_ = nullptr;
std::unique_ptr<AudioParameters> AudioRendererNapi::sAudioParameters_ = nullptr;

namespace {
    const int ARGS_ONE = 1;

    const int PARAM0 = 0;

    const int ERROR = -1;
    const int SUCCESS = 0;

    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioRendererNapi"};
}

AudioRendererNapi::AudioRendererNapi()
    : audioRenderer_(nullptr), contentType_(CONTENT_TYPE_MUSIC), streamUsage_(STREAM_USAGE_MEDIA),
      deviceRole_(OUTPUT_DEVICE), deviceType_(DEVICE_TYPE_SPEAKER), env_(nullptr), wrapper_(nullptr) {}

AudioRendererNapi::~AudioRendererNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

void AudioRendererNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    if (nativeObject != nullptr) {
        auto obj = static_cast<AudioRendererNapi *>(nativeObject);
        delete obj;
    }
}

napi_value AudioRendererNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value constructor;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_get_undefined(env, &result);

    napi_property_descriptor audio_renderer_properties[] = {
        DECLARE_NAPI_FUNCTION("setParams", SetParams),
        DECLARE_NAPI_FUNCTION("getParams", GetParams),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("write", Write),
        DECLARE_NAPI_FUNCTION("getAudioTime", GetAudioTime),
        DECLARE_NAPI_FUNCTION("pause", Pause),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("getBufferSize", GetBufferSize)
    };

    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAudioRenderer", CreateAudioRenderer)
    };

    status = napi_define_class(env, AUDIO_RENDERER_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Construct, nullptr,
        sizeof(audio_renderer_properties) / sizeof(audio_renderer_properties[PARAM0]),
        audio_renderer_properties, &constructor);
    if (status != napi_ok) {
        return result;
    }

    status = napi_create_reference(env, constructor, refCount, &sConstructor_);
    if (status == napi_ok) {
        status = napi_set_named_property(env, exports, AUDIO_RENDERER_NAPI_CLASS_NAME.c_str(), constructor);
        if (status == napi_ok) {
            status = napi_define_properties(env, exports,
                                            sizeof(static_prop) / sizeof(static_prop[PARAM0]), static_prop);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    HiLog::Error(LABEL, "Failure in AudioRendererNapi::Init()");

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
            case AudioManagerNapi::VOICE_ASSISTANT:
                streamType = AudioStreamType::STREAM_MUSIC;
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

napi_value AudioRendererNapi::Construct(napi_env env, napi_callback_info info)
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
    HiLog::Info(LABEL, "AudioRendererNapi: Audio stream type: %{public}d", streamType);
    if (streamType != AudioStreamType::STREAM_DEFAULT) {
        unique_ptr<AudioRendererNapi> obj = make_unique<AudioRendererNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->audioRenderer_
                = AudioRenderer::Create(static_cast<AudioStreamType>(streamType));
            status = napi_wrap(env, jsThis, static_cast<void*>(obj.get()),
                               AudioRendererNapi::Destructor, nullptr, &(obj->wrapper_));
            if (status == napi_ok) {
                obj.release();
                return jsThis;
            }
        }
    }

    HiLog::Error(LABEL, "Failed in AudioRendererNapi::Construct()!");

    return result;
}

napi_value AudioRendererNapi::CreateAudioRenderer(napi_env env, napi_callback_info info)
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

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        status = napi_new_instance(env, constructor, argCount, args, &result);
        if (status == napi_ok) {
            return result;
        }
    }

    HiLog::Error(LABEL, "Create audio renderer failed");
    napi_get_undefined(env, &result);

    return result;
}

int32_t AudioRendererNapi::SetAudioParameters(napi_env env, napi_value arg)
{
    int32_t format;
    int32_t channels;
    int32_t samplingRate;
    int32_t encoding;
    int32_t contentType = static_cast<int32_t>(CONTENT_TYPE_MUSIC);
    int32_t usage = static_cast<int32_t>(STREAM_USAGE_MEDIA);
    int32_t deviceRole = static_cast<int32_t>(OUTPUT_DEVICE);
    int32_t deviceType = static_cast<int32_t>(DEVICE_TYPE_SPEAKER);

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

napi_value AudioRendererNapi::SetParams(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value jsThis = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_get_undefined(env, &result);

    status = napi_get_cb_info(env, info, &argc, argv, &jsThis, nullptr);
    if ((status != napi_ok) || (jsThis == nullptr) || (argc > ARGS_ONE)) {
        HiLog::Error(LABEL, "Set params invalid arguments");
        return result;
    }

    AudioRendererNapi *rendererNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&rendererNapi));
    if (status != napi_ok || (rendererNapi == nullptr)) {
        HiLog::Error(LABEL, "Set params failed!");
        return result;
    }

    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, argv[PARAM0], &valueType);
    if (valueType != napi_object) {
        HiLog::Error(LABEL, "invalid parameter type!");
        return result;
    }

    int32_t ret = rendererNapi->SetAudioParameters(env, argv[PARAM0]);
    if (ret == SUCCESS) {
        AudioRendererParams rendererParams;
        rendererParams.sampleFormat = sAudioParameters_->format;
        rendererParams.sampleRate =  sAudioParameters_->samplingRate;
        rendererParams.channelCount = sAudioParameters_->channels;
        rendererParams.encodingType = sAudioParameters_->encoding;
        rendererNapi->contentType_ = sAudioParameters_->contentType;
        rendererNapi->streamUsage_ = sAudioParameters_->usage;
        rendererNapi->deviceRole_ = sAudioParameters_->deviceRole;
        rendererNapi->deviceType_ = sAudioParameters_->deviceType;
        rendererNapi->audioRenderer_->SetParams(rendererParams);
        HiLog::Info(LABEL, "Set params success");
    }

    return result;
}

napi_value AudioRendererNapi::GetParams(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value jsThis = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        HiLog::Error(LABEL, "Get params invalid arguments");
        return result;
    }

    AudioRendererNapi *rendererNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&rendererNapi));
    if (status != napi_ok || (rendererNapi == nullptr)) {
        HiLog::Error(LABEL, "Renderer get params failed!");
        return result;
    }

    AudioRendererParams rendererParams;
    if (rendererNapi->audioRenderer_->GetParams(rendererParams) == 0) {
        unique_ptr<AudioParameters> audioParams = make_unique<AudioParameters>();
        audioParams->format = rendererParams.sampleFormat;
        audioParams->samplingRate = rendererParams.sampleRate;
        audioParams->channels = rendererParams.channelCount;
        audioParams->encoding = rendererParams.encodingType;
        audioParams->contentType = rendererNapi->contentType_;
        audioParams->usage = rendererNapi->streamUsage_;
        audioParams->deviceRole = rendererNapi->deviceRole_;
        audioParams->deviceType = rendererNapi->deviceType_;

        result = AudioParametersNapi::CreateAudioParametersWrapper(env, audioParams);
    }

    return result;
}

napi_value AudioRendererNapi::Start(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value jsThis = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        HiLog::Error(LABEL, "Start invalid arguments");
        return result;
    }

    AudioRendererNapi *rendererNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&rendererNapi));
    if (status != napi_ok || (rendererNapi == nullptr)) {
        HiLog::Error(LABEL, "Renderer start failed!");
        return result;
    }

    bool isStarted = rendererNapi->audioRenderer_->Start();
    napi_get_boolean(env, isStarted, &result);

    return result;
}

napi_value AudioRendererNapi::Write(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value jsThis = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_get_undefined(env, &result);

    status = napi_get_cb_info(env, info, &argc, argv, &jsThis, nullptr);
    if ((status != napi_ok) || (jsThis == nullptr) || (argc != ARGS_ONE)) {
        HiLog::Error(LABEL, "Write invalid arguments");
        return result;
    }

    AudioRendererNapi *rendererNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&rendererNapi));
    if (status != napi_ok || (rendererNapi == nullptr)) {
        HiLog::Error(LABEL, "Renderer write failed!");
        return result;
    }

    size_t bufferLen = 0;
    void *data = nullptr;

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if ((i == PARAM0) && (valueType == napi_object)) {
            status = napi_get_arraybuffer_info(env, argv[i], &data, &bufferLen);
            if ((status != napi_ok) || (data == nullptr)) {
                return result;
            }
        } else {
            HiLog::Error(LABEL, "Renderer write invalid param!");
            return result;
        }
    }

    auto buffer = std::make_unique<uint8_t[]>(bufferLen);
    if (buffer == nullptr) {
        HiLog::Error(LABEL, "Renderer write buffer allocation failed");
        return result;
    }

    if (memcpy_s(buffer.get(), bufferLen, data, bufferLen)) {
        HiLog::Info(LABEL, "Renderer mem copy failed");
        return result;
    }

    size_t bytesWritten = 0;
    size_t minBytes = 4;
    while ((bytesWritten < bufferLen) && ((bufferLen - bytesWritten) > minBytes)) {
        bytesWritten += rendererNapi->audioRenderer_->Write(buffer.get() + bytesWritten, bufferLen - bytesWritten);
        HiLog::Error(LABEL, "Bytes written: %{public}zu", bytesWritten);
        if (bytesWritten < 0) {
            break;
        }
    }

    napi_create_int32(env, bytesWritten, &result);

    return result;
}

napi_value AudioRendererNapi::GetAudioTime(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value jsThis = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        HiLog::Error(LABEL, "Get audio time invalid arguments");
        return result;
    }

    AudioRendererNapi *rendererNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&rendererNapi));
    if (status != napi_ok || (rendererNapi == nullptr)) {
        HiLog::Error(LABEL, "Renderer release failed!");
        return result;
    }

    Timestamp timestamp;
    if (rendererNapi->audioRenderer_->GetAudioTime(timestamp, Timestamp::Timestampbase::MONOTONIC)) {
        const uint64_t secToNanosecond = 1000000000;
        uint64_t time = timestamp.time.tv_nsec + timestamp.time.tv_sec * secToNanosecond;
        napi_create_int64(env, time, &result);
    }

    return result;
}

napi_value AudioRendererNapi::Pause(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value jsThis = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        HiLog::Error(LABEL, "Stop invalid arguments");
        return result;
    }

    AudioRendererNapi *rendererNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&rendererNapi));
    if (status != napi_ok || (rendererNapi == nullptr)) {
        HiLog::Error(LABEL, "Renderer pause failed!");
        return result;
    }

    bool isStopped = rendererNapi->audioRenderer_->Pause();
    napi_get_boolean(env, isStopped, &result);

    return result;
}

napi_value AudioRendererNapi::Stop(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value jsThis = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        HiLog::Error(LABEL, "Stop invalid arguments");
        return result;
    }

    AudioRendererNapi *rendererNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&rendererNapi));
    if (status != napi_ok || (rendererNapi == nullptr)) {
        HiLog::Error(LABEL, "Renderer stop failed!");
        return result;
    }

    bool isStopped = rendererNapi->audioRenderer_->Stop();
    napi_get_boolean(env, isStopped, &result);

    return result;
}

napi_value AudioRendererNapi::Release(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value jsThis = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        HiLog::Error(LABEL, "Release invalid arguments");
        return result;
    }

    AudioRendererNapi *rendererNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&rendererNapi));
    if (status != napi_ok || (rendererNapi == nullptr)) {
        HiLog::Error(LABEL, "Renderer release failed!");
        return result;
    }

    bool isReleased = rendererNapi->audioRenderer_->Release();
    napi_get_boolean(env, isReleased, &result);

    return result;
}

napi_value AudioRendererNapi::GetBufferSize(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value jsThis = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        HiLog::Error(LABEL, "Get buffer size invalid arguments");
        return result;
    }

    AudioRendererNapi *rendererNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&rendererNapi));
    if (status != napi_ok || (rendererNapi == nullptr)) {
        HiLog::Error(LABEL, "Renderer release failed!");
        return result;
    }

    size_t bufferSize;
    int32_t ret = rendererNapi->audioRenderer_->GetBufferSize(bufferSize);
    if (ret == SUCCESS) {
        HiLog::Info(LABEL, "Renderer get buffer success");
        napi_create_int32(env, bufferSize, &result);
    }

    return result;
}
}
}