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

#include "audio_errors.h"
#include "audio_log.h"
#include "audio_rounting_available_devicechange_callback.h"

namespace OHOS {
namespace AudioStandard {
AudioRountingAvailableDeviceChangeCallbackNapi::AudioRountingAvailableDeviceChangeCallbackNapi(napi_env env)
{
    env_ = env;
    AUDIO_DEBUG_LOG("AudioRountingAvailableDeviceChangeCallbackNapi: instance create");
}

AudioRountingAvailableDeviceChangeCallbackNapi::~AudioRountingAvailableDeviceChangeCallbackNapi()
{
    AUDIO_DEBUG_LOG("AudioRountingAvailableDeviceChangeCallbackNapi: instance destroy");
}

void AudioRountingAvailableDeviceChangeCallbackNapi::OnAvailableDeviceChange(
    const AudioDeviceUsage usage, const DeviceChangeAction &deviceChangeAction)
{
    AUDIO_INFO_LOG("OnAvailableDeviceChange:DeviceChangeType: %{public}d, DeviceFlag:%{public}d",
        deviceChangeAction.type, deviceChangeAction.flag);
    for (auto it = availableDeviceChangeCbList_.begin(); it != availableDeviceChangeCbList_.end(); it++) {
        if (usage == (*it).second) {
            std::unique_ptr<AudioRountingJsCallback> cb = std::make_unique<AudioRountingJsCallback>();
            cb->callback = (*it).first;
            cb->callbackName = AVAILABLE_DEVICE_CHANGE_CALLBACK_NAME;
            cb->deviceChangeAction = deviceChangeAction;
            OnJsCallbackAvailbleDeviceChange(cb);
        }
    }
}

void AudioRountingAvailableDeviceChangeCallbackNapi::SaveRoutingAvailbleDeviceChangeCbRef(AudioDeviceUsage usage,
    napi_value callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    napi_ref callbackRef = nullptr;
    const int32_t refCount = 1;

    for (auto it = availableDeviceChangeCbList_.begin(); it != availableDeviceChangeCbList_.end(); ++it) {
        bool isSameCallback = AudioCommonNapi::IsSameCallback(env_, callback, (*it).first->cb_);
        CHECK_AND_RETURN_LOG(!isSameCallback,
            "SaveRoutingAvailbleDeviceChangeCbRef: audio manager has same callback, nothing to do");
    }

    napi_status status = napi_create_reference(env_, callback, refCount, &callbackRef);
    CHECK_AND_RETURN_LOG(status == napi_ok && callback != nullptr,
        "SaveCallbackReference: creating reference for callback fail");
    std::shared_ptr<AutoRef> cb = std::make_shared<AutoRef>(env_, callbackRef);
    availableDeviceChangeCbList_.push_back({cb, usage});
    AUDIO_INFO_LOG("SaveRoutingAvailbleDeviceChange callback ref success, usage [%{public}d], list size [%{public}zu]",
        usage, availableDeviceChangeCbList_.size());
}

void AudioRountingAvailableDeviceChangeCallbackNapi::RemoveRoutingAvailbleDeviceChangeCbRef(napi_env env,
    napi_value callback)
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = availableDeviceChangeCbList_.begin(); it != availableDeviceChangeCbList_.end(); ++it) {
        bool isSameCallback = AudioCommonNapi::IsSameCallback(env_, callback, (*it).first->cb_);
        if (isSameCallback) {
            AUDIO_INFO_LOG("RemoveRoutingAvailbleDeviceChangeCbRef: find js callback, delete it");
            napi_delete_reference(env, (*it).first->cb_);
            (*it).first->cb_ = nullptr;
            availableDeviceChangeCbList_.erase(it);
            return;
        }
    }
    AUDIO_INFO_LOG("RemoveRoutingAvailbleDeviceChangeCbRef: js callback no find");
}

void AudioRountingAvailableDeviceChangeCallbackNapi::RemoveAllRoutinAvailbleDeviceChangeCb()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = availableDeviceChangeCbList_.begin(); it != availableDeviceChangeCbList_.end(); ++it) {
        napi_delete_reference(env_, (*it).first->cb_);
        (*it).first->cb_ = nullptr;
    }
    availableDeviceChangeCbList_.clear();
    AUDIO_INFO_LOG("RemoveAllCallbacks: remove all js callbacks success");
}

int32_t AudioRountingAvailableDeviceChangeCallbackNapi::GetRoutingAvailbleDeviceChangeCbListSize()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return availableDeviceChangeCbList_.size();
}

static void SetValueInt32(const napi_env& env, const std::string& fieldStr, const int intValue, napi_value& result)
{
    napi_value value = nullptr;
    napi_create_int32(env, intValue, &value);
    napi_set_named_property(env, result, fieldStr.c_str(), value);
}

static void SetValueString(const napi_env &env, const std::string &fieldStr, const std::string &stringValue,
    napi_value &result)
{
    napi_value value = nullptr;
    napi_create_string_utf8(env, stringValue.c_str(), NAPI_AUTO_LENGTH, &value);
    napi_set_named_property(env, result, fieldStr.c_str(), value);
}

