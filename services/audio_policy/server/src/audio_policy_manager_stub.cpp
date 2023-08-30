/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "audio_policy_manager_stub.h"

#include "audio_errors.h"
#include "audio_log.h"
#include "audio_policy_ipc_interface_code.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;

void AudioPolicyManagerStub::ReadStreamChangeInfo(MessageParcel &data, const AudioMode &mode,
    AudioStreamChangeInfo &streamChangeInfo)
{
    if (mode == AUDIO_MODE_PLAYBACK) {
        streamChangeInfo.audioRendererChangeInfo.Unmarshalling(data);
        return;
    } else {
        // mode == AUDIO_MODE_RECORDING
        streamChangeInfo.audioCapturerChangeInfo.Unmarshalling(data);
    }
}

void AudioPolicyManagerStub::GetMaxVolumeLevelInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioVolumeType volumeType = static_cast<AudioVolumeType>(data.ReadInt32());
    int32_t maxLevel = GetMaxVolumeLevel(volumeType);
    reply.WriteInt32(maxLevel);
}

void AudioPolicyManagerStub::GetMinVolumeLevelInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioVolumeType volumeType = static_cast<AudioVolumeType>(data.ReadInt32());
    int32_t minLevel = GetMinVolumeLevel(volumeType);
    reply.WriteInt32(minLevel);
}

void AudioPolicyManagerStub::SetSystemVolumeLevelInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioStreamType streamType = static_cast<AudioStreamType>(data.ReadInt32());
    int32_t volumeLevel = data.ReadInt32();
    API_VERSION api_v = static_cast<API_VERSION>(data.ReadInt32());
    int result = SetSystemVolumeLevel(streamType, volumeLevel, api_v);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::SetRingerModeInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioRingerMode rMode = static_cast<AudioRingerMode>(data.ReadInt32());
    API_VERSION api_v = static_cast<API_VERSION>(data.ReadInt32());
    int32_t result = SetRingerMode(rMode, api_v);
    reply.WriteInt32(result);
}

#ifdef FEATURE_DTMF_TONE
void AudioPolicyManagerStub::GetToneInfoInternal(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<ToneInfo> ltoneInfo = GetToneConfig(data.ReadInt32());
    if (ltoneInfo == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyManagerStub: GetToneInfoInternal obj is null");
        return;
    }
    ltoneInfo->Marshalling(data);
}

void AudioPolicyManagerStub::GetSupportedTonesInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t lToneListSize = 0;
    std::vector<int32_t> lToneList = GetSupportedTones();
    lToneListSize = static_cast<int32_t>(lToneList.size());
    reply.WriteInt32(lToneListSize);
    for (int i = 0; i < lToneListSize; i++) {
        reply.WriteInt32(lToneList[i]);
    }
}
#endif

void AudioPolicyManagerStub::GetRingerModeInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioRingerMode rMode = GetRingerMode();
    reply.WriteInt32(static_cast<int>(rMode));
}

void AudioPolicyManagerStub::SetAudioSceneInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioScene audioScene = static_cast<AudioScene>(data.ReadInt32());
    int32_t result = SetAudioScene(audioScene);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::SetMicrophoneMuteInternal(MessageParcel &data, MessageParcel &reply)
{
    bool isMute = data.ReadBool();
    int32_t result = SetMicrophoneMute(isMute);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::SetMicrophoneMuteAudioConfigInternal(MessageParcel &data, MessageParcel &reply)
{
    bool isMute = data.ReadBool();
    int32_t result = SetMicrophoneMuteAudioConfig(isMute);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::IsMicrophoneMuteInternal(MessageParcel &data, MessageParcel &reply)
{
    API_VERSION api_v = static_cast<API_VERSION>(data.ReadInt32());
    int32_t result = IsMicrophoneMute(api_v);
    reply.WriteBool(result);
}

void AudioPolicyManagerStub::GetAudioSceneInternal(MessageParcel & /* data */, MessageParcel &reply)
{
    AudioScene audioScene = GetAudioScene();
    reply.WriteInt32(static_cast<int>(audioScene));
}

void AudioPolicyManagerStub::GetSystemVolumeLevelInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioStreamType streamType = static_cast<AudioStreamType>(data.ReadInt32());
    int32_t volumeLevel = GetSystemVolumeLevel(streamType);
    reply.WriteInt32(volumeLevel);
}

void AudioPolicyManagerStub::SetLowPowerVolumeInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t streamId = data.ReadInt32();
    float volume = data.ReadFloat();
    int result = SetLowPowerVolume(streamId, volume);
    if (result == SUCCESS)
        reply.WriteInt32(AUDIO_OK);
    else
        reply.WriteInt32(AUDIO_ERR);
}

