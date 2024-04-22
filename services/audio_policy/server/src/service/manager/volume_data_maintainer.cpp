/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#undef LOG_TAG
#define LOG_TAG "VolumeDataMaintainer"

#include "volume_data_maintainer.h"

#include <map>

#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "audio_errors.h"
#include "audio_policy_manager_factory.h"

namespace OHOS {
namespace AudioStandard {
VolumeDataMaintainer::AudioSettingProvider* VolumeDataMaintainer::AudioSettingProvider::instance_;
std::mutex VolumeDataMaintainer::AudioSettingProvider::mutex_;
sptr<IRemoteObject> VolumeDataMaintainer::AudioSettingProvider::remoteObj_;

const std::string SETTING_COLUMN_KEYWORD = "KEYWORD";
const std::string SETTING_COLUMN_VALUE = "VALUE";
const std::string SETTING_URI_PROXY = "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";
const std::string SETTING_USER_URI_PROXY = "datashare:///com.ohos.settingsdata/entry/settingsdata/USER_SETTINGSDATA_";
const std::string SETTING_USER_SECURE_URI_PROXY =
    "datashare:///com.ohos.settingsdata/entry/settingsdata/USER_SETTINGSDATA_SECURE_";
constexpr const char *SETTINGS_DATA_EXT_URI = "datashare:///com.ohos.settingsdata.DataAbility";

const std::string AUDIO_SAFE_VOLUME_STATE = "audio_safe_volume_state";
const std::string AUDIO_SAFE_VOLUME_STATE_BT = "audio_safe_volume_state_bt";
const std::string UNSAFE_VOLUME_MUSIC_ACTIVE_MS = "unsafe_volume_music_active_ms";
const std::string UNSAFE_VOLUME_MUSIC_ACTIVE_MS_BT = "unsafe_volume_music_active_ms_bt";
const std::string SETTINGS_CLONED = "settingsCloned";

static const std::vector<VolumeDataMaintainer::VolumeDataMaintainerStreamType> VOLUME_MUTE_STREAM_TYPE = {
    // all volume types except STREAM_ALL
    VolumeDataMaintainer::VT_STREAM_DEFAULT,
    VolumeDataMaintainer::VT_STREAM_VOICE_CALL,
    VolumeDataMaintainer::VT_STREAM_SYSTEM,
    VolumeDataMaintainer::VT_STREAM_RING,
    VolumeDataMaintainer::VT_STREAM_MUSIC,
    VolumeDataMaintainer::VT_STREAM_ALARM,
    VolumeDataMaintainer::VT_STREAM_NOTIFICATION,
    VolumeDataMaintainer::VT_STREAM_BLUETOOTH_SCO,
    VolumeDataMaintainer::VT_STREAM_SYSTEM_ENFORCED,
    VolumeDataMaintainer::VT_STREAM_DTMF,
    VolumeDataMaintainer::VT_STREAM_TTS,
    VolumeDataMaintainer::VT_STREAM_ACCESSIBILITY,
    VolumeDataMaintainer::VT_STREAM_ASSISTANT,
};

static const std::vector<DeviceType> DEVICE_TYPE_LIST = {
    // The three devices represent the three volume groups(build-in, wireless, wired).
    DEVICE_TYPE_SPEAKER,
    DEVICE_TYPE_BLUETOOTH_A2DP,
    DEVICE_TYPE_WIRED_HEADSET
};

static std::map<VolumeDataMaintainer::VolumeDataMaintainerStreamType, AudioStreamType> AUDIO_STREAMTYPE_MAP = {
    {VolumeDataMaintainer::VT_STREAM_DEFAULT, STREAM_DEFAULT},
    {VolumeDataMaintainer::VT_STREAM_VOICE_CALL, STREAM_VOICE_CALL},
    {VolumeDataMaintainer::VT_STREAM_SYSTEM, STREAM_SYSTEM},
    {VolumeDataMaintainer::VT_STREAM_RING, STREAM_RING},
    {VolumeDataMaintainer::VT_STREAM_MUSIC, STREAM_MUSIC},
    {VolumeDataMaintainer::VT_STREAM_ALARM, STREAM_ALARM},
    {VolumeDataMaintainer::VT_STREAM_NOTIFICATION, STREAM_NOTIFICATION},
    {VolumeDataMaintainer::VT_STREAM_BLUETOOTH_SCO, STREAM_BLUETOOTH_SCO},
    {VolumeDataMaintainer::VT_STREAM_SYSTEM_ENFORCED, STREAM_SYSTEM_ENFORCED},
    {VolumeDataMaintainer::VT_STREAM_DTMF, STREAM_DTMF},
    {VolumeDataMaintainer::VT_STREAM_TTS, STREAM_TTS},
    {VolumeDataMaintainer::VT_STREAM_ACCESSIBILITY, STREAM_ACCESSIBILITY},
};

static std::map<AudioStreamType, std::string> AUDIO_STREAMTYPE_VOLUME_MAP = {
    {STREAM_MUSIC, "music_volume"},
    {STREAM_RING, "ring_volume"},
    {STREAM_SYSTEM, "system_volume"},
    {STREAM_NOTIFICATION, "notification_volume"},
    {STREAM_ALARM, "alarm_volume"},
    {STREAM_DTMF, "dtmf_volume"},
    {STREAM_VOICE_CALL, "voice_call_volume"},
    {STREAM_VOICE_ASSISTANT, "voice_assistant_volume"},
    {STREAM_ACCESSIBILITY, "accessibility_volume"},
    {STREAM_ULTRASONIC, "ultrasonic_volume"},
    {STREAM_WAKEUP,  "wakeup"},
};

static std::map<AudioStreamType, std::string> AUDIO_STREAMTYPE_MUTE_STATUS_MAP = {
    {STREAM_MUSIC, "music_mute_status"},
    {STREAM_RING, "ring_mute_status"},
    {STREAM_SYSTEM, "system_mute_status"},
    {STREAM_NOTIFICATION, "notification_mute_status"},
    {STREAM_ALARM, "alarm_mute_status"},
    {STREAM_DTMF, "dtmf_mute_status"},
    {STREAM_VOICE_CALL, "voice_call_mute_status"},
    {STREAM_VOICE_ASSISTANT, "voice_assistant_mute_status"},
    {STREAM_ACCESSIBILITY, "accessibility_mute_status"},
    {STREAM_ULTRASONIC, "unltrasonic_mute_status"},
};

VolumeDataMaintainer::VolumeDataMaintainer()
{
    AUDIO_DEBUG_LOG("VolumeDataMaintainer Create");
}

VolumeDataMaintainer::~VolumeDataMaintainer()
{
    AUDIO_DEBUG_LOG("VolumeDataMaintainer Destory");
}

bool VolumeDataMaintainer::SetFirstBoot(bool fristBoot)
{
    AudioSettingProvider& settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    const std::string settingKey = "first_boot";
    ErrCode ret = settingProvider.PutBoolValue(settingKey, fristBoot);
    if (ret != SUCCESS) {
        AUDIO_WARNING_LOG("Failed to set fristboot :%{public}d", ret);
        return false;
    }
    return true;
}

bool VolumeDataMaintainer::GetFirstBoot(bool &firstBoot)
{
    AudioSettingProvider& settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    const std::string settingKey = "first_boot";
    bool value;
    ErrCode ret = settingProvider.GetBoolValue(settingKey, value);
    if (ret != SUCCESS) {
        AUDIO_WARNING_LOG("Failed to get fristboot :%{public}d", ret);
        return false;
    }
    firstBoot = value;
    return true;
}

bool VolumeDataMaintainer::SaveVolume(DeviceType type, AudioStreamType streamType, int32_t volumeLevel)
{
    std::string volumeKey = GetVolumeKeyForDataShare(type, streamType);
    if (!volumeKey.compare("")) {
        AUDIO_ERR_LOG("[device %{public}d, streamType %{public}d] is not supported for datashare",
            type, streamType);
        return false;
    }

    AudioSettingProvider& audioSettingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    ErrCode ret = audioSettingProvider.PutIntValue(volumeKey, volumeLevel, "system");
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("Save Volume To DataBase volumeMap failed");
        return false;
    }
    return true;
}

bool VolumeDataMaintainer::GetVolume(DeviceType deviceType, AudioStreamType streamType)
{
    std::lock_guard<std::mutex> lock(volumeMutex_);
    return GetVolumeInternal(deviceType, streamType);
}

bool VolumeDataMaintainer::GetVolumeInternal(DeviceType deviceType, AudioStreamType streamType)
{
    std::string volumeKey = GetVolumeKeyForDataShare(deviceType, streamType);
    if (!volumeKey.compare("")) {
        AUDIO_ERR_LOG("[device %{public}d, streamType %{public}d] is not supported for datashare",
            deviceType, streamType);
        return false;
    }

    AudioSettingProvider& audioSettingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    int32_t volumeValue = 0;
    ErrCode ret = audioSettingProvider.GetIntValue(volumeKey, volumeValue, "system");
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("Get Volume FromDataBase volumeMap failed");
        return false;
    } else {
        volumeLevelMap_[streamType] = volumeValue;
        AUDIO_INFO_LOG("Get Volume FromDataBase volumeMap from datashare %{public}d", volumeValue);
    }

