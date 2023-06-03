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

#ifndef ST_AUDIO_ADAPTER_MANAGER_H
#define ST_AUDIO_ADAPTER_MANAGER_H

#include <list>
#include <unordered_map>

#include "audio_service_adapter.h"
#include "distributed_kv_data_manager.h"
#include "iaudio_policy_interface.h"
#include "types.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
using namespace OHOS::DistributedKv;

class AudioAdapterManager : public IAudioPolicyInterface {
public:
    static constexpr std::string_view HDI_SINK = "libmodule-hdi-sink.z.so";
    static constexpr std::string_view HDI_SOURCE = "libmodule-hdi-source.z.so";
    static constexpr std::string_view PIPE_SINK = "libmodule-pipe-sink.z.so";
    static constexpr std::string_view PIPE_SOURCE = "libmodule-pipe-source.z.so";
    static constexpr std::string_view CLUSTER_SINK = "libmodule-cluster-sink.z.so";
    static constexpr std::string_view EFFECT_SINK = "libmodule-effect-sink.z.so";
    static constexpr uint32_t KVSTORE_CONNECT_RETRY_COUNT = 5;
    static constexpr uint32_t KVSTORE_CONNECT_RETRY_DELAY_TIME = 200000;
    static constexpr float MIN_VOLUME = 0.0f;

    bool Init();
    void Deinit(void);
    void InitKVStore();
    bool ConnectServiceAdapter();

    std::string GetPolicyManagerName();

    static IAudioPolicyInterface& GetInstance()
    {
        static AudioAdapterManager audioAdapterManager;
        return audioAdapterManager;
    }

    virtual ~AudioAdapterManager() {}

    int32_t GetMaxVolumeLevel(AudioVolumeType volumeType);

    int32_t GetMinVolumeLevel(AudioVolumeType volumeType);

    int32_t SetSystemVolumeLevel(AudioStreamType streamType, int32_t volumeLevel, bool isFromVolumeKey = false);

    int32_t GetSystemVolumeLevel(AudioStreamType streamType, bool isFromVolumeKey = false);

    float GetSystemVolumeDb(AudioStreamType streamType);

    int32_t SetStreamMute(AudioStreamType streamType, bool mute);

    int32_t SetSourceOutputStreamMute(int32_t uid, bool setMute);

    bool GetStreamMute(AudioStreamType streamType);

    bool IsStreamActive(AudioStreamType streamType);

    std::vector<SinkInfo> GetAllSinks();

    std::vector<SinkInput> GetAllSinkInputs();

    std::vector<SourceOutput> GetAllSourceOutputs();

    AudioIOHandle OpenAudioPort(const AudioModuleInfo &audioModuleInfo);

    int32_t CloseAudioPort(AudioIOHandle ioHandle);

    int32_t SelectDevice(DeviceRole deviceRole, InternalDeviceType deviceType, std::string name);

    int32_t SetDeviceActive(AudioIOHandle ioHandle, InternalDeviceType deviceType, std::string name, bool active);

    void SetVolumeForSwitchDevice(InternalDeviceType deviceType);

    int32_t MoveSinkInputByIndexOrName(uint32_t sinkInputId, uint32_t sinkIndex, std::string sinkName);

    int32_t MoveSourceOutputByIndexOrName(uint32_t sourceOutputId, uint32_t sourceIndex, std::string sourceName);

    int32_t SetRingerMode(AudioRingerMode ringerMode);

    AudioRingerMode GetRingerMode(void) const;

    int32_t SetAudioSessionCallback(AudioSessionCallback *callback);

    int32_t SuspendAudioDevice(std::string &name, bool isSuspend);

    bool SetSinkMute(const std::string &sinkName, bool isMute);

    float CalculateVolumeDb(int32_t volumeLevel);

    int32_t SetSystemSoundUri(const std::string &key, const std::string &uri);

    std::string GetSystemSoundUri(const std::string &key);

    float GetMinStreamVolume(void) const;

    float GetMaxStreamVolume(void) const;

private:
    friend class PolicyCallbackImpl;

    static constexpr int32_t MAX_VOLUME_LEVEL = 15;
    static constexpr int32_t MIN_VOLUME_LEVEL = 0;
    static constexpr int32_t DEFAULT_VOLUME_LEVEL = 7;
    static constexpr int32_t CONST_FACTOR = 100;
    static constexpr float MIN_STREAM_VOLUME = 0.0f;
    static constexpr float MAX_STREAM_VOLUME = 1.0f;

    struct UserData {
        AudioAdapterManager *thiz;
        AudioStreamType streamType;
        float volume;
        bool mute;
        bool isCorked;
        uint32_t idx;
    };

    const std::vector<AudioStreamType> streamTypeList_ = {
        STREAM_MUSIC,
        STREAM_RING,
        STREAM_VOICE_CALL,
        STREAM_VOICE_ASSISTANT,
        STREAM_ALARM,
        STREAM_ACCESSIBILITY,
        STREAM_ULTRASONIC
    };

