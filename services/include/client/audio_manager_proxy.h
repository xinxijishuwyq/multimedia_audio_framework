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

#ifndef ST_AUDIO_MANAGER_PROXY_H
#define ST_AUDIO_MANAGER_PROXY_H

#include "iremote_proxy.h"
#include "audio_svc_manager.h"
#include "audio_manager_base.h"
#include "audio_device_descriptor.h"

namespace OHOS {
class AudioManagerProxy : public IRemoteProxy<IStandardAudioService> {
public:
    explicit AudioManagerProxy(const sptr<IRemoteObject> &impl);
    virtual ~AudioManagerProxy() = default;

    void SetVolume(AudioSvcManager::AudioVolumeType volumeType, int32_t volume) override;
    int32_t GetVolume(AudioSvcManager::AudioVolumeType volumeType) override;
    int32_t GetMaxVolume(AudioSvcManager::AudioVolumeType volumeType) override;
    int32_t GetMinVolume(AudioSvcManager::AudioVolumeType volumeType) override;
    std::vector<sptr<AudioDeviceDescriptor>> GetDevices(AudioDeviceDescriptor::DeviceFlag deviceFlag) override;
private:
    static inline BrokerDelegator<AudioManagerProxy> delegator_;
};
} // namespace OHOS
#endif // ST_AUDIO_MANAGER_PROXY_H
