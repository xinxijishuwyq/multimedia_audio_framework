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

#include "audio_manager_proxy.h"
#include "audio_svc_manager.h"
#include "media_log.h"

namespace OHOS {
AudioManagerProxy::AudioManagerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardAudioService>(impl)
{
}

void AudioManagerProxy::SetVolume(AudioSvcManager::AudioVolumeType volumeType, int32_t volume)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int>(volumeType));
    data.WriteInt32(volume);
    int error = Remote()->SendRequest(SET_VOLUME, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("set volume failed, error: %d", error);
        return;
    }
}

int32_t AudioManagerProxy::GetVolume(AudioSvcManager::AudioVolumeType volumeType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int>(volumeType));
    int error = Remote()->SendRequest(GET_VOLUME, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("Get volume failed, error: %d", error);
        return error;
    }
    int volume = reply.ReadInt32();
    return volume;
}

int32_t AudioManagerProxy::GetMaxVolume(AudioSvcManager::AudioVolumeType volumeType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int>(volumeType));
    int error = Remote()->SendRequest(GET_MAX_VOLUME, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("Get max volume failed, error: %d", error);
        return error;
    }
    int volume = reply.ReadInt32();
    return volume;
}

int32_t AudioManagerProxy::GetMinVolume(AudioSvcManager::AudioVolumeType volumeType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int>(volumeType));
    int error = Remote()->SendRequest(GET_MIN_VOLUME, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("Get min volume failed, error: %d", error);
        return error;
    }
    int volume = reply.ReadInt32();
    return volume;
}

std::vector<sptr<AudioDeviceDescriptor>> AudioManagerProxy::GetDevices(AudioDeviceDescriptor::DeviceFlag deviceFlag)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int>(deviceFlag));
    int error = Remote()->SendRequest(GET_DEVICES, data, reply, option);
    std::vector<sptr<AudioDeviceDescriptor>> deviceInfo;
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("Get device failed, error: %d", error);
        return deviceInfo;
    }
    int size = reply.ReadInt32();
    for (int i = 0; i < size; i++) {
        deviceInfo.push_back(AudioDeviceDescriptor::Unmarshalling(reply));
    }
    return deviceInfo;
}
} // namespace OHOS
