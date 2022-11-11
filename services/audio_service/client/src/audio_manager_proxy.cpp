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

#include "audio_manager_proxy.h"
#include "audio_system_manager.h"
#include "audio_log.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
AudioManagerProxy::AudioManagerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardAudioService>(impl)
{
}

int32_t AudioManagerProxy::GetMaxVolume(AudioVolumeType volumeType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(static_cast<int>(volumeType));
    int32_t error = Remote()->SendRequest(GET_MAX_VOLUME, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("Get max volume failed, error: %d", error);
        return error;
    }

    int32_t volume = reply.ReadInt32();
    return volume;
}

int32_t AudioManagerProxy::GetMinVolume(AudioVolumeType volumeType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(static_cast<int32_t>(volumeType));

    int32_t error = Remote()->SendRequest(GET_MIN_VOLUME, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("Get min volume failed, error: %d", error);
        return error;
    }

    int32_t volume = reply.ReadInt32();
    return volume;
}

int32_t AudioManagerProxy::SetMicrophoneMute(bool isMute)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteBool(isMute);
    int32_t error = Remote()->SendRequest(SET_MICROPHONE_MUTE, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("SetMicrophoneMute failed, error: %d", error);
        return error;
    }

    int32_t result = reply.ReadInt32();
    return result;
}

bool AudioManagerProxy::IsMicrophoneMute()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return false;
    }
    int32_t error = Remote()->SendRequest(IS_MICROPHONE_MUTE, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("IsMicrophoneMute failed, error: %d", error);
        return false;
    }

    bool isMute = reply.ReadBool();
    return isMute;
}

int32_t AudioManagerProxy::SetVoiceVolume(float volume)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return -1;
    }

    data.WriteFloat(volume);

    int32_t error = Remote()->SendRequest(SET_VOICE_VOLUME, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("SetVoiceVolume failed, error: %d", error);
        return false;
    }

    int32_t result = reply.ReadInt32();
    return result;
}

int32_t AudioManagerProxy::SetAudioScene(AudioScene audioScene, DeviceType activeDevice)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return -1;
    }

    data.WriteInt32(static_cast<int32_t>(audioScene));
    data.WriteInt32(static_cast<int32_t>(activeDevice));

    int32_t error = Remote()->SendRequest(SET_AUDIO_SCENE, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("SetAudioScene failed, error: %d", error);
        return false;
    }

    int32_t result = reply.ReadInt32();
    return result;
}

std::vector<sptr<AudioDeviceDescriptor>> AudioManagerProxy::GetDevices(DeviceFlag deviceFlag)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<sptr<AudioDeviceDescriptor>> deviceInfo;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return deviceInfo;
    }
    data.WriteInt32(static_cast<int32_t>(deviceFlag));

    int32_t error = Remote()->SendRequest(GET_DEVICES, data, reply, option);
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

const std::string AudioManagerProxy::GetAudioParameter(const std::string &key)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return "";
    }
    data.WriteString(static_cast<std::string>(key));
    int32_t error = Remote()->SendRequest(GET_AUDIO_PARAMETER, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("Get audio parameter failed, error: %d", error);
        const std::string value = "";
        return value;
    }

    const std::string value = reply.ReadString();
    return value;
}

const std::string AudioManagerProxy::GetAudioParameter(const std::string& networkId, const AudioParamKey key,
    const std::string& condition)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return "";
    }
    data.WriteString(static_cast<std::string>(networkId));
    data.WriteInt32(static_cast<int32_t>(key));
    data.WriteString(static_cast<std::string>(condition));
    int32_t error = Remote()->SendRequest(GET_REMOTE_AUDIO_PARAMETER, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("Get audio parameter failed, error: %d", error);
        const std::string value = "";
        return value;
    }

    const std::string value = reply.ReadString();
    return value;
}

void AudioManagerProxy::SetAudioParameter(const std::string &key, const std::string &value)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return;
    }
    data.WriteString(static_cast<std::string>(key));
    data.WriteString(static_cast<std::string>(value));
    int32_t error = Remote()->SendRequest(SET_AUDIO_PARAMETER, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("Get audio parameter failed, error: %d", error);
        return;
    }
}

