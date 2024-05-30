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

void AudioStateManager::SetPerferredMediaRenderDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    perferredMediaRenderDevice_ = deviceDescriptor;
}

void AudioStateManager::SetPerferredCallRenderDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    perferredCallRenderDevice_ = deviceDescriptor;
}

void AudioStateManager::SetPerferredCallCaptureDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    std::lock_guard<std::mutex> lock(mutex_);
    perferredCallCaptureDevice_ = deviceDescriptor;
}

void AudioStateManager::SetPerferredRingRenderDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    perferredRingRenderDevice_ = deviceDescriptor;
}

void AudioStateManager::SetPerferredRecordCaptureDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    perferredRecordCaptureDevice_ = deviceDescriptor;
}

void AudioStateManager::SetPerferredToneRenderDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    perferredToneRenderDevice_ = deviceDescriptor;
}

unique_ptr<AudioDeviceDescriptor> AudioStateManager::GetPreferredMediaRenderDevice()
{
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(perferredMediaRenderDevice_);
    return devDesc;
}

unique_ptr<AudioDeviceDescriptor> AudioStateManager::GetPreferredCallRenderDevice()
{
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(perferredCallRenderDevice_);
    return devDesc;
}

unique_ptr<AudioDeviceDescriptor> AudioStateManager::GetPerferredCallCaptureDevice()
{
    std::lock_guard<std::mutex> lock(mutex_);
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(perferredCallCaptureDevice_);
    return devDesc;
}

unique_ptr<AudioDeviceDescriptor> AudioStateManager::GetPerferredRingRenderDevice()
{
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(perferredRingRenderDevice_);
    return devDesc;
}

unique_ptr<AudioDeviceDescriptor> AudioStateManager::GetPerferredRecordCaptureDevice()
{
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(perferredRecordCaptureDevice_);
    return devDesc;
}

unique_ptr<AudioDeviceDescriptor> AudioStateManager::GetPerferredToneRenderDevice()
{
    unique_ptr<AudioDeviceDescriptor> devDesc = make_unique<AudioDeviceDescriptor>(perferredToneRenderDevice_);
    return devDesc;
}

} // namespace AudioStandard
} // namespace OHOS

