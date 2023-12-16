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

#include "audio_policy_manager.h"
#include "audio_log.h"
#include "audio_policy_proxy.h"
#include "microphone_descriptor.h"

namespace {
constexpr int MAX_PID_COUNT = 1000;
}

namespace OHOS {
namespace AudioStandard {
using namespace std;

AudioPolicyProxy::AudioPolicyProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IAudioPolicy>(impl)
{
}

void AudioPolicyProxy::WriteStreamChangeInfo(MessageParcel &data,
    const AudioMode &mode, const AudioStreamChangeInfo &streamChangeInfo)
{
    if (mode == AUDIO_MODE_PLAYBACK) {
        streamChangeInfo.audioRendererChangeInfo.Marshalling(data);
    } else {
        streamChangeInfo.audioCapturerChangeInfo.Marshalling(data);
    }
}

int32_t AudioPolicyProxy::GetMaxVolumeLevel(AudioVolumeType volumeType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("GetMaxVolumeLevel: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(static_cast<int32_t>(volumeType));
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_MAX_VOLUMELEVEL), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get max volume failed, error: %d", error);
        return error;
    }
    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::GetMinVolumeLevel(AudioVolumeType volumeType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("GetMinVolumeLevel: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(static_cast<int32_t>(volumeType));
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_MIN_VOLUMELEVEL), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get min volume failed, error: %d", error);
        return error;
    }
    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::SetSystemVolumeLevel(AudioVolumeType volumeType, int32_t volumeLevel, API_VERSION api_v)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }

    data.WriteInt32(static_cast<int32_t>(volumeType));
    data.WriteInt32(volumeLevel);
    data.WriteInt32(static_cast<int32_t>(api_v));
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_SYSTEM_VOLUMELEVEL), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("set volume failed, error: %d", error);
        return error;
    }
    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::SetRingerMode(AudioRingerMode ringMode, API_VERSION api_v)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(static_cast<int>(ringMode));
    data.WriteInt32(static_cast<int32_t>(api_v));
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_RINGER_MODE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("set ringermode failed, error: %d", error);
        return error;
    }

    return reply.ReadInt32();
}

#ifdef FEATURE_DTMF_TONE
std::vector<int32_t> AudioPolicyProxy::GetSupportedTones()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int32_t lListSize = 0;
    AUDIO_DEBUG_LOG("get GetSupportedTones,");
    std::vector<int> lSupportedToneList = {};
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return lSupportedToneList;
    }
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_SUPPORTED_TONES), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get ringermode failed, error: %d", error);
    }
    lListSize = reply.ReadInt32();
    for (int i = 0; i < lListSize; i++) {
        lSupportedToneList.push_back(reply.ReadInt32());
    }
    AUDIO_DEBUG_LOG("get GetSupportedTones, %{public}d", lListSize);
    return lSupportedToneList;
}

std::shared_ptr<ToneInfo> AudioPolicyProxy::GetToneConfig(int32_t ltonetype)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::shared_ptr<ToneInfo> spToneInfo =  std::make_shared<ToneInfo>();
    if (spToneInfo == nullptr) {
        return nullptr;
    }
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return spToneInfo;
    }
    data.WriteInt32(ltonetype);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_TONEINFO), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get toneinfo failed, error: %d", error);
    }

    spToneInfo->Unmarshalling(reply);
    AUDIO_DEBUG_LOG("get rGetToneConfig returned,");
    return spToneInfo;
}
#endif

int32_t AudioPolicyProxy::SetMicrophoneMute(bool isMute)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteBool(isMute);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_MICROPHONE_MUTE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("set microphoneMute failed, error: %d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::SetMicrophoneMuteAudioConfig(bool isMute)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteBool(isMute);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_MICROPHONE_MUTE_AUDIO_CONFIG), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("set microphoneMute failed, error: %d", error);
        return error;
    }

    return reply.ReadInt32();
}

bool AudioPolicyProxy::IsMicrophoneMute(API_VERSION api_v)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(static_cast<int32_t>(api_v));
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::IS_MICROPHONE_MUTE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("set microphoneMute failed, error: %d", error);
        return error;
    }

    return reply.ReadBool();
}

AudioRingerMode AudioPolicyProxy::GetRingerMode()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return RINGER_MODE_NORMAL;
    }
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_RINGER_MODE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get ringermode failed, error: %d", error);
    }
    return static_cast<AudioRingerMode>(reply.ReadInt32());
}

int32_t AudioPolicyProxy::SetAudioScene(AudioScene scene)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(static_cast<int>(scene));
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_AUDIO_SCENE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("set audio scene failed, error: %d", error);
        return error;
    }

    return reply.ReadInt32();
}

AudioScene AudioPolicyProxy::GetAudioScene()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return AUDIO_SCENE_DEFAULT;
    }
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_AUDIO_SCENE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get audio scene failed, error: %d", error);
    }
    return static_cast<AudioScene>(reply.ReadInt32());
}

int32_t AudioPolicyProxy::GetSystemVolumeLevel(AudioVolumeType volumeType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(static_cast<int32_t>(volumeType));
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_SYSTEM_VOLUMELEVEL), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get volume failed, error: %d", error);
        return error;
    }
    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::SetLowPowerVolume(int32_t streamId, float volume)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }

    data.WriteInt32(streamId);
    data.WriteFloat(volume);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_LOW_POWER_STREM_VOLUME), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("set low power stream volume failed, error: %d", error);
        return error;
    }
    return reply.ReadInt32();
}

float AudioPolicyProxy::GetLowPowerVolume(int32_t streamId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(streamId);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_LOW_POWRR_STREM_VOLUME), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get low power stream volume failed, error: %d", error);
        return error;
    }
    return reply.ReadFloat();
}

float AudioPolicyProxy::GetSingleStreamVolume(int32_t streamId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(streamId);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_SINGLE_STREAM_VOLUME), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get single stream volume failed, error: %d", error);
        return error;
    }
    return reply.ReadFloat();
}

