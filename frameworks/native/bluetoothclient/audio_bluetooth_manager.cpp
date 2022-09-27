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

#include "audio_bluetooth_manager.h"

#include "audio_info.h"
#include "audio_errors.h"
#include "audio_log.h"
#include "bluetooth_def.h"
#include "bluetooth_types.h"
#include "bluetooth_host.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace Bluetooth {
using namespace AudioStandard;

IDeviceStatusObserver *g_deviceObserver = nullptr;
A2dpSource *AudioA2dpManager::a2dpInstance_ = nullptr;
AudioA2dpListener AudioA2dpManager::a2dpListener_;
HandsFreeAudioGateway *AudioHfpManager::hfpInstance_ = nullptr;
AudioHfpListener AudioHfpManager::hfpListener_;
int AudioA2dpManager::connectionState_ = static_cast<int>(BTConnectState::DISCONNECTED);
bool AudioA2dpManager::bluetoothSinkLoaded_ = false;
BluetoothRemoteDevice AudioA2dpManager::bluetoothRemoteDevice_;

std::mutex g_deviceLock;
std::mutex g_a2dpInstanceLock;

static bool GetAudioStreamInfo(A2dpCodecInfo codecInfo, AudioStreamInfo &audioStreamInfo)
{
    switch (codecInfo.sampleRate) {
        case A2DP_SBC_SAMPLE_RATE_48000_USER:
            audioStreamInfo.samplingRate = SAMPLE_RATE_48000;
            break;
        case A2DP_SBC_SAMPLE_RATE_44100_USER:
            audioStreamInfo.samplingRate = SAMPLE_RATE_44100;
            break;
        case A2DP_SBC_SAMPLE_RATE_32000_USER:
            audioStreamInfo.samplingRate = SAMPLE_RATE_32000;
            break;
        case A2DP_SBC_SAMPLE_RATE_16000_USER:
            audioStreamInfo.samplingRate = SAMPLE_RATE_16000;
            break;
        default:
            return false;
    }

    switch (codecInfo.bitsPerSample) {
        case A2DP_SAMPLE_BITS_16_USER:
            audioStreamInfo.format = SAMPLE_S16LE;
            break;
        case A2DP_SAMPLE_BITS_24_USER:
            audioStreamInfo.format = SAMPLE_S24LE;
            break;
        case A2DP_SAMPLE_BITS_32_USER:
            audioStreamInfo.format = SAMPLE_S32LE;
            break;
        default:
            return false;
    }

    switch (codecInfo.channelMode) {
        case A2DP_SBC_CHANNEL_MODE_STEREO_USER:
            audioStreamInfo.channels = STEREO;
            break;
        case A2DP_SBC_CHANNEL_MODE_MONO_USER:
            audioStreamInfo.channels = MONO;
            break;
        default:
            return false;
    }

    audioStreamInfo.encoding = ENCODING_PCM;

    return true;
}

int32_t RegisterDeviceObserver(IDeviceStatusObserver &observer)
{
    std::lock_guard<std::mutex> deviceLock(g_deviceLock);
    g_deviceObserver = &observer;
    return SUCCESS;
}

void UnregisterDeviceObserver()
{
    std::lock_guard<std::mutex> deviceLock(g_deviceLock);
    g_deviceObserver = nullptr;
}

void AudioA2dpManager::RegisterBluetoothA2dpListener()
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);

    std::lock_guard<std::mutex> a2dpLock(g_a2dpInstanceLock);

    a2dpInstance_ = A2dpSource::GetProfile();
    CHECK_AND_RETURN_LOG(a2dpInstance_ != nullptr, "Failed to obtain A2DP profile instance");

    a2dpInstance_->RegisterObserver(&a2dpListener_);
}

void AudioA2dpManager::UnregisterBluetoothA2dpListener()
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);

    std::lock_guard<std::mutex> a2dpLock(g_a2dpInstanceLock);

    CHECK_AND_RETURN_LOG(a2dpInstance_ != nullptr, "A2DP profile instance unavailable");

    a2dpInstance_->DeregisterObserver(&a2dpListener_);
    a2dpInstance_ = nullptr;
}

void AudioA2dpManager::ConnectBluetoothA2dpSink()
{
    // Update device when hdi service is available
    if (connectionState_ != static_cast<int>(BTConnectState::CONNECTED)) {
        AUDIO_ERR_LOG("bluetooth state is not on");
        return;
    }

    std::lock_guard<std::mutex> deviceLock(g_deviceLock);

    if (g_deviceObserver == nullptr) {
        AUDIO_INFO_LOG("device observer is null");
        return;
    }

    AudioStreamInfo streamInfo = {};
    A2dpCodecStatus codecStatus = A2dpSource::GetProfile()->GetCodecStatus(bluetoothRemoteDevice_);
    if (!GetAudioStreamInfo(codecStatus.codecInfo, streamInfo)) {
        AUDIO_ERR_LOG("OnConnectionStateChanged: Unsupported a2dp codec info");
        return;
    }

    g_deviceObserver->OnDeviceStatusUpdated(DEVICE_TYPE_BLUETOOTH_A2DP, true,
        bluetoothRemoteDevice_.GetDeviceAddr(), bluetoothRemoteDevice_.GetDeviceName(), streamInfo);
    bluetoothSinkLoaded_ = true;
}