void AudioPolicyManagerStub::GetLowPowerVolumeInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t streamId = data.ReadInt32();
    float volume = GetLowPowerVolume(streamId);
    reply.WriteFloat(volume);
}

void AudioPolicyManagerStub::GetSingleStreamVolumeInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t streamId = data.ReadInt32();
    float volume = GetSingleStreamVolume(streamId);
    reply.WriteFloat(volume);
}

void AudioPolicyManagerStub::SetStreamMuteInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioStreamType streamType = static_cast<AudioStreamType>(data.ReadInt32());
    bool mute = data.ReadBool();
    API_VERSION api_v = static_cast<API_VERSION>(data.ReadInt32());
    int result = SetStreamMute(streamType, mute, api_v);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::GetStreamMuteInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioStreamType streamType = static_cast<AudioStreamType>(data.ReadInt32());
    bool mute = GetStreamMute(streamType);
    reply.WriteBool(mute);
}

void AudioPolicyManagerStub::IsStreamActiveInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioStreamType streamType = static_cast<AudioStreamType>(data.ReadInt32());
    bool isActive = IsStreamActive(streamType);
    reply.WriteBool(isActive);
}

void AudioPolicyManagerStub::AdjustVolumeByStepInternal(MessageParcel &data, MessageParcel &reply)
{
    VolumeAdjustType adjustType = static_cast<VolumeAdjustType>(data.ReadInt32());
    int32_t result = AdjustVolumeByStep(adjustType);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::GetSystemVolumeInDbInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioVolumeType volumeType = static_cast<AudioVolumeType>(data.ReadInt32());
    int32_t volumeLevel = data.ReadInt32();
    DeviceType deviceType = static_cast<DeviceType>(data.ReadInt32());
    float result = GetSystemVolumeInDb(volumeType, volumeLevel, deviceType);
    reply.WriteFloat(result);
}

void AudioPolicyManagerStub::IsVolumeUnadjustableInternal(MessageParcel &data, MessageParcel &reply)
{
    bool isVolumeUnadjustable = IsVolumeUnadjustable();
    reply.WriteBool(isVolumeUnadjustable);
}

void AudioPolicyManagerStub::AdjustSystemVolumeByStepInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioVolumeType volumeType = static_cast<AudioVolumeType>(data.ReadInt32());
    VolumeAdjustType adjustType = static_cast<VolumeAdjustType>(data.ReadInt32());
    int32_t result = AdjustSystemVolumeByStep(volumeType, adjustType);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::GetDevicesInternal(MessageParcel &data, MessageParcel &reply)
{
    AUDIO_DEBUG_LOG("GET_DEVICES AudioManagerStub");
    int deviceFlag = data.ReadInt32();
    DeviceFlag deviceFlagConfig = static_cast<DeviceFlag>(deviceFlag);
    std::vector<sptr<AudioDeviceDescriptor>> devices = GetDevices(deviceFlagConfig);
    int32_t size = static_cast<int32_t>(devices.size());
    AUDIO_DEBUG_LOG("GET_DEVICES size= %{public}d", size);
    reply.WriteInt32(size);
    for (int i = 0; i < size; i++) {
        devices[i]->Marshalling(reply);
    }
}