int32_t AudioPolicyProxy::SetStreamMute(AudioVolumeType volumeType, bool mute, API_VERSION api_v)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(static_cast<int32_t>(volumeType));
    data.WriteBool(mute);
    data.WriteInt32(static_cast<int32_t>(api_v));
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_STREAM_MUTE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("set mute failed, error: %d", error);
        return error;
    }
    return reply.ReadInt32();
}

bool AudioPolicyProxy::GetStreamMute(AudioVolumeType volumeType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return false;
    }
    data.WriteInt32(static_cast<int32_t>(volumeType));
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_STREAM_MUTE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get mute failed, error: %d", error);
        return false;
    }
    return reply.ReadBool();
}

bool AudioPolicyProxy::IsStreamActive(AudioVolumeType volumeType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return false;
    }
    data.WriteInt32(static_cast<int32_t>(volumeType));
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::IS_STREAM_ACTIVE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("isStreamActive failed, error: %d", error);
        return false;
    }
    return reply.ReadBool();
}

std::vector<sptr<AudioDeviceDescriptor>> AudioPolicyProxy::GetDevices(DeviceFlag deviceFlag)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<sptr<AudioDeviceDescriptor>> deviceInfo;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return deviceInfo;
    }
    data.WriteInt32(static_cast<int32_t>(deviceFlag));
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_DEVICES), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("Get devices failed, error: %d", error);
        return deviceInfo;
    }

    int32_t size = reply.ReadInt32();
    for (int32_t i = 0; i < size; i++) {
        deviceInfo.push_back(AudioDeviceDescriptor::Unmarshalling(reply));
    }

    return deviceInfo;
}

int32_t AudioPolicyProxy::SetWakeUpAudioCapturer(InternalAudioCapturerOptions options)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    options.streamInfo.Marshalling(data);
    options.capturerInfo.Marshalling(data);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_WAKEUP_AUDIOCAPTURER), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("CreateWakeUpAudioCapturer failed, error: %d", error);
        return -1;
    }
    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::CloseWakeUpAudioCapturer()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::CLOSE_WAKEUP_AUDIOCAPTURER), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("CloseWakeUpAudioCapturer failed, error: %d", error);
        return -1;
    }
    return reply.ReadInt32();
}
std::vector<sptr<AudioDeviceDescriptor>> AudioPolicyProxy::GetPreferredOutputDeviceDescriptors(
    AudioRendererInfo &rendererInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<sptr<AudioDeviceDescriptor>> deviceInfo;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return deviceInfo;
    }

    if (!rendererInfo.Marshalling(data)) {
        AUDIO_ERR_LOG("AudioRendererInfo Marshalling() failed");
        return deviceInfo;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_ACTIVE_OUTPUT_DEVICE_DESCRIPTORS), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("Get out devices failed, error: %d", error);
        return deviceInfo;
    }

    int32_t size = reply.ReadInt32();
    for (int32_t i = 0; i < size; i++) {
        deviceInfo.push_back(AudioDeviceDescriptor::Unmarshalling(reply));
    }

    return deviceInfo;
}

std::vector<sptr<AudioDeviceDescriptor>> AudioPolicyProxy::GetPreferredInputDeviceDescriptors(
    AudioCapturerInfo &captureInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<sptr<AudioDeviceDescriptor>> deviceInfo;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("GetPreferredInputDeviceDescriptors: WriteInterfaceToken failed");
        return deviceInfo;
    }

    if (!captureInfo.Marshalling(data)) {
        AUDIO_ERR_LOG("AudioCapturerInfo Marshalling() failed");
        return deviceInfo;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_PREFERRED_INTPUT_DEVICE_DESCRIPTORS), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("Get preferred input devices failed, error: %d", error);
        return deviceInfo;
    }

    int32_t size = reply.ReadInt32();
    for (int32_t i = 0; i < size; i++) {
        deviceInfo.push_back(AudioDeviceDescriptor::Unmarshalling(reply));
    }

    return deviceInfo;
}

int32_t AudioPolicyProxy::SetDeviceActive(InternalDeviceType deviceType, bool active)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(static_cast<int32_t>(deviceType));
    data.WriteBool(active);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_DEVICE_ACTIVE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("set device active failed, error: %d", error);
        return error;
    }
    return reply.ReadInt32();
}

bool AudioPolicyProxy::IsDeviceActive(InternalDeviceType deviceType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return false;
    }
    data.WriteInt32(static_cast<int32_t>(deviceType));
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::IS_DEVICE_ACTIVE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("is device active failed, error: %d", error);
        return false;
    }
    return reply.ReadBool();
}

DeviceType AudioPolicyProxy::GetActiveOutputDevice()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return DEVICE_TYPE_INVALID;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_ACTIVE_OUTPUT_DEVICE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get active output device failed, error: %d", error);
        return DEVICE_TYPE_INVALID;
    }

    return static_cast<DeviceType>(reply.ReadInt32());
}

DeviceType AudioPolicyProxy::GetActiveInputDevice()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return DEVICE_TYPE_INVALID;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_ACTIVE_INPUT_DEVICE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get active input device failed, error: %d", error);
        return DEVICE_TYPE_INVALID;
    }

    return static_cast<DeviceType>(reply.ReadInt32());
}

int32_t AudioPolicyProxy::SelectOutputDevice(sptr<AudioRendererFilter> audioRendererFilter,
    std::vector<sptr<AudioDeviceDescriptor>> audioDeviceDescriptors)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    if (!audioRendererFilter->Marshalling(data)) {
        AUDIO_ERR_LOG("AudioRendererFilter Marshalling() failed");
        return -1;
    }
    int size = audioDeviceDescriptors.size();
    int validSize = 20; // Use 20 as limit.
    if (size <= 0 || size > validSize) {
        AUDIO_ERR_LOG("SelectOutputDevice get invalid device size.");
        return -1;
    }
    data.WriteInt32(size);
    for (auto audioDeviceDescriptor : audioDeviceDescriptors) {
        if (!audioDeviceDescriptor->Marshalling(data)) {
            AUDIO_ERR_LOG("AudioDeviceDescriptor Marshalling() failed");
            return -1;
        }
    }
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SELECT_OUTPUT_DEVICE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("SelectOutputDevice failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}
std::string AudioPolicyProxy::GetSelectedDeviceInfo(int32_t uid, int32_t pid, AudioStreamType streamType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return "";
    }
    data.WriteInt32(uid);
    data.WriteInt32(pid);
    data.WriteInt32(static_cast<int32_t>(streamType));
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_SELECTED_DEVICE_INFO), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("GetSelectedDeviceInfo failed, error: %{public}d", error);
        return "";
    }

    return reply.ReadString();
}

