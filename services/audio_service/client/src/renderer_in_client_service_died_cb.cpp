/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef LOG_TAG
#define LOG_TAG "RendererInClientPolicyServiceDiedCb"
#endif

#include "renderer_in_client_service_died_cb.h"

namespace OHOS {
namespace AudioStandard {
RendererInClientPolicyServiceDiedCallbackImpl::RendererInClientPolicyServiceDiedCallbackImpl()
{
    AUDIO_DEBUG_LOG("instance create");
}

RendererInClientPolicyServiceDiedCallbackImpl::~RendererInClientPolicyServiceDiedCallbackImpl()
{
    AUDIO_DEBUG_LOG("instance destory");
}

void RendererInClientPolicyServiceDiedCallbackImpl::OnAudioPolicyServiceDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    AUDIO_INFO_LOG("OnAudioPolicyServiceDied");
    if (policyServiceDiedCallback_ != nullptr) {
        policyServiceDiedCallback_->OnAudioPolicyServiceDied();
    }
}

void RendererInClientPolicyServiceDiedCallbackImpl::SaveRendererOrCapturerPolicyServiceDiedCB(
    const std::shared_ptr<RendererOrCapturerPolicyServiceDiedCallback> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (callback != nullptr) {
        policyServiceDiedCallback_ = callback;
    }
}

void RendererInClientPolicyServiceDiedCallbackImpl::RemoveRendererOrCapturerPolicyServiceDiedCB()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (policyServiceDiedCallback_ != nullptr) {
        policyServiceDiedCallback_ = nullptr;
    }
}
} // namespace AudioStandard
} // namespace OHOS