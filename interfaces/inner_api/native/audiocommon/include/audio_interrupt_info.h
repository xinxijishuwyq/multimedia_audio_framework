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
#ifndef AUDIO_INTERRUPT_INFO_H
#define AUDIO_INTERRUPT_INFO_H

#include <parcel.h>
#include <audio_stream_info.h>
#include <audio_source_type.h>

namespace OHOS {
namespace AudioStandard {
enum ActionTarget {
    CURRENT = 0,
    INCOMING,
    BOTH
};

enum AudioFocuState {
    ACTIVE = 0,
    DUCK,
    PAUSE,
    STOP
};

enum InterruptMode {
    SHARE_MODE = 0,
    INDEPENDENT_MODE = 1
};

/**
 * Enumerates the audio interrupt request type.
 */
enum InterruptRequestType {
    INTERRUPT_REQUEST_TYPE_DEFAULT = 0,
};

/**
 * Enumerates audio interrupt request result type.
 */
enum InterruptRequestResultType {
    INTERRUPT_REQUEST_GRANT = 0,
    INTERRUPT_REQUEST_REJECT = 1
};

enum InterruptType {
    INTERRUPT_TYPE_BEGIN = 1,
    INTERRUPT_TYPE_END = 2,
};

enum InterruptHint {
    INTERRUPT_HINT_NONE = 0,
    INTERRUPT_HINT_RESUME,
    INTERRUPT_HINT_PAUSE,
    INTERRUPT_HINT_STOP,
    INTERRUPT_HINT_DUCK,
    INTERRUPT_HINT_UNDUCK
};

enum InterruptForceType {
    /**
     * Force type, system change audio state.
     */
    INTERRUPT_FORCE = 0,
    /**
     * Share type, application change audio state.
     */
    INTERRUPT_SHARE
};

struct InterruptEvent {
    /**
     * Interrupt event type, begin or end
     */
    InterruptType eventType;
    /**
     * Interrupt force type, force or share
     */
    InterruptForceType forceType;
    /**
     * Interrupt hint type. In force type, the audio state already changed,
     * but in share mode, only provide a hint for application to decide.
     */
    InterruptHint hintType;
};

// Used internally only by AudioFramework
struct InterruptEventInternal {
    InterruptType eventType;
    InterruptForceType forceType;
    InterruptHint hintType;
    float duckVolume;
};

enum AudioInterruptChangeType {
    ACTIVATE_AUDIO_INTERRUPT = 0,
    DEACTIVATE_AUDIO_INTERRUPT = 1,
};

// Below APIs are added to handle compilation error in call manager
// Once call manager adapt to new interrupt APIs, this will be removed
enum InterruptActionType {
    TYPE_ACTIVATED = 0,
    TYPE_INTERRUPT = 1
};

struct InterruptAction {
    InterruptActionType actionType;
    InterruptType interruptType;
    InterruptHint interruptHint;
    bool activated;
};

struct AudioFocusEntry {
    InterruptForceType forceType;
    InterruptHint hintType;
    ActionTarget actionOn;
    bool isReject;
};

struct AudioFocusType {
    AudioStreamType streamType;
    SourceType sourceType;
    bool isPlay;
    bool operator==(const AudioFocusType &value) const
    {
        return streamType == value.streamType && sourceType == value.sourceType && isPlay == value.isPlay;
    }

    bool operator<(const AudioFocusType &value) const
    {
        return streamType < value.streamType || (streamType == value.streamType && sourceType < value.sourceType);
    }

    bool operator>(const AudioFocusType &value) const
    {
        return streamType > value.streamType || (streamType == value.streamType && sourceType > value.sourceType);
    }
};

class AudioInterrupt : public Parcelable {
public:
    StreamUsage streamUsage;
    ContentType contentType;
    AudioFocusType audioFocusType;
    uint32_t sessionID;
    bool pauseWhenDucked;
    int32_t pid { -1 };
    InterruptMode mode { SHARE_MODE };
    bool parallelPlayFlag {false};

    AudioInterrupt() = default;
    AudioInterrupt(StreamUsage streamUsage_, ContentType contentType_, AudioFocusType audioFocusType_,
        uint32_t sessionID_) : streamUsage(streamUsage_), contentType(contentType_), audioFocusType(audioFocusType_),
        sessionID(sessionID_) {}
    ~AudioInterrupt() = default;
    bool Marshalling(Parcel &parcel) const override
    {
        return parcel.WriteInt32(static_cast<int32_t>(streamUsage))
            && parcel.WriteInt32(static_cast<int32_t>(contentType))
            && parcel.WriteInt32(static_cast<int32_t>(audioFocusType.streamType))
            && parcel.WriteInt32(static_cast<int32_t>(audioFocusType.sourceType))
            && parcel.WriteBool(audioFocusType.isPlay)
            && parcel.WriteUint32(sessionID)
            && parcel.WriteBool(pauseWhenDucked)
            && parcel.WriteInt32(pid)
            && parcel.WriteInt32(static_cast<int32_t>(mode))
            && parcel.WriteBool(parallelPlayFlag);
    }
    void Unmarshalling(Parcel &parcel)
    {
        streamUsage = static_cast<StreamUsage>(parcel.ReadInt32());
        contentType = static_cast<ContentType>(parcel.ReadInt32());
        audioFocusType.streamType = static_cast<AudioStreamType>(parcel.ReadInt32());
        audioFocusType.sourceType = static_cast<SourceType>(parcel.ReadInt32());
        audioFocusType.isPlay = parcel.ReadBool();
        sessionID = parcel.ReadUint32();
        pauseWhenDucked = parcel.ReadBool();
        pid = parcel.ReadInt32();
        mode = static_cast<InterruptMode>(parcel.ReadInt32());
        parallelPlayFlag = parcel.ReadBool();
    }
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_INTERRUPT_INFO_H