int32_t AudioPolicyProxy::SelectInputDevice(sptr<AudioCapturerFilter> audioCapturerFilter,
    std::vector<sptr<AudioDeviceDescriptor>> audioDeviceDescriptors)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    if (!audioCapturerFilter->Marshalling(data)) {
        AUDIO_ERR_LOG("AudioCapturerFilter Marshalling() failed");
        return -1;
    }
    int size = audioDeviceDescriptors.size();
    int validSize = 20; // Use 20 as limit.
    if (size <= 0 || size > validSize) {
        AUDIO_ERR_LOG("SelectInputDevice get invalid device size.");
        return -1;
    }
    data.WriteInt32(size);
    for (auto audioDeviceDescriptor : audioDeviceDescriptors) {
        if (!audioDeviceDescriptor->Marshalling(data)) {
            AUDIO_ERR_LOG("AudioDeviceDescriptor Marshalling() failed");
            return -1;
        }
    }
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SELECT_INPUT_DEVICE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("SelectInputDevice failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::ConfigDistributedRoutingRole(const sptr<AudioDeviceDescriptor> descriptor, CastType type)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    if (descriptor == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyProxy: ConfigDistributedRoutingRole descriptor is null");
        return -1;
    }
    if (!descriptor->Marshalling(data)) {
        AUDIO_ERR_LOG("AudioDeviceDescriptor marshalling failed");
        return -1;
    }
    data.WriteInt32(static_cast<int32_t>(type));
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::CONFIG_DISTRIBUTED_ROUTING_ROLE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("ConfigDistributedRoutingRole failed error : %{public}d", error);
        return error;
    }
    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::SetDistributedRoutingRoleCallback(const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (object == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyProxy: SetDistributedRoutingRoleCallback object is null");
        return ERR_NULL_OBJECT;
    }
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    (void)data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_DISTRIBUTED_ROUTING_ROLE_CALLBACK), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("SetDistributedRoutingRoleCallback failed error : %{public}d", error);
        return error;
    }
    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UnsetDistributedRoutingRoleCallback()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, ERROR, "data writeInterfaceToken failed");

    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UNSET_DISTRIBUTED_ROUTING_ROLE_CALLBACK), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, error,
        "AudioPolicyProxy UnsetDistributedRoutingRoleCallback failed error : %{public}d", error);
    return reply.ReadInt32();
}

void AudioPolicyProxy::ReadAudioFocusInfo(MessageParcel &reply,
    std::list<std::pair<AudioInterrupt, AudioFocuState>> &focusInfoList)
{
    std::pair<AudioInterrupt, AudioFocuState> focusInfo;
    focusInfo.first.Unmarshalling(reply);
    focusInfo.second = static_cast<AudioFocuState>(reply.ReadInt32());
    focusInfoList.push_back(focusInfo);
}

int32_t AudioPolicyProxy::GetAudioFocusInfoList(std::list<std::pair<AudioInterrupt, AudioFocuState>> &focusInfoList,
    const int32_t zoneID)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG(" GetAudioFocusInfoList WriteInterfaceToken failed");
        return ERROR;
    }
    data.WriteInt32(zoneID);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_AUDIO_FOCUS_INFO_LIST), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("GetAudioFocusInfoList, error: %d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    int32_t size = reply.ReadInt32();
    focusInfoList = {};
    if (ret < 0) {
        return ret;
    } else {
        for (int32_t i = 0; i < size; i++) {
            ReadAudioFocusInfo(reply, focusInfoList);
        }
        return SUCCESS;
    }
}

int32_t AudioPolicyProxy::ActivateAudioInterrupt(const AudioInterrupt &audioInterrupt, const int32_t zoneID)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(zoneID);
    audioInterrupt.Marshalling(data);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::ACTIVATE_INTERRUPT), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: activate interrupt failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::DeactivateAudioInterrupt(const AudioInterrupt &audioInterrupt, const int32_t zoneID)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(zoneID);
    audioInterrupt.Marshalling(data);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::DEACTIVATE_INTERRUPT), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: deactivate interrupt failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::RequestAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }

    data.WriteInt32(clientId);
    audioInterrupt.Marshalling(data);

    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::REQUEST_AUDIO_FOCUS), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: activate interrupt failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::AbandonAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(clientId);
    audioInterrupt.Marshalling(data);

    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::ABANDON_AUDIO_FOCUS), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: deactivate interrupt failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

AudioStreamType AudioPolicyProxy::GetStreamInFocus(const int32_t zoneID)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return STREAM_DEFAULT;
    }
    data.WriteInt32(zoneID);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_STREAM_IN_FOCUS), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get stream in focus failed, error: %d", error);
    }
    return static_cast<AudioStreamType>(reply.ReadInt32());
}

int32_t AudioPolicyProxy::GetSessionInfoInFocus(AudioInterrupt &audioInterrupt, const int32_t zoneID)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(zoneID);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_SESSION_INFO_IN_FOCUS), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy::GetSessionInfoInFocus failed, error: %d", error);
    }
    audioInterrupt.Unmarshalling(reply);

    return reply.ReadInt32();
}

bool AudioPolicyProxy::CheckRecordingCreate(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid,
    SourceType sourceType)
{
    AUDIO_DEBUG_LOG("CheckRecordingCreate: [tid : %{public}d]", appTokenId);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("CheckRecordingCreate: WriteInterfaceToken failed");
        return false;
    }

    data.WriteUint32(appTokenId);
    data.WriteUint64(appFullTokenId);
    data.WriteInt32(appUid);
    data.WriteInt32(sourceType);

    int result = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::QUERY_MICROPHONE_PERMISSION), data, reply, option);
    if (result != ERR_NONE) {
        AUDIO_ERR_LOG("CheckRecordingCreate failed, result: %{public}d", result);
        return false;
    }

    return reply.ReadBool();
}