// Prepare for future optimization
void AudioA2dpManager::DisconnectBluetoothA2dpSink()
{
    if (bluetoothSinkLoaded_) {
        AUDIO_WARNING_LOG("bluetooth sink still loaded, some error may occur!");
    }
}

void AudioA2dpListener::OnConnectionStateChanged(const BluetoothRemoteDevice &device, int state)
{
    AUDIO_INFO_LOG("OnConnectionStateChanged: state: %{public}d", state);

    // Record connection state and device for hdi start time to check
    AudioA2dpManager::SetConnectionState(state);
    if (state == static_cast<int>(BTConnectState::CONNECTED)) {
        AudioA2dpManager::SetBluetoothRemoteDevice(device);
    }

    // Currently disconnect need to be done in OnConnectionStateChanged instead of hdi service stopped
    if (state == static_cast<int>(BTConnectState::DISCONNECTED)) {
        std::lock_guard<std::mutex> deviceLock(g_deviceLock);

        if (g_deviceObserver == nullptr) {
            AUDIO_INFO_LOG("device observer is null");
            return;
        }

        AudioStreamInfo streamInfo = {};
        g_deviceObserver->OnDeviceStatusUpdated(DEVICE_TYPE_BLUETOOTH_A2DP, false,
            device.GetDeviceAddr(), device.GetDeviceName(), streamInfo);
        AudioA2dpManager::SetBluetoothSinkLoaded(false);
    }
}

void AudioA2dpListener::OnConfigurationChanged(const BluetoothRemoteDevice &device, const A2dpCodecInfo &codecInfo,
    int error)
{
    AUDIO_INFO_LOG("OnConfigurationChanged: sampleRate: %{public}d, channels: %{public}d, format: %{public}d",
        codecInfo.sampleRate, codecInfo.channelMode, codecInfo.bitsPerSample);

    std::lock_guard<std::mutex> deviceLock(g_deviceLock);

    if (g_deviceObserver == nullptr) {
        AUDIO_INFO_LOG("device observer is null");
        return;
    }

    AudioStreamInfo streamInfo = {};
    if (!GetAudioStreamInfo(codecInfo, streamInfo)) {
        AUDIO_ERR_LOG("OnConfigurationChanged: Unsupported a2dp codec info");
        return;
    }

    g_deviceObserver->OnDeviceConfigurationChanged(DEVICE_TYPE_BLUETOOTH_A2DP, device.GetDeviceAddr(),
        device.GetDeviceName(), streamInfo);
}

void AudioA2dpListener::OnPlayingStatusChanged(const BluetoothRemoteDevice &device, int playingState, int error)
{
    AUDIO_INFO_LOG("OnPlayingStatusChanged, state: %{public}d, error: %{public}d", playingState, error);
}

void AudioHfpManager::RegisterBluetoothScoListener()
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    hfpInstance_ = HandsFreeAudioGateway::GetProfile();
    CHECK_AND_RETURN_LOG(hfpInstance_ != nullptr, "Failed to obtain HFP AG profile instance");

    hfpInstance_->RegisterObserver(&hfpListener_);
}

void AudioHfpManager::UnregisterBluetoothScoListener()
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    CHECK_AND_RETURN_LOG(hfpInstance_ != nullptr, "HFP AG profile instance unavailable");

    hfpInstance_->DeregisterObserver(&hfpListener_);
    hfpInstance_ = nullptr;
}

void AudioHfpListener::OnScoStateChanged(const BluetoothRemoteDevice &device, int state)
{
    AUDIO_INFO_LOG("Entered %{public}s [%{public}d]", __func__, state);

    std::lock_guard<std::mutex> deviceLock(g_deviceLock);

    if (g_deviceObserver == nullptr) {
        AUDIO_INFO_LOG("device observer is null");
        return;
    }

    HfpScoConnectState scoState = static_cast<HfpScoConnectState>(state);
    if (scoState == HfpScoConnectState::SCO_CONNECTED || scoState == HfpScoConnectState::SCO_DISCONNECTED) {
        AudioStreamInfo streamInfo = {};
        bool isConnected = (scoState == HfpScoConnectState::SCO_CONNECTED) ? true : false;
        g_deviceObserver->OnDeviceStatusUpdated(DEVICE_TYPE_BLUETOOTH_SCO, isConnected, device.GetDeviceAddr(),
            device.GetDeviceName(), streamInfo);
    }
}

} // namespace Bluetooth
} // namespace OHOS