static void SetDeviceDescriptors(const napi_env& env, napi_value &valueParam, const AudioDeviceDescriptor &deviceInfo)
{
    SetValueInt32(env, "deviceRole", static_cast<int32_t>(deviceInfo.deviceRole_), valueParam);
    SetValueInt32(env, "deviceType", static_cast<int32_t>(deviceInfo.deviceType_), valueParam);
    SetValueInt32(env, "id", static_cast<int32_t>(deviceInfo.deviceId_), valueParam);
    SetValueString(env, "name", deviceInfo.deviceName_, valueParam);
    SetValueString(env, "address", deviceInfo.macAddress_, valueParam);
    SetValueString(env, "networkId", deviceInfo.networkId_, valueParam);
    SetValueString(env, "displayName", deviceInfo.displayName_, valueParam);
    SetValueInt32(env, "interruptGroupId", static_cast<int32_t>(deviceInfo.interruptGroupId_), valueParam);
    SetValueInt32(env, "volumeGroupId", static_cast<int32_t>(deviceInfo.volumeGroupId_), valueParam);

    napi_value value = nullptr;
    napi_value sampleRates;
    size_t size = deviceInfo.audioStreamInfo_.samplingRate.size();
    napi_create_array_with_length(env, size, &sampleRates);
    size_t count = 0;
    for (const auto &samplingRate : deviceInfo.audioStreamInfo_.samplingRate) {
        napi_create_int32(env, samplingRate, &value);
        napi_set_element(env, sampleRates, count, value);
        count++;
    }
    napi_set_named_property(env, valueParam, "sampleRates", sampleRates);

    napi_value channelCounts;
    size = deviceInfo.audioStreamInfo_.channels.size();
    napi_create_array_with_length(env, size, &channelCounts);
    count = 0;
    for (const auto &channels : deviceInfo.audioStreamInfo_.channels) {
        napi_create_int32(env, channels, &value);
        napi_set_element(env, channelCounts, count, value);
        count++;
    }
    napi_set_named_property(env, valueParam, "channelCounts", channelCounts);

    napi_value channelMasks;
    napi_create_array_with_length(env, 1, &channelMasks);
    napi_create_int32(env, deviceInfo.channelMasks_, &value);
    napi_set_element(env, channelMasks, 0, value);
    napi_set_named_property(env, valueParam, "channelMasks", channelMasks);

    napi_value channelIndexMasks;
    napi_create_array_with_length(env, 1, &channelIndexMasks);
    napi_create_int32(env, deviceInfo.channelIndexMasks_, &value);
    napi_set_element(env, channelIndexMasks, 0, value);
    napi_set_named_property(env, valueParam, "channelIndexMasks", channelIndexMasks);

    napi_value encodingTypes;
    napi_create_array_with_length(env, 1, &encodingTypes);
    napi_create_int32(env, deviceInfo.audioStreamInfo_.encoding, &value);
    napi_set_element(env, encodingTypes, 0, value);
    napi_set_named_property(env, valueParam, "encodingTypes", encodingTypes);
}

static void NativeDeviceChangeActionToJsObj(const napi_env& env, napi_value& jsObj, const DeviceChangeAction &action)
{
    napi_create_object(env, &jsObj);
    SetValueInt32(env, "type", static_cast<int32_t>(action.type), jsObj);

    napi_value value = nullptr;
    napi_value jsArray;
    size_t size = action.deviceDescriptors.size();
    napi_create_array_with_length(env, size, &jsArray);

    for (size_t i = 0; i < size; i++) {
        if (action.deviceDescriptors[i] != nullptr) {
            (void)napi_create_object(env, &value);
            SetDeviceDescriptors(env, value, action.deviceDescriptors[i]);
            napi_set_element(env, jsArray, i, value);
        }
    }

    napi_set_named_property(env, jsObj, "deviceDescriptors", jsArray);
}

void AudioRountingAvailableDeviceChangeCallbackNapi::WorkAvailbleDeviceChangeDone(uv_work_t *work, int status)
{
    // Js Thread
    CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
    AudioRountingJsCallback *event = reinterpret_cast<AudioRountingJsCallback *>(work->data);
    CHECK_AND_RETURN_LOG(event != nullptr, "event is nullptr");
    std::string request = event->callbackName;

    CHECK_AND_RETURN_LOG(event->callback != nullptr, "event is nullptr");
    napi_env env = event->callback->env_;
    napi_ref callback = event->callback->cb_;

    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env, &scope);
    CHECK_AND_RETURN_LOG(scope != nullptr, "scope is nullptr");
    do {
        CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
        napi_value jsCallback = nullptr;
        napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
        CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
            request.c_str());

        // Call back function
        napi_value args[1] = { nullptr };
        NativeDeviceChangeActionToJsObj(env, args[0], event->deviceChangeAction);
        CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr,
            "%{public}s fail to create DeviceChange callback", request.c_str());

        const size_t argCount = 1;
        napi_value result = nullptr;
        nstatus = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
        CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to call DeviceChange callback", request.c_str());
    } while (0);
    napi_close_handle_scope(env, scope);
    delete event;
    delete work;
}

void AudioRountingAvailableDeviceChangeCallbackNapi::OnJsCallbackAvailbleDeviceChange(
    std::unique_ptr<AudioRountingJsCallback> &jsCb)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_LOG(loop != nullptr, "loop is null: No memory");

    uv_work_t *work = new(std::nothrow) uv_work_t;
    CHECK_AND_RETURN_LOG(work != nullptr, "OnJsCallbackDeviceChange: No memory");

    if (jsCb.get() == nullptr) {
        AUDIO_ERR_LOG("AudioManagerCallbackNapi: OnJsCallbackDeviceChange: jsCb.get() is null");
        delete work;
        return;
    }

    work->data = reinterpret_cast<void *>(jsCb.get());

    int ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t *work) {},
        WorkAvailbleDeviceChangeDone, uv_qos_default);
    if (ret != 0) {
        AUDIO_ERR_LOG("Failed to execute libuv work queue");
        delete work;
    } else {
        jsCb.release();
    }
}
}  // namespace AudioStandard
}  // namespace OHOS