/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <iostream>
#include <cstddef>
#include <cstdint>
#include "audio_info.h"
#include "audio_policy_server.h"
#include "audio_manager_listener_stub.h"
#include "message_parcel.h"
#include "audio_policy_client.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
const std::u16string FORMMGR_INTERFACE_TOKEN = u"IAudioPolicy";
const int32_t SYSTEM_ABILITY_ID = 3009;
const bool RUN_ON_CREATE = false;
const int32_t LIMITSIZE = 4;

void AudioVolumeFuzzTest(const uint8_t *rawData, size_t size)
{
    if (rawData == nullptr || size < LIMITSIZE) {
        return;
    }
    std::shared_ptr<AudioPolicyServer> AudioPolicyServerPtr =
        std::make_shared<AudioPolicyServer>(SYSTEM_ABILITY_ID, RUN_ON_CREATE);

    AudioStreamType streamType = *reinterpret_cast<const AudioStreamType *>(rawData);
    int32_t volume = *reinterpret_cast<const int32_t *>(rawData);
    int32_t streamId = *reinterpret_cast<const int32_t *>(rawData);
    bool mute = *reinterpret_cast<const bool *>(rawData);
    AudioPolicyServerPtr->SetSystemVolumeLevel(streamType, volume);
    AudioPolicyServerPtr->GetSystemVolumeLevel(streamType);
    AudioPolicyServerPtr->SetLowPowerVolume(streamId, volume);
    AudioPolicyServerPtr->GetLowPowerVolume(streamId);
    AudioPolicyServerPtr->GetSingleStreamVolume(streamId);
    AudioPolicyServerPtr->SetStreamMute(streamType, mute);
    AudioPolicyServerPtr->GetStreamMute(streamType);
    AudioPolicyServerPtr->IsStreamActive(streamType);
}

void AudioDeviceFuzzTest(const uint8_t *rawData, size_t size)
{
    if (rawData == nullptr || size < LIMITSIZE) {
        return;
    }
    std::shared_ptr<AudioPolicyServer> AudioPolicyServerPtr =
        std::make_shared<AudioPolicyServer>(SYSTEM_ABILITY_ID, RUN_ON_CREATE);

    MessageParcel data;
    data.WriteInterfaceToken(FORMMGR_INTERFACE_TOKEN);
    data.WriteBuffer(rawData, size);
    data.RewindRead(0);

    InternalDeviceType deviceType = *reinterpret_cast<const InternalDeviceType *>(rawData);
    bool active = *reinterpret_cast<const bool *>(rawData);
    AudioPolicyServerPtr->SetDeviceActive(deviceType, active);
    AudioPolicyServerPtr->IsDeviceActive(deviceType);

    AudioRingerMode ringMode = *reinterpret_cast<const AudioRingerMode *>(rawData);
    AudioPolicyServerPtr->SetRingerMode(ringMode);

#ifdef FEATURE_DTMF_TONE
    int32_t ltonetype = *reinterpret_cast<const int32_t *>(rawData);
    AudioPolicyServerPtr->GetToneConfig(ltonetype);
#endif

    AudioScene audioScene = *reinterpret_cast<const AudioScene *>(rawData);
    AudioPolicyServerPtr->SetAudioScene(audioScene);

    bool mute = *reinterpret_cast<const bool *>(rawData);
    AudioPolicyServerPtr->SetMicrophoneMute(mute);

    const sptr<AudioStandard::AudioDeviceDescriptor> deviceDescriptor = new AudioStandard::AudioDeviceDescriptor();
    CastType type = *reinterpret_cast<const CastType *>(rawData);
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    AudioPolicyServerPtr->ConfigDistributedRoutingRole(deviceDescriptor, type);
    AudioPolicyServerPtr->SetDistributedRoutingRoleCallback(object);
    AudioPolicyServerPtr->UnsetDistributedRoutingRoleCallback();
    AudioPolicyServerPtr->SetAudioDeviceRefinerCallback(object);
    AudioPolicyServerPtr->UnsetAudioDeviceRefinerCallback();
    AudioPolicyServerPtr->TriggerFetchDevice();
}

