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

#ifndef ST_AUDIO_ROUTING_MANAGER_H
#define ST_AUDIO_ROUTING_MANAGER_H

#include <iostream>

#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {
class AudioManagerMicStateChangeCallback {
public:
    virtual ~AudioManagerMicStateChangeCallback() = default;
    /**
     * Called when the microphone state changes
     *
     * @param micStateChangeEvent Microphone Status Information.
     */
    virtual void OnMicStateUpdated(const MicStateChangeEvent &micStateChangeEvent) = 0;
};

class AudioRoutingManager {
public:
    AudioRoutingManager() = default;
    virtual ~AudioRoutingManager() = default;

    static AudioRoutingManager *GetInstance();
    int32_t SetMicStateChangeCallback(const std::shared_ptr<AudioManagerMicStateChangeCallback> &callback);
private:
    uint32_t GetCallingPid();
};
} // namespace AudioStandard
} // namespace OHOS
#endif // ST_AUDIO_ROUTING_MANAGER_H