    return true;
}

void VolumeDataMaintainer::SetStreamVolume(AudioStreamType streamType, int32_t volumeLevel)
{
    std::lock_guard<std::mutex> lock(volumeMutex_);
    SetStreamVolumeInternal(streamType, volumeLevel);
}

void VolumeDataMaintainer::SetStreamVolumeInternal(AudioStreamType streamType, int32_t volumeLevel)
{
    AudioStreamType streamForVolumeMap = GetStreamForVolumeMap(streamType);
    volumeLevelMap_[streamForVolumeMap] = volumeLevel;
}

int32_t VolumeDataMaintainer::GetStreamVolume(AudioStreamType streamType)
{
    std::lock_guard<std::mutex> lock(volumeMutex_);
    return GetStreamVolumeInternal(streamType);
}

int32_t VolumeDataMaintainer::GetStreamVolumeInternal(AudioStreamType streamType)
{
    AudioStreamType streamForVolumeMap = GetStreamForVolumeMap(streamType);
    return volumeLevelMap_[streamForVolumeMap];
}

std::unordered_map<AudioStreamType, int32_t> VolumeDataMaintainer::GetVolumeMap()
{
    std::lock_guard<std::mutex> lock(volumeMutex_);
    return volumeLevelMap_;
}

bool VolumeDataMaintainer::SaveMuteStatus(DeviceType deviceType, AudioStreamType streamType,
    bool muteStatus)
{
    std::lock_guard<std::mutex> lock(muteStatusMutex_);
    return SaveMuteStatusInternal(deviceType, streamType, muteStatus);
}

