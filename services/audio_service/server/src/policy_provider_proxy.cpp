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

#include "policy_provider_proxy.h"
#include "audio_log.h"
#include "audio_errors.h"

namespace OHOS {
namespace AudioStandard {
PolicyProviderProxy::PolicyProviderProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IPolicyProviderIpc>(impl)
{
    AUDIO_INFO_LOG("PolicyProviderProxy()");
}

PolicyProviderProxy::~PolicyProviderProxy()
{
    AUDIO_INFO_LOG("~PolicyProviderProxy()");
}

int32_t PolicyProviderProxy::GetProcessDeviceInfo(const AudioProcessConfig &config, DeviceInfo &deviceInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(data.WriteInterfaceToken(GetDescriptor()), ERROR, "Write descriptor failed!");

    ProcessConfig::WriteConfigToParcel(config, data);
    int ret = Remote()->SendRequest(IPolicyProviderMsg::GET_DEVICE_INFO, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AUDIO_OK, ERR_OPERATION_FAILED, "GetProcessDeviceInfo failed, error: %{public}d",
        ret);

    deviceInfo.deviceType = static_cast<DeviceType>(reply.ReadInt32());
    deviceInfo.deviceRole = static_cast<DeviceRole>(reply.ReadInt32());
    deviceInfo.deviceId = reply.ReadInt32();
    deviceInfo.channelMasks = reply.ReadInt32();
    deviceInfo.deviceName = reply.ReadString();
    deviceInfo.macAddress = reply.ReadString();

    // AudioStreamInfo
    deviceInfo.audioStreamInfo.samplingRate = static_cast<AudioSamplingRate>(reply.ReadInt32());
    deviceInfo.audioStreamInfo.encoding = static_cast<AudioEncodingType>(reply.ReadInt32());
    deviceInfo.audioStreamInfo.format = static_cast<AudioSampleFormat>(reply.ReadInt32());
    deviceInfo.audioStreamInfo.channels = static_cast<AudioChannel>(reply.ReadInt32());

    deviceInfo.networkId = reply.ReadString();
    deviceInfo.displayName = reply.ReadString();
    deviceInfo.interruptGroupId = reply.ReadInt32();
    deviceInfo.volumeGroupId = reply.ReadInt32();

    return SUCCESS;
}

int32_t PolicyProviderProxy::InitSharedVolume(std::shared_ptr<AudioSharedMemory> &buffer)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(data.WriteInterfaceToken(GetDescriptor()), ERROR, "Write descriptor failed!");

    int ret = Remote()->SendRequest(IPolicyProviderMsg::INIT_VOLUME_MAP, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AUDIO_OK, ERR_OPERATION_FAILED, "InitSharedVolume failed, error: %{public}d", ret);
    buffer = AudioSharedMemory::ReadFromParcel(reply);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, ERR_OPERATION_FAILED, "ReadFromParcel failed");
    return SUCCESS;
}
} // namespace AudioStandard
} // namespace OHOS