void AudioPolicyManagerStub::SetWakeUpAudioCapturerInternal(MessageParcel &data, MessageParcel &reply)
{
    AUDIO_DEBUG_LOG("SetWakeUpAudioCapturerInternal AudioManagerStub");
    InternalAudioCapturerOptions capturerOptions;
    capturerOptions.streamInfo.Unmarshalling(data);
    capturerOptions.capturerInfo.Unmarshalling(data);
    int32_t result = SetWakeUpAudioCapturer(capturerOptions);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::CloseWakeUpAudioCapturerInternal(MessageParcel &data, MessageParcel &reply)
{
    AUDIO_DEBUG_LOG("CloseWakeUpAudioCapturerInternal AudioManagerStub");
    int32_t result = CloseWakeUpAudioCapturer();
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::GetPreferredOutputDeviceDescriptorsInternal(MessageParcel &data, MessageParcel &reply)
{
    AUDIO_DEBUG_LOG("GET_ACTIVE_OUTPUT_DEVICE_DESCRIPTORS AudioManagerStub");
    AudioRendererInfo rendererInfo;
    std::vector<sptr<AudioDeviceDescriptor>> devices = GetPreferredOutputDeviceDescriptors(rendererInfo);
    int32_t size = static_cast<int32_t>(devices.size());
    AUDIO_DEBUG_LOG("GET_ACTIVE_OUTPUT_DEVICE_DESCRIPTORS size= %{public}d", size);
    reply.WriteInt32(size);
    for (int i = 0; i < size; i++) {
        devices[i]->Marshalling(reply);
    }
}

void AudioPolicyManagerStub::GetPreferredInputDeviceDescriptorsInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioCapturerInfo captureInfo;
    std::vector<sptr<AudioDeviceDescriptor>> devices = GetPreferredInputDeviceDescriptors(captureInfo);
    size_t size = static_cast<int32_t>(devices.size());
    AUDIO_DEBUG_LOG("GET_PREFERRED_INTPUT_DEVICE_DESCRIPTORS size= %{public}zu", size);
    reply.WriteInt32(size);
    for (size_t i = 0; i < size; i++) {
        devices[i]->Marshalling(reply);
    }
}

void AudioPolicyManagerStub::SetDeviceActiveInternal(MessageParcel &data, MessageParcel &reply)
{
    InternalDeviceType deviceType = static_cast<InternalDeviceType>(data.ReadInt32());
    bool active = data.ReadBool();
    int32_t result = SetDeviceActive(deviceType, active);
    if (result == SUCCESS)
        reply.WriteInt32(AUDIO_OK);
    else
        reply.WriteInt32(AUDIO_ERR);
}

void AudioPolicyManagerStub::IsDeviceActiveInternal(MessageParcel &data, MessageParcel &reply)
{
    InternalDeviceType deviceType = static_cast<InternalDeviceType>(data.ReadInt32());
    bool result = IsDeviceActive(deviceType);
    reply.WriteBool(result);
}

void AudioPolicyManagerStub::GetActiveOutputDeviceInternal(MessageParcel &data, MessageParcel &reply)
{
    InternalDeviceType deviceType = GetActiveOutputDevice();
    reply.WriteInt32(static_cast<int>(deviceType));
}

void AudioPolicyManagerStub::GetActiveInputDeviceInternal(MessageParcel &data, MessageParcel &reply)
{
    InternalDeviceType deviceType = GetActiveInputDevice();
    reply.WriteInt32(static_cast<int>(deviceType));
}

void AudioPolicyManagerStub::SetPreferredOutputDeviceChangeCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t clientId = data.ReadInt32();
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    if (object == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyManagerStub: object is null");
        return;
    }
    int32_t result = SetPreferredOutputDeviceChangeCallback(clientId, object);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::SetPreferredInputDeviceChangeCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    if (object == nullptr) {
        AUDIO_ERR_LOG("object is null");
        return;
    }
    int32_t result = SetPreferredInputDeviceChangeCallback(object);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::UnsetPreferredOutputDeviceChangeCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t clientId = data.ReadInt32();
    int32_t result = UnsetPreferredOutputDeviceChangeCallback(clientId);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::UnsetPreferredInputDeviceChangeCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t result = UnsetPreferredInputDeviceChangeCallback();
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::WriteAudioFocusInfo(MessageParcel &reply,
    const std::pair<AudioInterrupt, AudioFocuState> &focusInfo)
{
    focusInfo.first.Marshalling(reply);
    reply.WriteInt32(focusInfo.second);
}

void AudioPolicyManagerStub::GetAudioFocusInfoListInternal(MessageParcel &data, MessageParcel &reply)
{
    std::list<std::pair<AudioInterrupt, AudioFocuState>> focusInfoList;
    int32_t result = GetAudioFocusInfoList(focusInfoList);
    int32_t size = static_cast<int32_t>(focusInfoList.size());
    reply.WriteInt32(result);
    reply.WriteInt32(size);
    if (result == SUCCESS) {
        AUDIO_DEBUG_LOG("GetAudioFocusInfoList size= %{public}d", size);
        for (std::pair<AudioInterrupt, AudioFocuState> focusInfo : focusInfoList) {
            WriteAudioFocusInfo(reply, focusInfo);
        }
    }
}

void AudioPolicyManagerStub::RegisterFocusInfoChangeCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t clientId = data.ReadInt32();
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    if (object == nullptr) {
        AUDIO_ERR_LOG("AudioFocusInfoCallback obj is null");
        return;
    }
    int32_t result = RegisterFocusInfoChangeCallback(clientId, object);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::UnregisterFocusInfoChangeCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t clientId = data.ReadInt32();
    int32_t result = UnregisterFocusInfoChangeCallback(clientId);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::SetRingerModeCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t clientId = data.ReadInt32();
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    API_VERSION api_v = static_cast<API_VERSION>(data.ReadInt32());
    if (object == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyManagerStub: SetRingerModeCallback obj is null");
        return;
    }
    int32_t result = SetRingerModeCallback(clientId, object, api_v);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::SetMicStateChangeCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t clientId = data.ReadInt32();
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    if (object == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyManagerStub: AudioInterruptCallback obj is null");
        return;
    }
    int32_t result = SetMicStateChangeCallback(clientId, object);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::UnsetRingerModeCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t clientId = data.ReadInt32();
    int32_t result = UnsetRingerModeCallback(clientId);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::SelectOutputDeviceInternal(MessageParcel &data, MessageParcel &reply)
{
    sptr<AudioRendererFilter> audioRendererFilter = AudioRendererFilter::Unmarshalling(data);
    if (audioRendererFilter == nullptr) {
        AUDIO_ERR_LOG("AudioRendererFilter unmarshall fail.");
        return;
    }

    int validSize = 20; // Use 20 as limit.
    int size = data.ReadInt32();
    if (size <= 0 || size > validSize) {
        AUDIO_ERR_LOG("SelectOutputDevice get invalid device size.");
        return;
    }
    std::vector<sptr<AudioDeviceDescriptor>> targetOutputDevice;
    for (int i = 0; i < size; i++) {
        sptr<AudioDeviceDescriptor> audioDeviceDescriptor = AudioDeviceDescriptor::Unmarshalling(data);
        if (audioDeviceDescriptor == nullptr) {
            AUDIO_ERR_LOG("Unmarshalling fail.");
            return;
        }
        targetOutputDevice.push_back(audioDeviceDescriptor);
    }

    int32_t ret = SelectOutputDevice(audioRendererFilter, targetOutputDevice);
    reply.WriteInt32(ret);
}

void AudioPolicyManagerStub::GetSelectedDeviceInfoInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t uid = data.ReadInt32();
    int32_t pid = data.ReadInt32();
    AudioStreamType streamType =  static_cast<AudioStreamType>(data.ReadInt32());

    std::string deviceName = GetSelectedDeviceInfo(uid, pid, streamType);
    reply.WriteString(deviceName);
}

