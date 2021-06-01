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

#include "audio_manager_base.h"
#include "audio_svc_manager.h"
#include "audio_device_descriptor.h"
#include "media_log.h"

namespace OHOS {
int AudioManagerStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    MEDIA_DEBUG_LOG("OnRemoteRequest, cmd = %{public}u", code);
    if (!IsPermissionValid()) {
        MEDIA_ERR_LOG("caller app not acquired audio permission");
        return MEDIA_PERMISSION_DENIED;
    }
    switch (code) {
        case SET_VOLUME: {
            MEDIA_DEBUG_LOG("SET_VOLUME AudioManagerStub");
            int volumeType = data.ReadInt32();
            MEDIA_DEBUG_LOG("SET_VOLUME volumeType received from client= %{public}d", volumeType);
            AudioSvcManager::AudioVolumeType volumeStreamConfig =
                   static_cast<AudioSvcManager::AudioVolumeType>(volumeType);
            MEDIA_DEBUG_LOG("SET_VOLUME volumeType= %{public}d", volumeStreamConfig);
            int vol = data.ReadInt32();
            SetVolume(volumeStreamConfig, vol);
            return MEDIA_OK;
        }
        case GET_VOLUME: {
            MEDIA_DEBUG_LOG("GET_VOLUME AudioManagerStub");
            int volumeType = data.ReadInt32();
            MEDIA_DEBUG_LOG("GET_VOLUME volumeType received from client= %{public}d", volumeType);
            AudioSvcManager::AudioVolumeType volumeStreamConfig =
                   static_cast<AudioSvcManager::AudioVolumeType>(volumeType);
            MEDIA_DEBUG_LOG("GET_VOLUME volumeType= %{public}d", volumeStreamConfig);
            int ret = GetVolume(volumeStreamConfig);
            reply.WriteInt32(ret);
            return MEDIA_OK;
        }
        case GET_MAX_VOLUME: {
            MEDIA_DEBUG_LOG("GET_MAX_VOLUME AudioManagerStub");
            int volumeType = data.ReadInt32();
            MEDIA_DEBUG_LOG("GET_MAX_VOLUME volumeType received from client= %{public}d", volumeType);
            AudioSvcManager::AudioVolumeType volumeStreamConfig =
                   static_cast<AudioSvcManager::AudioVolumeType>(volumeType);
            MEDIA_DEBUG_LOG("GET_MAX_VOLUME volumeType= %{public}d", volumeStreamConfig);
            int ret = GetMaxVolume(volumeStreamConfig);
            reply.WriteInt32(ret);
            return MEDIA_OK;
        }
        case GET_MIN_VOLUME: {
            MEDIA_DEBUG_LOG("GET_MIN_VOLUME AudioManagerStub");
            int volumeType = data.ReadInt32();
            MEDIA_DEBUG_LOG("GET_MIN_VOLUME volumeType received from client= %{public}d", volumeType);
            AudioSvcManager::AudioVolumeType volumeStreamConfig =
                   static_cast<AudioSvcManager::AudioVolumeType>(volumeType);
            MEDIA_DEBUG_LOG("GET_MIN_VOLUME volumeType= %{public}d", volumeStreamConfig);
            int ret = GetMinVolume(volumeStreamConfig);
            reply.WriteInt32(ret);
            return MEDIA_OK;
        }
        case GET_DEVICES: {
            MEDIA_DEBUG_LOG("GET_DEVICES AudioManagerStub");
            int deviceFlag = data.ReadInt32();
            AudioDeviceDescriptor::DeviceFlag deviceFlagConfig =
                   static_cast<AudioDeviceDescriptor::DeviceFlag>(deviceFlag);
            std::vector<sptr<AudioDeviceDescriptor>> devices = GetDevices(deviceFlagConfig);
            int32_t size = devices.size();
            MEDIA_DEBUG_LOG("GET_DEVICES size= %{public}d", size);
            reply.WriteInt32(size);
            for (int i = 0; i < size; i++) {
                devices[i]->Marshalling(reply);
            }
            return MEDIA_OK;
        }
        default: {
            MEDIA_ERR_LOG("default case, need check AudioManagerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

bool AudioManagerStub::IsPermissionValid()
{
    return true;
}
} // namespace OHOS
