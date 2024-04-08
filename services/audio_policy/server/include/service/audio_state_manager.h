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

#ifndef ST_AUDIO_STATE_MANAGER_H
#define ST_AUDIO_STATE_MANAGER_H

#include "audio_system_manager.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;

class AudioStateManager {
public:
    static AudioStateManager& GetAudioStateManager()
    {
        static AudioStateManager audioStateManager;
        return audioStateManager;
    }

    // Set media render device selected by the user
    void SetPerferredMediaRenderDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor);

    // Set call render device selected by the user
    void SetPerferredCallRenderDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor);

    // Set call capture device selected by the user
    void SetPerferredCallCaptureDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor);

    // Set ring render device selected by the user
    void SetPerferredRingRenderDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor);

    // Set record capture device selected by the user
    void SetPerferredRecordCaptureDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor);

    // Set tone render device selected by the user
    void SetPerferredToneRenderDevice(const sptr<AudioDeviceDescriptor> &deviceDescriptor);

    // Get media render device selected by the user
    unique_ptr<AudioDeviceDescriptor> GetPerferredMediaRenderDevice();

    // Get call render device selected by the user
    unique_ptr<AudioDeviceDescriptor> GetPerferredCallRenderDevice();

    // Get call capture device selected by the user
    unique_ptr<AudioDeviceDescriptor> GetPerferredCallCaptureDevice();

    // Get ring render device selected by the user
    unique_ptr<AudioDeviceDescriptor> GetPerferredRingRenderDevice();

    // Get record capture device selected by the user
    unique_ptr<AudioDeviceDescriptor> GetPerferredRecordCaptureDevice();

    // Get tone render device selected by the user
    unique_ptr<AudioDeviceDescriptor> GetPerferredToneRenderDevice();

private:
    AudioStateManager() {};
    ~AudioStateManager() {};
    sptr<AudioDeviceDescriptor> perferredMediaRenderDevice_ = new(std::nothrow) AudioDeviceDescriptor();
    sptr<AudioDeviceDescriptor> perferredCallRenderDevice_ = new(std::nothrow) AudioDeviceDescriptor();
    sptr<AudioDeviceDescriptor> perferredCallCaptureDevice_ = new(std::nothrow) AudioDeviceDescriptor();
    sptr<AudioDeviceDescriptor> perferredRingRenderDevice_ = new(std::nothrow) AudioDeviceDescriptor();
    sptr<AudioDeviceDescriptor> perferredRecordCaptureDevice_ = new(std::nothrow) AudioDeviceDescriptor();
    sptr<AudioDeviceDescriptor> perferredToneRenderDevice_ = new(std::nothrow) AudioDeviceDescriptor();
    std::mutex mutex_;
};

} // namespace AudioStandard
} // namespace OHOS

#endif // ST_AUDIO_STATE_MANAGER_H

