/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef AUDIO_POLICY_SERVICE_PROXY_H
#define AUDIO_POLICY_SERVICE_PROXY_H

#include "iremote_proxy.h"
#include "audio_system_manager.h"
#include "audio_manager_base.h"

namespace OHOS {
namespace AudioStandard {
class AudioPolicyServiceProxy : public IRemoteProxy<IStandardAudioService> {
public:
    explicit AudioPolicyServiceProxy(const sptr<IRemoteObject> &impl);
    virtual ~AudioPolicyServiceProxy() = default;
    int32_t GetMaxVolume(AudioVolumeType volumeType) override;
    int32_t GetMinVolume(AudioVolumeType volumeType) override;
    int32_t SetMicrophoneMute(bool isMute) override;
    bool IsMicrophoneMute() override;
    int32_t SetAudioScene(AudioScene audioScene, DeviceType activeDevice) override;
    std::vector<sptr<AudioDeviceDescriptor>> GetDevices(DeviceFlag deviceFlag) override;
    const std::string GetAudioParameter(const std::string &key) override;
	const std::string GetAudioParameter(const std::string& networkId, const AudioParamKey key,
		const std::string& condition) override;
    void SetAudioParameter(const std::string &key, const std::string &value) override;
	void SetAudioParameter(const std::string& networkId, const AudioParamKey key, const std::string& condition,
        const std::string& value) override;

    int32_t UpdateActiveDeviceRoute(DeviceType type, DeviceFlag flag) override;
    const char *RetrieveCookie(int32_t &size) override;
    void NotifyDeviceInfo(std::string networkId, bool connected) override;
    int32_t SetParameterCallback(const sptr<IRemoteObject>& object) override;
private:
    static inline BrokerDelegator<AudioPolicyServiceProxy> delegator_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_POLICY_SERVICE_PROXY_H
