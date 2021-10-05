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

#include "audio_policy_manager.h"
#include "audio_policy_proxy.h"
#include "media_log.h"

namespace OHOS {
namespace AudioStandard {
AudioPolicyProxy::AudioPolicyProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IAudioPolicy>(impl)
{
}

void AudioPolicyProxy::WriteAudioInteruptParams(MessageParcel &data, const AudioInterrupt &audioInterrupt)
{
    data.WriteInt32(static_cast<int32_t>(audioInterrupt.streamUsage));
    data.WriteInt32(static_cast<int32_t>(audioInterrupt.contentType));
    data.WriteInt32(static_cast<int32_t>(audioInterrupt.streamType));
    data.WriteInt32(audioInterrupt.sessionID);
}

int32_t AudioPolicyProxy::SetStreamVolume(AudioStreamType streamType, float volume)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int32_t>(streamType));
    data.WriteFloat(volume);

    int32_t error = Remote()->SendRequest(SET_STREAM_VOLUME, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("set volume failed, error: %d", error);
        return error;
    }
    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::SetRingerMode(AudioRingerMode ringMode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int>(ringMode));

    int32_t error = Remote()->SendRequest(SET_RINGER_MODE, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("set ringermode failed, error: %d", error);
        return error;
    }

    return reply.ReadInt32();
}

AudioRingerMode AudioPolicyProxy::GetRingerMode()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    int32_t error = Remote()->SendRequest(GET_RINGER_MODE, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("get ringermode failed, error: %d", error);
    }
    return static_cast<AudioRingerMode>(reply.ReadInt32());
}

float AudioPolicyProxy::GetStreamVolume(AudioStreamType streamType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int32_t>(streamType));

    int32_t error = Remote()->SendRequest(GET_STREAM_VOLUME, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("get volume failed, error: %d", error);
        return error;
    }
    return reply.ReadFloat();
}

int32_t AudioPolicyProxy::SetStreamMute(AudioStreamType streamType, bool mute)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int32_t>(streamType));
    data.WriteBool(mute);

    int32_t error = Remote()->SendRequest(SET_STREAM_MUTE, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("set mute failed, error: %d", error);
        return error;
    }
    return reply.ReadInt32();
}

bool AudioPolicyProxy::GetStreamMute(AudioStreamType streamType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int32_t>(streamType));

    int32_t error = Remote()->SendRequest(GET_STREAM_MUTE, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("get mute failed, error: %d", error);
        return error;
    }
    return reply.ReadBool();
}

bool AudioPolicyProxy::IsStreamActive(AudioStreamType streamType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int32_t>(streamType));

    int32_t error = Remote()->SendRequest(IS_STREAM_ACTIVE, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("isStreamActive failed, error: %d", error);
        return false;
    }
    return reply.ReadBool();
}

int32_t AudioPolicyProxy::SetDeviceActive(InternalDeviceType deviceType, bool active)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int32_t>(deviceType));
    data.WriteBool(active);

    int32_t error = Remote()->SendRequest(SET_DEVICE_ACTIVE, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("set device active failed, error: %d", error);
        return error;
    }
    return reply.ReadInt32();
}

bool AudioPolicyProxy::IsDeviceActive(InternalDeviceType deviceType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int32_t>(deviceType));

    int32_t error = Remote()->SendRequest(IS_DEVICE_ACTIVE, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("is device active failed, error: %d", error);
        return false;
    }
    return reply.ReadBool();
}

int32_t AudioPolicyProxy::SetAudioManagerCallback(const AudioStreamType streamType, const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (object == nullptr) {
        MEDIA_ERR_LOG("AudioPolicyProxy: SetAudioManagerCallback object is null");
        return ERR_NULL_OBJECT;
    }

    data.WriteInt32(static_cast<int32_t>(streamType));
    (void)data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(SET_CALLBACK, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("AudioPolicyProxy: set callback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UnsetAudioManagerCallback(const AudioStreamType streamType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInt32(static_cast<int32_t>(streamType));
    int error = Remote()->SendRequest(UNSET_CALLBACK, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("AudioPolicyProxy: unset callback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::ActivateAudioInterrupt(const AudioInterrupt &audioInterrupt)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    WriteAudioInteruptParams(data, audioInterrupt);
    int error = Remote()->SendRequest(ACTIVATE_INTERRUPT, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("AudioPolicyProxy: activate interrupt failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::DeactivateAudioInterrupt(const AudioInterrupt &audioInterrupt)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    WriteAudioInteruptParams(data, audioInterrupt);
    int error = Remote()->SendRequest(DEACTIVATE_INTERRUPT, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("AudioPolicyProxy: deactivate interrupt failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}
} // namespace AudioStandard
} // namespace OHOS