bool AudioPolicyProxy::CheckRecordingStateChange(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid,
    AudioPermissionState state)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("CheckRecordingStateChange: WriteInterfaceToken failed");
        return false;
    }

    data.WriteUint32(appTokenId);
    data.WriteUint64(appFullTokenId);
    data.WriteInt32(appUid);
    data.WriteInt32(state);

    int result = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_USING_PEMISSION_FROM_PRIVACY), data, reply, option);
    if (result != ERR_NONE) {
        AUDIO_ERR_LOG("CheckRecordingStateChange failed, result: %{public}d", result);
        return false;
    }

    return reply.ReadBool();
}

int32_t AudioPolicyProxy::ReconfigureAudioChannel(const uint32_t &count, DeviceType deviceType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("ReconfigureAudioChannel: WriteInterfaceToken failed");
        return IPC_PROXY_ERR;
    }

    data.WriteUint32(count);
    data.WriteInt32(deviceType);

    int result = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::RECONFIGURE_CHANNEL), data, reply, option);
    if (result != ERR_NONE) {
        AUDIO_ERR_LOG("ReconfigureAudioChannel failed, result: %{public}d", result);
        return ERR_TRANSACTION_FAILED;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::GetAudioLatencyFromXml()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: GetAudioLatencyFromXml WriteInterfaceToken failed");
        return IPC_PROXY_ERR;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_AUDIO_LATENCY), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("GetAudioLatencyFromXml, error: %d", error);
        return ERR_TRANSACTION_FAILED;
    }

    return reply.ReadInt32();
}

bool AudioPolicyProxy::IsVolumeUnadjustable()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("IsVolumeUnadjustable: WriteInterfaceToken failed");
        return false;
    }
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::IS_VOLUME_UNADJUSTABLE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("isvolumeadjustable failed, error: %d", error);
    }
    return reply.ReadBool();
}

int32_t AudioPolicyProxy::AdjustVolumeByStep(VolumeAdjustType adjustType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: AdjustVolumeByStep WriteInterfaceToken failed");
        return IPC_PROXY_ERR;
    }

    data.WriteInt32(adjustType);

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::ADJUST_VOLUME_BY_STEP), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("GetAudioLatencyFromXml, error: %d", error);
        return ERR_TRANSACTION_FAILED;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::AdjustSystemVolumeByStep(AudioVolumeType volumeType, VolumeAdjustType adjustType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: AdjustSystemVolumeByStep WriteInterfaceToken failed");
        return IPC_PROXY_ERR;
    }
    data.WriteInt32(volumeType);
    data.WriteInt32(adjustType);

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::ADJUST_SYSTEM_VOLUME_BY_STEP), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("GetAudioLatencyFromXml, error: %d", error);
        return ERR_TRANSACTION_FAILED;
    }

    return reply.ReadInt32();
}

float AudioPolicyProxy::GetSystemVolumeInDb(AudioVolumeType volumeType, int32_t volumeLevel, DeviceType deviceType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: GetSystemVolumeInDb failed");
        return false;
    }

    data.WriteInt32(static_cast<int32_t>(volumeType));
    data.WriteInt32(volumeLevel);
    data.WriteInt32(static_cast<int32_t>(deviceType));

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_SYSTEM_VOLUME_IN_DB), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("GetSystemVolumeInDb failed, error: %d", error);
    }

    return reply.ReadFloat();
}

uint32_t AudioPolicyProxy::GetSinkLatencyFromXml()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: GetSinkLatencyFromXml WriteInterfaceToken failed");
        return 0;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_SINK_LATENCY), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("GetSinkLatencyFromXml, error: %d", error);
        return 0;
    }

    return reply.ReadUint32();
}

int32_t AudioPolicyProxy::RegisterTracker(AudioMode &mode, AudioStreamChangeInfo &streamChangeInfo,
    const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("RegisterTracker WriteInterfaceToken failed");
        return ERROR;
    }

    if (object == nullptr) {
        AUDIO_ERR_LOG("Register Tracker Event object is null");
        return ERR_NULL_OBJECT;
    }

    data.WriteUint32(mode);
    WriteStreamChangeInfo(data, mode, streamChangeInfo);
    data.WriteRemoteObject(object);

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::REGISTER_TRACKER), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("RegisterTracker event failed , error: %d", error);
        return ERROR;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UpdateTracker(AudioMode &mode, AudioStreamChangeInfo &streamChangeInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("UpdateTracker: WriteInterfaceToken failed");
        return ERROR;
    }

    data.WriteUint32(mode);
    WriteStreamChangeInfo(data, mode, streamChangeInfo);

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UPDATE_TRACKER), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("UpdateTracker event failed , error: %d", error);
        return ERROR;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::GetCurrentRendererChangeInfos(
    vector<unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    AUDIO_DEBUG_LOG("AudioPolicyProxy::GetCurrentRendererChangeInfos");

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("GetCurrentRendererChangeInfo: WriteInterfaceToken failed");
        return ERROR;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_RENDERER_CHANGE_INFOS), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("Get Renderer change info event failed , error: %d", error);
        return ERROR;
    }

    int32_t size = reply.ReadInt32();
    while (size > 0) {
        unique_ptr<AudioRendererChangeInfo> rendererChangeInfo = make_unique<AudioRendererChangeInfo>();
        CHECK_AND_RETURN_RET_LOG(rendererChangeInfo != nullptr, ERR_MEMORY_ALLOC_FAILED, "No memory!!");
        rendererChangeInfo->Unmarshalling(reply);
        audioRendererChangeInfos.push_back(move(rendererChangeInfo));
        size--;
    }

    return SUCCESS;
}