void AudioPolicyManagerStub::SelectInputDeviceInternal(MessageParcel &data, MessageParcel &reply)
{
    sptr<AudioCapturerFilter> audioCapturerFilter = AudioCapturerFilter::Unmarshalling(data);
    if (audioCapturerFilter == nullptr) {
        AUDIO_ERR_LOG("AudioCapturerFilter unmarshall fail.");
        return;
    }

    int validSize = 10; // Use 10 as limit.
    int size = data.ReadInt32();
    if (size <= 0 || size > validSize) {
        AUDIO_ERR_LOG("SelectInputDevice get invalid device size.");
        return;
    }
    std::vector<sptr<AudioDeviceDescriptor>> targetInputDevice;
    for (int i = 0; i < size; i++) {
        sptr<AudioDeviceDescriptor> audioDeviceDescriptor = AudioDeviceDescriptor::Unmarshalling(data);
        if (audioDeviceDescriptor == nullptr) {
            AUDIO_ERR_LOG("Unmarshalling fail.");
            return;
        }
        targetInputDevice.push_back(audioDeviceDescriptor);
    }

    int32_t ret = SelectInputDevice(audioCapturerFilter, targetInputDevice);
    reply.WriteInt32(ret);
}

void AudioPolicyManagerStub::SetDeviceChangeCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t clientId = data.ReadInt32();
    DeviceFlag flag = static_cast<DeviceFlag>(data.ReadInt32());
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    if (object == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyManagerStub: AudioInterruptCallback obj is null");
        return;
    }
    int32_t result = SetDeviceChangeCallback(clientId, flag, object);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::UnsetDeviceChangeCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t clientId = data.ReadInt32();
    DeviceFlag flag = static_cast<DeviceFlag>(data.ReadInt32());
    int32_t result = UnsetDeviceChangeCallback(clientId, flag);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::SetInterruptCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    uint32_t sessionID = data.ReadUint32();
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    if (object == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyManagerStub: AudioInterruptCallback obj is null");
        return;
    }
    int32_t result = SetAudioInterruptCallback(sessionID, object);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::UnsetInterruptCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    uint32_t sessionID = data.ReadUint32();
    int32_t result = UnsetAudioInterruptCallback(sessionID);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::ActivateInterruptInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioInterrupt audioInterrupt = {};
    audioInterrupt.Unmarshalling(data);
    int32_t result = ActivateAudioInterrupt(audioInterrupt);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::DeactivateInterruptInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioInterrupt audioInterrupt = {};
    audioInterrupt.Unmarshalling(data);
    int32_t result = DeactivateAudioInterrupt(audioInterrupt);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::SetAudioManagerInterruptCbInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t clientId = data.ReadInt32();
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    if (object == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyManagerStub: AudioInterruptCallback obj is null");
        return;
    }
    int32_t result = SetAudioManagerInterruptCallback(clientId, object);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::UnsetAudioManagerInterruptCbInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t clientId = data.ReadInt32();
    int32_t result = UnsetAudioManagerInterruptCallback(clientId);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::RequestAudioFocusInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioInterrupt audioInterrupt = {};
    int32_t clientId = data.ReadInt32();
    audioInterrupt.Unmarshalling(data);
    int32_t result = RequestAudioFocus(clientId, audioInterrupt);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::AbandonAudioFocusInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioInterrupt audioInterrupt = {};
    int32_t clientId = data.ReadInt32();
    audioInterrupt.Unmarshalling(data);
    int32_t result = AbandonAudioFocus(clientId, audioInterrupt);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::GetStreamInFocusInternal(MessageParcel & /* data */, MessageParcel &reply)
{
    AudioStreamType streamInFocus = GetStreamInFocus();
    reply.WriteInt32(static_cast<int32_t>(streamInFocus));
}