bool VolumeDataMaintainer::SaveMuteStatusInternal(DeviceType deviceType, AudioStreamType streamType,
    bool muteStatus)
{
    std::string muteKey = GetMuteKeyForDataShare(deviceType, streamType);
    if (!muteKey.compare("")) {
        AUDIO_ERR_LOG("[device %{public}d, streamType %{public}d] is not supported for "\
            "datashare", deviceType, streamType);
        return false;
    }

    AudioSettingProvider& audioSettingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    ErrCode ret = audioSettingProvider.PutBoolValue(muteKey, muteStatus, "system");
    if (ret != SUCCESS) {
        AUDIO_WARNING_LOG("Failed to write mutestatus: %{public}d to setting db! Err: %{public}d", muteStatus, ret);
    } else {
        AudioStreamType streamForVolumeMap = GetStreamForVolumeMap(streamType);
        muteStatusMap_[streamForVolumeMap] = muteStatus;
        AUDIO_DEBUG_LOG("muteKey:%{public}s, muteStatus:%{public}d", muteKey.c_str(), muteStatus);
    }

    return true;
}

bool VolumeDataMaintainer::GetMuteStatus(DeviceType deviceType, AudioStreamType streamType)
{
    std::lock_guard<std::mutex> lock(muteStatusMutex_);
    return GetMuteStatusInternal(deviceType, streamType);
}

bool VolumeDataMaintainer::GetMuteStatusInternal(DeviceType deviceType, AudioStreamType streamType)
{
    std::string muteKey = GetMuteKeyForDataShare(deviceType, streamType);
    if (!muteKey.compare("")) {
        AUDIO_ERR_LOG("[device %{public}d, streamType %{public}d] is not supported for "\
            "datashare", deviceType, streamType);
        return false;
    }

    AudioSettingProvider& audioSettingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    bool muteStatus = false;
    ErrCode ret = audioSettingProvider.GetBoolValue(muteKey, muteStatus, "system");
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("Get MuteStatus From DataBase muteStatus failed");
        return false;
    } else {
        muteStatusMap_[streamType] = muteStatus;
        AUDIO_DEBUG_LOG("Get MuteStatus From DataBase muteStatus from datashare %{public}d", muteStatus);
    }

    return true;
}

bool VolumeDataMaintainer::GetStreamMute(AudioStreamType streamType)
{
    std::lock_guard<std::mutex> lock(muteStatusMutex_);
    return GetStreamMuteInternal(streamType);
}

bool VolumeDataMaintainer::GetStreamMuteInternal(AudioStreamType streamType)
{
    AudioStreamType streamForVolumeMap = GetStreamForVolumeMap(streamType);
    return muteStatusMap_[streamForVolumeMap];
}

void VolumeDataMaintainer::UpdateMuteStatusForVolume(DeviceType deviceType,
    AudioStreamType streamType, int32_t volumeLevel)
{
    std::lock_guard<std::mutex> lock(muteStatusMutex_);
    UpdateMuteStatusForVolumeInternal(deviceType, streamType, volumeLevel);
}

void VolumeDataMaintainer::UpdateMuteStatusForVolumeInternal(DeviceType deviceType,
    AudioStreamType streamType, int32_t volumeLevel)
{
    AudioStreamType streamForVolumeMap = GetStreamForVolumeMap(streamType);

    // The mute status is automatically updated based on the stream volume
    if (volumeLevel > 0 && GetStreamMuteInternal(streamType)) {
        muteStatusMap_[streamForVolumeMap] = false;
        SaveMuteStatusInternal(deviceType, streamType, false);
    } else if (volumeLevel == 0 && !GetStreamMuteInternal(streamType)) {
        muteStatusMap_[streamForVolumeMap] = true;
        SaveMuteStatusInternal(deviceType, streamType, true);
    }
}

