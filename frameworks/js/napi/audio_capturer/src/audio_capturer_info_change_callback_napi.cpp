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

#include <uv.h>
#include "audio_errors.h"
#include "audio_log.h"
#include "audio_capturer_info_change_callback_napi.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
AudioCapturerInfoChangeCallbackNapi::AudioCapturerInfoChangeCallbackNapi(napi_env env)
    : env_(env)
{
    AUDIO_DEBUG_LOG("Instance create");
}

AudioCapturerInfoChangeCallbackNapi::~AudioCapturerInfoChangeCallbackNapi()
{
    AUDIO_DEBUG_LOG("Instance destroy");
}

void AudioCapturerInfoChangeCallbackNapi::SaveCallbackReference(napi_value args)
{
    std::lock_guard<std::mutex> lock(mutex_);
    napi_ref callback = nullptr;
    const int32_t refCount = 1;

    napi_status status = napi_create_reference(env_, args, refCount, &callback);
    CHECK_AND_RETURN_LOG(status == napi_ok && callback != nullptr,
        "Creating reference for callback fail");

    callback_ = callback;
}

bool AudioCapturerInfoChangeCallbackNapi::ContainSameJsCallback(napi_value args)
{
    bool isEquals = false;
    napi_value copyValue = nullptr;

    napi_get_reference_value(env_, callback_, &copyValue);

    if (args == nullptr) {
        return false;
    }
    if (napi_ok != napi_strict_equals(env_, copyValue, args, &isEquals)) {
        AUDIO_ERR_LOG("Get napi_strict_equals failed");
        return false;
    }

    if (isEquals == true) {
        AUDIO_INFO_LOG("Js Callback already exist");
        return true;
    }

    return isEquals;
}

void AudioCapturerInfoChangeCallbackNapi::OnStateChange(const AudioCapturerChangeInfo &capturerChangeInfo)
{
    OnJsCallbackCapturerChangeInfo(callback_, capturerChangeInfo);
}

static void SetValueInt32(const napi_env& env, const std::string& fieldStr, const int intValue, napi_value& obj)
{
    napi_value value = nullptr;
    napi_create_int32(env, intValue, &value);
    napi_set_named_property(env, obj, fieldStr.c_str(), value);
}

static void SetValueBoolean(const napi_env& env, const std::string& fieldStr, const bool boolValue, napi_value& result)
{
    napi_value value = nullptr;
    napi_get_boolean(env, boolValue, &value);
    napi_set_named_property(env, result, fieldStr.c_str(), value);
}

static void SetValueString(const napi_env &env, const std::string &fieldStr, const std::string &stringValue,
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
    SetValueString(env, "displayName", deviceInfo.displayName, valueParam);
    SetValueInt32(env, "interruptGroupId", static_cast<int32_t>(deviceInfo.interruptGroupId), valueParam);
    SetValueInt32(env, "volumeGroupId", static_cast<int32_t>(deviceInfo.volumeGroupId), valueParam);

    napi_value value = nullptr;

    auto setToArray = [&env, &valueParam, &value](const auto &set, const char* utf8Name) {
        napi_value napiValue;
        size_t size = set.size();
        napi_create_array_with_length(env, size, &napiValue);
        size_t count = 0;
        for (const auto &valueOfSet : set) {
            napi_create_int32(env, valueOfSet, &value);
            napi_set_element(env, napiValue, count, value);
            count++;
        }
        napi_set_named_property(env, valueParam, utf8Name, napiValue);
    };

    setToArray(deviceInfo.audioStreamInfo.samplingRate, "sampleRates");

    setToArray(deviceInfo.audioStreamInfo.channels, "channelCounts");

    napi_value channelMasks;
    napi_create_array_with_length(env, 1, &channelMasks);
    napi_create_int32(env, deviceInfo.channelMasks, &value);
    napi_set_element(env, channelMasks, 0, value);
    napi_set_named_property(env, valueParam, "channelMasks", channelMasks);

    napi_value channelIndexMasks;
    napi_create_array_with_length(env, 1, &channelIndexMasks);
    napi_create_int32(env, deviceInfo.channelIndexMasks, &value);
    napi_set_element(env, channelIndexMasks, 0, value);
    napi_set_named_property(env, valueParam, "channelIndexMasks", channelIndexMasks);

    napi_value encodingTypes;
    napi_create_array_with_length(env, 1, &encodingTypes);
    napi_create_int32(env, deviceInfo.audioStreamInfo.encoding, &value);
    napi_set_element(env, encodingTypes, 0, value);
    napi_set_named_property(env, valueParam, "encodingTypes", encodingTypes);

    napi_set_element(env, jsDeviceDescriptorsObj, 0, valueParam);
    napi_set_named_property(env, jsChangeInfoObj, "deviceDescriptors", jsDeviceDescriptorsObj);
}

