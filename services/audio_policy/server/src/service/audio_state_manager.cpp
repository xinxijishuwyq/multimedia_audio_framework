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

#include "audio_state_manager.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {

void AudioStateManager::SetPreferredMediaRenderDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    preferredMediaRenderDevice_ = deviceDescriptor;
}

void AudioStateManager::SetPreferredCallRenderDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    preferredCallRenderDevice_ = deviceDescriptor;
}

void AudioStateManager::SetPreferredCallCaptureDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    std::lock_guard<std::mutex> lock(mutex_);
    preferredCallCaptureDevice_ = deviceDescriptor;
}

void AudioStateManager::SetPreferredRingRenderDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    preferredRingRenderDevice_ = deviceDescriptor;
}

void AudioStateManager::SetPreferredRecordCaptureDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    preferredRecordCaptureDevice_ = deviceDescriptor;
}

void AudioStateManager::SetPreferredToneRenderDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    preferredToneRenderDevice_ = deviceDescriptor;
}

unique_ptr<AudioDeviceDescriptor> AudioStateManager::GetPreferredMediaRenderDevice()
{
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(preferredMediaRenderDevice_);
    return devDesc;
}

unique_ptr<AudioDeviceDescriptor> AudioStateManager::GetPreferredCallRenderDevice()
{
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(preferredCallRenderDevice_);
    return devDesc;
}

unique_ptr<AudioDeviceDescriptor> AudioStateManager::GetPreferredCallCaptureDevice()
{
    std::lock_guard<std::mutex> lock(mutex_);
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(preferredCallCaptureDevice_);
    return devDesc;
}

unique_ptr<AudioDeviceDescriptor> AudioStateManager::GetPreferredRingRenderDevice()
{
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(preferredRingRenderDevice_);
    return devDesc;
}

unique_ptr<AudioDeviceDescriptor> AudioStateManager::GetPreferredRecordCaptureDevice()
{
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(preferredRecordCaptureDevice_);
    return devDesc;
}

unique_ptr<AudioDeviceDescriptor> AudioStateManager::GetPreferredToneRenderDevice()
{
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(preferredToneRenderDevice_);
    return devDesc;
}

} // namespace AudioStandard
} // namespace OHOS