void AudioInterruptFuzzTest(const uint8_t *rawData, size_t size)
{
    if (rawData == nullptr || size < LIMITSIZE) {
        return;
    }
    std::shared_ptr<AudioPolicyServer> AudioPolicyServerPtr =
        std::make_shared<AudioPolicyServer>(SYSTEM_ABILITY_ID, RUN_ON_CREATE);

    MessageParcel data;
    data.WriteInterfaceToken(FORMMGR_INTERFACE_TOKEN);
    data.WriteBuffer(rawData, size);
    data.RewindRead(0);

    sptr<IRemoteObject> object = data.ReadRemoteObject();
    uint32_t sessionID = *reinterpret_cast<const uint32_t *>(rawData);
    AudioPolicyServerPtr->SetAudioInterruptCallback(sessionID, object);
    AudioPolicyServerPtr->UnsetAudioInterruptCallback(sessionID);

    int32_t clientId = *reinterpret_cast<const uint32_t *>(rawData);
    AudioPolicyServerPtr->SetAudioManagerInterruptCallback(clientId, object);

    AudioInterrupt audioInterrupt;
    audioInterrupt.contentType = *reinterpret_cast<const ContentType *>(rawData);
    audioInterrupt.streamUsage = *reinterpret_cast<const StreamUsage *>(rawData);
    audioInterrupt.audioFocusType.streamType = *reinterpret_cast<const AudioStreamType *>(rawData);
    AudioPolicyServerPtr->RequestAudioFocus(clientId, audioInterrupt);
    AudioPolicyServerPtr->AbandonAudioFocus(clientId, audioInterrupt);

    std::list<std::pair<AudioInterrupt, AudioFocuState>> focusInfoList = {};
    std::pair<AudioInterrupt, AudioFocuState> focusInfo = {};
    focusInfo.first.streamUsage = *reinterpret_cast<const StreamUsage *>(rawData);
    focusInfo.first.contentType = *reinterpret_cast<const ContentType *>(rawData);
    focusInfo.first.audioFocusType.streamType = *reinterpret_cast<const AudioStreamType *>(rawData);
    focusInfo.first.audioFocusType.sourceType = *reinterpret_cast<const SourceType *>(rawData);
    focusInfo.first.audioFocusType.isPlay = *reinterpret_cast<const bool *>(rawData);
    focusInfo.first.sessionId = *reinterpret_cast<const int32_t *>(rawData);
    focusInfo.first.pauseWhenDucked = *reinterpret_cast<const bool *>(rawData);
    focusInfo.first.pid = *reinterpret_cast<const int32_t *>(rawData);
    focusInfo.first.mode = *reinterpret_cast<const InterruptMode *>(rawData);
    focusInfo.second = *reinterpret_cast<const AudioFocuState *>(rawData);
    focusInfoList.push_back(focusInfo);
    AudioPolicyServerPtr->GetAudioFocusInfoList(focusInfoList);
}

void AudioPolicyFuzzTest(const uint8_t *rawData, size_t size)
{
    if (rawData == nullptr || size < LIMITSIZE) {
        return;
    }
    std::shared_ptr<AudioPolicyServer> AudioPolicyServerPtr =
        std::make_shared<AudioPolicyServer>(SYSTEM_ABILITY_ID, RUN_ON_CREATE);

    MessageParcel data;
    data.WriteInterfaceToken(FORMMGR_INTERFACE_TOKEN);
    data.WriteBuffer(rawData, size);
    data.RewindRead(0);

    sptr<IRemoteObject> object = data.ReadRemoteObject();
    AudioPolicyServerPtr->RegisterPolicyCallbackClient(object);

    uint32_t sessionID = *reinterpret_cast<const uint32_t *>(rawData);
    AudioPolicyServerPtr->OnSessionRemoved(sessionID);

    AudioPolicyServer::DeathRecipientId id =
        *reinterpret_cast<const AudioPolicyServer::DeathRecipientId *>(rawData);
    AudioPolicyServerPtr->RegisterClientDeathRecipient(object, id);
}