    AudioAdapterManager()
        : ringerMode_(RINGER_MODE_NORMAL),
          audioPolicyKvStore_(nullptr)
    {
        volumeLevelMap_[STREAM_MUSIC] = DEFAULT_VOLUME_LEVEL;
        volumeLevelMap_[STREAM_RING] = DEFAULT_VOLUME_LEVEL;
        volumeLevelMap_[STREAM_VOICE_CALL] = DEFAULT_VOLUME_LEVEL;
        volumeLevelMap_[STREAM_VOICE_ASSISTANT] = DEFAULT_VOLUME_LEVEL;
        volumeLevelMap_[STREAM_ALARM] = DEFAULT_VOLUME_LEVEL;
        volumeLevelMap_[STREAM_ACCESSIBILITY] = DEFAULT_VOLUME_LEVEL;
        volumeLevelMap_[STREAM_ULTRASONIC] = MAX_VOLUME_LEVEL;
    }

    bool ConnectToPulseAudio(void);
    std::string GetModuleArgs(const AudioModuleInfo &audioModuleInfo) const;
    std::string GetStreamNameByStreamType(DeviceType deviceType, AudioStreamType streamType);
    AudioStreamType GetStreamIDByType(std::string streamType);
    AudioStreamType GetStreamForVolumeMap(AudioStreamType streamType);
    bool InitAudioPolicyKvStore(bool& isFirstBoot);
    void InitVolumeMap(bool isFirstBoot);
    bool LoadVolumeMap(void);
    void WriteVolumeToKvStore(DeviceType type, AudioStreamType streamType, int32_t volumeLevel);
    bool LoadVolumeFromKvStore(DeviceType type, AudioStreamType streamType);
    void InitRingerMode(bool isFirstBoot);
    bool LoadRingerMode(void);
    void WriteRingerModeToKvStore(AudioRingerMode ringerMode);
    void InitMuteStatusMap(bool isFirstBoot);
    bool LoadMuteStatusMap(void);
    bool LoadMuteStatusFromKvStore(DeviceType deviceType, AudioStreamType streamType);
    void WriteMuteStatusToKvStore(DeviceType deviceType, AudioStreamType streamType, bool muteStatus);
    std::string GetStreamTypeKeyForMute(DeviceType deviceType, AudioStreamType streamType);
    int32_t WriteSystemSoundUriToKvStore(const std::string &key, const std::string &uri);
    std::string LoadSystemSoundUriFromKvStore(const std::string &key);
    void UpdateRingerModeForVolume(AudioStreamType streamType, int32_t volumeLevel);
    void UpdateMuteStatusForVolume(AudioStreamType streamType, int32_t volumeLevel);

    template<typename T>
    std::vector<uint8_t> TransferTypeToByteArray(const T &t)
    {
        return std::vector<uint8_t>(reinterpret_cast<uint8_t *>(const_cast<T *>(&t)),
            reinterpret_cast<uint8_t *>(const_cast<T *>(&t)) + sizeof(T));
    }

    template<typename T>
    T TransferByteArrayToType(const std::vector<uint8_t> &data)
    {
        if (data.size() != sizeof(T) || data.size() == 0) {
            constexpr int tSize = sizeof(T);
            uint8_t tContent[tSize] = { 0 };
            return *reinterpret_cast<T *>(tContent);
        }
        return *reinterpret_cast<T *>(const_cast<uint8_t *>(&data[0]));
    }

    std::unique_ptr<AudioServiceAdapter> audioServiceAdapter_;
    std::unordered_map<AudioStreamType, int32_t> volumeLevelMap_;
    std::unordered_map<AudioStreamType, bool> muteStatusMap_;
    DeviceType currentActiveDevice_ = DeviceType::DEVICE_TYPE_SPEAKER;
    AudioRingerMode ringerMode_;
    std::shared_ptr<SingleKvStore> audioPolicyKvStore_;
    AudioSessionCallback *sessionCallback_;
    bool testModeOn_ {false};

    std::vector<DeviceType> deviceList_ = {
        DEVICE_TYPE_SPEAKER,
        DEVICE_TYPE_USB_HEADSET,
        DEVICE_TYPE_BLUETOOTH_A2DP,
        DEVICE_TYPE_WIRED_HEADSET
    };
};

class PolicyCallbackImpl : public AudioServiceAdapterCallback {
public:
    explicit PolicyCallbackImpl(std::shared_ptr<AudioAdapterManager> audioAdapterManager)
    {
        audioAdapterManager_ = audioAdapterManager;
    }

    ~PolicyCallbackImpl()
    {
        audioAdapterManager_ = nullptr;
    }

    std::pair<float, bool> OnGetVolumeDbCb(std::string streamType)
    {
        AudioStreamType streamForVolumeMap = audioAdapterManager_->GetStreamForVolumeMap(
            audioAdapterManager_->GetStreamIDByType(streamType));

        int32_t volumeLevel = audioAdapterManager_->volumeLevelMap_[streamForVolumeMap];
        bool muteStatus = audioAdapterManager_->muteStatusMap_[streamForVolumeMap];
        return std::make_pair(audioAdapterManager_->CalculateVolumeDb(volumeLevel), muteStatus);
    }

    void OnSessionRemoved(const uint32_t sessionID)
    {
        AUDIO_DEBUG_LOG("PolicyCallbackImpl OnSessionRemoved: Session ID %{public}d", sessionID);
        if (audioAdapterManager_->sessionCallback_ == nullptr) {
            AUDIO_DEBUG_LOG("PolicyCallbackImpl audioAdapterManager_->sessionCallback_ == nullptr"
                "not firing OnSessionRemoved");
        } else {
            audioAdapterManager_->sessionCallback_->OnSessionRemoved(sessionID);
        }
    }
private:
    std::shared_ptr<AudioAdapterManager> audioAdapterManager_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // ST_PULSEAUDIO_ADAPTER_MANAGER_H
