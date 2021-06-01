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

#ifndef ST_AUDIO_SERVER_H
#define ST_AUDIO_SERVER_H

#include <mutex>
#include <unordered_map>

#include "iremote_stub.h"
#include "system_ability.h"
#include "audio_svc_manager.h"
#include "audio_manager_base.h"
#include "audio_device_descriptor.h"

namespace OHOS {
class AudioServer : public SystemAbility, public AudioManagerStub {
    DECLARE_SYSTEM_ABILITY(AudioServer);
public:
    DISALLOW_COPY_AND_MOVE(AudioServer);
    explicit AudioServer(int32_t systemAbilityId, bool runOnCreate = true);
    virtual ~AudioServer() = default;
    void OnDump() override;
    void OnStart() override;
    void OnStop() override;
    void SetVolume(AudioSvcManager::AudioVolumeType volumeType, int32_t volume) override;
    int32_t GetVolume(AudioSvcManager::AudioVolumeType volumeType) override;
    int32_t GetMaxVolume(AudioSvcManager::AudioVolumeType volumeType) override;
    int32_t GetMinVolume(AudioSvcManager::AudioVolumeType volumeType) override;
    std::vector<sptr<AudioDeviceDescriptor>> GetDevices(AudioDeviceDescriptor::DeviceFlag deviceFlag) override;
private:
    static const int32_t MAX_VOLUME = 15;
    static const int32_t MIN_VOLUME = 0;
    static std::unordered_map<int, int> AudioStreamVolumeMap;
    std::vector<sptr<AudioDeviceDescriptor>> audioDeviceDescriptor_;
};
} // namespace OHOS
#endif // ST_AUDIO_SERVER_H
