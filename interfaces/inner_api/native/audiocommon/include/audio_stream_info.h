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

const uint32_t CH_MODE_OFFSET = 44;
const uint32_t CH_HOA_ORDNUM_OFFSET = 0;
const uint32_t CH_HOA_COMORD_OFFSET = 8;
const uint32_t CH_HOA_NOR_OFFSET = 12;
const uint64_t CH_MODE_MASK = ((1ULL << 4) - 1ULL) << CH_MODE_OFFSET;
const uint64_t CH_HOA_ORDNUM_MASK = ((1ULL << 8) - 1ULL) << CH_HOA_ORDNUM_OFFSET;
const uint64_t CH_HOA_COMORD_MASK = ((1ULL << 4) - 1ULL) << CH_HOA_COMORD_OFFSET;
const uint64_t CH_HOA_NOR_MASK = ((1ULL << 4) - 1ULL) << CH_HOA_NOR_OFFSET;

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
     * Indicates audio streams for ForceStop.
     */
    STREAM_INTERNAL_FORCE_STOP = 22,
    /**
     * Indicates the max value of audio stream type (except STREAM_ALL).
     */
    STREAM_TYPE_MAX = STREAM_INTERNAL_FORCE_STOP,

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
    uint64_t channelLayout = 0ULL;
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
    CHANNEL_8 = 8,
    CHANNEL_9 = 9,
    CHANNEL_10 = 10,
    CHANNEL_11 = 11,
    CHANNEL_12 = 12,
    CHANNEL_13 = 13,
    CHANNEL_14 = 14,
    CHANNEL_15 = 15,
    CHANNEL_16 = 16
};

enum AudioChannelSet: uint64_t {
    FRONT_LEFT = 1ULL << 0U, FRONT_RIGHT = 1ULL << 1U, FRONT_CENTER = 1ULL << 2U,
    LOW_FREQUENCY = 1ULL << 3U,
    BACK_LEFT = 1ULL << 4U, BACK_RIGHT = 1ULL << 5U,
    FRONT_LEFT_OF_CENTER = 1ULL << 6U, FRONT_RIGHT_OF_CENTER = 1ULL << 7U,
    BACK_CENTER = 1ULL << 8U,
    SIDE_LEFT = 1ULL << 9U, SIDE_RIGHT = 1ULL << 10U,
    TOP_CENTER = 1ULL << 11U,
    TOP_FRONT_LEFT = 1ULL << 12U, TOP_FRONT_CENTER = 1ULL << 13U, TOP_FRONT_RIGHT = 1ULL << 14U,
    TOP_BACK_LEFT = 1ULL << 15U, TOP_BACK_CENTER = 1ULL << 16U, TOP_BACK_RIGHT = 1ULL << 17U,
    STEREO_LEFT = 1ULL << 29U, STEREO_RIGHT = 1ULL << 30U,
    WIDE_LEFT = 1ULL << 31U, WIDE_RIGHT = 1ULL << 32U,
    SURROUND_DIRECT_LEFT = 1ULL << 33U, SURROUND_DIRECT_RIGHT = 1ULL << 34U,
    LOW_FREQUENCY_2 = 1ULL << 35U,
    TOP_SIDE_LEFT = 1ULL << 36U, TOP_SIDE_RIGHT = 1ULL << 37U,
    BOTTOM_FRONT_CENTER = 1ULL << 38U, BOTTOM_FRONT_LEFT = 1ULL << 39U, BOTTOM_FRONT_RIGHT = 1ULL << 40U
};

enum ChannelMode: uint64_t {
    CHANNEL_MDOE_NORMAL = 0ULL,
    CHANNEL_MDOE_AMBISONIC = 1ULL,
    CHANNEL_MDOE_BINAURAL = 2ULL
};

enum HOAOrderNumber: uint64_t {
    HOA_ORDNUM_1 = 1ULL,
    HOA_ORDNUM_2 = 2ULL,
    HOA_ORDNUM_3 = 3ULL
};

enum HOAComponentOrdering: uint64_t {
    HOA_COMORD_ACN = 0ULL,
    HOA_COMORD_FUMA = 1ULL,
};

enum HOANormalization: uint64_t {
    HOA_NOR_N3D = 0ULL,
    HOA_NOR_SN3D = 1ULL,
};

