/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

namespace OHOS {
namespace AudioStandard {
using namespace std;

int32_t AudioPolicyProxy::SetRingerModeCallback(const int32_t clientId,
    const sptr<IRemoteObject> &object, API_VERSION api_v)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (object == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyProxy: SetRingerModeCallback object is null");
        return ERR_NULL_OBJECT;
    }
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(clientId);
    (void)data.WriteRemoteObject(object);
    data.WriteInt32(static_cast<int32_t>(api_v));
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_RINGERMODE_CALLBACK), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: set ringermode callback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UnsetRingerModeCallback(const int32_t clientId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(clientId);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UNSET_RINGERMODE_CALLBACK), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: unset ringermode callback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::SetMicStateChangeCallback(const int32_t clientId, const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (object == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyProxy: SetMicStateChangeCallback object is null");
        return ERR_NULL_OBJECT;
    }
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }

    data.WriteInt32(clientId);
    (void)data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_MIC_STATE_CHANGE_CALLBACK), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: SetMicStateChangeCallback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::SetDeviceChangeCallback(const int32_t clientId, const DeviceFlag flag,
    const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (object == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyProxy: SetDeviceChangeCallback object is null");
        return ERR_NULL_OBJECT;
    }
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }

    data.WriteInt32(clientId);
    data.WriteInt32(flag);
    (void)data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_DEVICE_CHANGE_CALLBACK), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: SetDeviceChangeCallback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UnsetDeviceChangeCallback(const int32_t clientId, DeviceFlag flag)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(clientId);
    data.WriteInt32(flag);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UNSET_DEVICE_CHANGE_CALLBACK), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: unset device change callback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::SetPreferredOutputDeviceChangeCallback(const int32_t clientId,
    const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (object == nullptr) {
        AUDIO_ERR_LOG("SetPreferredOutputDeviceChangeCallback object is null");
        return ERR_NULL_OBJECT;
    }
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("WriteInterfaceToken failed");
        return -1;
    }

    data.WriteInt32(clientId);
    (void)data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_ACTIVE_OUTPUT_DEVICE_CHANGE_CALLBACK), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("SetPreferredOutputDeviceChangeCallback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::SetPreferredInputDeviceChangeCallback(const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (object == nullptr) {
        AUDIO_ERR_LOG("SetPreferredInputDeviceChangeCallback object is null");
        return ERR_NULL_OBJECT;
    }
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("WriteInterfaceToken failed");
        return -1;
    }

    (void)data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_ACTIVE_INPUT_DEVICE_CHANGE_CALLBACK), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("SetPreferredInputDeviceChangeCallback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UnsetPreferredOutputDeviceChangeCallback(const int32_t clientId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(clientId);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UNSET_ACTIVE_OUTPUT_DEVICE_CHANGE_CALLBACK),
        data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("unset active output device change callback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UnsetPreferredInputDeviceChangeCallback()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("WriteInterfaceToken failed");
        return -1;
    }
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UNSET_ACTIVE_INPUT_DEVICE_CHANGE_CALLBACK),
        data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("unset active input device change callback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::RegisterFocusInfoChangeCallback(const int32_t clientId,
    const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (object == nullptr) {
        AUDIO_ERR_LOG("RegisterFocusInfoChangeCallback object is null");
        return ERR_NULL_OBJECT;
    }
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("WriteInterfaceToken failed");
        return -1;
    }

    data.WriteInt32(clientId);
    (void)data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::REGISTER_FOCUS_INFO_CHANGE_CALLBACK), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("RegisterFocusInfoChangeCallback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UnregisterFocusInfoChangeCallback(const int32_t clientId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(clientId);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UNREGISTER_FOCUS_INFO_CHANGE_CALLBACK),
        data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("unset focus info change callback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::SetAudioInterruptCallback(const uint32_t sessionID, const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (object == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyProxy: SetAudioInterruptCallback object is null");
        return ERR_NULL_OBJECT;
    }
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteUint32(sessionID);
    (void)data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_CALLBACK), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: set callback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UnsetAudioInterruptCallback(const uint32_t sessionID)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteUint32(sessionID);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UNSET_CALLBACK), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: unset callback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::SetAudioManagerInterruptCallback(const int32_t clientId, const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (object == nullptr) {
        AUDIO_ERR_LOG("AudioPolicyProxy: SetAudioManagerInterruptCallback object is null");
        return ERR_NULL_OBJECT;
    }
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    data.WriteInt32(clientId);
    (void)data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_INTERRUPT_CALLBACK), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: set callback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UnsetAudioManagerInterruptCallback(const int32_t clientId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }

    data.WriteInt32(clientId);

    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UNSET_INTERRUPT_CALLBACK), data, reply, option);
    if (error != ERR_NONE) {
        AUDIO_ERR_LOG("AudioPolicyProxy: unset callback failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::SetVolumeKeyEventCallback(const int32_t clientPid,
    const sptr<IRemoteObject> &object, API_VERSION api_v)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }
    if (object == nullptr) {
        AUDIO_ERR_LOG("VolumeKeyEventCallback object is null");
        return ERR_NULL_OBJECT;
    }

    data.WriteInt32(clientPid);
    data.WriteRemoteObject(object);
    data.WriteInt32(static_cast<int32_t>(api_v));
    int result = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_VOLUME_KEY_EVENT_CALLBACK), data, reply, option);
    if (result != ERR_NONE) {
        AUDIO_ERR_LOG("SetAudioVolumeKeyEventCallback failed, result: %{public}d", result);
        return result;
    }

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UnsetVolumeKeyEventCallback(const int32_t clientPid)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        AUDIO_ERR_LOG("AudioPolicyProxy: WriteInterfaceToken failed");
        return -1;
    }

    data.WriteInt32(clientPid);
    int result = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UNSET_VOLUME_KEY_EVENT_CALLBACK), data, reply, option);
    if (result != ERR_NONE) {
        AUDIO_ERR_LOG("UnsetVolumeKeyEventCallback failed, result: %{public}d", result);
        return result;
    }

    return reply.ReadInt32();
}


int32_t AudioPolicyProxy::SetAvailableDeviceChangeCallback(const int32_t clientId, const AudioDeviceUsage usage,
    const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_NULL_OBJECT, "SetAvailableDeviceChangeCallback object is null");

    bool token = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, ERROR, "data WriteInterfaceToken failed");
    token = data.WriteInt32(clientId) && data.WriteInt32(usage);
    CHECK_AND_RETURN_RET_LOG(token, ERROR, "data write failed");

    token = data.WriteRemoteObject(object);
    CHECK_AND_RETURN_RET_LOG(token, ERROR, "data WriteRemoteObject failed");

    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::SET_AVAILABLE_DEVICE_CHANGE_CALLBACK), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, error,
        "AudioPolicyProxy: SetAvailableDeviceChangeCallback failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t AudioPolicyProxy::UnsetAvailableDeviceChangeCallback(const int32_t clientId, AudioDeviceUsage usage)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, ERROR, "data WriteInterfaceToken failed");
    token = data.WriteInt32(clientId) && data.WriteInt32(usage);
    CHECK_AND_RETURN_RET_LOG(token, ERROR, "data write failed");

    int error = Remote()->SendRequest(
        static_cast<uint32_t>(AudioPolicyInterfaceCode::UNSET_AVAILABLE_DEVICE_CHANGE_CALLBACK), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, error,
        "AudioPolicyProxy: UnsetAvailableDeviceChangeCallback failed, error: %{public}d", error);

    return reply.ReadInt32();
}
} // namespace AudioStandard
} // namespace OHOS