void AudioPolicyManagerStub::GetSessionInfoInFocusInternal(MessageParcel & /* data */, MessageParcel &reply)
{
    uint32_t invalidSessionID = static_cast<uint32_t>(-1);
    AudioInterrupt audioInterrupt {STREAM_USAGE_UNKNOWN, CONTENT_TYPE_UNKNOWN,
        {AudioStreamType::STREAM_DEFAULT, SourceType::SOURCE_TYPE_INVALID, true}, invalidSessionID};
    int32_t ret = GetSessionInfoInFocus(audioInterrupt);
    audioInterrupt.Marshalling(reply);
    reply.WriteInt32(ret);
}

void AudioPolicyManagerStub::SetVolumeKeyEventCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t clientPid =  data.ReadInt32();
    sptr<IRemoteObject> remoteObject = data.ReadRemoteObject();
    API_VERSION api_v = static_cast<API_VERSION>(data.ReadInt32());
    if (remoteObject == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyManagerStub: AudioManagerCallback obj is null");
        return;
    }
    int ret = SetVolumeKeyEventCallback(clientPid, remoteObject, api_v);
    reply.WriteInt32(ret);
}

void AudioPolicyManagerStub::UnsetVolumeKeyEventCallbackInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t clientPid = data.ReadInt32();
    int ret = UnsetVolumeKeyEventCallback(clientPid);
    reply.WriteInt32(ret);
}

void AudioPolicyManagerStub::CheckRecordingCreateInternal(MessageParcel &data, MessageParcel &reply)
{
    uint32_t appTokenId = data.ReadUint32();
    uint64_t appFullTokenId = data.ReadUint64();
    uint32_t appUid = data.ReadInt32();
    bool ret = CheckRecordingCreate(appTokenId, appFullTokenId, appUid);
    reply.WriteBool(ret);
}

void AudioPolicyManagerStub::CheckRecordingStateChangeInternal(MessageParcel &data, MessageParcel &reply)
{
    uint32_t appTokenId = data.ReadUint32();
    uint64_t appFullTokenId = data.ReadUint64();
    int32_t appUid = data.ReadInt32();
    AudioPermissionState state = static_cast<AudioPermissionState>(data.ReadInt32());
    bool ret = CheckRecordingStateChange(appTokenId, appFullTokenId, appUid, state);
    reply.WriteBool(ret);
}

void AudioPolicyManagerStub::GetAudioLatencyFromXmlInternal(MessageParcel &data, MessageParcel &reply)
{
    int ret = GetAudioLatencyFromXml();
    reply.WriteInt32(ret);
}

void AudioPolicyManagerStub::GetSinkLatencyFromXmlInternal(MessageParcel &data, MessageParcel &reply)
{
    uint32_t ret = GetSinkLatencyFromXml();
    reply.WriteUint32(ret);
}

void AudioPolicyManagerStub::ReconfigureAudioChannelInternal(MessageParcel &data, MessageParcel &reply)
{
    uint32_t count = data.ReadUint32();
    DeviceType deviceType = static_cast<DeviceType>(data.ReadInt32());
    int32_t ret = ReconfigureAudioChannel(count, deviceType);
    reply.WriteInt32(ret);
}

void AudioPolicyManagerStub::RegisterAudioRendererEventListenerInternal(MessageParcel &data, MessageParcel &reply)
{
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:register event listener entered");
    int32_t clientUid =  data.ReadInt32();
    sptr<IRemoteObject> remoteObject = data.ReadRemoteObject();
    if (remoteObject == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyManagerStub: AudioRendererStateCallback obj is null");
        return;
    }
    int ret = RegisterAudioRendererEventListener(clientUid, remoteObject);
    reply.WriteInt32(ret);
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:register event listener exit");
}

