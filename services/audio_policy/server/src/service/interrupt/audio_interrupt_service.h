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

#ifndef ST_AUDIO_INTERRUPT_SERVICE_H
#define ST_AUDIO_INTERRUPT_SERVICE_H

#include <mutex>

#include "i_audio_interrupt_interface.h"
#include "audio_interrupt_info.h"

namespace OHOS {
namespace AudioStandard {

class AudioInterruptService : public IAudioInterruptInteface {
public:
    static AudioInterruptService& GetService()
    {
        static AudioInterruptService service;
        return service;
    }

    // deprecated interrupt interfaces
    int32_t SetAudioManagerInterruptCallback(const sptr<IRemoteObject> &object);
    int32_t UnsetAudioManagerInterruptCallback();
    int32_t RequestAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt);
    int32_t AbandonAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt);

    // modern interrupt interfaces
    int32_t SetAudioInterruptCallback(const int32_t zoneId, const uint32_t sessionId,
        const sptr<IRemoteObject> &object);
    int32_t UnsetAudioInterruptCallback(const int32_t zoneId, const uint32_t sessionId);
    int32_t ActivateAudioInterrupt(const int32_t zoneId, const AudioInterrupt &audioInterrupt);
    int32_t DeactivateAudioInterrupt(const int32_t zoneId, const AudioInterrupt &audioInterrupt);

    // zone debug interfaces
    int32_t CreateAudioInterruptZone(const int32_t zoneId, const set<int32_t> pids);
    int32_t ReleaseAudioInterruptZone(const int32_t zoneId);
    int32_t AddAudioInterruptZonePids(const int32_t zoneId, const set<int32_t> pids);
    int32_t RemoveAudioInterruptZonePids(const int32_t zoneId, const set<int32_t> pids);

    // info interfaces
    int32_t GetAudioFocusInfoList(const int32_t zoneId,
        std::list<std::pair<AudioInterrupt, AudioFocuState>> &focusInfoList);
    int32_t SetAudioFocusInfoCallback(const int32_t zoneId, const sptr<IRemoteObject> &object);

private:
    AudioInterruptService();
    virtual ~AudioInterruptService();

    std::mutex mutex_;
};
} // namespace AudioStandard
} // namespace OHOS

#endif // ST_AUDIO_INTERRUPT_SERVICE_H