bool VolumeDataMaintainer::GetMuteAffected(int32_t &affected)
{
    AudioSettingProvider& settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    const std::string settingKey = "mute_streams_affected";
    int32_t value = 0;
    ErrCode ret = settingProvider.GetIntValue(settingKey, value, "system");
    if (ret != SUCCESS) {
        AUDIO_WARNING_LOG("Failed to get muteaffected failed Err: %{public}d", ret);
        return false;
    } else {
        affected = value;
    }
    return true;
}

bool VolumeDataMaintainer::GetMuteTransferStatus(bool &status)
{
    AudioSettingProvider& settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    const std::string settingKey = "need_mute_affected_transfer";
    ErrCode ret = settingProvider.GetBoolValue(settingKey, status);
    if (ret != SUCCESS) {
        AUDIO_WARNING_LOG("Failed to get muteaffected failed Err: %{public}d", ret);
        return false;
    }
    return true;
}

bool VolumeDataMaintainer::SetMuteAffectedToMuteStatusDataBase(int32_t affected)
{
    // transfer mute_streams_affected to mutestatus
    for (auto &streamtype : VOLUME_MUTE_STREAM_TYPE) {
        if (affected & (1 << streamtype)) {
            for (auto &device : DEVICE_TYPE_LIST) {
                // save mute status to database
                SaveMuteStatusInternal(device, AUDIO_STREAMTYPE_MAP[streamtype], true);
            }
        }
    }
    return true;
}

bool VolumeDataMaintainer::SaveMuteTransferStatus(bool status)
{
    AudioSettingProvider& settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    const std::string settingKey = "need_mute_affected_transfer";
    ErrCode ret = settingProvider.PutIntValue(settingKey, status);
    if (ret != SUCCESS) {
        AUDIO_WARNING_LOG("Failed to SaveMuteTransferStatus: %{public}d to setting db! Err: %{public}d", status, ret);
        return false;
    }
    return true;
}

bool VolumeDataMaintainer::SaveRingerMode(AudioRingerMode ringerMode)
{
    AudioSettingProvider& settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    const std::string settingKey = "ringer_mode";
    ErrCode ret = settingProvider.PutIntValue(settingKey, static_cast<int32_t>(ringerMode));
    if (ret != SUCCESS) {
        AUDIO_WARNING_LOG("Failed to write ringer_mode: %{public}d to setting db! Err: %{public}d", ringerMode, ret);
        return false;
    }
    return true;
}

bool VolumeDataMaintainer::GetRingerMode(AudioRingerMode &ringerMode)
{
    AudioSettingProvider& settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    const std::string settingKey = "ringer_mode";
    int32_t value = 0;
    ErrCode ret = settingProvider.GetIntValue(settingKey, value);
    if (ret != SUCCESS) {
        AUDIO_WARNING_LOG("Failed to write ringer_mode: %{public}d to setting db! Err: %{public}d", ringerMode, ret);
        return false;
    } else {
        ringerMode = static_cast<AudioRingerMode>(value);
    }
    return true;
}

bool VolumeDataMaintainer::SaveSafeStatus(DeviceType deviceType, SafeStatus safeStatus)
{
    AudioSettingProvider& settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    ErrCode ret = SUCCESS;
    switch (deviceType) {
        case DEVICE_TYPE_BLUETOOTH_A2DP:
        case DEVICE_TYPE_BLUETOOTH_SCO:
            ret = settingProvider.PutIntValue(AUDIO_SAFE_VOLUME_STATE_BT, static_cast<int32_t>(safeStatus));
            break;
        case DEVICE_TYPE_WIRED_HEADSET:
        case DEVICE_TYPE_USB_HEADSET:
        case DEVICE_TYPE_USB_ARM_HEADSET:
            ret = settingProvider.PutIntValue(AUDIO_SAFE_VOLUME_STATE, static_cast<int32_t>(safeStatus));
            break;
        default:
            AUDIO_WARNING_LOG("the device type not support safe volume");
            return false;
    }
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("device:%{public}d, insert failed, safe status:%{public}d", deviceType, safeStatus);
        return false;
    }
    return true;
}

bool VolumeDataMaintainer::GetSafeStatus(DeviceType deviceType, SafeStatus &safeStatus)
{
    AudioSettingProvider& settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    ErrCode ret = SUCCESS;
    int32_t value = 0;
    switch (deviceType) {
        case DEVICE_TYPE_BLUETOOTH_A2DP:
        case DEVICE_TYPE_BLUETOOTH_SCO:
            ret = settingProvider.GetIntValue(AUDIO_SAFE_VOLUME_STATE_BT, value);
            break;
        case DEVICE_TYPE_WIRED_HEADSET:
        case DEVICE_TYPE_USB_HEADSET:
        case DEVICE_TYPE_USB_ARM_HEADSET:
            ret = settingProvider.GetIntValue(AUDIO_SAFE_VOLUME_STATE, value);
            break;
        default:
            AUDIO_WARNING_LOG("the device type not support safe volume");
            return false;
    }
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("device:%{public}d, insert failed, safe status:%{public}d", deviceType, safeStatus);
        return false;
    }
    if (value > static_cast<int32_t>(SAFE_ACTIVE)) {
        value = value - MAX_SAFE_STATUS;
    }
    safeStatus = static_cast<SafeStatus>(value);
    return true;
}

