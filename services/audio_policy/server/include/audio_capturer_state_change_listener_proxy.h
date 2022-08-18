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

#ifndef AUDIO_CAPTURER_STATE_CHANGE_LISTENER_PROXY_H
#define AUDIO_CAPTURER_STATE_CHANGE_LISTENER_PROXY_H

#include "audio_stream_manager.h"
#include "i_standard_capturer_state_change_listener.h"

namespace OHOS {
namespace AudioStandard {
class AudioCapturerStateChangeListenerProxy : public IRemoteProxy<IStandardCapturerStateChangeListener> {
public:
    explicit AudioCapturerStateChangeListenerProxy(const sptr<IRemoteObject> &impl);
    virtual ~AudioCapturerStateChangeListenerProxy();
    DISALLOW_COPY_AND_MOVE(AudioCapturerStateChangeListenerProxy);
    void OnCapturerStateChange(const std::vector<std::unique_ptr<AudioCapturerChangeInfo>>
        &audioCapturerChangeInfos) override;

private:
    static inline BrokerDelegator<AudioCapturerStateChangeListenerProxy> delegator_;
    void WriteCapturerChangeInfo(MessageParcel &data, const std::unique_ptr<AudioCapturerChangeInfo>
        &capturerChangeInfo);
};

class AudioCapturerStateChangeListenerCallback : public AudioCapturerStateChangeCallback {
public:
    AudioCapturerStateChangeListenerCallback(const sptr<IStandardCapturerStateChangeListener> &listener,
        bool hasBTPermission);
    virtual ~AudioCapturerStateChangeListenerCallback();
    DISALLOW_COPY_AND_MOVE(AudioCapturerStateChangeListenerCallback);
    void OnCapturerStateChange(const std::vector<std::unique_ptr<AudioCapturerChangeInfo>>
        &audioCapturerChangeInfos) override;
private:
    sptr<IStandardCapturerStateChangeListener> listener_ = nullptr;
    bool hasBTPermission_ = true;
    void UpdateDeviceInfo(const std::vector<std::unique_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos);
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_CAPTURER_STATE_CHANGE_LISTENER_PROXY_H