void AudioPolicyOtherFuzzTest(const uint8_t *rawData, size_t size)
{
    if (rawData == nullptr || size < LIMITSIZE) {
        return;
    }
    std::shared_ptr<AudioPolicyServer> AudioPolicyServerPtr =
        std::make_shared<AudioPolicyServer>(SYSTEM_ABILITY_ID, RUN_ON_CREATE);

    int pid = *reinterpret_cast<const int *>(rawData);
    AudioPolicyServerPtr->RegisteredTrackerClientDied(pid);

    AudioStreamInfo audioStreamInfo = {};
    audioStreamInfo.samplingRate = *reinterpret_cast<const AudioSamplingRate *>(rawData);
    audioStreamInfo.channels = *reinterpret_cast<const AudioChannel *>(rawData);
    audioStreamInfo.format = *reinterpret_cast<const AudioSampleFormat *>(rawData);
    audioStreamInfo.encoding = *reinterpret_cast<const AudioEncodingType *>(rawData);
    AudioPolicyServerPtr->IsAudioRendererLowLatencySupported(audioStreamInfo);

    int32_t clientUid = *reinterpret_cast<const int32_t *>(rawData);
    StreamSetState streamSetState = *reinterpret_cast<const StreamSetState *>(rawData);
    AudioStreamType audioStreamType = *reinterpret_cast<const AudioStreamType *>(rawData);
    AudioPolicyServerPtr->UpdateStreamState(clientUid, streamSetState, audioStreamType);
    
    int32_t sessionId = *reinterpret_cast<const int32_t *>(rawData);
    AudioPolicyServerPtr->GetAudioCapturerMicrophoneDescriptors(sessionId);

    sptr<AudioStandard::AudioDeviceDescriptor> deviceDescriptor = new AudioStandard::AudioDeviceDescriptor();
    deviceDescriptor->deviceType_ = *reinterpret_cast<const DeviceType *>(rawData);
    deviceDescriptor->deviceRole_ = *reinterpret_cast<const DeviceRole *>(rawData);
    AudioPolicyServerPtr->GetHardwareOutputSamplingRate(deviceDescriptor);

    AudioPolicyServerPtr->GetAvailableMicrophones();

    std::string macAddress(reinterpret_cast<const char*>(rawData), size - 1);
    bool support = *reinterpret_cast<const bool *>(rawData);
    int32_t volume = *reinterpret_cast<const int32_t *>(rawData);
    bool updateUi = *reinterpret_cast<const bool *>(rawData);
    AudioPolicyServerPtr->SetDeviceAbsVolumeSupported(macAddress, support);
    AudioPolicyServerPtr->SetA2dpDeviceVolume(macAddress, volume, updateUi);

    bool state = *reinterpret_cast<const bool *>(rawData);
    AudioPolicyServerPtr->SetCaptureSilentState(state);

    AudioPolicyServerPtr->IsHighResolutionExist();
    bool highResExist = *reinterpret_cast<const bool *>(rawData);
    AudioPolicyServerPtr->SetHighResolutionExist(highResExist);
}

void AudioVolumeKeyCallbackStub(const uint8_t *rawData, size_t size)
{
    if (rawData == nullptr || size < LIMITSIZE) {
        return;
    }
    sptr<AudioPolicyClientStub> listener =
        static_cast<sptr<AudioPolicyClientStub>>(new(std::nothrow) AudioPolicyClientStubImpl());
    VolumeEvent volumeEvent = {};
    volumeEvent.volumeType =  *reinterpret_cast<const AudioStreamType *>(rawData);
    volumeEvent.volume = *reinterpret_cast<const int32_t *>(rawData);
    volumeEvent.updateUi = *reinterpret_cast<const bool *>(rawData);
    volumeEvent.volumeGroupId = *reinterpret_cast<const int32_t *>(rawData);
    std::string id(reinterpret_cast<const char*>(rawData), size - 1);
    volumeEvent.networkId = id;

    MessageParcel data;
    data.WriteInt32(static_cast<int32_t>(AudioPolicyClientCode::ON_VOLUME_KEY_EVENT));
    data.WriteInt32(static_cast<int32_t>(volumeEvent.volumeType));
    data.WriteInt32(volumeEvent.volume);
    data.WriteBool(volumeEvent.updateUi);
    data.WriteInt32(volumeEvent.volumeGroupId);
    data.WriteString(volumeEvent.networkId);
    MessageParcel reply;
    MessageOption option;
    listener->OnRemoteRequest(static_cast<uint32_t>(UPDATE_CALLBACK_CLIENT), data, reply, option);
}

} // namespace AudioStandard
} // namesapce OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioStandard::AudioVolumeFuzzTest(data, size);
    OHOS::AudioStandard::AudioDeviceFuzzTest(data, size);
    OHOS::AudioStandard::AudioInterruptFuzzTest(data, size);
    OHOS::AudioStandard::AudioPolicyFuzzTest(data, size);
    OHOS::AudioStandard::AudioPolicyOtherFuzzTest(data, size);
    OHOS::AudioStandard::AudioVolumeKeyCallbackStub(data, size);
    return 0;
}

