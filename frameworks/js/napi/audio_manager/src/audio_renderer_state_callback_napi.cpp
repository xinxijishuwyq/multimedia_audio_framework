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

#include "audio_renderer_state_callback_napi.h"

#include <uv.h>

#include "audio_errors.h"
#include "audio_log.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
AudioRendererStateCallbackNapi::AudioRendererStateCallbackNapi(napi_env env)
    : env_(env)
{
    AUDIO_DEBUG_LOG("AudioRendererStateCallbackNapi: instance create");
}

AudioRendererStateCallbackNapi::~AudioRendererStateCallbackNapi()
{
    AUDIO_DEBUG_LOG("AudioRendererStateCallbackNapi: instance destroy");
}

void AudioRendererStateCallbackNapi::SaveCallbackReference(napi_value args)
{
    std::lock_guard<std::mutex> lock(mutex_);
    napi_ref callback = nullptr;
    const int32_t refCount = 1;
    napi_status status = napi_create_reference(env_, args, refCount, &callback);
    CHECK_AND_RETURN_LOG(status == napi_ok && callback != nullptr,
                         "AudioRendererStateCallbackNapi: creating reference for callback fail");

    std::shared_ptr<AutoRef> cb = std::make_shared<AutoRef>(env_, callback);
    CHECK_AND_RETURN_LOG(cb != nullptr, "AudioRendererStateCallbackNapi: creating callback failed");

    rendererStateCallback_ = cb;
}

void AudioRendererStateCallbackNapi::OnRendererStateChange(
    const std::vector<std::unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    AUDIO_INFO_LOG("AudioRendererStateCallbackNapi: OnRendererStateChange entered");

    std::lock_guard<std::mutex> lock(mutex_);

    std::unique_ptr<AudioRendererStateJsCallback> cb = std::make_unique<AudioRendererStateJsCallback>();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory!!");

    std::vector<std::unique_ptr<AudioRendererChangeInfo>> rendererChangeInfos;
    for (const auto &changeInfo : audioRendererChangeInfos) {
        rendererChangeInfos.push_back(std::make_unique<AudioRendererChangeInfo>(*changeInfo));
    }

    AUDIO_DEBUG_LOG("AudioRendererStateCallbackNapi::OnRendererStateChange Number of entries %{public}u",
        static_cast<uint32_t>(rendererChangeInfos.size()));
    int index = 0;
    for (auto it = rendererChangeInfos.begin(); it != rendererChangeInfos.end(); it++) {
        AudioRendererChangeInfo audioRendererChangeInfo = **it;
        AUDIO_DEBUG_LOG("audioRendererChangeInfos_[%{public}d]", index++);
        AUDIO_DEBUG_LOG("clientUID = %{public}d", audioRendererChangeInfo.clientUID);
        AUDIO_DEBUG_LOG("sessionId = %{public}d", audioRendererChangeInfo.sessionId);
        AUDIO_DEBUG_LOG("rendererState = %{public}d", audioRendererChangeInfo.rendererState);
    }

    cb->callback = rendererStateCallback_;
    cb->changeInfos = move(rendererChangeInfos);

    return OnJsCallbackRendererState(cb);
}

static void SetValueInt32(const napi_env& env, const std::string& fieldStr, const int intValue, napi_value& obj)
{
    napi_value value = nullptr;
    napi_create_int32(env, intValue, &value);
    napi_set_named_property(env, obj, fieldStr.c_str(), value);
}

static void NativeRendererChangeInfoToJsObj(const napi_env &env, napi_value &jsArrayChangeInfoObj,
    const vector<unique_ptr<AudioRendererChangeInfo>> &changeInfos)
{
    napi_value jsChangeInfoObj = nullptr;
    napi_value jsRenInfoObj = nullptr;

    size_t size = changeInfos.size();
    int32_t position = 0;
    napi_create_array_with_length(env, size, &jsArrayChangeInfoObj);
    for (const unique_ptr<AudioRendererChangeInfo> &changeInfo: changeInfos) {
        napi_create_object(env, &jsChangeInfoObj);
        SetValueInt32(env, "streamId", static_cast<int32_t>(changeInfo->sessionId), jsChangeInfoObj);
        SetValueInt32(env, "rendererState", static_cast<int32_t>(changeInfo->rendererState), jsChangeInfoObj);
        SetValueInt32(env, "clientUid", static_cast<int32_t>(changeInfo->clientUID), jsChangeInfoObj);
        napi_create_object(env, &jsRenInfoObj);
        SetValueInt32(env, "content", static_cast<int32_t>(changeInfo->rendererInfo.contentType), jsRenInfoObj);
        SetValueInt32(env, "usage", static_cast<int32_t>(changeInfo->rendererInfo.streamUsage), jsRenInfoObj);
        SetValueInt32(env, "rendererFlags", changeInfo->rendererInfo.rendererFlags, jsRenInfoObj);
        napi_set_named_property(env, jsChangeInfoObj, "rendererInfo", jsRenInfoObj);

        napi_set_element(env, jsArrayChangeInfoObj, position, jsChangeInfoObj);
        position++;
    }
}

void AudioRendererStateCallbackNapi::OnJsCallbackRendererState(std::unique_ptr<AudioRendererStateJsCallback> &jsCb)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        AUDIO_ERR_LOG("AudioRendererStateCallbackNapi: OnJsCallbackRendererState: No memory");
        return;
    }

    work->data = reinterpret_cast<void *>(jsCb.get());

    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        AudioRendererStateJsCallback *event = reinterpret_cast<AudioRendererStateJsCallback *>(work->data);
        napi_env env = event->callback->env_;
        napi_ref callback = event->callback->cb_;

        do {
            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "callback get reference value fail");

            // Call back function
            napi_value args[1] = { nullptr };
            NativeRendererChangeInfoToJsObj(env, args[0], event->changeInfos);

            CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr,
                " fail to convert to jsobj");

            const size_t argCount = 1;
            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "Fail to call renderstate callback");
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        AUDIO_ERR_LOG("Failed to execute libuv work queue");
        delete work;
    } else {
        jsCb.release();
    }
}
}  // namespace AudioStandard
}  // namespace OHOS
