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

#ifndef RENDERER_IN_CLIENT_SERVICE_DIED_CB_H
#define RENDERER_IN_CLIENT_SERVICE_DIED_CB_H

#include <string>
#include <mutex>

#include "audio_service_log.h"
#include "audio_info.h"
#include "i_audio_stream.h"

namespace OHOS {
namespace AudioStandard {
class RendererInClientPolicyServiceDiedCallbackImpl : public AudioStreamPolicyServiceDiedCallback {
public:
    RendererInClientPolicyServiceDiedCallbackImpl();
    virtual ~RendererInClientPolicyServiceDiedCallbackImpl();
    void OnAudioPolicyServiceDied() override;
    void SaveRendererOrCapturerPolicyServiceDiedCB(
        const std::shared_ptr<RendererOrCapturerPolicyServiceDiedCallback> &callback);
    void RemoveRendererOrCapturerPolicyServiceDiedCB();

private:
    std::mutex mutex_;
    std::shared_ptr<RendererOrCapturerPolicyServiceDiedCallback> policyServiceDiedCallback_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // RENDERER_IN_CLIENT_SERVICE_DIED_CB_H