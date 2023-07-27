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


#include "audio_errors.h"
#include "audio_manager_proxy.h"
#include "audio_policy_manager.h"
#include "audio_log.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "audio_routing_manager.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;
AudioRoutingManager *AudioRoutingManager::GetInstance()
{
    static AudioRoutingManager audioRoutingManager;
    return &audioRoutingManager;
}

uint32_t AudioRoutingManager::GetCallingPid()
{
    return getpid();
}

int32_t AudioRoutingManager::SetMicStateChangeCallback(
    const std::shared_ptr<AudioManagerMicStateChangeCallback> &callback)
{
    AudioSystemManager* audioSystemManager = AudioSystemManager::GetInstance();
    std::shared_ptr<AudioGroupManager> groupManager = audioSystemManager->GetGroupManager(DEFAULT_VOLUME_GROUP_ID);
    if (groupManager == nullptr) {
        AUDIO_ERR_LOG("setMicrophoneMuteCallback falied, groupManager is null");
        return ERR_INVALID_PARAM;
    }
    return groupManager->SetMicStateChangeCallback(callback);
}

int32_t AudioRoutingManager::GetPreferOutputDeviceForRendererInfo(AudioRendererInfo rendererInfo,
    std::vector<sptr<AudioDeviceDescriptor>> &desc)
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);

    desc = AudioPolicyManager::GetInstance().GetPreferOutputDeviceDescriptors(rendererInfo);

    return SUCCESS;
}

int32_t AudioRoutingManager::SetPreferOutputDeviceChangeCallback(AudioRendererInfo rendererInfo,
    const std::shared_ptr<AudioPreferOutputDeviceChangeCallback>& callback)
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    if (callback == nullptr) {
        AUDIO_ERR_LOG("SetPreferOutputDeviceChangeCallback: callback is nullptr");
        return ERR_INVALID_PARAM;
    }

    int32_t clientId = static_cast<int32_t>(GetCallingPid());
    return AudioPolicyManager::GetInstance().SetPreferOutputDeviceChangeCallback(clientId, callback);
}

int32_t AudioRoutingManager::UnsetPreferOutputDeviceChangeCallback()
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    int32_t clientId = static_cast<int32_t>(GetCallingPid());
    return AudioPolicyManager::GetInstance().UnsetPreferOutputDeviceChangeCallback(clientId);
}
} // namespace AudioStandard
} // namespace OHOS
