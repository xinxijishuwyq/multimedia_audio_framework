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
#include "napi_audio_renderer_callback.h"
#include "napi_param_utils.h"
#include "napi_audio_error.h"

namespace OHOS {
namespace AudioStandard {
NapiAudioRendererCallback::NapiAudioRendererCallback(napi_env env)
    : env_(env)
{
    AUDIO_DEBUG_LOG("instance create");
}

NapiAudioRendererCallback::~NapiAudioRendererCallback()
{
    AUDIO_DEBUG_LOG("instance destroy");
}

void NapiAudioRendererCallback::OnInterrupt(const InterruptEvent &interruptEvent)
{
    std::lock_guard<std::mutex> lock(mutex_);
    AUDIO_DEBUG_LOG("OnInterrupt is called,hintType: %{public}d", interruptEvent.hintType);
}

void NapiAudioRendererCallback::OnStateChange(const RendererState state,
    const StateChangeCmdType __attribute__((unused)) cmdType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    AUDIO_DEBUG_LOG("OnStateChange is called, state: %{public}d", state);
}
}  // namespace AudioStandard
}  // namespace OHOS