int32_t AudioPolicyProxy::GetCurrentCapturerChangeInfos(
    vector<unique_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    AUDIO_DEBUG_LOG("AudioPolicyProxy::GetCurrentCapturerChangeInfos");

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("GetCurrentCapturerChangeInfos: WriteInterfaceToken failed");
        return ERROR;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_CAPTURER_CHANGE_INFOS), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("Get capturer change info event failed , error: %d", error);
        return ERROR;
    }

    int32_t size = reply.ReadInt32();
    while (size > 0) {
        unique_ptr<AudioCapturerChangeInfo> capturerChangeInfo = make_unique<AudioCapturerChangeInfo>();
        CHECK_AND_RETURN_RET_LOG(capturerChangeInfo != nullptr, ERR_MEMORY_ALLOC_FAILED, "No memory!!");
        capturerChangeInfo->Unmarshalling(reply);
        audioCapturerChangeInfos.push_back(move(capturerChangeInfo));
        size--;
    }

    return SUCCESS;
}

int32_t AudioPolicyProxy::UpdateStreamState(const int32_t clientUid, StreamSetState streamSetState,
    AudioStreamType audioStreamType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    AUDIO_DEBUG_LOG("AudioPolicyProxy::UpdateStreamState");

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("UpdateStreamState: WriteInterfaceToken failed");
        return ERROR;
    }

    data.WriteInt32(static_cast<int32_t>(clientUid));
    data.WriteInt32(static_cast<int32_t>(streamSetState));
    data.WriteInt32(static_cast<int32_t>(audioStreamType));

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UPDATE_STREAM_STATE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("UPDATE_STREAM_STATE stream changed info event failed , error: %d", error);
        return ERROR;
    }

    return SUCCESS;
}

int32_t AudioPolicyProxy::GetVolumeGroupInfos(std::string networkId, std::vector<sptr<VolumeGroupInfo>> &infos)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG(" GetVolumeGroupById WriteInterfaceToken failed");
        return ERROR;
    }
    data.WriteString(networkId);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_VOLUME_GROUP_INFO), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("GetVolumeGroupInfo, error: %d", error);
        return error;
    }

    int32_t ret = reply.ReadInt32();
    if (ret > 0) {
        for (int32_t i = 0; i < ret; i++) {
            infos.push_back(VolumeGroupInfo::Unmarshalling(reply));
        }
        return SUCCESS;
    } else {
        return ret;
    }
}

int32_t AudioPolicyProxy::GetNetworkIdByGroupId(int32_t groupId, std::string &networkId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG(" GetNetworkIdByGroupId WriteInterfaceToken failed");
        return ERROR;
    }
    data.WriteInt32(groupId);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_NETWORKID_BY_GROUP_ID), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("GetNetworkIdByGroupId, error: %d", error);
        return error;
    }

    networkId = reply.ReadString();
    int32_t ret = reply.ReadInt32();
    return ret;
}

bool AudioPolicyProxy::IsAudioRendererLowLatencySupported(const AudioStreamInfo &audioStreamInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("IsAudioRendererLowLatencySupported WriteInterfaceToken failed");
        return IPC_PROXY_ERR;
    }
    audioStreamInfo.Marshalling(data);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::IS_AUDIO_RENDER_LOW_LATENCY_SUPPORTED), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("IsAudioRendererLowLatencySupported, error: %d", error);
        return ERR_TRANSACTION_FAILED;
    }

    return reply.ReadBool();
}

int32_t AudioPolicyProxy::SetSystemSoundUri(const std::string &key, const std::string &uri)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("SetSystemSoundUri WriteInterfaceToken failed");
        return IPC_PROXY_ERR;
    }
    data.WriteString(key);
    data.WriteString(uri);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_SYSTEM_SOUND_URI), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("SetSystemSoundUri failed, error: %d", error);
        return error;
    }
    return reply.ReadInt32();
}

std::string AudioPolicyProxy::GetSystemSoundUri(const std::string &key)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("GetSystemSoundUri WriteInterfaceToken failed");
        return "";
    }
    data.WriteString(key);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_SYSTEM_SOUND_URI), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("GetSystemSoundUri failed, error: %d", error);
        return "";
    }
    return reply.ReadString();
}

float AudioPolicyProxy::GetMinStreamVolume()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("GetMinStreamVolume WriteInterfaceToken failed");
        return -1;
    }
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_MIN_VOLUME_STREAM), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get min volume for stream failed, error: %d", error);
        return error;
    }
    return reply.ReadFloat();
}

float AudioPolicyProxy::GetMaxStreamVolume()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("GetMaxStreamVolume: WriteInterfaceToken failed");
        return -1;
    }
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_MAX_VOLUME_STREAM), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get max volume for stream failed, error: %d", error);
        return error;
    }
    return reply.ReadFloat();
}

int32_t AudioPolicyProxy::GetMaxRendererInstances()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("GetMaxRendererInstances WriteInterfaceToken failed");
        return ERROR;
    }
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_MAX_RENDERER_INSTANCES), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("GetMaxRendererInstances failed, error: %d", error);
        return ERROR;
    }
    return reply.ReadInt32();
}

static void PreprocessMode(Stream &stream, MessageParcel &reply, int countMode)
{
    int j, k;
    for (j = 0; j < countMode; j++) {
        StreamEffectMode streamEffectMode;
        streamEffectMode.mode = reply.ReadString();
        int countDev = reply.ReadInt32();
        if (countDev > 0) {
            for (k = 0; k < countDev; k++) {
                string type = reply.ReadString();
                string chain = reply.ReadString();
                streamEffectMode.devicePort.push_back({type, chain});
            }
        }
        stream.streamEffectMode.push_back(streamEffectMode);
    }
}

static Stream PreprocessProcess(MessageParcel &reply)
{
    Stream stream;
    stream.scene = reply.ReadString();
    int countMode = reply.ReadInt32();
    if (countMode > 0) {
        PreprocessMode(stream, reply, countMode);
    }
    return stream;
}

static void PostprocessMode(Stream &stream, MessageParcel &reply, int countMode)
{
    int j, k;
    for (j = 0; j < countMode; j++) {
        StreamEffectMode streamEffectMode;
        streamEffectMode.mode = reply.ReadString();
        int countDev = reply.ReadInt32();
        if (countDev > 0) {
            for (k = 0; k < countDev; k++) {
                string type = reply.ReadString();
                string chain = reply.ReadString();
                streamEffectMode.devicePort.push_back({type, chain});
            }
        }
        stream.streamEffectMode.push_back(streamEffectMode);
    }
}