enum AudioChannelLayout: uint64_t {
    CH_LAYOUT_UNKNOWN = 0ULL,
    /** Channel count: 1*/
    CH_LAYOUT_MONO = FRONT_CENTER,
    /** Channel count: 2*/
    CH_LAYOUT_STEREO = FRONT_LEFT | FRONT_RIGHT,
    /** Channel count: 3*/
    CH_LAYOUT_2POINT1 = CH_LAYOUT_STEREO | LOW_FREQUENCY,
    CH_LAYOUT_2_1 = CH_LAYOUT_STEREO | BACK_CENTER,
    CH_LAYOUT_SURROUND = CH_LAYOUT_STEREO | FRONT_CENTER,
    /** Channel count: 4*/
    CH_LAYOUT_3POINT1 = CH_LAYOUT_SURROUND | LOW_FREQUENCY,
    CH_LAYOUT_4POINT0 = CH_LAYOUT_SURROUND | BACK_CENTER,
    CH_LAYOUT_2_2 = CH_LAYOUT_STEREO | SIDE_LEFT | SIDE_RIGHT,
    CH_LAYOUT_QUAD = CH_LAYOUT_STEREO | BACK_LEFT | BACK_RIGHT,
    CH_LAYOUT_2POINT0POINT2 = CH_LAYOUT_STEREO | TOP_SIDE_LEFT | TOP_SIDE_RIGHT,
    /** Channel count: 5*/
    CH_LAYOUT_4POINT1 = CH_LAYOUT_4POINT0 | LOW_FREQUENCY,
    CH_LAYOUT_5POINT0 = CH_LAYOUT_SURROUND | SIDE_LEFT | SIDE_RIGHT,
    CH_LAYOUT_5POINT0_BACK = CH_LAYOUT_SURROUND | BACK_LEFT | BACK_RIGHT,
    CH_LAYOUT_2POINT1POINT2 = CH_LAYOUT_2POINT0POINT2 | LOW_FREQUENCY,
    CH_LAYOUT_3POINT0POINT2 = CH_LAYOUT_2POINT0POINT2 | FRONT_CENTER,
    /** Channel count: 6*/
    CH_LAYOUT_5POINT1 = CH_LAYOUT_5POINT0 | LOW_FREQUENCY,
    CH_LAYOUT_5POINT1_BACK = CH_LAYOUT_5POINT0_BACK | LOW_FREQUENCY,
    CH_LAYOUT_6POINT0 = CH_LAYOUT_5POINT0 | BACK_CENTER,
    CH_LAYOUT_HEXAGONAL = CH_LAYOUT_5POINT0_BACK | BACK_CENTER,
    CH_LAYOUT_3POINT1POINT2 = CH_LAYOUT_3POINT1 | TOP_FRONT_LEFT | TOP_FRONT_RIGHT,
    /** Channel count: 7*/
    CH_LAYOUT_6POINT1 = CH_LAYOUT_5POINT1 | BACK_CENTER,
    CH_LAYOUT_6POINT1_BACK = CH_LAYOUT_5POINT1_BACK | BACK_CENTER,
    CH_LAYOUT_7POINT0 = CH_LAYOUT_5POINT0 | BACK_LEFT | BACK_RIGHT,
    /** Channel count: 8*/
    CH_LAYOUT_7POINT1 = CH_LAYOUT_5POINT1 | BACK_LEFT | BACK_RIGHT,
    CH_LAYOUT_OCTAGONAL = CH_LAYOUT_5POINT0 | BACK_CENTER | BACK_LEFT | BACK_RIGHT,
    CH_LAYOUT_5POINT1POINT2 = CH_LAYOUT_5POINT1 | TOP_SIDE_LEFT | TOP_SIDE_RIGHT,
    /** Channel count: 10*/
    CH_LAYOUT_5POINT1POINT4 = CH_LAYOUT_5POINT1 | TOP_FRONT_LEFT | TOP_FRONT_RIGHT | TOP_BACK_LEFT |TOP_BACK_RIGHT,
    CH_LAYOUT_7POINT1POINT2 = CH_LAYOUT_7POINT1 | TOP_SIDE_LEFT | TOP_SIDE_RIGHT,
    /** Channel count: 12*/
    CH_LAYOUT_7POINT1POINT4 = CH_LAYOUT_7POINT1 | TOP_FRONT_LEFT | TOP_FRONT_RIGHT | TOP_BACK_LEFT | TOP_BACK_RIGHT,
    /** Channel count: 14*/
    CH_LAYOUT_9POINT1POINT4 = CH_LAYOUT_7POINT1POINT4 | WIDE_LEFT | WIDE_RIGHT,
    /** Channel count: 16*/
    CH_LAYOUT_9POINT1POINT6 = CH_LAYOUT_9POINT1POINT4 | TOP_SIDE_LEFT | TOP_SIDE_RIGHT,
    /** HOA: fitst order*/
    CH_LAYOUT_HOA_ORDER1_ACN_N3D = (CHANNEL_MDOE_AMBISONIC << CH_MODE_OFFSET) | (HOA_ORDNUM_1 << CH_HOA_ORDNUM_OFFSET)
        | (HOA_COMORD_ACN << CH_HOA_COMORD_OFFSET) | (HOA_NOR_N3D << CH_HOA_NOR_OFFSET),
    CH_LAYOUT_HOA_ORDER1_ACN_SN3D = (CHANNEL_MDOE_AMBISONIC << CH_MODE_OFFSET) | (HOA_ORDNUM_1 << CH_HOA_ORDNUM_OFFSET)
        | (HOA_COMORD_ACN << CH_HOA_COMORD_OFFSET) | (HOA_NOR_SN3D << CH_HOA_NOR_OFFSET),
    CH_LAYOUT_HOA_ORDER1_FUMA = (CHANNEL_MDOE_AMBISONIC << CH_MODE_OFFSET) | (HOA_ORDNUM_1 << CH_HOA_ORDNUM_OFFSET)
        | (HOA_COMORD_FUMA << CH_HOA_COMORD_OFFSET),
    /** HOA: second order*/
    CH_LAYOUT_HOA_ORDER2_ACN_N3D = (CHANNEL_MDOE_AMBISONIC << CH_MODE_OFFSET) | (HOA_ORDNUM_2 << CH_HOA_ORDNUM_OFFSET)
        | (HOA_COMORD_ACN << CH_HOA_COMORD_OFFSET) | (HOA_NOR_N3D << CH_HOA_NOR_OFFSET),
    CH_LAYOUT_HOA_ORDER2_ACN_SN3D = (CHANNEL_MDOE_AMBISONIC << CH_MODE_OFFSET) | (HOA_ORDNUM_2 << CH_HOA_ORDNUM_OFFSET)
        | (HOA_COMORD_ACN << CH_HOA_COMORD_OFFSET) | (HOA_NOR_SN3D << CH_HOA_NOR_OFFSET),
    CH_LAYOUT_HOA_ORDER2_FUMA = (CHANNEL_MDOE_AMBISONIC << CH_MODE_OFFSET) | (HOA_ORDNUM_2 << CH_HOA_ORDNUM_OFFSET)
        | (HOA_COMORD_FUMA << CH_HOA_COMORD_OFFSET),
    /** HOA: third order*/
    CH_LAYOUT_HOA_ORDER3_ACN_N3D = (CHANNEL_MDOE_AMBISONIC << CH_MODE_OFFSET) | (HOA_ORDNUM_3 << CH_HOA_ORDNUM_OFFSET)
        | (HOA_COMORD_ACN << CH_HOA_COMORD_OFFSET) | (HOA_NOR_N3D << CH_HOA_NOR_OFFSET),
    CH_LAYOUT_HOA_ORDER3_ACN_SN3D = (CHANNEL_MDOE_AMBISONIC << CH_MODE_OFFSET) | (HOA_ORDNUM_3 << CH_HOA_ORDNUM_OFFSET)
        | (HOA_COMORD_ACN << CH_HOA_COMORD_OFFSET) | (HOA_NOR_SN3D << CH_HOA_NOR_OFFSET),
    CH_LAYOUT_HOA_ORDER3_FUMA = (CHANNEL_MDOE_AMBISONIC << CH_MODE_OFFSET) | (HOA_ORDNUM_3 << CH_HOA_ORDNUM_OFFSET)
        | (HOA_COMORD_FUMA << CH_HOA_COMORD_OFFSET),
};

