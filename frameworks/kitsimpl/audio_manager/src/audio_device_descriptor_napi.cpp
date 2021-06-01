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

#include "audio_device_descriptor_napi.h"
#include "audio_svc_manager.h"
#include "hilog/log.h"

using namespace OHOS;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

napi_ref AudioDeviceDescriptorNapi::sConstructor_ = nullptr;
sptr<AudioDeviceDescriptor> AudioDeviceDescriptorNapi::sAudioDescriptor_ = nullptr;

namespace {
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioDeviceDescriptorNapi"};
}

AudioDeviceDescriptorNapi::AudioDeviceDescriptorNapi()
    : env_(nullptr), wrapper_(nullptr) {
}

AudioDeviceDescriptorNapi::~AudioDeviceDescriptorNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    audioDescriptor_ = nullptr;
}

void AudioDeviceDescriptorNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    HiLog::Debug(LABEL, "Destructor() is called!");
    if (nativeObject != nullptr) {
        reinterpret_cast<AudioDeviceDescriptorNapi*>(nativeObject)->~AudioDeviceDescriptorNapi();
    }
}

static AudioDeviceDescriptorNapi::DeviceType GetJSDeviceType(AudioDeviceDescriptor::DeviceType deviceType)
{
    AudioDeviceDescriptorNapi::DeviceType result;

    HiLog::Debug(LABEL, "GetJSDeviceType() is called!, %{public}d", deviceType);
    switch (deviceType) {
        case AudioDeviceDescriptor::SPEAKER:
            result = AudioDeviceDescriptorNapi::SPEAKER;
            break;

        case AudioDeviceDescriptor::WIRED_HEADSET:
            result = AudioDeviceDescriptorNapi::WIRED_HEADSET;
            break;

        case AudioDeviceDescriptor::BLUETOOTH_SCO:
            result = AudioDeviceDescriptorNapi::BLUETOOTH_SCO;
            break;

        case AudioDeviceDescriptor::BLUETOOTH_A2DP:
            result = AudioDeviceDescriptorNapi::BLUETOOTH_A2DP;
            break;

        case AudioDeviceDescriptor::MIC:
            result = AudioDeviceDescriptorNapi::MIC;
            break;

        default:
            result = AudioDeviceDescriptorNapi::INVALID;
            HiLog::Error(LABEL, "Unknown device type!");
            break;
    }
    return result;
}

static AudioDeviceDescriptorNapi::DeviceRole GetJSDeviceRole(AudioDeviceDescriptor::DeviceRole deviceRole)
{
    AudioDeviceDescriptorNapi::DeviceRole result;

    HiLog::Debug(LABEL, "GetJSDeviceRole() is called!, %{public}d", deviceRole);
    switch (deviceRole) {
        case AudioDeviceDescriptor::INPUT_DEVICE:
            result = AudioDeviceDescriptorNapi::INPUT_DEVICE;
            break;

        case AudioDeviceDescriptor::OUTPUT_DEVICE:
            result = AudioDeviceDescriptorNapi::OUTPUT_DEVICE;
            break;

        default:
            result = AudioDeviceDescriptorNapi::INPUT_DEVICE;
            HiLog::Error(LABEL, "Unknown device role!");
            break;
    }
    return result;
}

napi_value AudioDeviceDescriptorNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value constructor;
    napi_property_descriptor audio_dev_desc_properties[] = {
        DECLARE_NAPI_GETTER("deviceRole", GetDeviceRole),
        DECLARE_NAPI_GETTER("deviceType", GetDeviceType)
    };
    const int32_t refCount = 1;

    HiLog::Debug(LABEL, "AudioDeviceDescriptorNapi::Init is called!");
    status = napi_define_class(env, AUDIO_DEVICE_DESCRIPTOR_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Construct,
                               nullptr, sizeof(audio_dev_desc_properties) / sizeof(audio_dev_desc_properties[0]),
                               audio_dev_desc_properties, &constructor);
    if (status == napi_ok) {
        status = napi_create_reference(env, constructor, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, AUDIO_DEVICE_DESCRIPTOR_NAPI_CLASS_NAME.c_str(),
                                             constructor);
            if (status == napi_ok) {
                HiLog::Info(LABEL, "All props and functions are configured..");
                return exports;
            }
        }
    }
    HiLog::Error(LABEL, "Failure in AudioDeviceDescriptorNapi::Init()");
    return exports;
}