static Stream PostprocessProcess(MessageParcel &reply)
{
    Stream stream;
    stream.scene = reply.ReadString();
    int countMode = reply.ReadInt32();
    if (countMode > 0) {
        PostprocessMode(stream, reply, countMode);
    }
    return stream;
}

static int32_t QueryEffectSceneModeChkReply(int countPre, int countPost)
{
    if ((countPre < 0) || (countPre > AUDIO_EFFECT_COUNT_UPPER_LIMIT)) {
        AUDIO_ERR_LOG("QUERY_EFFECT_SCENEMODE read replyParcel failed");
        return -1;
    }
    if ((countPost < 0) || (countPost > AUDIO_EFFECT_COUNT_UPPER_LIMIT)) {
        AUDIO_ERR_LOG("QUERY_EFFECT_SCENEMODE read replyParcel failed");
        return -1;
    }
    return 0;
}

int32_t AudioPolicyProxy::QueryEffectSceneMode(SupportedEffectConfig &supportedEffectConfig)
{
    int i;
    int32_t error;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("QueryEffectSceneMode: WriteInterfaceToken failed");
        return -1;
    }
    error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::QUERY_EFFECT_SCENEMODE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get scene & mode failed, error: %d", error);
        return error;
    }
    int countPre = reply.ReadInt32();
    int countPost = reply.ReadInt32();
    error = QueryEffectSceneModeChkReply(countPre, countPost);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get scene & mode failed, error: %d", error);
        return error;
    }
    // preprocess
    Stream stream;
    if (countPre > 0) {
        ProcessNew preProcessNew;
        for (i = 0; i < countPre; i++) {
            stream = PreprocessProcess(reply);
            preProcessNew.stream.push_back(stream);
        }
        supportedEffectConfig.preProcessNew = preProcessNew;
    }
    // postprocess
    if (countPost > 0) {
        ProcessNew postProcessNew;
        for (i = 0; i < countPost; i++) {
            stream = PostprocessProcess(reply);
            postProcessNew.stream.push_back(stream);
        }
        supportedEffectConfig.postProcessNew = postProcessNew;
    }
    return 0;
}

int32_t AudioPolicyProxy::SetPlaybackCapturerFilterInfos(const AudioPlaybackCaptureConfig &config, uint32_t appTokenId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG(" SetPlaybackCapturerFilterInfos WriteInterfaceToken failed");
        return ERROR;
    }
    data.WriteInt32(static_cast<int32_t>(config.silentCapture));
    size_t ss = config.filterOptions.usages.size();
    data.WriteUint32(ss);
    for (size_t i = 0; i < ss; i++) {
        data.WriteInt32(static_cast<int32_t>(config.filterOptions.usages[i]));
    }
    data.WriteUint32(appTokenId);

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_PLAYBACK_CAPTURER_FILTER_INFO), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("SetPlaybackCapturerFilterInfos failed, error: %d", error);
        return ERROR;
    }
    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::GetHardwareOutputSamplingRate(const sptr<AudioDeviceDescriptor> &desc)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("GetHardwareOutputSamplingRate: WriteInterfaceToken failed");
        return -1;
    }

    if (!desc->Marshalling(data)) {
        AUDIO_ERR_LOG("AudioDeviceDescriptor Marshalling() failed");
        return -1;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_HARDWARE_OUTPUT_SAMPLING_RATE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("GetHardwareOutputSamplingRate event failed , error: %d", error);
        return ERROR;
    }

    return reply.ReadInt32();
}

std::vector<sptr<MicrophoneDescriptor>> AudioPolicyProxy::GetAudioCapturerMicrophoneDescriptors(
    int32_t sessionId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<sptr<MicrophoneDescriptor>> micDescs;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return micDescs;
    }

    data.WriteInt32(sessionId);
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_AUDIO_CAPTURER_MICROPHONE_DESCRIPTORS),
        data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("Get audio capturer microphonedescriptors failed, error: %d", error);
        return micDescs;
    }

    int32_t size = reply.ReadInt32();
    for (int32_t i = 0; i < size; i++) {
        micDescs.push_back(MicrophoneDescriptor::Unmarshalling(reply));
    }

    return micDescs;
}

std::vector<sptr<MicrophoneDescriptor>> AudioPolicyProxy::GetAvailableMicrophones()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<sptr<MicrophoneDescriptor>> micDescs;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return micDescs;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_AVAILABLE_MICROPHONE_DESCRIPTORS), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("Get available microphonedescriptors failed, error: %d", error);
        return micDescs;
    }

    int32_t size = reply.ReadInt32();
    for (int32_t i = 0; i < size; i++) {
        micDescs.push_back(MicrophoneDescriptor::Unmarshalling(reply));
    }

    return micDescs;
}

int32_t AudioPolicyProxy::SetDeviceAbsVolumeSupported(const std::string &macAddress, const bool support)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG(" SetDeviceAbsVolumeSupported WriteInterfaceToken failed");
        return ERROR;
    }
    data.WriteString(macAddress);
    data.WriteBool(support);

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_DEVICE_ABSOLUTE_VOLUME_SUPPORTED), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("SetDeviceAbsVolumeSupported failed, error: %d", error);
        return ERROR;
    }
    return reply.ReadInt32();
}

bool AudioPolicyProxy::IsAbsVolumeScene()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG(" IsAbsVolumeScene WriteInterfaceToken failed");
        return ERROR;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_ABS_VOLUME_SCENE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("IsAbsVolumeScene failed, error: %d", error);
        return ERROR;
    }
    return reply.ReadBool();
}

int32_t AudioPolicyProxy::SetA2dpDeviceVolume(const std::string &macAddress, const int32_t volume,
    const bool updateUi)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG(" SetA2dpDeviceVolume WriteInterfaceToken failed");
        return ERROR;
    }
    data.WriteString(macAddress);
    data.WriteInt32(volume);
    data.WriteBool(updateUi);

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_A2DP_DEVICE_VOLUME), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("SetA2dpDeviceVolume failed, error: %d", error);
        return ERROR;
    }
    return reply.ReadInt32();
}

