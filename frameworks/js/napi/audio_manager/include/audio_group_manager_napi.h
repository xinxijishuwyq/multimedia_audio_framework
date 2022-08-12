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

#ifndef AUDIO_GROUP_MNGR_NAPI_H_
#define AUDIO_GROUP_MNGR_NAPI_H_

#include <iostream>
#include <map>
#include <vector>
#include "audio_system_manager.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "audio_manager_napi.h"

namespace OHOS {
namespace AudioStandard {
static const std::string AUDIO_GROUP_MNGR_NAPI_CLASS_NAME = "AudioGroupManager";

class AudioGroupManagerNapi {
public:

    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreateAudioGroupManagerWrapper(napi_env env, int32_t groupId);

private:
    static void Destructor(napi_env env, void *nativeObject, void *finalize_hint);
    static void GetGroupManagerAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static napi_value Construct(napi_env env, napi_callback_info info);
    static napi_value GetVolume(napi_env env, napi_callback_info info);
    static napi_value SetVolume(napi_env env, napi_callback_info info);
    static napi_value GetMaxVolume(napi_env env, napi_callback_info info);
    static napi_value GetMinVolume(napi_env env, napi_callback_info info);
    static napi_value SetMute(napi_env env, napi_callback_info info);
    static napi_value IsStreamMute(napi_env env, napi_callback_info info);
    std::shared_ptr<AudioGroupManager> audioGroupMngr_ = nullptr;

    napi_ref wrapper_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_GROUP_MNGR_NAPI_H_