bool VolumeDataMaintainer::SaveSafeVolumeTime(DeviceType deviceType, int64_t time)
{
    AudioSettingProvider& settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    ErrCode ret = SUCCESS;
    switch (deviceType) {
        case DEVICE_TYPE_BLUETOOTH_A2DP:
        case DEVICE_TYPE_BLUETOOTH_SCO:
            ret = settingProvider.PutLongValue(UNSAFE_VOLUME_MUSIC_ACTIVE_MS_BT, time, "secure");
            break;
        case DEVICE_TYPE_WIRED_HEADSET:
        case DEVICE_TYPE_USB_HEADSET:
        case DEVICE_TYPE_USB_ARM_HEADSET:
            ret = settingProvider.PutLongValue(UNSAFE_VOLUME_MUSIC_ACTIVE_MS, time, "secure");
            break;
        default:
            AUDIO_WARNING_LOG("the device type not support safe volume");
            return false;
    }
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("device:%{public}d, insert failed", deviceType);
        return false;
    }

    return true;
}

bool VolumeDataMaintainer::GetSafeVolumeTime(DeviceType deviceType, int64_t &time)
{
    AudioSettingProvider& settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    ErrCode ret = SUCCESS;
    switch (deviceType) {
        case DEVICE_TYPE_BLUETOOTH_A2DP:
        case DEVICE_TYPE_BLUETOOTH_SCO:
            ret = settingProvider.GetLongValue(UNSAFE_VOLUME_MUSIC_ACTIVE_MS_BT, time, "secure");
            break;
        case DEVICE_TYPE_WIRED_HEADSET:
        case DEVICE_TYPE_USB_HEADSET:
        case DEVICE_TYPE_USB_ARM_HEADSET:
            ret = settingProvider.GetLongValue(UNSAFE_VOLUME_MUSIC_ACTIVE_MS, time, "secure");
            break;
        default:
            AUDIO_WARNING_LOG("the device type not support safe mode");
            return false;
    }
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("device:%{public}d, get safe active time failed", deviceType);
        return false;
    }
    return true;
}

bool VolumeDataMaintainer::SaveSystemSoundUrl(const std::string &key, const std::string &value)
{
    AudioSettingProvider& settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    ErrCode ret = settingProvider.PutStringValue(key, value);
    if (ret != SUCCESS) {
        AUDIO_WARNING_LOG("Failed to system sound url: %{public}s to setting db! Err: %{public}d", value.c_str(), ret);
        return false;
    }
    return true;
}

bool VolumeDataMaintainer::GetSystemSoundUrl(const std::string &key, std::string &value)
{
    AudioSettingProvider& settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    ErrCode ret = settingProvider.GetStringValue(key, value);
    if (ret != SUCCESS) {
        AUDIO_WARNING_LOG("Failed to get systemsoundurl failed Err: %{public}d", ret);
        return false;
    }
    return true;
}

void VolumeDataMaintainer::RegisterCloned()
{
    AudioSettingProvider& settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    AudioSettingObserver::UpdateFunc updateFunc = [&](const std::string& key) {
        int32_t value = 0;
        ErrCode result =
            AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID).GetIntValue(SETTINGS_CLONED, value);
        if ((value == 1) && (result == SUCCESS)) {
            AUDIO_INFO_LOG("Get SETTINGS_CLONED success");
            AudioPolicyManagerFactory::GetAudioPolicyManager().DoRestoreData();
        }
    };
    sptr<AudioSettingObserver> observer = settingProvider.CreateObserver(SETTINGS_CLONED, updateFunc);
    ErrCode ret = settingProvider.RegisterObserver(observer);
    if (ret != ERR_OK) {
        AUDIO_ERR_LOG("RegisterObserver failed");
    }
}

std::string VolumeDataMaintainer::GetDeviceTypeName(DeviceType deviceType)
{
    std::string type = "";
    switch (deviceType) {
        case DEVICE_TYPE_EARPIECE:
        case DEVICE_TYPE_SPEAKER:
            type = "_builtin";
            return type;
        case DEVICE_TYPE_BLUETOOTH_A2DP:
        case DEVICE_TYPE_BLUETOOTH_SCO:
            type = "_wireless";
            return type;
        case DEVICE_TYPE_WIRED_HEADSET:
        case DEVICE_TYPE_USB_HEADSET:
        case DEVICE_TYPE_USB_ARM_HEADSET:
            type = "_wired";
            return type;
        default:
            AUDIO_ERR_LOG("device %{public}d is not supported for dataShare", deviceType);
            return "";
    }
}

