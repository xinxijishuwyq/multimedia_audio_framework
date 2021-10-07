/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef ST_AUDIO_POLICY_MANAGER_H
#define ST_AUDIO_POLICY_MANAGER_H

#include <cstdint>
#include "audio_info.h"
#include "audio_policy_manager_listener_stub.h"
#include "audio_system_manager.h"

namespace OHOS {
namespace AudioStandard {
using InternalDeviceType = DeviceType;

class AudioPolicyManager {
public:
    static AudioPolicyManager& GetInstance()
    {
        static AudioPolicyManager policyManager;
        return policyManager;
    }

    int32_t SetStreamVolume(AudioStreamType streamType, float volume);

    float GetStreamVolume(AudioStreamType streamType);

    int32_t SetStreamMute(AudioStreamType streamType, bool mute);

    bool GetStreamMute(AudioStreamType streamType);

    bool IsStreamActive(AudioStreamType streamType);

    int32_t SetDeviceActive(InternalDeviceType deviceType, bool active);

    bool IsDeviceActive(InternalDeviceType deviceType);

    int32_t SetRingerMode(AudioRingerMode ringMode);

    AudioRingerMode GetRingerMode();

    int32_t SetAudioManagerCallback(const AudioStreamType streamType,
                                    const std::shared_ptr<AudioManagerCallback> &callback);

    int32_t UnsetAudioManagerCallback(const AudioStreamType streamType);

    int32_t ActivateAudioInterrupt(const AudioInterrupt &audioInterrupt);

    int32_t DeactivateAudioInterrupt(const AudioInterrupt &audioInterrupt);
private:
    AudioPolicyManager()
    {
        Init();
    }
    ~AudioPolicyManager() {}

    void Init();
    std::shared_ptr<AudioManagerCallback> callback_ = nullptr;
    sptr<AudioPolicyManagerListenerStub> listenerStub_ = nullptr;
};
} // namespce AudioStandard
} // namespace OHOS

#endif // ST_AUDIO_POLICY_MANAGER_H
