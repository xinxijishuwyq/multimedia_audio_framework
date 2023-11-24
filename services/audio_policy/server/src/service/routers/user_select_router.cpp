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

#include "user_select_router.h"
#include "audio_log.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
unique_ptr<AudioDeviceDescriptor> UserSelectRouter::GetMediaRenderDevice(StreamUsage streamUsage, int32_t clientUID)
{
    unique_ptr<AudioDeviceDescriptor> perDev_ =
        AudioStateManager::GetAudioStateManager().GetPerferredMediaRenderDevice();
    unique_ptr<AudioDeviceDescriptor> defaultDevice =
        AudioDeviceManager::GetAudioDeviceManager().GetRenderDefaultDevice();
    vector<unique_ptr<AudioDeviceDescriptor>> publicDevices =
        AudioDeviceManager::GetAudioDeviceManager().GetMediaRenderPublicDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> privacyDevices =
        AudioDeviceManager::GetAudioDeviceManager().GetMediaRenderPrivacyDevices();
    publicDevices.push_back(std::move(defaultDevice));
    publicDevices.insert(publicDevices.end(),
        std::make_move_iterator(privacyDevices.begin()), std::make_move_iterator(privacyDevices.end()));
    if (perDev_->deviceId_ == 0) {
        AUDIO_INFO_LOG(" PerferredMediaRenderDevice is null");
        return make_unique<AudioDeviceDescriptor>();
    } else {
        AUDIO_INFO_LOG(" PerferredMediaRenderDevice deviceId is %{public}d", perDev_->deviceId_);
        return RouterBase::GetPairCaptureDevice(perDev_, publicDevices);
    }
}

unique_ptr<AudioDeviceDescriptor> UserSelectRouter::GetCallRenderDevice(StreamUsage streamUsage, int32_t clientUID)
{
    unique_ptr<AudioDeviceDescriptor> perDev_ =
        AudioStateManager::GetAudioStateManager().GetPerferredCallRenderDevice();
    vector<unique_ptr<AudioDeviceDescriptor>> callDevices =
        AudioDeviceManager::GetAudioDeviceManager().GetAvailableDevicesByUsage(CALL_OUTPUT_DEVICES);
    if (perDev_->deviceId_ == 0) {
        AUDIO_INFO_LOG(" PerferredCallRenderDevice is null");
        return make_unique<AudioDeviceDescriptor>();
    } else {
        AUDIO_INFO_LOG(" PerferredCallRenderDevice deviceId is %{public}d", perDev_->deviceId_);
        return RouterBase::GetPairCaptureDevice(perDev_, callDevices);
    }
}

unique_ptr<AudioDeviceDescriptor> UserSelectRouter::GetCallCaptureDevice(SourceType sourceType, int32_t clientUID)
{
    return make_unique<AudioDeviceDescriptor>();
}

unique_ptr<AudioDeviceDescriptor> UserSelectRouter::GetRingRenderDevice(StreamUsage streamUsage, int32_t clientUID)
{
    return make_unique<AudioDeviceDescriptor>();
}

unique_ptr<AudioDeviceDescriptor> UserSelectRouter::GetRecordCaptureDevice(SourceType sourceType, int32_t clientUID)
{
    return make_unique<AudioDeviceDescriptor>();
}

unique_ptr<AudioDeviceDescriptor> UserSelectRouter::GetToneRenderDevice(StreamUsage streamUsage, int32_t clientUID)
{
    return make_unique<AudioDeviceDescriptor>();
}

} // namespace AudioStandard
} // namespace OHOS