std::string VolumeDataMaintainer::GetVolumeKeyForDataShare(DeviceType deviceType, AudioStreamType streamType)
{
    std::string type = "";
    type = AUDIO_STREAMTYPE_VOLUME_MAP[streamType];
    if (type == "") {
        AUDIO_ERR_LOG("streamType %{public}d is not supported for datashare", streamType);
        return "";
    }

    std::string deviceTypeName = GetDeviceTypeName(deviceType);
    if (deviceTypeName == "") {
        AUDIO_ERR_LOG("device %{public}d is not supported for datashare", deviceType);
        return "";
    }
    return type + deviceTypeName;
}

std::string VolumeDataMaintainer::GetMuteKeyForDataShare(DeviceType deviceType, AudioStreamType streamType)
{
    std::string type = "";
    type = AUDIO_STREAMTYPE_MUTE_STATUS_MAP[streamType];
    if (type == "") {
        AUDIO_ERR_LOG("streamType %{public}d is not supported for datashare", streamType);
        return "";
    }

    std::string deviceTypeName = GetDeviceTypeName(deviceType);
    if (deviceTypeName == "") {
        AUDIO_ERR_LOG("device %{public}d is not supported for datashare", deviceType);
        return "";
    }
    return type + deviceTypeName;
}

AudioStreamType VolumeDataMaintainer::GetStreamForVolumeMap(AudioStreamType streamType)
{
    switch (streamType) {
        case STREAM_VOICE_CALL:
        case STREAM_VOICE_MESSAGE:
            return STREAM_VOICE_CALL;
        case STREAM_RING:
        case STREAM_SYSTEM:
        case STREAM_NOTIFICATION:
        case STREAM_SYSTEM_ENFORCED:
        case STREAM_DTMF:
            return STREAM_RING;
        case STREAM_MUSIC:
        case STREAM_MEDIA:
        case STREAM_MOVIE:
        case STREAM_GAME:
        case STREAM_SPEECH:
        case STREAM_NAVIGATION:
            return STREAM_MUSIC;
        case STREAM_VOICE_ASSISTANT:
            return STREAM_VOICE_ASSISTANT;
        case STREAM_ALARM:
            return STREAM_ALARM;
        case STREAM_ACCESSIBILITY:
            return STREAM_ACCESSIBILITY;
        case STREAM_ULTRASONIC:
            return STREAM_ULTRASONIC;
        default:
            return STREAM_MUSIC;
    }
}

VolumeDataMaintainer::AudioSettingProvider::~AudioSettingProvider()
{
    instance_ = nullptr;
    remoteObj_ = nullptr;
}

void VolumeDataMaintainer::AudioSettingObserver::OnChange()
{
    if (update_) {
        update_(key_);
    }
}

void VolumeDataMaintainer::AudioSettingObserver::SetKey(const std::string &key)
{
    key_ = key;
}

const std::string& VolumeDataMaintainer::AudioSettingObserver::GetKey()
{
    return key_;
}

void VolumeDataMaintainer::AudioSettingObserver::SetUpdateFunc(UpdateFunc &func)
{
    update_ = func;
}

VolumeDataMaintainer::AudioSettingProvider& VolumeDataMaintainer::AudioSettingProvider::GetInstance(
    int32_t systemAbilityId)
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = new AudioSettingProvider();
            Initialize(systemAbilityId);
        }
    }
    return *instance_;
}

ErrCode VolumeDataMaintainer::AudioSettingProvider::GetIntValue(const std::string &key, int32_t &value,
    std::string tableType)
{
    int64_t valueLong;
    ErrCode ret = GetLongValue(key, valueLong, tableType);
    if (ret != ERR_OK) {
        return ret;
    }
    value = static_cast<int32_t>(valueLong);
    return ERR_OK;
}

ErrCode VolumeDataMaintainer::AudioSettingProvider::GetLongValue(const std::string &key, int64_t &value,
    std::string tableType)
{
    std::string valueStr;
    ErrCode ret = GetStringValue(key, valueStr, tableType);
    if (ret != ERR_OK) {
        return ret;
    }
    value = static_cast<int64_t>(strtoll(valueStr.c_str(), nullptr, MAX_STRING_LENGTH));
    return ERR_OK;
}

ErrCode VolumeDataMaintainer::AudioSettingProvider::GetBoolValue(const std::string &key, bool &value,
    std::string tableType)
{
    std::string valueStr;
    ErrCode ret = GetStringValue(key, valueStr, tableType);
    if (ret != ERR_OK) {
        return ret;
    }
    value = (valueStr == "true");
    return ERR_OK;
}

ErrCode VolumeDataMaintainer::AudioSettingProvider::PutIntValue(const std::string &key, int32_t value,
    std::string tableType, bool needNotify)
{
    return PutStringValue(key, std::to_string(value), tableType, needNotify);
}

ErrCode VolumeDataMaintainer::AudioSettingProvider::PutLongValue(const std::string &key, int64_t value,
    std::string tableType, bool needNotify)
{
    return PutStringValue(key, std::to_string(value), tableType, needNotify);
}

