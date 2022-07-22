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

#include "audio_log.h"
#include "audio_system_manager.h"

namespace OHOS {
namespace AudioStandard {
/**
 * @brief The AudioDeviceDescriptor provides
 *         different sets of audio devices and their roles
 */
AudioDeviceDescriptor::AudioDeviceDescriptor(DeviceType type, DeviceRole role, int32_t interruptGroupId,
    int32_t volumeGroupId, std::string networkId)
    : deviceType_(type), deviceRole_(role), interruptGroupId_(interruptGroupId), volumeGroupId_(volumeGroupId),
    networkId_(networkId)
{}

AudioDeviceDescriptor::AudioDeviceDescriptor(DeviceType type, DeviceRole role) : deviceType_(type), deviceRole_(role)
{
    if (((deviceType_ == DEVICE_TYPE_WIRED_HEADSET) || (deviceType_ == DEVICE_TYPE_USB_HEADSET)
        || (deviceType_ == DEVICE_TYPE_BLUETOOTH_A2DP)) && (deviceRole_ == INPUT_DEVICE)) {
        deviceId_ = deviceType_ + DEVICE_TYPE_MAX;
    } else {
        deviceId_ = deviceType_;
    }

    audioStreamInfo_ = {};
    channelMasks_ = 0;
    deviceName_ = "";
    macAddress_ = "";

    volumeGroupId_ = 0;
    interruptGroupId_ = 0;
    networkId_ = LOCAL_NETWORK_ID;
}

AudioDeviceDescriptor::AudioDeviceDescriptor()
    : AudioDeviceDescriptor(DeviceType::DEVICE_TYPE_NONE, DeviceRole::DEVICE_ROLE_NONE)
{}

AudioDeviceDescriptor::AudioDeviceDescriptor(const AudioDeviceDescriptor &deviceDescriptor)
{
    deviceId_ = deviceDescriptor.deviceId_;
    deviceName_ = deviceDescriptor.deviceName_;
    macAddress_ = deviceDescriptor.macAddress_;
    deviceType_ = deviceDescriptor.deviceType_;
    deviceRole_ = deviceDescriptor.deviceRole_;
    audioStreamInfo_.channels = deviceDescriptor.audioStreamInfo_.channels;
    audioStreamInfo_.encoding = deviceDescriptor.audioStreamInfo_.encoding;
    audioStreamInfo_.format = deviceDescriptor.audioStreamInfo_.format;
    audioStreamInfo_.samplingRate = deviceDescriptor.audioStreamInfo_.samplingRate;
    channelMasks_ = deviceDescriptor.channelMasks_;
    volumeGroupId_ = deviceDescriptor.volumeGroupId_;
    interruptGroupId_ = deviceDescriptor.interruptGroupId_;
    networkId_ = deviceDescriptor.networkId_;
}

AudioDeviceDescriptor::~AudioDeviceDescriptor()
{}

bool AudioDeviceDescriptor::Marshalling(Parcel &parcel) const
{
    parcel.WriteInt32(deviceType_);
    parcel.WriteInt32(deviceRole_);
    parcel.WriteInt32(deviceId_);

    parcel.WriteInt32(audioStreamInfo_.channels);
    parcel.WriteInt32(audioStreamInfo_.encoding);
    parcel.WriteInt32(audioStreamInfo_.format);
    parcel.WriteInt32(audioStreamInfo_.samplingRate);
    parcel.WriteInt32(channelMasks_);

    parcel.WriteString(deviceName_);
    parcel.WriteString(macAddress_);

    parcel.WriteInt32(interruptGroupId_);
    parcel.WriteInt32(volumeGroupId_);
    parcel.WriteString(networkId_);
    return true;
}

sptr<AudioDeviceDescriptor> AudioDeviceDescriptor::Unmarshalling(Parcel &in)
{
    sptr<AudioDeviceDescriptor> audioDeviceDescriptor = new(std::nothrow) AudioDeviceDescriptor();
    if (audioDeviceDescriptor == nullptr) {
        return nullptr;
    }

    audioDeviceDescriptor->deviceType_ = static_cast<DeviceType>(in.ReadInt32());
    audioDeviceDescriptor->deviceRole_ = static_cast<DeviceRole>(in.ReadInt32());
    audioDeviceDescriptor->deviceId_ = in.ReadInt32();

    audioDeviceDescriptor->audioStreamInfo_.channels = static_cast<AudioChannel>(in.ReadInt32());
    audioDeviceDescriptor->audioStreamInfo_.encoding = static_cast<AudioEncodingType>(in.ReadInt32());
    audioDeviceDescriptor->audioStreamInfo_.format = static_cast<AudioSampleFormat>(in.ReadInt32());
    audioDeviceDescriptor->audioStreamInfo_.samplingRate = static_cast<AudioSamplingRate>(in.ReadInt32());
    audioDeviceDescriptor->channelMasks_ = in.ReadInt32();

    audioDeviceDescriptor->deviceName_ = in.ReadString();
    audioDeviceDescriptor->macAddress_ = in.ReadString();

    audioDeviceDescriptor->interruptGroupId_ = in.ReadInt32();
    audioDeviceDescriptor->volumeGroupId_ = in.ReadInt32();
    audioDeviceDescriptor->networkId_ = in.ReadString();

    return audioDeviceDescriptor;
}

void AudioDeviceDescriptor::SetDeviceInfo(std::string deviceName, std::string macAddress)
{
    deviceName_ = deviceName;
    macAddress_ = macAddress;
}

void AudioDeviceDescriptor::SetDeviceCapability(const AudioStreamInfo &audioStreamInfo, int32_t channelMask)
{
    audioStreamInfo_.channels = audioStreamInfo.channels;
    audioStreamInfo_.encoding = audioStreamInfo.encoding;
    audioStreamInfo_.format = audioStreamInfo.format;
    audioStreamInfo_.samplingRate = audioStreamInfo.samplingRate;
    channelMasks_ = channelMask;
}
} // namespace AudioStandard
} // namespace OHOS