void AudioPolicyManagerStub::UnregisterAudioRendererEventListenerInternal(MessageParcel &data, MessageParcel &reply)
{
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:unregister event listener entered");
    int32_t clientUid = data.ReadInt32();
    int ret = UnregisterAudioRendererEventListener(clientUid);
    reply.WriteInt32(ret);
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:unregister event listener exit");
}

void AudioPolicyManagerStub::RegisterAudioCapturerEventListenerInternal(MessageParcel &data, MessageParcel &reply)
{
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:cap register event listener entered");
    int32_t clientUid =  data.ReadInt32();
    sptr<IRemoteObject> remoteObject = data.ReadRemoteObject();
    if (remoteObject == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyManagerStub: AudioCapturerStateCallback obj is null");
        return;
    }
    int ret = RegisterAudioCapturerEventListener(clientUid, remoteObject);
    reply.WriteInt32(ret);
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:cap register event listener exit");
}

void AudioPolicyManagerStub::UnregisterAudioCapturerEventListenerInternal(MessageParcel &data, MessageParcel &reply)
{
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:cap unnregister event listener entered");
    int32_t clientUid = data.ReadInt32();
    int ret = UnregisterAudioCapturerEventListener(clientUid);
    reply.WriteInt32(ret);
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:cap unregister event listener exit");
}

void AudioPolicyManagerStub::RegisterTrackerInternal(MessageParcel &data, MessageParcel &reply)
{
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:register tracker internal entered");

    AudioStreamChangeInfo streamChangeInfo = {};
    AudioMode mode = static_cast<AudioMode> (data.ReadInt32());
    ReadStreamChangeInfo(data, mode, streamChangeInfo);
    sptr<IRemoteObject> remoteObject = data.ReadRemoteObject();
    if (remoteObject == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyManagerStub: Client Tracker obj is null");
        return;
    }

    int ret = RegisterTracker(mode, streamChangeInfo, remoteObject);
    reply.WriteInt32(ret);
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:register tracker internal ret = %{public}d", ret);
}

void AudioPolicyManagerStub::UpdateTrackerInternal(MessageParcel &data, MessageParcel &reply)
{
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:update tracker internal entered");

    AudioStreamChangeInfo streamChangeInfo = {};
    AudioMode mode = static_cast<AudioMode> (data.ReadInt32());
    ReadStreamChangeInfo(data, mode, streamChangeInfo);
    int ret = UpdateTracker(mode, streamChangeInfo);
    reply.WriteInt32(ret);
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:update tracker internal ret = %{public}d", ret);
}

void AudioPolicyManagerStub::GetRendererChangeInfosInternal(MessageParcel &data, MessageParcel &reply)
{
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:Renderer change info internal entered");

    size_t size = 0;
    vector<unique_ptr<AudioRendererChangeInfo>> audioRendererChangeInfos;
    int ret = GetCurrentRendererChangeInfos(audioRendererChangeInfos);
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("AudioPolicyManagerStub:GetRendererChangeInfos Error!!");
        reply.WriteInt32(size);
        return;
    }

    size = audioRendererChangeInfos.size();
    reply.WriteInt32(size);
    for (const unique_ptr<AudioRendererChangeInfo> &rendererChangeInfo: audioRendererChangeInfos) {
        if (!rendererChangeInfo) {
            AUDIO_ERR_LOG("AudioPolicyManagerStub:Renderer change info null, something wrong!!");
            continue;
        }
        rendererChangeInfo->Marshalling(reply);
    }

    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:Renderer change info internal exit");
}

void AudioPolicyManagerStub::GetCapturerChangeInfosInternal(MessageParcel &data, MessageParcel &reply)
{
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:Capturer change info internal entered");
    size_t size = 0;
    vector<unique_ptr<AudioCapturerChangeInfo>> audioCapturerChangeInfos;
    int32_t ret = GetCurrentCapturerChangeInfos(audioCapturerChangeInfos);
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("AudioPolicyManagerStub:GetCapturerChangeInfos Error!!");
        reply.WriteInt32(size);
        return;
    }

    size = audioCapturerChangeInfos.size();
    reply.WriteInt32(size);
    for (const unique_ptr<AudioCapturerChangeInfo> &capturerChangeInfo: audioCapturerChangeInfos) {
        if (!capturerChangeInfo) {
            AUDIO_ERR_LOG("AudioPolicyManagerStub:Capturer change info null, something wrong!!");
            continue;
        }
        capturerChangeInfo->Marshalling(reply);
    }
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:Capturer change info internal exit");
}