ErrCode VolumeDataMaintainer::AudioSettingProvider::PutBoolValue(const std::string &key, bool value,
    std::string tableType, bool needNotify)
{
    std::string valueStr = value ? "true" : "false";
    return PutStringValue(key, valueStr, tableType, needNotify);
}

bool VolumeDataMaintainer::AudioSettingProvider::IsValidKey(const std::string &key)
{
    std::string value;
    ErrCode ret = GetStringValue(key, value);
    return (ret != ERR_NAME_NOT_FOUND) && (!value.empty());
}

sptr<VolumeDataMaintainer::AudioSettingObserver> VolumeDataMaintainer::AudioSettingProvider::CreateObserver(
    const std::string &key, VolumeDataMaintainer::AudioSettingObserver::UpdateFunc &func)
{
    sptr<AudioSettingObserver> observer = new AudioSettingObserver();
    observer->SetKey(key);
    observer->SetUpdateFunc(func);
    return observer;
}

void VolumeDataMaintainer::AudioSettingProvider::ExecRegisterCb(const sptr<AudioSettingObserver> &observer)
{
    if (observer == nullptr) {
        AUDIO_ERR_LOG("observer is nullptr");
        return;
    }
    observer->OnChange();
}

ErrCode VolumeDataMaintainer::AudioSettingProvider::RegisterObserver(const sptr<AudioSettingObserver> &observer)
{
    std::string callingIdentity = IPCSkeleton::ResetCallingIdentity();
    auto uri = AssembleUri(observer->GetKey());
    auto helper = CreateDataShareHelper();
    if (helper == nullptr) {
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return ERR_NO_INIT;
    }
    helper->RegisterObserver(uri, observer);
    helper->NotifyChange(uri);
    std::thread execCb(VolumeDataMaintainer::AudioSettingProvider::ExecRegisterCb, observer);
    execCb.detach();
    ReleaseDataShareHelper(helper);
    IPCSkeleton::SetCallingIdentity(callingIdentity);
    AUDIO_DEBUG_LOG("succeed to register observer of uri=%{public}s", uri.ToString().c_str());
    return ERR_OK;
}

ErrCode VolumeDataMaintainer::AudioSettingProvider::UnregisterObserver(const sptr<AudioSettingObserver> &observer)
{
    std::string callingIdentity = IPCSkeleton::ResetCallingIdentity();
    auto uri = AssembleUri(observer->GetKey());
    auto helper = CreateDataShareHelper();
    if (helper == nullptr) {
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return ERR_NO_INIT;
    }
    helper->UnregisterObserver(uri, observer);
    ReleaseDataShareHelper(helper);
    IPCSkeleton::SetCallingIdentity(callingIdentity);
    AUDIO_DEBUG_LOG("succeed to unregister observer of uri=%{public}s", uri.ToString().c_str());
    return ERR_OK;
}

void VolumeDataMaintainer::AudioSettingProvider::Initialize(int32_t systemAbilityId)
{
    auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sam == nullptr) {
        AUDIO_ERR_LOG("GetSystemAbilityManager return nullptr");
        return;
    }
    auto remoteObj = sam->GetSystemAbility(systemAbilityId);
    if (remoteObj == nullptr) {
        AUDIO_ERR_LOG("GetSystemAbility return nullptr, systemAbilityId=%{public}d", systemAbilityId);
        return;
    }
    remoteObj_ = remoteObj;
}

ErrCode VolumeDataMaintainer::AudioSettingProvider::GetStringValue(const std::string &key,
    std::string &value, std::string tableType)
{
    std::string callingIdentity = IPCSkeleton::ResetCallingIdentity();
    auto helper = CreateDataShareHelper(tableType);
    if (helper == nullptr) {
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return ERR_NO_INIT;
    }
    std::vector<std::string> columns = {SETTING_COLUMN_VALUE};
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SETTING_COLUMN_KEYWORD, key);
    Uri uri(AssembleUri(key, tableType));
    auto resultSet = helper->Query(uri, predicates, columns);
    ReleaseDataShareHelper(helper);
    if (resultSet == nullptr) {
        AUDIO_ERR_LOG("helper->Query return nullptr");
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return ERR_INVALID_OPERATION;
    }
    int32_t count;
    resultSet->GetRowCount(count);
    if (count == 0) {
        AUDIO_WARNING_LOG("not found value, key=%{public}s, count=%{public}d", key.c_str(), count);
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return ERR_NAME_NOT_FOUND;
    }
    const int32_t INDEX = 0;
    resultSet->GoToRow(INDEX);
    int32_t ret = resultSet->GetString(INDEX, value);
    if (ret != SUCCESS) {
        AUDIO_WARNING_LOG("resultSet->GetString return not ok, ret=%{public}d", ret);
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return ERR_INVALID_VALUE;
    }
    resultSet->Close();
    IPCSkeleton::SetCallingIdentity(callingIdentity);
    return ERR_OK;
}

