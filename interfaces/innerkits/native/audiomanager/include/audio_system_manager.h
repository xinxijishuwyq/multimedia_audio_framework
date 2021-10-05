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

#ifndef ST_AUDIO_SYSTEM_MANAGER_H
#define ST_AUDIO_SYSTEM_MANAGER_H

#include <cstdlib>

#include "parcel.h"
#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {
class AudioDeviceDescriptor;
class AudioDeviceDescriptor : public Parcelable {
    friend class AudioSystemManager;
public:
    DeviceType getType();
    DeviceRole getRole();
    DeviceType deviceType_;
    DeviceRole deviceRole_;
    AudioDeviceDescriptor();
    virtual ~AudioDeviceDescriptor();
    bool Marshalling(Parcel &parcel) const override;
    static AudioDeviceDescriptor *Unmarshalling(Parcel &parcel);
};

class AudioManagerCallback {
public:
    virtual ~AudioManagerCallback() = default;
    /**
     * Called when an interrupt is received.
     *
     * @param interruptAction Indicates the InterruptAction information needed by client.
     * For details, refer InterruptAction enum in audio_info.h
     */
    virtual void OnInterrupt(const InterruptAction &interruptAction) = 0;
};

/**
 * @brief The AudioSystemManager class is an abstract definition of audio manager.
 *        Provides a series of client/interfaces for audio management
 */

class AudioSystemManager {
public:
    enum AudioVolumeType {
        /**
         * Indicates audio streams default.
         */
        STREAM_DEFAULT = -1,
        /**
         * Indicates audio streams for system sounds.
         */
        STREAM_SYSTEM = 0,
        /**
         * Indicates audio streams for music playback.
         */
        STREAM_MUSIC = 1,
        /**
         * Indicates audio streams for ringtones.
         */
        STREAM_RING = 2,
        /**
         * Indicates audio streams media.
         */
        STREAM_MEDIA = 3,
        /**
         * Indicates audio streams of voices in calls.
         */
        STREAM_VOICE_CALL = 4,
        /**
         * Indicates Audio streams for voice assistant
         */
        STREAM_VOICE_ASSISTANT = 5,
        /**
         * Indicates audio streams for alarms.
         */
        STREAM_ALARM = 6,
        /**
         * Indicates audio streams for notifications.
         */
        STREAM_NOTIFICATION = 7,
        /**
         * Indicates audio streams for voice calls routed through a connected Bluetooth device.
         */
        STREAM_BLUETOOTH_SCO = 8,
        /**
         * Indicates audio streams for enforced audible.
         */
        STREAM_ENFORCED_AUDIBLE = 9,
        /**
         * Indicates audio streams for dual-tone multi-frequency (DTMF) tones.
         */
        STREAM_DTMF = 10,
        /**
         * Indicates audio streams exclusively transmitted through the speaker (text-to-speech) of a device.
         */
        STREAM_TTS =  11,
        /**
         * Indicates audio streams used for prompts in terms of accessibility.
         */
        STREAM_ACCESSIBILITY = 12
    };

    static AudioSystemManager *GetInstance();
    static float MapVolumeToHDI(int32_t volume);
    static int32_t MapVolumeFromHDI(float volume);
    int32_t SetVolume(AudioSystemManager::AudioVolumeType volumeType, int32_t volume) const;
    int32_t GetVolume(AudioSystemManager::AudioVolumeType volumeType) const;
    int32_t GetMaxVolume(AudioSystemManager::AudioVolumeType volumeType) const;
    int32_t GetMinVolume(AudioSystemManager::AudioVolumeType volumeType) const;
    int32_t SetMute(AudioSystemManager::AudioVolumeType volumeType, bool mute) const;
    bool IsStreamMute(AudioSystemManager::AudioVolumeType volumeType) const;
    int32_t SetMicrophoneMute(bool isMute) const;
    bool IsMicrophoneMute(void) const;
    std::vector<sptr<AudioDeviceDescriptor>> GetDevices(DeviceFlag deviceFlag) const;
    const std::string GetAudioParameter(const std::string key) const;
    void SetAudioParameter(const std::string key, const std::string value) const;
    int32_t SetDeviceActive(ActiveDeviceType deviceType, bool flag) const;
    bool IsDeviceActive(ActiveDeviceType deviceType) const;
    bool IsStreamActive(AudioSystemManager::AudioVolumeType volumeType) const;
    bool SetRingerMode(AudioRingerMode ringMode) const;
    AudioRingerMode GetRingerMode() const;
    int32_t SetAudioManagerCallback(const AudioSystemManager::AudioVolumeType streamType,
                                    const std::shared_ptr<AudioManagerCallback> &callback);
    int32_t UnsetAudioManagerCallback(const AudioSystemManager::AudioVolumeType streamType) const;
    int32_t ActivateAudioInterrupt(const AudioInterrupt &audioInterrupt);
    int32_t DeactivateAudioInterrupt(const AudioInterrupt &audioInterrupt) const;
private:
    AudioSystemManager();
    virtual ~AudioSystemManager();
    void init();
    static constexpr int32_t MAX_VOLUME_LEVEL = 15;
    static constexpr int32_t MIN_VOLUME_LEVEL = 0;
    static constexpr int32_t CONST_FACTOR = 100;
    std::shared_ptr<AudioManagerCallback> callback_ = nullptr;
    AudioInterrupt audioInterrupt_ = {STREAM_USAGE_UNKNOWN, CONTENT_TYPE_UNKNOWN, AudioStreamType::STREAM_DEFAULT, 0};
    AudioSystemManager::AudioVolumeType cbStreamType_ = AudioSystemManager::AudioVolumeType::STREAM_DEFAULT;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // ST_AUDIO_SYSTEM_MANAGER_H