void AudioPolicyManagerStub::UpdateStreamStateInternal(MessageParcel &data, MessageParcel &reply)
{
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:UpdateStreamStateInternal change info internal entered");
    int32_t clientUid = data.ReadInt32();
    StreamSetState streamSetState = static_cast<StreamSetState>(data.ReadInt32());
    AudioStreamType streamType = static_cast<AudioStreamType>(data.ReadInt32());

    int32_t result = UpdateStreamState(clientUid, streamSetState, streamType);
    reply.WriteInt32(result);
    AUDIO_DEBUG_LOG("AudioPolicyManagerStub:UpdateStreamStateInternal change info internal exit");
}

void AudioPolicyManagerStub::GetVolumeGroupInfoInternal(MessageParcel& data, MessageParcel& reply)
{
    AUDIO_DEBUG_LOG("GetVolumeGroupInfoInternal entered");
    std::string networkId = data.ReadString();
    std::vector<sptr<VolumeGroupInfo>> groupInfos;
    int32_t ret = GetVolumeGroupInfos(networkId, groupInfos);
    int32_t size = static_cast<int32_t>(groupInfos.size());
    AUDIO_DEBUG_LOG("GET_DEVICES size= %{public}d", size);
    
    if (ret == SUCCESS && size > 0) {
        reply.WriteInt32(size);
        for (int i = 0; i < size; i++) {
            groupInfos[i]->Marshalling(reply);
        }
    } else {
        reply.WriteInt32(ret);
    }
    
    AUDIO_DEBUG_LOG("GetVolumeGroups internal exit");
}

void AudioPolicyManagerStub::GetNetworkIdByGroupIdInternal(MessageParcel& data, MessageParcel& reply)
{
    AUDIO_DEBUG_LOG("GetNetworkIdByGroupId entered");
    int32_t groupId = data.ReadInt32();
    std::string networkId;
    int32_t ret = GetNetworkIdByGroupId(groupId, networkId);

    reply.WriteString(networkId);
    reply.WriteInt32(ret);
}

void AudioPolicyManagerStub::IsAudioRendererLowLatencySupportedInternal(MessageParcel &data, MessageParcel &reply)
{
    AudioStreamInfo audioStreamInfo = {};
    audioStreamInfo.samplingRate = static_cast<AudioSamplingRate>(data.ReadInt32());
    audioStreamInfo.channels = static_cast<AudioChannel>(data.ReadInt32());
    audioStreamInfo.format = static_cast<OHOS::AudioStandard::AudioSampleFormat>(data.ReadInt32());
    audioStreamInfo.encoding = static_cast<AudioEncodingType>(data.ReadInt32());
    bool isSupported = IsAudioRendererLowLatencySupported(audioStreamInfo);
    reply.WriteBool(isSupported);
}

void AudioPolicyManagerStub::SetSystemSoundUriInternal(MessageParcel &data, MessageParcel &reply)
{
    std::string key = data.ReadString();
    std::string value = data.ReadString();
    int32_t result =  SetSystemSoundUri(key, value);
    reply.WriteInt32(result);
}

void AudioPolicyManagerStub::GetSystemSoundUriInternal(MessageParcel &data, MessageParcel &reply)
{
    std::string key = data.ReadString();
    std::string result = GetSystemSoundUri(key);
    reply.WriteString(result);
}

void AudioPolicyManagerStub::GetMinStreamVolumeInternal(MessageParcel &data, MessageParcel &reply)
{
    float volume = GetMinStreamVolume();
    reply.WriteFloat(volume);
}

void AudioPolicyManagerStub::GetMaxStreamVolumeInternal(MessageParcel &data, MessageParcel &reply)
{
    float volume = GetMaxStreamVolume();
    reply.WriteFloat(volume);
}

void AudioPolicyManagerStub::GetMaxRendererInstancesInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t result =  GetMaxRendererInstances();
    reply.WriteInt32(result);
}