static void NativeCapturerChangeInfoToJsObj(const napi_env &env, napi_value &jsChangeDeviceInfoObj,
    const AudioCapturerChangeInfo &capturerChangeInfo)
{
    napi_value jsCapInfoObj = nullptr;
    napi_create_object(env, &jsChangeDeviceInfoObj);
    SetValueInt32(env, "streamId", capturerChangeInfo.sessionId, jsChangeDeviceInfoObj);
    SetValueInt32(env, "clientUid", capturerChangeInfo.clientUID, jsChangeDeviceInfoObj);
    SetValueInt32(env, "capturerState", static_cast<int32_t>(capturerChangeInfo.capturerState), jsChangeDeviceInfoObj);
    SetValueBoolean(env, "muted", capturerChangeInfo.muted, jsChangeDeviceInfoObj);

    napi_create_object(env, &jsCapInfoObj);
    SetValueInt32(env, "source", static_cast<int32_t>(capturerChangeInfo.capturerInfo.sourceType), jsCapInfoObj);
    SetValueInt32(env, "capturerFlags", capturerChangeInfo.capturerInfo.capturerFlags, jsCapInfoObj);
    napi_set_named_property(env, jsChangeDeviceInfoObj, "capturerInfo", jsCapInfoObj);
 
    SetDeviceDescriptors(env, jsChangeDeviceInfoObj, capturerChangeInfo.inputDeviceInfo);
}

void AudioCapturerInfoChangeCallbackNapi::WorkCallbackCompleted(uv_work_t *work, int status)
{
    // Js Thread
    std::shared_ptr<AudioCapturerChangeInfoJsCallback> context(
        static_cast<AudioCapturerChangeInfoJsCallback*>(work->data),
        [work](AudioCapturerChangeInfoJsCallback* ptr) {
            delete ptr;
            delete work;
    });

    AudioCapturerChangeInfoJsCallback *event = reinterpret_cast<AudioCapturerChangeInfoJsCallback*>(work->data);
    if (event == nullptr || event->callback_ == nullptr) {
        AUDIO_ERR_LOG("OnJsCallbackCapturerChangeInfo: no memory");
        return;
    }
    napi_env env = event->env_;
    napi_ref callback = event->callback_;

    do {
        napi_value jsCallback = nullptr;
        napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
        CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "Callback get reference value fail");
        // Call back function
        napi_value args[1] = { nullptr };
        NativeCapturerChangeInfoToJsObj(env, args[0], event->capturerChangeInfo_);
        CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr,
            "Fail to convert to jsobj");

        const size_t argCount = 1;
        napi_value result = nullptr;
        nstatus = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
        CHECK_AND_BREAK_LOG(nstatus == napi_ok, "Fail to call capturer callback");
    } while (0);
}

void AudioCapturerInfoChangeCallbackNapi::OnJsCallbackCapturerChangeInfo(napi_ref method,
    const AudioCapturerChangeInfo &capturerChangeInfo)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        AUDIO_ERR_LOG("Loop is nullptr");
        return;
    }

    if (method == nullptr) {
        AUDIO_ERR_LOG("Method is nullptr");
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        AUDIO_ERR_LOG("New uv_work_t failed: no memory");
    }

    work->data = new AudioCapturerChangeInfoJsCallback {method, env_, capturerChangeInfo};
    if (work->data == nullptr) {
        AUDIO_ERR_LOG("New object failed: no memory");
    }

    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, WorkCallbackCompleted);
    if (ret != 0) {
        AUDIO_ERR_LOG("Failed to execute libuv work queue");
        delete work;
    }
}
}  // namespace AudioStandard
}  // namespace OHOS