const std::vector<AudioChannelLayout> RENDERER_SUPPORTED_CHANNELLAYOUTS {
    CH_LAYOUT_UNKNOWN,
    CH_LAYOUT_MONO,
    CH_LAYOUT_STEREO,
    CH_LAYOUT_2POINT1, CH_LAYOUT_2_1, CH_LAYOUT_SURROUND,
    CH_LAYOUT_3POINT1, CH_LAYOUT_4POINT0, CH_LAYOUT_2_2, CH_LAYOUT_QUAD, CH_LAYOUT_2POINT0POINT2,
    CH_LAYOUT_4POINT1, CH_LAYOUT_5POINT0, CH_LAYOUT_5POINT0_BACK, CH_LAYOUT_2POINT1POINT2, CH_LAYOUT_3POINT0POINT2,
    CH_LAYOUT_5POINT1, CH_LAYOUT_5POINT1_BACK, CH_LAYOUT_6POINT0, CH_LAYOUT_HEXAGONAL, CH_LAYOUT_3POINT1POINT2,
    CH_LAYOUT_6POINT1, CH_LAYOUT_6POINT1_BACK, CH_LAYOUT_7POINT0,
    CH_LAYOUT_7POINT1, CH_LAYOUT_OCTAGONAL, CH_LAYOUT_5POINT1POINT2,
    CH_LAYOUT_5POINT1POINT4, CH_LAYOUT_7POINT1POINT2
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
    CHANNEL_8,
    CHANNEL_10,
    CHANNEL_12,
    CHANNEL_14,
    CHANNEL_16,
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
    AudioChannelLayout channelLayout  = AudioChannelLayout::CH_LAYOUT_UNKNOWN;
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