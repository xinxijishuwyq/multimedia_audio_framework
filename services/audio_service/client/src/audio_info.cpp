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

#include "audio_info.h"

#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
AudioInterrupt::AudioInterrupt(StreamUsage streamUsage_, ContentType contentType_, AudioFocusType audioFocusType_,
    uint32_t sessionID_) : streamUsage(streamUsage_), contentType(contentType_), audioFocusType(audioFocusType_),
    sessionID(sessionID_)
{}

bool AudioInterrupt::Marshalling(Parcel &parcel) const
{
    return parcel.WriteInt32(static_cast<int32_t>(streamUsage))
        && parcel.WriteInt32(static_cast<int32_t>(contentType))
        && parcel.WriteInt32(static_cast<int32_t>(audioFocusType.streamType))
        && parcel.WriteInt32(static_cast<int32_t>(audioFocusType.sourceType))
        && parcel.WriteBool(audioFocusType.isPlay)
        && parcel.WriteUint32(sessionID)
        && parcel.WriteBool(pauseWhenDucked)
        && parcel.WriteInt32(pid)
        && parcel.WriteInt32(static_cast<int32_t>(mode));
}

void AudioInterrupt::Unmarshalling(Parcel &parcel)
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
}

bool WriteDeviceInfo(Parcel &parcel, const DeviceInfo &deviceInfo)
{
    return parcel.WriteInt32(static_cast<int32_t>(deviceInfo.deviceType))
        && parcel.WriteInt32(static_cast<int32_t>(deviceInfo.deviceRole))
        && parcel.WriteInt32(deviceInfo.deviceId)
        && parcel.WriteInt32(deviceInfo.channelMasks)
        && parcel.WriteInt32(deviceInfo.channelIndexMasks)
        && parcel.WriteString(deviceInfo.deviceName)
        && parcel.WriteString(deviceInfo.macAddress)
        && deviceInfo.audioStreamInfo.Marshalling(parcel)
        && parcel.WriteString(deviceInfo.networkId)
        && parcel.WriteString(deviceInfo.displayName)
        && parcel.WriteInt32(deviceInfo.interruptGroupId)
        && parcel.WriteInt32(deviceInfo.volumeGroupId)
        && parcel.WriteBool(deviceInfo.isLowLatencyDevice);
}

void ReadDeviceInfo(Parcel &parcel, DeviceInfo &deviceInfo)
{
    deviceInfo.deviceType = static_cast<DeviceType>(parcel.ReadInt32());
    deviceInfo.deviceRole = static_cast<DeviceRole>(parcel.ReadInt32());
    deviceInfo.deviceId = parcel.ReadInt32();
    deviceInfo.channelMasks = parcel.ReadInt32();
    deviceInfo.channelIndexMasks = parcel.ReadInt32();
    deviceInfo.deviceName = parcel.ReadString();
    deviceInfo.macAddress = parcel.ReadString();
    deviceInfo.audioStreamInfo.Unmarshalling(parcel);
    deviceInfo.networkId = parcel.ReadString();
    deviceInfo.displayName = parcel.ReadString();
    deviceInfo.interruptGroupId = parcel.ReadInt32();
    deviceInfo.volumeGroupId = parcel.ReadInt32();
    deviceInfo.isLowLatencyDevice = parcel.ReadBool();
}

AudioRendererChangeInfo::AudioRendererChangeInfo(const AudioRendererChangeInfo &audioRendererChangeInfo)
{
    *this = audioRendererChangeInfo;
}

bool AudioRendererChangeInfo::Marshalling(Parcel &parcel) const
{
    return parcel.WriteInt32(createrUID)
        && parcel.WriteInt32(clientUID)
        && parcel.WriteInt32(sessionId)
        && parcel.WriteInt32(tokenId)
        && parcel.WriteInt32(static_cast<int32_t>(rendererInfo.contentType))
        && parcel.WriteInt32(static_cast<int32_t>(rendererInfo.streamUsage))
        && parcel.WriteInt32(rendererInfo.rendererFlags)
        && parcel.WriteInt32(static_cast<int32_t>(rendererState))
        && WriteDeviceInfo(parcel, outputDeviceInfo);
}