static void PreprocessMode(SupportedEffectConfig &supportedEffectConfig, MessageParcel &reply, int32_t i, int32_t j)
{
    reply.WriteString(supportedEffectConfig.preProcessNew.stream[i].streamEffectMode[j].mode);
    int32_t countDev = supportedEffectConfig.preProcessNew.stream[i].streamEffectMode[j].devicePort.size();
    reply.WriteInt32(countDev);
    if (countDev > 0) {
        for (int32_t k = 0; k < countDev; k++) {
            reply.WriteString(supportedEffectConfig.preProcessNew.stream[i].streamEffectMode[j].devicePort[k].type);
            reply.WriteString(supportedEffectConfig.preProcessNew.stream[i].streamEffectMode[j].devicePort[k].chain);
        }
    }
}
static void PreprocessProcess(SupportedEffectConfig &supportedEffectConfig, MessageParcel &reply, int32_t i)
{
    reply.WriteString(supportedEffectConfig.preProcessNew.stream[i].scene);
    int32_t countMode = supportedEffectConfig.preProcessNew.stream[i].streamEffectMode.size();
    reply.WriteInt32(countMode);
    if (countMode > 0) {
        for (int32_t j = 0; j < countMode; j++) {
            PreprocessMode(supportedEffectConfig, reply, i, j);
        }
    }
}
static void PostprocessMode(SupportedEffectConfig &supportedEffectConfig, MessageParcel &reply, int32_t i, int32_t j)
{
    reply.WriteString(supportedEffectConfig.postProcessNew.stream[i].streamEffectMode[j].mode);
    int32_t countDev = supportedEffectConfig.postProcessNew.stream[i].streamEffectMode[j].devicePort.size();
    reply.WriteInt32(countDev);
    if (countDev > 0) {
        for (int32_t k = 0; k < countDev; k++) {
            reply.WriteString(supportedEffectConfig.postProcessNew.stream[i].streamEffectMode[j].devicePort[k].type);
            reply.WriteString(supportedEffectConfig.postProcessNew.stream[i].streamEffectMode[j].devicePort[k].chain);
        }
    }
}
static void PostprocessProcess(SupportedEffectConfig &supportedEffectConfig, MessageParcel &reply, int32_t i)
{
    // i th stream
    reply.WriteString(supportedEffectConfig.postProcessNew.stream[i].scene);
    int countMode = supportedEffectConfig.postProcessNew.stream[i].streamEffectMode.size();
    reply.WriteInt32(countMode);
    if (countMode > 0) {
        for (int32_t j = 0; j < countMode; j++) {
            PostprocessMode(supportedEffectConfig, reply, i, j);
        }
    }
}

void AudioPolicyManagerStub::QueryEffectSceneModeInternal(MessageParcel &data, MessageParcel &reply)
{
    int32_t i;
    SupportedEffectConfig supportedEffectConfig;
    int32_t ret = QueryEffectSceneMode(supportedEffectConfig); // audio_policy_server.cpp
    if (ret == -1) {
        AUDIO_ERR_LOG("default mode is unavailable !");
        return;
    }

    int32_t countPre = supportedEffectConfig.preProcessNew.stream.size();
    int32_t countPost = supportedEffectConfig.postProcessNew.stream.size();
    reply.WriteInt32(countPre);
    reply.WriteInt32(countPost);

    if (countPre > 0) {
        for (i = 0; i < countPre; i++) {
            PreprocessProcess(supportedEffectConfig, reply, i);
        }
    }
    if (countPost > 0) {
        for (i = 0; i < countPost; i++) {
            PostprocessProcess(supportedEffectConfig, reply, i);
        }
    }
}

void AudioPolicyManagerStub::SetPlaybackCapturerFilterInfosInternal(MessageParcel &data, MessageParcel &reply)
{
    uint32_t maxUsageNum = 30;
    AudioPlaybackCaptureConfig config;
    int32_t flag = data.ReadInt32();
    if (flag == 1) {
        config.silentCapture = true;
    }
    uint32_t ss = data.ReadUint32();
    if (ss >= maxUsageNum) {
        reply.WriteInt32(ERROR);
        return;
    }
    for (uint32_t i = 0; i < ss; i++) {
        int32_t tmp_usage = data.ReadInt32();
        if (std::find(AUDIO_SUPPORTED_STREAM_USAGES.begin(), AUDIO_SUPPORTED_STREAM_USAGES.end(), tmp_usage) ==
            AUDIO_SUPPORTED_STREAM_USAGES.end()) {
            continue;
        }
        config.filterOptions.usages.push_back(static_cast<StreamUsage>(tmp_usage));
    }
    uint32_t appTokenId = data.ReadUint32();

    int32_t ret = SetPlaybackCapturerFilterInfos(config, appTokenId);
    reply.WriteInt32(ret);
}

int AudioPolicyManagerStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        AUDIO_ERR_LOG("OnRemoteRequest: ReadInterfaceToken failed");
        return -1;
    }
    if (code <= static_cast<uint32_t>(AudioPolicyInterfaceCode::AUDIO_POLICY_MANAGER_CODE_MAX)) {
        (this->*handlers[code])(data, reply);
        return AUDIO_OK;
    }
    AUDIO_ERR_LOG("default case, need check AudioPolicyManagerStub");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}
} // namespace audio_policy
} // namespace OHOS
