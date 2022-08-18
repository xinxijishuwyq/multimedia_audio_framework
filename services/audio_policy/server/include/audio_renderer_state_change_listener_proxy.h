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

#ifndef AUDIO_RENDERER_STATE_CHANGE_LISTENER_PROXY_H
#define AUDIO_RENDERER_STATE_CHANGE_LISTENER_PROXY_H

#include "audio_stream_manager.h"
#include "i_standard_renderer_state_change_listener.h"

namespace OHOS {
namespace AudioStandard {
class AudioRendererStateChangeListenerProxy : public IRemoteProxy<IStandardRendererStateChangeListener> {
public:
    explicit AudioRendererStateChangeListenerProxy(const sptr<IRemoteObject> &impl);
    virtual ~AudioRendererStateChangeListenerProxy();
    DISALLOW_COPY_AND_MOVE(AudioRendererStateChangeListenerProxy);
    void OnRendererStateChange(
        const std::vector<std::unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos) override;

private:
    static inline BrokerDelegator<AudioRendererStateChangeListenerProxy> delegator_;
    void WriteRendererChangeInfo(MessageParcel &data,
        const std::unique_ptr<AudioRendererChangeInfo> &rendererChangeInfo);
};

class AudioRendererStateChangeListenerCallback : public AudioRendererStateChangeCallback {
public:
    AudioRendererStateChangeListenerCallback(const sptr<IStandardRendererStateChangeListener> &listener,
        bool hasBTPermission);
    virtual ~AudioRendererStateChangeListenerCallback();
    DISALLOW_COPY_AND_MOVE(AudioRendererStateChangeListenerCallback);
    void OnRendererStateChange(
        const std::vector<std::unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos) override;
private:
    sptr<IStandardRendererStateChangeListener> listener_ = nullptr;
    bool hasBTPermission_ = true;
    void UpdateDeviceInfo(const std::vector<std::unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos);
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_RENDERER_STATE_CHANGE_LISTENER_PROXY_H
