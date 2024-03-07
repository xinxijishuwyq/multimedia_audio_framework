/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef I_AUDIO_POLICY_INTERFACE_H
#define I_AUDIO_POLICY_INTERFACE_H

#include "audio_module_info.h"
#include "audio_info.h"
#include "audio_policy_manager.h"
#include "audio_policy_ipc_interface_code.h"
#include "audio_session_callback.h"
#include "audio_volume_config.h"

#include <memory>
#include <string>

namespace OHOS {
namespace AudioStandard {
class IAudioPolicyInterface {
public:
    virtual ~IAudioPolicyInterface() {}

    virtual bool Init() = 0;

    virtual void InitKVStore() = 0;

    virtual bool ConnectServiceAdapter() = 0;

    virtual int32_t GetMaxVolumeLevel(AudioVolumeType volumeType) = 0;

    virtual int32_t GetMinVolumeLevel(AudioVolumeType volumeType) = 0;

    virtual int32_t SetSystemVolumeLevel(AudioStreamType streamType, int32_t volumeLevel, bool isFromVolumeKey) = 0;

    virtual int32_t GetSystemVolumeLevel(AudioStreamType streamType, bool isFromVolumeKey) = 0;

    virtual float GetSystemVolumeDb(AudioStreamType streamType) = 0;

    virtual int32_t SetStreamMute(AudioStreamType streamType, bool mute) = 0;

    virtual int32_t SetSourceOutputStreamMute(int32_t uid, bool setMute) = 0;

    virtual bool GetStreamMute(AudioStreamType streamType) = 0;

    virtual std::vector<SinkInfo> GetAllSinks() = 0;

    virtual std::vector<SinkInput> GetAllSinkInputs() = 0;

    virtual std::vector<SourceOutput> GetAllSourceOutputs() = 0;

    virtual AudioIOHandle OpenAudioPort(const AudioModuleInfo &audioPortInfo) = 0;

    virtual AudioIOHandle LoadLoopback(const LoopbackModuleInfo &moduleInfo) = 0;

    virtual int32_t CloseAudioPort(AudioIOHandle ioHandle) = 0;

    virtual int32_t SelectDevice(DeviceRole deviceRole, InternalDeviceType deviceType, std::string name);

    virtual int32_t SetDeviceActive(AudioIOHandle ioHandle, InternalDeviceType deviceType,
                                    std::string name, bool active) = 0;

    virtual int32_t MoveSinkInputByIndexOrName(uint32_t sinkInputId, uint32_t sinkIndex, std::string sinkName) = 0;

    virtual int32_t MoveSourceOutputByIndexOrName(uint32_t sourceOutputId,
        uint32_t sourceIndex, std::string sourceName) = 0;

    virtual int32_t SetRingerMode(AudioRingerMode ringerMode) = 0;

    virtual AudioRingerMode GetRingerMode() const = 0;

    virtual int32_t SetAudioSessionCallback(AudioSessionCallback *callback) = 0;

    virtual int32_t SuspendAudioDevice(std::string &name, bool isSuspend) = 0;

    virtual void SetVolumeForSwitchDevice(InternalDeviceType deviceType) = 0;

    virtual bool SetSinkMute(const std::string &sinkName, bool isMute) = 0;

    virtual float CalculateVolumeDb(int32_t volumeLevel) = 0;

    virtual int32_t SetSystemSoundUri(const std::string &key, const std::string &uri) = 0;

    virtual std::string GetSystemSoundUri(const std::string &key) = 0;

    virtual float GetMinStreamVolume() const = 0;

    virtual float GetMaxStreamVolume() const = 0;
    
    virtual int32_t UpdateSwapDeviceStatus() = 0;

    virtual bool IsVolumeUnadjustable() = 0;

    virtual void GetStreamVolumeInfoMap(StreamVolumeInfoMap &streamVolumeInfos);

    virtual void SetAbsVolumeScene(bool isAbsVolumeScene) = 0;

    virtual bool IsAbsVolumeScene() const = 0;

    virtual float GetSystemVolumeInDb(AudioVolumeType volumeType, int32_t volumeLevel, DeviceType deviceType) = 0;
};
} // namespace AudioStandard
} // namespace OHOS

#endif // I_AUDIO_POLICY_INTERFACE_H
