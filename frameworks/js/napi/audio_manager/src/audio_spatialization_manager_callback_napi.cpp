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

#include "audio_spatialization_manager_callback_napi.h"

#include <uv.h>

#include "audio_errors.h"
#include "audio_log.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
AudioSpatializationEnabledChangeCallbackNapi::AudioSpatializationEnabledChangeCallbackNapi(napi_env env)
    : env_(env)
{
    AUDIO_DEBUG_LOG("AudioSpatializationEnabledChangeCallbackNapi: instance create");
}

AudioSpatializationEnabledChangeCallbackNapi::~AudioSpatializationEnabledChangeCallbackNapi()
{
    AUDIO_DEBUG_LOG("AudioSpatializationEnabledChangeCallbackNapi: instance destroy");
}

void AudioSpatializationEnabledChangeCallbackNapi::SaveSpatializationEnabledChangeCallbackReference(napi_value args)
{
    std::lock_guard<std::mutex> lock(mutex_);
    napi_ref callback = nullptr;
    const int32_t refCount = 1;

    for (auto it = spatializationEnabledChangeCbList_.begin(); it != spatializationEnabledChangeCbList_.end(); ++it) {
        bool isSameCallback = AudioCommonNapi::IsSameCallback(env_, args, (*it)->cb_);
        CHECK_AND_RETURN_LOG(!isSameCallback, "SaveCallbackReference: spatialization manager has same callback");
    }

    napi_status status = napi_create_reference(env_, args, refCount, &callback);
    CHECK_AND_RETURN_LOG(status == napi_ok && callback != nullptr,
        "AudioSpatializationEnabledChangeCallbackNapi: creating reference for callback fail");

    std::shared_ptr<AutoRef> cb = std::make_shared<AutoRef>(env_, callback);
    CHECK_AND_RETURN_LOG(cb != nullptr, "AudioSpatializationEnabledChangeCallbackNapi: creating callback failed");

    spatializationEnabledChangeCbList_.push_back(cb);
}

void AudioSpatializationEnabledChangeCallbackNapi::RemoveSpatializationEnabledChangeCallbackReference(napi_env env,
    napi_value args)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = spatializationEnabledChangeCbList_.begin(); it != spatializationEnabledChangeCbList_.end(); ++it) {
        bool isSameCallback = AudioCommonNapi::IsSameCallback(env_, args, (*it)->cb_);
        if (isSameCallback) {
            AUDIO_INFO_LOG("RemoveSpatializationEnabledChangeCallbackReference: find js callback, delete it");
            napi_delete_reference(env, (*it)->cb_);
            (*it)->cb_ = nullptr;
            spatializationEnabledChangeCbList_.erase(it);
            return;
        }
    }
    AUDIO_INFO_LOG("RemoveSpatializationEnabledChangeCallbackReference: js callback no find");
}

void AudioSpatializationEnabledChangeCallbackNapi::RemoveAllSpatializationEnabledChangeCallbackReference()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = spatializationEnabledChangeCbList_.begin(); it != spatializationEnabledChangeCbList_.end(); ++it) {
        napi_delete_reference(env_, (*it)->cb_);
        (*it)->cb_ = nullptr;
    }
    spatializationEnabledChangeCbList_.clear();
    AUDIO_INFO_LOG("RemoveAllSpatializationEnabledChangeCallbackReference: remove all js callbacks success");
}

int32_t AudioSpatializationEnabledChangeCallbackNapi::GetSpatializationEnabledChangeCbListSize()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return spatializationEnabledChangeCbList_.size();
}

void AudioSpatializationEnabledChangeCallbackNapi::OnSpatializationEnabledChange(const bool &enabled)
{
    AUDIO_INFO_LOG("OnSpatializationEnabledChange entered");

    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = spatializationEnabledChangeCbList_.begin(); it != spatializationEnabledChangeCbList_.end(); it++) {
        std::unique_ptr<AudioSpatializationEnabledJsCallback> cb =
            std::make_unique<AudioSpatializationEnabledJsCallback>();
        CHECK_AND_RETURN_LOG(cb != nullptr, "No memory!!");
        cb->callback = (*it);
        cb->enabled = enabled;
        OnJsCallbackSpatializationEnabled(cb);
    }

    return;
}

void AudioSpatializationEnabledChangeCallbackNapi::OnJsCallbackSpatializationEnabled(
    std::unique_ptr<AudioSpatializationEnabledJsCallback> &jsCb)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        AUDIO_ERR_LOG("AudioSpatializationEnabledChangeCallbackNapi: OnJsCallbackSpatializationEnabled: No memory");
        return;
    }

    work->data = reinterpret_cast<void *>(jsCb.get());

    int ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        AudioSpatializationEnabledJsCallback *event =
            reinterpret_cast<AudioSpatializationEnabledJsCallback *>(work->data);
        if (event == nullptr || event->callback == nullptr) {
            AUDIO_ERR_LOG("AudioSpatializationEnabledChangeCallbackNapi: OnJsCallbackSpatializationEnabled: No memory");
            return;
        }
        napi_env env = event->callback->env_;
        napi_ref callback = event->callback->cb_;

        do {
            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "callback get reference value fail");

            // Call back function
            napi_value args[1] = { nullptr };
            napi_get_boolean(env, event->enabled, &args[0]);

            CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr, " fail to convert to jsobj");

            const size_t argCount = 1;
            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "Fail to call spatialization enabled callback");
        } while (0);
        delete event;
        delete work;
    }, uv_qos_default);
    if (ret != 0) {
        AUDIO_ERR_LOG("Failed to execute libuv work queue");
        delete work;
    } else {
        jsCb.release();
    }
}