void AudioManagerProxy::SetAudioParameter(const std::string& networkId, const AudioParamKey key,
    const std::string& condition, const std::string& value)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return;
    }
    data.WriteString(static_cast<std::string>(networkId));
    data.WriteInt32(static_cast<int32_t>(key));
    data.WriteString(static_cast<std::string>(condition));
    data.WriteString(static_cast<std::string>(value));
    int32_t error = Remote()->SendRequest(SET_REMOTE_AUDIO_PARAMETER, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("Get audio parameter failed, error: %d", error);
        return;
    }
}

const char *AudioManagerProxy::RetrieveCookie(int32_t &size)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    const char *cookieInfo = nullptr;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return nullptr;
    }

    int32_t error = Remote()->SendRequest(RETRIEVE_COOKIE, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("retrieve cookie failed, error: %d", error);
        return nullptr;
    }

    size = reply.ReadInt32();
    if (size > 0) {
        cookieInfo = reinterpret_cast<const char *>(reply.ReadRawData(size));
    }

    return cookieInfo;
}

uint64_t AudioManagerProxy::GetTransactionId(DeviceType deviceType, DeviceRole deviceRole)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    uint32_t transactionId = 0;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return transactionId;
    }

    data.WriteInt32(static_cast<int32_t>(deviceType));
    data.WriteInt32(static_cast<int32_t>(deviceRole));

    int32_t error = Remote()->SendRequest(GET_TRANSACTION_ID, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("get transaction id failed, error: %d", error);
        return transactionId;
    }

    transactionId = reply.ReadUint64();

    return transactionId;
}

void AudioManagerProxy::NotifyDeviceInfo(std::string networkId, bool connected)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return;
    }
    data.WriteString(networkId);
    data.WriteBool(connected);
    int32_t error = Remote()->SendRequest(NOTIFY_DEVICE_INFO, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("Get audio parameter failed, error: %d", error);
        return;
    }
}

int32_t AudioManagerProxy::CheckRemoteDeviceState(std::string networkId, DeviceRole deviceRole, bool isStartDevice)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return ERR_TRANSACTION_FAILED;
    }
    data.WriteString(networkId);
    data.WriteInt32(static_cast<int32_t>(deviceRole));
    data.WriteBool(isStartDevice);
    int32_t error = Remote()->SendRequest(CHECK_REMOTE_DEVICE_STATE, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("CheckRemoteDeviceState failed in proxy, error: %d", error);
        return error;
    }
    return reply.ReadInt32();
}

int32_t AudioManagerProxy::UpdateActiveDeviceRoute(DeviceType type, DeviceFlag flag)
{
    AUDIO_DEBUG_LOG("[%{public}s]", __func__);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioManagerProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(type);
    data.WriteInt32(flag);

    auto error = Remote()->SendRequest(UPDATE_ROUTE_REQ, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("UpdateActiveDeviceRoute failed, error: %{public}d", error);
        return false;
    }

    auto result = reply.ReadInt32();
    AUDIO_DEBUG_LOG("[UPDATE_ROUTE_REQ] result %{public}d", result);
    return result;
}

int32_t AudioManagerProxy::SetParameterCallback(const sptr<IRemoteObject>& object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (object == nullptr) {
        AUDIO_ERR_LOG("AudioManagerProxy: SetParameterCallback object is null");
        return ERR_NULL_OBJECT;
    }
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }

    (void)data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(SET_PARAMETER_CALLBACK, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: SetParameterCallback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

void AudioManagerProxy::SetAudioMonoState(bool audioMono)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return;
    }
    (void)data.WriteBool(audioMono);
    int error = Remote()->SendRequest(SET_AUDIO_MONO_STATE, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: SetAudioMonoState failed, error: %{public}d", error);
        return;
    }
}

void AudioManagerProxy::SetAudioBalanceValue(float audioBalance)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return;
    }
    (void)data.WriteFloat(audioBalance);
    int error = Remote()->SendRequest(SET_AUDIO_BALANCE_VALUE, data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: SetAudioBalanceValue failed, error: %{public}d", error);
        return;
    }
}
} // namespace AudioStandard
} // namespace OHOS