void AudioRendererChangeInfo::Unmarshalling(Parcel &parcel)
{
    createrUID = parcel.ReadInt32();
    clientUID = parcel.ReadInt32();
    sessionId = parcel.ReadInt32();
    tokenId = parcel.ReadInt32();

    rendererInfo.contentType = static_cast<ContentType>(parcel.ReadInt32());
    rendererInfo.streamUsage = static_cast<StreamUsage>(parcel.ReadInt32());
    rendererInfo.rendererFlags = parcel.ReadInt32();

    rendererState = static_cast<RendererState>(parcel.ReadInt32());
    ReadDeviceInfo(parcel, outputDeviceInfo);
}

AudioCapturerChangeInfo::AudioCapturerChangeInfo(const AudioCapturerChangeInfo &audioCapturerChangeInfo)
{
    *this = audioCapturerChangeInfo;
}

AudioCapturerChangeInfo::AudioCapturerChangeInfo()
{}

AudioCapturerChangeInfo::~AudioCapturerChangeInfo()
{}

bool AudioCapturerChangeInfo::Marshalling(Parcel &parcel) const
{
    return parcel.WriteInt32(createrUID)
        && parcel.WriteInt32(clientUID)
        && parcel.WriteInt32(sessionId)
        && capturerInfo.Marshalling(parcel)
        && parcel.WriteInt32(static_cast<int32_t>(capturerState))
        && WriteDeviceInfo(parcel, inputDeviceInfo)
        && parcel.WriteBool(muted);
}

void AudioCapturerChangeInfo::Unmarshalling(Parcel &parcel)
{
    createrUID = parcel.ReadInt32();
    clientUID = parcel.ReadInt32();
    sessionId = parcel.ReadInt32();
    capturerInfo.Unmarshalling(parcel);
    capturerState = static_cast<CapturerState>(parcel.ReadInt32());
    ReadDeviceInfo(parcel, inputDeviceInfo);
    muted = parcel.ReadBool();
}

AudioCapturerInfo::AudioCapturerInfo(const AudioCapturerInfo &audioCapturerInfo)
{
    *this = audioCapturerInfo;
}

bool AudioCapturerInfo::Marshalling(Parcel &parcel) const
{
    return parcel.WriteInt32(static_cast<int32_t>(sourceType))
        && parcel.WriteInt32(capturerFlags);
}

void AudioCapturerInfo::Unmarshalling(Parcel &parcel)
{
    sourceType = static_cast<SourceType>(parcel.ReadInt32());
    capturerFlags = parcel.ReadInt32();
}

#ifdef FEATURE_DTMF_TONE
bool ToneSegment::Marshalling(Parcel &parcel) const
{
    parcel.WriteUint32(duration);
    parcel.WriteUint16(loopCnt);
    parcel.WriteUint16(loopIndx);
    for (uint32_t i = 0; i < TONEINFO_MAX_WAVES + 1; i++) {
        parcel.WriteUint16(waveFreq[i]);
    }
    return true;
}

void ToneSegment::Unmarshalling(Parcel &parcel)
{
    duration = parcel.ReadUint32();
    loopCnt = parcel.ReadUint16();
    loopIndx = parcel.ReadUint16();
    AUDIO_DEBUG_LOG("duration: %{public}d, loopCnt: %{public}d, loopIndx: %{public}d", duration, loopCnt, loopIndx);
    for (uint32_t i = 0; i < TONEINFO_MAX_WAVES + 1; i++) {
        waveFreq[i] = parcel.ReadUint16();
        AUDIO_DEBUG_LOG("wave[%{public}d]: %{public}d", i, waveFreq[i]);
    }
}

bool ToneInfo::Marshalling(Parcel &parcel) const
{
    parcel.WriteUint32(segmentCnt);
    parcel.WriteUint32(repeatCnt);
    parcel.WriteUint32(repeatSegment);
    for (uint32_t i = 0; i < segmentCnt; i++) {
        segments[i].Marshalling(parcel);
    }
    return true;
}

void ToneInfo::Unmarshalling(Parcel &parcel)
{
    segmentCnt = parcel.ReadUint32();
    repeatCnt = parcel.ReadUint32();
    repeatSegment = parcel.ReadUint32();
    AUDIO_DEBUG_LOG("segmentCnt: %{public}d, repeatCnt: %{public}d, repeatSegment: %{public}d",
        segmentCnt, repeatCnt, repeatSegment);
    for (uint32_t i = 0; i < segmentCnt; i++) {
        segments[i].Unmarshalling(parcel);
    }
}
#endif
} // namespace AudioStandard
} // namespace OHOS