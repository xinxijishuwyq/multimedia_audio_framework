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
#ifndef AUDIO_STREAM_INFO_H
#define AUDIO_STREAM_INFO_H

namespace OHOS {
namespace AudioStandard {
enum AudioStreamType {
    /**
     * Indicates audio streams default.
     */
    STREAM_DEFAULT = -1,
    /**
     * Indicates audio streams of voices in calls.
     */
    STREAM_VOICE_CALL = 0,
    /**
     * Indicates audio streams for music.
     */
    STREAM_MUSIC = 1,
    /**
     * Indicates audio streams for ringtones.
     */
    STREAM_RING = 2,
    /**
     * Indicates audio streams for media.
     * Deprecated
     */
    STREAM_MEDIA = 3,
    /**
     * Indicates audio streams used for voice assistant and text-to-speech (TTS).
     */
    STREAM_VOICE_ASSISTANT = 4,
    /**
     * Indicates audio streams for system sounds.
     */
    STREAM_SYSTEM = 5,
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
     * Deprecated
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
     * Deprecated
     */
    STREAM_TTS =  11,
    /**
     * Indicates audio streams used for prompts in terms of accessibility.
     */
    STREAM_ACCESSIBILITY = 12,
    /**
     * Indicates special scene used for recording.
     * Deprecated
     */
    STREAM_RECORDING = 13,
    /**
     * Indicates audio streams for movie.
     * New
     */
    STREAM_MOVIE = 14,
    /**
     * Indicates audio streams for game.
     * New
     */
    STREAM_GAME = 15,
    /**
     * Indicates audio streams for speech.
     * New
     */
    STREAM_SPEECH = 16,
    /**
     * Indicates audio streams for enforced audible.
     * New
     */
    STREAM_SYSTEM_ENFORCED = 17,
    /**
     * Indicates audio streams used for ultrasonic ranging.
     */
    STREAM_ULTRASONIC = 18,
    /**
     * Indicates audio streams for wakeup.
     */
    STREAM_WAKEUP = 19,
    /**
     * Indicates audio streams for voice message.
     */
    STREAM_VOICE_MESSAGE = 20,
    /**
     * Indicates audio streams for navigation.
     */
    STREAM_NAVIGATION = 21,
    /**
     * Indicates the max value of audio stream type (except STREAM_ALL).
     */
    STREAM_TYPE_MAX = STREAM_NAVIGATION,

