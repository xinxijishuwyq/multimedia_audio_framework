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

#ifndef ST_AUDIO_SVC_MANAGER_H
#define ST_AUDIO_SVC_MANAGER_H

#include <cstdlib>

#include "audio_device_descriptor.h"

namespace OHOS {
/**
 * @brief The AudioSvcManager class is an abstract definition of audio manager.
 *        Provides a series of client/interfaces for audio management
 */

class AudioSvcManager {
public:
enum AudioVolumeType {
        /**
         * Indicates audio streams of voices in calls.
         */
        STREAM_VOICE_CALL = 1,
        /**
         * Indicates audio streams for system sounds.
         */
        STREAM_SYSTEM = 2,
        /**
         * Indicates audio streams for ringtones.
         */
        STREAM_RING = 3,
        /**
         * Indicates audio streams for music playback.
         */
        STREAM_MUSIC = 4,
        /**
         * Indicates audio streams for alarms.
         */
        STREAM_ALARM = 5,
        /**
         * Indicates audio streams for notifications.
         */
        STREAM_NOTIFICATION = 6,
        /**
         * Indicates audio streams for voice calls routed through a connected Bluetooth device.
         */
        STREAM_BLUETOOTH_SCO = 7,
        /**
         * Indicates audio streams for dual-tone multi-frequency (DTMF) tones.
         */
        STREAM_DTMF = 8,
        /**
         * Indicates audio streams exclusively transmitted through the speaker (text-to-speech) of a device.
         */
        STREAM_TTS =  9,
        /**
         * Indicates audio streams used for prompts in terms of accessibility.
         */
        STREAM_ACCESSIBILITY = 10
    };
    static AudioSvcManager* GetInstance();
    void SetVolume(AudioSvcManager::AudioVolumeType volumeType, int32_t volume);
    int GetVolume(AudioSvcManager::AudioVolumeType volumeType);
    int GetMaxVolume(AudioSvcManager::AudioVolumeType volumeType);
    int GetMinVolume(AudioSvcManager::AudioVolumeType volumeType);
    std::vector<sptr<AudioDeviceDescriptor>> GetDevices(AudioDeviceDescriptor::DeviceFlag deviceFlag);
private:
    AudioSvcManager();
    virtual ~AudioSvcManager();
    void init();
};
} // namespace OHOS
#endif // ST_AUDIO_SVC_MANAGER_H
