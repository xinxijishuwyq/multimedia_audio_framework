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

#ifndef I_ST_AUDIO_MANAGER_BASE_H
#define I_ST_AUDIO_MANAGER_BASE_H

#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "audio_svc_manager.h"

namespace OHOS {
class IStandardAudioService : public IRemoteBroker {
public:
    /**
     * Set Volume.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual void SetVolume(AudioSvcManager::AudioVolumeType volumeType, int32_t volume) = 0;

    /**
     * Obtains current volume.
     *
     * @return Returns the current volume.
     */
    virtual int GetVolume(AudioSvcManager::AudioVolumeType volumeType) = 0;

    /**
     * Obtains max volume.
     *
     * @return Returns the max volume.
     */
    virtual int GetMaxVolume(AudioSvcManager::AudioVolumeType volumeType) = 0;

    /**
     * Obtains min volume.
     *
     * @return Returns the min volume.
     */
    virtual int GetMinVolume(AudioSvcManager::AudioVolumeType volumeType) = 0;

    /**
     * Obtains device array.
     *
     * @return Returns the array of audio device descriptor.
     */
    virtual std::vector<sptr<AudioDeviceDescriptor>> GetDevices(AudioDeviceDescriptor::DeviceFlag deviceFlag) = 0;

    enum {
        SET_VOLUME = 0,
        GET_VOLUME = 1,
        GET_MAX_VOLUME = 2,
        GET_MIN_VOLUME = 3,
        GET_DEVICES = 4,
    };

public:
    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardAudioService");
};

class AudioManagerStub : public IRemoteStub<IStandardAudioService> {
public:
    virtual int OnRemoteRequest(uint32_t code, MessageParcel &data,
                                MessageParcel &reply, MessageOption &option) override;
    bool IsPermissionValid();
};
} // namespace OHOS
#endif // I_ST_AUDIO_MANAGER_BASE_H