    /**
     * Indicates audio streams used for only one volume bar of a device.
     */
    STREAM_ALL = 100
};

/**
* Enumerates the stream usage.
*/
enum StreamUsage {
    STREAM_USAGE_UNKNOWN = 0,
    STREAM_USAGE_MEDIA = 1,
    STREAM_USAGE_MUSIC = 1,
    STREAM_USAGE_VOICE_COMMUNICATION = 2,
    STREAM_USAGE_VOICE_ASSISTANT = 3,
    STREAM_USAGE_ALARM = 4,
    STREAM_USAGE_VOICE_MESSAGE = 5,
    STREAM_USAGE_NOTIFICATION_RINGTONE = 6,
    STREAM_USAGE_RINGTONE = 6,
    STREAM_USAGE_NOTIFICATION = 7,
    STREAM_USAGE_ACCESSIBILITY = 8,
    STREAM_USAGE_SYSTEM = 9,
    STREAM_USAGE_MOVIE = 10,
    STREAM_USAGE_GAME = 11,
    STREAM_USAGE_AUDIOBOOK = 12,
    STREAM_USAGE_NAVIGATION = 13,
    STREAM_USAGE_DTMF = 14,
    STREAM_USAGE_ENFORCED_TONE = 15,
    STREAM_USAGE_ULTRASONIC = 16,
    //other StreamUsage
    STREAM_USAGE_RANGING,
    STREAM_USAGE_VOICE_MODEM_COMMUNICATION
};

/**
* Enumerates the audio content type.
*/
enum ContentType {
    CONTENT_TYPE_UNKNOWN = 0,
    CONTENT_TYPE_SPEECH = 1,
    CONTENT_TYPE_MUSIC = 2,
    CONTENT_TYPE_MOVIE = 3,
    CONTENT_TYPE_SONIFICATION = 4,
    CONTENT_TYPE_RINGTONE = 5,
    // other ContentType
    CONTENT_TYPE_PROMPT = 6,
    CONTENT_TYPE_GAME = 7,
    CONTENT_TYPE_DTMF = 8,
    CONTENT_TYPE_ULTRASONIC = 9
};

struct AudioStreamParams {
    uint32_t samplingRate;
    uint8_t encoding;
    uint8_t format;
    uint8_t channels;
};

// sampling rate
enum AudioSamplingRate {
    SAMPLE_RATE_8000 = 8000,
    SAMPLE_RATE_11025 = 11025,
    SAMPLE_RATE_12000 = 12000,
    SAMPLE_RATE_16000 = 16000,
    SAMPLE_RATE_22050 = 22050,
    SAMPLE_RATE_24000 = 24000,
    SAMPLE_RATE_32000 = 32000,
    SAMPLE_RATE_44100 = 44100,
    SAMPLE_RATE_48000 = 48000,
    SAMPLE_RATE_64000 = 64000,
    SAMPLE_RATE_96000 = 96000
};

enum AudioEncodingType {
    ENCODING_PCM = 0,
    ENCODING_INVALID = -1
};


// format
enum AudioSampleFormat {
    SAMPLE_U8 = 0,
    SAMPLE_S16LE = 1,
    SAMPLE_S24LE = 2,
    SAMPLE_S32LE = 3,
    SAMPLE_F32LE = 4,
    INVALID_WIDTH = -1
};

// channel
enum AudioChannel {
    MONO = 1,
    STEREO = 2,
    CHANNEL_3 = 3,
    CHANNEL_4 = 4,
    CHANNEL_5 = 5,
    CHANNEL_6 = 6,
    CHANNEL_7 = 7,
    CHANNEL_8 = 8
};

// Supported audio parameters for both renderer and capturer
const std::vector<AudioSampleFormat> AUDIO_SUPPORTED_FORMATS {
    SAMPLE_U8,
    SAMPLE_S16LE,
    SAMPLE_S24LE,
    SAMPLE_S32LE
};

const std::vector<AudioChannel> RENDERER_SUPPORTED_CHANNELS {
    MONO,
    STEREO,
    CHANNEL_3,
    CHANNEL_4,
    CHANNEL_5,
    CHANNEL_6,
    CHANNEL_7,
    CHANNEL_8
};

const std::vector<AudioChannel> CAPTURER_SUPPORTED_CHANNELS {
    MONO,
    STEREO,
    CHANNEL_3,
    CHANNEL_4,
    CHANNEL_5,
    CHANNEL_6
};


const std::vector<AudioEncodingType> AUDIO_SUPPORTED_ENCODING_TYPES {
    ENCODING_PCM
};

const std::vector<AudioSamplingRate> AUDIO_SUPPORTED_SAMPLING_RATES {
    SAMPLE_RATE_8000,
    SAMPLE_RATE_11025,
    SAMPLE_RATE_12000,
    SAMPLE_RATE_16000,
    SAMPLE_RATE_22050,
    SAMPLE_RATE_24000,
    SAMPLE_RATE_32000,
    SAMPLE_RATE_44100,
    SAMPLE_RATE_48000,
    SAMPLE_RATE_64000,
    SAMPLE_RATE_96000
};

const std::vector<StreamUsage> AUDIO_SUPPORTED_STREAM_USAGES {
    STREAM_USAGE_UNKNOWN,
    STREAM_USAGE_MEDIA,
    STREAM_USAGE_MUSIC,
    STREAM_USAGE_VOICE_COMMUNICATION,
    STREAM_USAGE_VOICE_ASSISTANT,
    STREAM_USAGE_ALARM,
    STREAM_USAGE_VOICE_MESSAGE,
    STREAM_USAGE_NOTIFICATION_RINGTONE,
    STREAM_USAGE_RINGTONE,
    STREAM_USAGE_NOTIFICATION,
    STREAM_USAGE_ACCESSIBILITY,
    STREAM_USAGE_SYSTEM,
    STREAM_USAGE_MOVIE,
    STREAM_USAGE_GAME,
    STREAM_USAGE_AUDIOBOOK,
    STREAM_USAGE_NAVIGATION,
    STREAM_USAGE_DTMF,
    STREAM_USAGE_ENFORCED_TONE,
    STREAM_USAGE_ULTRASONIC,
    STREAM_USAGE_RANGING,
    STREAM_USAGE_VOICE_MODEM_COMMUNICATION
};

// Supported audio parameters for fast audio stream
const std::vector<AudioSamplingRate> AUDIO_FAST_STREAM_SUPPORTED_SAMPLING_RATES {
    SAMPLE_RATE_48000,
};

const std::vector<AudioChannel> AUDIO_FAST_STREAM_SUPPORTED_CHANNELS {
    MONO,
    STEREO,
};

const std::vector<AudioSampleFormat> AUDIO_FAST_STREAM_SUPPORTED_FORMATS {
    SAMPLE_S16LE,
    SAMPLE_S32LE
};

struct BufferDesc {
    uint8_t* buffer;
    size_t bufLength;
    size_t dataLength;
};

class AudioStreamInfo {
public:
    AudioSamplingRate samplingRate;
    AudioEncodingType encoding = AudioEncodingType::ENCODING_PCM;
    AudioSampleFormat format;
    AudioChannel channels;
    constexpr AudioStreamInfo(AudioSamplingRate samplingRate_, AudioEncodingType encoding_, AudioSampleFormat format_,
        AudioChannel channels_) : samplingRate(samplingRate_), encoding(encoding_), format(format_), channels(channels_)
    {}
    AudioStreamInfo() = default;
    bool Marshalling(Parcel &parcel) const
    {
        return parcel.WriteInt32(static_cast<int32_t>(samplingRate))
            && parcel.WriteInt32(static_cast<int32_t>(encoding))
            && parcel.WriteInt32(static_cast<int32_t>(format))
            && parcel.WriteInt32(static_cast<int32_t>(channels));
    }
    void Unmarshalling(Parcel &parcel)
    {
        samplingRate = static_cast<AudioSamplingRate>(parcel.ReadInt32());
        encoding = static_cast<AudioEncodingType>(parcel.ReadInt32());
        format = static_cast<AudioSampleFormat>(parcel.ReadInt32());
        channels = static_cast<AudioChannel>(parcel.ReadInt32());
    }
};

struct AudioStreamData {
    AudioStreamInfo streamInfo;
    BufferDesc bufferDesc;
    int32_t volumeStart;
    int32_t volumeEnd;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_STREAM_INFO_H