std::vector<std::unique_ptr<AudioDeviceDescriptor>> AudioPolicyProxy::GetAvailableDevices(AudioDeviceUsage usage)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<std::unique_ptr<AudioDeviceDescriptor>> audioDeviceDescriptors;

    CHECK_AND_RETURN_RET_LOG(data.WriteInterfaceToken(GetDescriptor()),
        audioDeviceDescriptors, "WriteInterfaceToken failed");

    bool token = data.WriteInt32(static_cast<int32_t>(usage));
    CHECK_AND_RETURN_RET_LOG(token, audioDeviceDescriptors, "WriteInt32 usage failed");

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_AVAILABLE_DESCRIPTORS), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, audioDeviceDescriptors,
        "GetAvailableDevices failed, error: %d", error);

    int32_t size = reply.ReadInt32();
    for (int32_t i = 0; i < size; i++) {
        std::unique_ptr<AudioDeviceDescriptor> desc =
            std::make_unique<AudioDeviceDescriptor>(AudioDeviceDescriptor::Unmarshalling(reply));
        audioDeviceDescriptors.push_back(move(desc));
    }
    return audioDeviceDescriptors;
}

bool AudioPolicyProxy::IsSpatializationEnabled()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("IsSpatializationEnabled WriteInterfaceToken failed");
        return false;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::IS_SPATIALIZATION_ENABLED), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("IsSpatializationEnabled failed, error: %{public}d", error);
        return false;
    }
    return reply.ReadBool();
}

int32_t AudioPolicyProxy::SetSpatializationEnabled(const bool enable)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("SetSpatializationEnabled WriteInterfaceToken failed");
        return ERROR;
    }
    data.WriteBool(enable);

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_SPATIALIZATION_ENABLED), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("SetSpatializationEnabled failed, error: %{public}d", error);
        return ERROR;
    }
    return reply.ReadInt32();
}

bool AudioPolicyProxy::IsHeadTrackingEnabled()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("IsHeadTrackingEnabled WriteInterfaceToken failed");
        return false;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::IS_HEAD_TRACKING_ENABLED), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("IsHeadTrackingEnabled failed, error: %{public}d", error);
        return false;
    }
    return reply.ReadBool();
}

int32_t AudioPolicyProxy::SetHeadTrackingEnabled(const bool enable)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("SetHeadTrackingEnabled WriteInterfaceToken failed");
        return ERROR;
    }
    data.WriteBool(enable);

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_HEAD_TRACKING_ENABLED), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("SetHeadTrackingEnabled failed, error: %{public}d", error);
        return ERROR;
    }
    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::RegisterSpatializationEnabledEventListener(const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("RegisterSpatializationEnabledEventListener:: WriteInterfaceToken failed");
        return ERROR;
    }
    if (object == nullptr) {
        AUDIO_ERR_LOG("RegisterSpatializationEnabledEventListener Event object is null");
        return ERR_NULL_OBJECT;
    }

    data.WriteRemoteObject(object);
    int32_t error = Remote() ->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::REGISTER_SPATIALIZATION_ENABLED_EVENT), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("RegisterSpatializationEnabledEventListener failed , error: %{public}d", error);
        return ERROR;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::RegisterHeadTrackingEnabledEventListener(const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("RegisterHeadTrackingEnabledEventListener:: WriteInterfaceToken failed");
        return ERROR;
    }
    if (object == nullptr) {
        AUDIO_ERR_LOG("RegisterHeadTrackingEnabledEventListener Event object is null");
        return ERR_NULL_OBJECT;
    }

    data.WriteRemoteObject(object);
    int32_t error = Remote() ->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::REGISTER_HEAD_TRACKING_ENABLED_EVENT), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("RegisterHeadTrackingEnabledEventListener failed , error: %{public}d", error);
        return ERROR;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UnregisterSpatializationEnabledEventListener()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("UnregisterSpatializationEnabledEventListener:: WriteInterfaceToken failed");
        return ERROR;
    }

    int32_t error = Remote() ->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UNREGISTER_SPATIALIZATION_ENABLED_EVENT), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("UnregisterSpatializationEnabledEventListener failed , error: %{public}d", error);
        return ERROR;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UnregisterHeadTrackingEnabledEventListener()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("UnregisterHeadTrackingEnabledEventListener:: WriteInterfaceToken failed");
        return ERROR;
    }

    int32_t error = Remote() ->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UNREGISTER_HEAD_TRACKING_ENABLED_EVENT), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("UnregisterHeadTrackingEnabledEventListener failed , error: %{public}d", error);
        return ERROR;
    }

    return reply.ReadInt32();
}

AudioSpatializationState AudioPolicyProxy::GetSpatializationState(const StreamUsage streamUsage)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    AudioSpatializationState spatializationState = {false, false};

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("GetSpatializationState:: WriteInterfaceToken failed");
        return spatializationState;
    }

    data.WriteInt32(static_cast<int32_t>(streamUsage));
    int32_t error = Remote() ->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::GET_SPATIALIZATION_STATE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("GetSpatializationState failed , error: %{public}d", error);
        return spatializationState;
    }

    spatializationState.spatializationEnabled = reply.ReadBool();
    spatializationState.headTrackingEnabled = reply.ReadBool();

    return spatializationState;
}

bool AudioPolicyProxy::IsSpatializationSupported()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("IsSpatializationSupported WriteInterfaceToken failed");
        return false;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::IS_SPATIALIZATION_SUPPORTED), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("IsSpatializationSupported failed, error: %d", error);
        return false;
    }
    return reply.ReadBool();
}

bool AudioPolicyProxy::IsSpatializationSupportedForDevice(const std::string address)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("IsSpatializationSupportedForDevice WriteInterfaceToken failed");
        return ERROR;
    }
    data.WriteString(address);

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::IS_SPATIALIZATION_SUPPORTED_FOR_DEVICE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("IsSpatializationSupportedForDevice failed, error: %d", error);
        return ERROR;
    }
    return reply.ReadBool();
}