AudioHeadTrackingEnabledChangeCallbackNapi::AudioHeadTrackingEnabledChangeCallbackNapi(napi_env env)
    : env_(env)
{
    AUDIO_DEBUG_LOG("AudioHeadTrackingEnabledChangeCallbackNapi: instance create");
}

AudioHeadTrackingEnabledChangeCallbackNapi::~AudioHeadTrackingEnabledChangeCallbackNapi()
{
    AUDIO_DEBUG_LOG("AudioHeadTrackingEnabledChangeCallbackNapi: instance destroy");
}

void AudioHeadTrackingEnabledChangeCallbackNapi::SaveHeadTrackingEnabledChangeCallbackReference(napi_value args)
{
    std::lock_guard<std::mutex> lock(mutex_);
    napi_ref callback = nullptr;
    const int32_t refCount = 1;

    for (auto it = headTrackingEnabledChangeCbList_.begin(); it != headTrackingEnabledChangeCbList_.end(); ++it) {
        bool isSameCallback = AudioCommonNapi::IsSameCallback(env_, args, (*it)->cb_);
        CHECK_AND_RETURN_LOG(!isSameCallback, "SaveCallbackReference: spatialization manager has same callback");
    }

    napi_status status = napi_create_reference(env_, args, refCount, &callback);
    CHECK_AND_RETURN_LOG(status == napi_ok && callback != nullptr,
        "AudioHeadTrackingEnabledChangeCallbackNapi: creating reference for callback fail");

    std::shared_ptr<AutoRef> cb = std::make_shared<AutoRef>(env_, callback);
    CHECK_AND_RETURN_LOG(cb != nullptr, "AudioHeadTrackingEnabledChangeCallbackNapi: creating callback failed");

    headTrackingEnabledChangeCbList_.push_back(cb);
}

void AudioHeadTrackingEnabledChangeCallbackNapi::RemoveHeadTrackingEnabledChangeCallbackReference(napi_env env,
    napi_value args)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = headTrackingEnabledChangeCbList_.begin(); it != headTrackingEnabledChangeCbList_.end(); ++it) {
        bool isSameCallback = AudioCommonNapi::IsSameCallback(env_, args, (*it)->cb_);
        if (isSameCallback) {
            AUDIO_INFO_LOG("RemoveHeadTrackingEnabledChangeCallbackReference: find js callback, delete it");
            napi_delete_reference(env, (*it)->cb_);
            (*it)->cb_ = nullptr;
            headTrackingEnabledChangeCbList_.erase(it);
            return;
        }
    }
    AUDIO_INFO_LOG("RemoveHeadTrackingEnabledChangeCallbackReference: js callback no find");
}

void AudioHeadTrackingEnabledChangeCallbackNapi::RemoveAllHeadTrackingEnabledChangeCallbackReference()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = headTrackingEnabledChangeCbList_.begin(); it != headTrackingEnabledChangeCbList_.end(); ++it) {
        napi_delete_reference(env_, (*it)->cb_);
        (*it)->cb_ = nullptr;
    }
    headTrackingEnabledChangeCbList_.clear();
    AUDIO_INFO_LOG("RemoveAllHeadTrackingEnabledChangeCallbackReference: remove all js callbacks success");
}

int32_t AudioHeadTrackingEnabledChangeCallbackNapi::GetHeadTrackingEnabledChangeCbListSize()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return headTrackingEnabledChangeCbList_.size();
}

void AudioHeadTrackingEnabledChangeCallbackNapi::OnHeadTrackingEnabledChange(const bool &enabled)
{
    AUDIO_INFO_LOG("OnHeadTrackingEnabledChange entered");

    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = headTrackingEnabledChangeCbList_.begin(); it != headTrackingEnabledChangeCbList_.end(); it++) {
        std::unique_ptr<AudioHeadTrackingEnabledJsCallback> cb =
            std::make_unique<AudioHeadTrackingEnabledJsCallback>();
        CHECK_AND_RETURN_LOG(cb != nullptr, "No memory!!");
        cb->callback = (*it);
        cb->enabled = enabled;
        OnJsCallbackHeadTrackingEnabled(cb);
    }

    return;
}

void AudioHeadTrackingEnabledChangeCallbackNapi::OnJsCallbackHeadTrackingEnabled(
    std::unique_ptr<AudioHeadTrackingEnabledJsCallback> &jsCb)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        AUDIO_ERR_LOG("AudioHeadTrackingEnabledChangeCallbackNapi: OnJsCallbackHeadTrackingEnabled: No memory");
        return;
    }

    work->data = reinterpret_cast<void *>(jsCb.get());

    int ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        AudioHeadTrackingEnabledJsCallback *event =
            reinterpret_cast<AudioHeadTrackingEnabledJsCallback *>(work->data);
        if (event == nullptr || event->callback == nullptr) {
            AUDIO_ERR_LOG("AudioHeadTrackingEnabledChangeCallbackNapi: OnJsCallbackHeadTrackingEnabled: No memory");
            return;
        }
        napi_env env = event->callback->env_;
        napi_ref callback = event->callback->cb_;

        do {
            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "callback get reference value fail");

            // Call back function
            napi_value args[1] = { nullptr };
            napi_get_boolean(env, event->enabled, &args[0]);

            CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr, " fail to convert to jsobj");

            const size_t argCount = 1;
            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "Fail to call head tracking enabled callback");
        } while (0);
        delete event;
        delete work;
    }, uv_qos_default);
    if (ret != 0) {
        AUDIO_ERR_LOG("Failed to execute libuv work queue");
        delete work;
    } else {
        jsCb.release();
    }
}
}  // namespace AudioStandard
}  // namespace OHOS