napi_value AudioDeviceDescriptorNapi::Construct(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsThis = nullptr;
    size_t argCount = 0;

    HiLog::Debug(LABEL, "AudioDeviceDescriptorNapi::Construct() is called!");
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status == napi_ok) {
        AudioDeviceDescriptorNapi* obj = new AudioDeviceDescriptorNapi();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->audioDescriptor_ = sAudioDescriptor_;
            status = napi_wrap(env, jsThis, reinterpret_cast<void*>(obj),
                               AudioDeviceDescriptorNapi::Destructor, nullptr, &(obj->wrapper_));
            if (status == napi_ok) {
                HiLog::Debug(LABEL, "wrapping is succesful in AudioDeviceDescriptorNapi::Construct()");
                return jsThis;
            }
        }
    }
    HiLog::Error(LABEL, "Failed in AudioDeviceDescriptorNapi::Construct()!");
    napi_get_undefined(env, &jsThis);
    return jsThis;
}

napi_value AudioDeviceDescriptorNapi::CreateAudioDeviceDescriptorWrapper(napi_env env,
    sptr<AudioDeviceDescriptor> audioDeviceDescriptor)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    HiLog::Debug(LABEL, "CreateAudioDeviceDescriptorWrapper() is called!");
    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        sAudioDescriptor_ = audioDeviceDescriptor;
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        sAudioDescriptor_ = nullptr;
        if (status == napi_ok) {
            return result;
        }
    }
    HiLog::Error(LABEL, "Failed in AudioDeviceDescriptorNapi::CreateAudioDeviceDescriptorWrapper!, %{public}d", status);
    napi_get_undefined(env, &result);
    return result;
}

napi_value AudioDeviceDescriptorNapi::GetDeviceRole(napi_env env, napi_callback_info info)
{
    napi_status status;
    AudioDeviceDescriptorNapi* audioDeviceDescriptor = nullptr;
    size_t argc = 0;
    napi_value thisVar = nullptr;
    AudioDeviceDescriptor::DeviceRole deviceRole;
    napi_value jsResult = nullptr;

    HiLog::Debug(LABEL, "GetDeviceRole() is called!");
    napi_get_cb_info(env, info, &argc, nullptr, &thisVar, nullptr);
    NAPI_ASSERT(env, argc == 0, "Invalid number of arguments");
    status = napi_unwrap(env, thisVar, (void **)&audioDeviceDescriptor);
    if (status == napi_ok) {
        HiLog::Debug(LABEL, "Unwrapping is succesful in GetDeviceRole()!");
        deviceRole = audioDeviceDescriptor->audioDescriptor_->deviceRole_;
        HiLog::Debug(LABEL, "deviceRole: %{public}d", deviceRole);
        status = napi_create_int32(env, GetJSDeviceRole(deviceRole), &jsResult);
        if (status == napi_ok) {
            HiLog::Debug(LABEL, "returning device role!");
            return jsResult;
        }
    }
    HiLog::Debug(LABEL, "returning undefine from GetDeviceRole()!");
    napi_get_undefined(env, &jsResult);
    return jsResult;
}

napi_value AudioDeviceDescriptorNapi::GetDeviceType(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argc = 0;
    napi_value thisVar = nullptr;
    AudioDeviceDescriptor::DeviceType deviceType;
    napi_value jsResult = nullptr;
    AudioDeviceDescriptorNapi* audioDeviceDescriptor = nullptr;

    HiLog::Debug(LABEL, "GetDeviceType() is called!");
    napi_get_cb_info(env, info, &argc, nullptr, &thisVar, nullptr);
    NAPI_ASSERT(env, argc == 0, "Invalid number of arguments");
    status = napi_unwrap(env, thisVar, (void **)&audioDeviceDescriptor);
    if (status == napi_ok) {
        HiLog::Debug(LABEL, "Unwrapping is succesful in GetDeviceType()!");
        deviceType = audioDeviceDescriptor->audioDescriptor_->deviceType_;
        HiLog::Debug(LABEL, "deviceType: %{public}d", deviceType);
        status = napi_create_int32(env, GetJSDeviceType(deviceType), &jsResult);
        if (status == napi_ok) {
            HiLog::Debug(LABEL, "returning device type!");
            return jsResult;
        }
    }
    HiLog::Debug(LABEL, "returning undefined from GetDeviceType()!");
    napi_get_undefined(env, &jsResult);
    return jsResult;
}