bool AudioPolicyProxy::IsHeadTrackingSupported()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("IsHeadTrackingSupported WriteInterfaceToken failed");
        return false;
    }

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::IS_HEAD_TRACKING_SUPPORTED), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("IsHeadTrackingSupported failed, error: %d", error);
        return false;
    }
    return reply.ReadBool();
}

bool AudioPolicyProxy::IsHeadTrackingSupportedForDevice(const std::string address)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG(" IsHeadTrackingSupportedForDevice WriteInterfaceToken failed");
        return ERROR;
    }
    data.WriteString(address);

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::IS_HEAD_TRACKING_SUPPORTED_FOR_DEVICE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("IsHeadTrackingSupportedForDevice failed, error: %d", error);
        return ERROR;
    }
    return reply.ReadBool();
}

int32_t AudioPolicyProxy::UpdateSpatialDeviceState(const AudioSpatialDeviceState audioSpatialDeviceState)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG(" UpdateSpatialDeviceState WriteInterfaceToken failed");
        return ERROR;
    }
    data.WriteString(audioSpatialDeviceState.address);
    data.WriteBool(audioSpatialDeviceState.isSpatializationSupported);
    data.WriteBool(audioSpatialDeviceState.isHeadTrackingSupported);
    data.WriteInt32(static_cast<int32_t>(audioSpatialDeviceState.spatialDeviceType));

    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UPDATE_SPATIAL_DEVICE_STATE), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("UpdateSpatialDeviceState failed, error: %d", error);
        return ERROR;
    }
    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::RegisterSpatializationStateEventListener(const uint32_t sessionID,
    const StreamUsage streamUsage, const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("RegisterSpatializationStateEventListener:: WriteInterfaceToken failed");
        return ERROR;
    }
    if (object == nullptr) {
        AUDIO_ERR_LOG("RegisterSpatializationStateEventListener Event object is null");
        return ERR_NULL_OBJECT;
    }

    data.WriteInt32(static_cast<int32_t>(sessionID));
    data.WriteInt32(static_cast<int32_t>(streamUsage));
    data.WriteRemoteObject(object);
    int32_t error = Remote() ->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::REGISTER_SPATIALIZATION_STATE_EVENT), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("RegisterSpatializationStateEventListener failed , error: %{public}d", error);
        return ERROR;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UnregisterSpatializationStateEventListener(const uint32_t sessionID)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("UnregisterSpatializationStateEventListener:: WriteInterfaceToken failed");
        return ERROR;
    }

    data.WriteInt32(static_cast<int32_t>(sessionID));
    int32_t error = Remote() ->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UNREGISTER_SPATIALIZATION_STATE_EVENT), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("UnregisterSpatializationStateEventListener failed , error: %{public}d", error);
        return ERROR;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::RegisterPolicyCallbackClient(const sptr<IRemoteObject> &object, const int32_t zoneID)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (object == nullptr) {
        AUDIO_ERR_LOG("RegisterPolicyCallbackClient object is null");
        return ERR_NULL_OBJECT;
    }
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("WriteInterfaceToken failed");
        return -1;
    }

    data.WriteRemoteObject(object);
    data.WriteInt32(zoneID);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::REGISTER_POLICY_CALLBACK_CLIENT), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("RegisterPolicyCallbackClient failed, error: %d", error);
        return ERROR;
    }
    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::CreateAudioInterruptZone(const std::set<int32_t> pids, const int32_t zoneID)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    CHECK_AND_RETURN_RET_LOG(!data.WriteInterfaceToken(GetDescriptor()), -1, "WriteInterfaceToken failed");

    data.WriteInt32(zoneID);
    data.WriteInt32(pids.size());
    int32_t count = 0;
    for (int32_t pid : pids) {
        data.WriteInt32(pid);
        count++;
        if (count >= MAX_PID_COUNT) {
            break;
        }
    }

    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::CREATE_AUDIO_INTERRUPT_ZONE), data, reply, option);

    CHECK_AND_RETURN_RET_LOG(error != ERR_NONE, ERROR, "CreateAudioInterruptZone failed, error: %d", error);
    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::AddAudioInterruptZonePids(const std::set<int32_t> pids, const int32_t zoneID)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    CHECK_AND_RETURN_RET_LOG(!data.WriteInterfaceToken(GetDescriptor()), -1, "WriteInterfaceToken failed");
    data.WriteInt32(zoneID);
    data.WriteInt32(pids.size());
    int32_t count = 0;
    for (int32_t pid : pids) {
        data.WriteInt32(pid);
        count++;
        if (count >= MAX_PID_COUNT) {
            break;
        }
    }

    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::ADD_AUDIO_INTERRUPT_ZONE_PIDS), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error != ERR_NONE, ERROR, "AddAudioInterruptZonePids failed, error: %d", error);
    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::RemoveAudioInterruptZonePids(const std::set<int32_t> pids, const int32_t zoneID)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    CHECK_AND_RETURN_RET_LOG(!data.WriteInterfaceToken(GetDescriptor()), -1, "WriteInterfaceToken failed");
    data.WriteInt32(zoneID);
    data.WriteInt32(pids.size());
    int32_t count = 0;
    for (int32_t pid : pids) {
        data.WriteInt32(pid);
        count++;
        if (count >= MAX_PID_COUNT) {
            break;
        }
    }
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::REMOVE_AUDIO_INTERRUPT_ZONE_PIDS), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error != ERR_NONE, ERROR, "RemoveAudioInterruptZonePids failed, error: %d", error);

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::ReleaseAudioInterruptZone(const int32_t zoneID)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    CHECK_AND_RETURN_RET_LOG(!data.WriteInterfaceToken(GetDescriptor()), -1, "WriteInterfaceToken failed");
    data.WriteInt32(zoneID);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::RELEASE_AUDIO_INTERRUPT_ZONE), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error != ERR_NONE, ERROR, "ReleaseAudioInterruptZone failed, error: %d", error);
    return reply.ReadInt32();
}
} // namespace AudioStandard
} // namespace OHOS