ErrCode VolumeDataMaintainer::AudioSettingProvider::PutStringValue(const std::string &key, const std::string &value,
    std::string tableType, bool needNotify)
{
    std::string callingIdentity = IPCSkeleton::ResetCallingIdentity();
    auto helper = CreateDataShareHelper(tableType);
    if (helper == nullptr) {
        IPCSkeleton::SetCallingIdentity(callingIdentity);
        return ERR_NO_INIT;
    }
    DataShare::DataShareValueObject keyObj(key);
    DataShare::DataShareValueObject valueObj(value);
    DataShare::DataShareValuesBucket bucket;
    bucket.Put(SETTING_COLUMN_KEYWORD, keyObj);
    bucket.Put(SETTING_COLUMN_VALUE, valueObj);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SETTING_COLUMN_KEYWORD, key);
    Uri uri(AssembleUri(key, tableType));
    if (helper->Update(uri, predicates, bucket) <= 0) {
        AUDIO_DEBUG_LOG("no data exist, insert one row");
        helper->Insert(uri, bucket);
    }
    if (needNotify) {
        helper->NotifyChange(AssembleUri(key, tableType));
    }
    ReleaseDataShareHelper(helper);
    IPCSkeleton::SetCallingIdentity(callingIdentity);
    return ERR_OK;
}

int32_t VolumeDataMaintainer::AudioSettingProvider::GetCurrentUserId()
{
    std::vector<int> ids;
    int32_t currentuserId = -1;
    ErrCode result = AccountSA::OsAccountManager::QueryActiveOsAccountIds(ids);
    if (result != ERR_OK || ids.empty()) {
        AUDIO_WARNING_LOG("current userId is empty");
    } else {
        currentuserId = ids[0];
        AUDIO_INFO_LOG("current userId is :%{public}d", currentuserId);
    }
    return currentuserId;
}

std::shared_ptr<DataShare::DataShareHelper> VolumeDataMaintainer::AudioSettingProvider::CreateDataShareHelper(
    std::string tableType)
{
#ifdef SUPPORT_USER_ACCOUNT
    int32_t currentuserId = GetCurrentUserId();
    if (currentuserId < MIN_USER_ACCOUNT) {
        currentuserId = MIN_USER_ACCOUNT;
    }
#else
    int32_t currentuserId = -1;
#endif
    std::shared_ptr<DataShare::DataShareHelper> helper = nullptr;
    std::string SettingSystemUrlProxy = "";
    // deal with multi useraccount table
    if (currentuserId > 0 && tableType == "system") {
        SettingSystemUrlProxy =
            SETTING_USER_URI_PROXY + std::to_string(currentuserId) + "?Proxy=true";
        helper = DataShare::DataShareHelper::Creator(remoteObj_, SettingSystemUrlProxy, SETTINGS_DATA_EXT_URI);
    } else if (currentuserId > 0 && tableType == "secure") {
        SettingSystemUrlProxy =
            SETTING_USER_SECURE_URI_PROXY + std::to_string(currentuserId) + "?Proxy=true";
        helper = DataShare::DataShareHelper::Creator(remoteObj_, SettingSystemUrlProxy, SETTINGS_DATA_EXT_URI);
    } else {
        helper = DataShare::DataShareHelper::Creator(remoteObj_, SETTING_URI_PROXY, SETTINGS_DATA_EXT_URI);
    }
    if (helper == nullptr) {
        AUDIO_WARNING_LOG("helper is nullptr, uri=%{public}s", SettingSystemUrlProxy.c_str());
        return nullptr;
    }
    return helper;
}

bool VolumeDataMaintainer::AudioSettingProvider::ReleaseDataShareHelper(
    std::shared_ptr<DataShare::DataShareHelper> &helper)
{
    if (!helper->Release()) {
        AUDIO_WARNING_LOG("release helper fail");
        return false;
    }
    return true;
}

Uri VolumeDataMaintainer::AudioSettingProvider::AssembleUri(const std::string &key, std::string tableType)
{
#ifdef SUPPORT_USER_ACCOUNT
    int32_t currentuserId = GetCurrentUserId();
    if (currentuserId < MIN_USER_ACCOUNT) {
        currentuserId = MIN_USER_ACCOUNT;
    }
#else
    int32_t currentuserId = -1;
#endif
    std::string SettingSystemUrlProxy = "";

    // deal with multi useraccount table
    if (currentuserId > 0 && tableType == "system") {
        SettingSystemUrlProxy = SETTING_USER_URI_PROXY + std::to_string(currentuserId) + "?Proxy=true";
        Uri uri(SettingSystemUrlProxy + "&key=" + key);
        return uri;
    } else if (currentuserId > 0 && tableType == "secure") {
        SettingSystemUrlProxy = SETTING_USER_SECURE_URI_PROXY + std::to_string(currentuserId) + "?Proxy=true";
        Uri uri(SettingSystemUrlProxy + "&key=" + key);
        return uri;
    }
    Uri uri(SETTING_URI_PROXY + "&key=" + key);
    return uri;
}
} // namespace AudioStandard
} // namespace OHOS
