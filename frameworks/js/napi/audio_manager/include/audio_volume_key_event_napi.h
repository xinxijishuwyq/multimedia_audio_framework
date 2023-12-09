/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef AUDIO_VOLUME_KEY_EVENT_NAPI_CALLBACK_H_
#define AUDIO_VOLUME_KEY_EVENT_NAPI_CALLBACK_H_

#include <uv.h>
#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "audio_common_napi.h"
#include "audio_system_manager.h"

namespace OHOS {
namespace AudioStandard {
struct VolumeKeyEventWorkData {
    napi_env env_;
    napi_ref cb_;
};

struct VolumeKeyEventAutoRef {
    VolumeKeyEventAutoRef(napi_env env, napi_ref cb)
        : env_(env), cb_(cb)
    {
    }
    ~VolumeKeyEventAutoRef()
    {
        uv_loop_s *loop = nullptr;
        napi_get_uv_event_loop(env_, &loop);
        if (loop == nullptr) {
            return;
        }

        VolumeKeyEventWorkData *workData = new (std::nothrow) VolumeKeyEventWorkData();
        if (workData == nullptr) {
            return;
        }
        workData->env_ = env_;
        workData->cb_ = cb_;

        uv_work_t *work = new(std::nothrow) uv_work_t;
        if (work == nullptr) {
            delete workData;
            workData = nullptr;
            return;
        }
        work->data = (void *)workData;

        int ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
            // Js thread
            VolumeKeyEventWorkData *workData = reinterpret_cast<VolumeKeyEventWorkData *>(work->data);
            napi_env env = workData->env_;
            napi_ref cb = workData->cb_;
            if (env != nullptr && cb != nullptr) {
                (void)napi_delete_reference(env, cb);
            }
            delete workData;
            delete work;
        }, uv_qos_default);
        if (ret != 0) {
            delete work;
            work = nullptr;
            delete workData;
            workData = nullptr;
        }
    }
    napi_env env_;
    napi_ref cb_;
};

class AudioVolumeKeyEventNapi : public VolumeKeyEventCallback {
public:
    explicit AudioVolumeKeyEventNapi(napi_env env);
    virtual ~AudioVolumeKeyEventNapi();

    /**
     * @brief OnVolumeKeyEvent will be executed when hard volume key is pressed up/down
     *
     * @param streamType the stream type for which volume must be updated.
     * @param volumeLevel the volume level to be updated.
     * @param isUpdateUi whether to update volume level in UI.
    **/
    void OnVolumeKeyEvent(VolumeEvent volumeEvent) override;
    void SaveCallbackReference(const std::string &callbackName, napi_value callback);
    void Release();

private:
    struct AudioVolumeKeyEventJsCallback {
        std::shared_ptr<VolumeKeyEventAutoRef> callback = nullptr;
        std::string callbackName = "unknown";
        VolumeEvent volumeEvent;
    };

    void OnJsCallbackVolumeEvent(std::unique_ptr<AudioVolumeKeyEventJsCallback> &jsCb);

    std::shared_ptr<VolumeKeyEventAutoRef> audioVolumeKeyEventJsCallback_ = nullptr;
    std::mutex mutex_;
    napi_env env_;

    static napi_ref sConstructor_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_VOLUME_KEY_EVENT_NAPI_CALLBACK_H_
