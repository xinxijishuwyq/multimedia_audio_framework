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

#ifndef ST_AUDIO_GROUP_MANAGER_H
#define ST_AUDIO_GROUP_MANAGER_H

#include <cstdlib>
#include <map>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "parcel.h"
#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {
class AudioGroupManager {
public:
    const std::vector<AudioVolumeType> GET_STREAM_ALL_VOLUME_TYPES {
        STREAM_MUSIC,
        STREAM_RING,
        STREAM_VOICE_CALL,
        STREAM_VOICE_ASSISTANT
    };
    AudioGroupManager(int32_t groupId);
    virtual ~AudioGroupManager();
    static float MapVolumeToHDI(int32_t volume);
    static int32_t MapVolumeFromHDI(float volume);
    int32_t SetVolume(AudioVolumeType volumeType, int32_t volume);
    int32_t GetVolume(AudioVolumeType volumeType);
    int32_t GetMaxVolume(AudioVolumeType volumeType);
    int32_t GetMinVolume(AudioVolumeType volumeType);
    int32_t SetMute(AudioVolumeType volumeType, bool mute);
    bool IsStreamMute(AudioVolumeType volumeType);
    void Init();
    bool IsAlived();
    int32_t GetGroupId();
private:
    int32_t groupId_;
    std::string netWorkId_ = LOCAL_NETWORK_ID;
    static constexpr int32_t MAX_VOLUME_LEVEL = 15;
    static constexpr int32_t MIN_VOLUME_LEVEL = 0;
    static constexpr int32_t CONST_FACTOR = 100;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // ST_AUDIO_GROUP_MANAGER_H
