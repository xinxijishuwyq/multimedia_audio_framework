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

#include "audio_bluetooth_manager.h"
#include "bluetooth_def.h"
#include "audio_errors.h"
#include "audio_log.h"
#include "bluetooth_device_manager.h"
#include "bluetooth_device_utils.h"

namespace OHOS {
namespace Bluetooth {
using namespace AudioStandard;

A2dpSource *AudioA2dpManager::a2dpInstance_ = nullptr;
std::shared_ptr<AudioA2dpListener> AudioA2dpManager::a2dpListener_ = std::make_shared<AudioA2dpListener>();
int AudioA2dpManager::connectionState_ = static_cast<int>(BTConnectState::DISCONNECTED);
BluetoothRemoteDevice AudioA2dpManager::activeA2dpDevice_;
std::mutex g_a2dpInstanceLock;
HandsFreeAudioGateway *AudioHfpManager::hfpInstance_ = nullptr;
std::shared_ptr<AudioHfpListener> AudioHfpManager::hfpListener_ = std::make_shared<AudioHfpListener>();
AudioScene AudioHfpManager::scene_ = AUDIO_SCENE_DEFAULT;
BluetoothRemoteDevice AudioHfpManager::activeHfpDevice_;
std::mutex g_hfpInstanceLock;
std::mutex g_audioSceneLock;

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
        case A2DP_L2HCV2_SAMPLE_RATE_96000_USER:
            audioStreamInfo.samplingRate = SAMPLE_RATE_96000;
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

void AudioA2dpManager::RegisterBluetoothA2dpListener()
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    std::lock_guard<std::mutex> a2dpLock(g_a2dpInstanceLock);
    a2dpInstance_ = A2dpSource::GetProfile();
    CHECK_AND_RETURN_LOG(a2dpInstance_ != nullptr, "Failed to obtain A2DP profile instance");
    a2dpInstance_->RegisterObserver(a2dpListener_);
}

void AudioA2dpManager::UnregisterBluetoothA2dpListener()
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    std::lock_guard<std::mutex> a2dpLock(g_a2dpInstanceLock);
    CHECK_AND_RETURN_LOG(a2dpInstance_ != nullptr, "A2DP profile instance unavailable");
    a2dpInstance_->DeregisterObserver(a2dpListener_);
    a2dpInstance_ = nullptr;
}

void AudioA2dpManager::DisconnectBluetoothA2dpSink()
{
    int connectionState = static_cast<int>(BTConnectState::DISCONNECTED);
    a2dpListener_->OnConnectionStateChanged(activeA2dpDevice_, connectionState);
    MediaBluetoothDeviceManager::ClearAllA2dpBluetoothDevice();
}

int32_t AudioA2dpManager::SetActiveA2dpDevice(const std::string& macAddress)
{
    std::lock_guard<std::mutex> a2dpLock(g_a2dpInstanceLock);
    a2dpInstance_ = A2dpSource::GetProfile();
    CHECK_AND_RETURN_RET_LOG(a2dpInstance_ != nullptr, ERROR, "Failed to obtain A2DP profile instance");
    BluetoothRemoteDevice device;
    if (MediaBluetoothDeviceManager::GetConnectedA2dpBluetoothDevice(macAddress, device) != SUCCESS) {
        AUDIO_ERR_LOG("SetActiveA2dpDevice: the configuring A2DP device doesn't exist.");
        return ERROR;
    }
    int32_t ret = a2dpInstance_->SetActiveSinkDevice(device);
    CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "SetActiveA2dpDevice failed. result: %{public}d", ret);
    activeA2dpDevice_ = device;
    return SUCCESS;
}

std::string AudioA2dpManager::GetActiveA2dpDevice()
{
    std::lock_guard<std::mutex> a2dpLock(g_a2dpInstanceLock);
    a2dpInstance_ = A2dpSource::GetProfile();
    CHECK_AND_RETURN_RET_LOG(a2dpInstance_ != nullptr, "", "Failed to obtain A2DP profile instance");
    BluetoothRemoteDevice device = a2dpInstance_->GetActiveSinkDevice();
    return device.GetDeviceAddr();
}

int32_t AudioA2dpManager::SetDeviceAbsVolume(const std::string& macAddress, int32_t volume)
{
    BluetoothRemoteDevice device;
    if (MediaBluetoothDeviceManager::GetConnectedA2dpBluetoothDevice(macAddress, device) != SUCCESS) {
        AUDIO_ERR_LOG("SetDeviceAbsVolume: the configuring A2DP device doesn't exist.");
        return ERROR;
    }
    return AvrcpTarget::GetProfile()->SetDeviceAbsoluteVolume(device, volume);
}

int32_t AudioA2dpManager::GetA2dpDeviceStreamInfo(const std::string& macAddress,
    AudioStreamInfo &streamInfo)
{
    std::lock_guard<std::mutex> a2dpLock(g_a2dpInstanceLock);
    a2dpInstance_ = A2dpSource::GetProfile();
    CHECK_AND_RETURN_RET_LOG(a2dpInstance_ != nullptr, ERROR, "Failed to obtain A2DP profile instance");
    BluetoothRemoteDevice device;
    if (MediaBluetoothDeviceManager::GetConnectedA2dpBluetoothDevice(macAddress, device) != SUCCESS) {
        AUDIO_ERR_LOG("GetA2dpDeviceStreamInfo: the configuring A2DP device doesn't exist.");
        return ERROR;
    }
    A2dpCodecStatus codecStatus = a2dpInstance_->GetCodecStatus(device);
    if (!GetAudioStreamInfo(codecStatus.codecInfo, streamInfo)) {
        AUDIO_ERR_LOG("GetA2dpDeviceStreamInfo: Unsupported a2dp codec info");
        return ERROR;
    }
    return SUCCESS;
}

bool AudioA2dpManager::HasA2dpDeviceConnected()
{
    a2dpInstance_ = A2dpSource::GetProfile();
    if (!a2dpInstance_) {
        return false;
    }
    std::vector<int32_t> states {static_cast<int32_t>(BTConnectState::CONNECTED)};
    std::vector<BluetoothRemoteDevice> devices;
    a2dpInstance_->GetDevicesByStates(states, devices);

    return !devices.empty();
}

int32_t AudioA2dpManager::A2dpOffloadSessionRequest(const std::vector<A2dpStreamInfo> &info)
{
    return a2dpInstance_->A2dpOffloadSessionRequest(activeA2dpDevice_, info);
}

int32_t AudioA2dpManager::OffloadStartPlaying(const std::vector<int32_t> &sessionsID)
{
    return a2dpInstance_->OffloadStartPlaying(activeA2dpDevice_, sessionsID);
}

int32_t AudioA2dpManager::OffloadStopPlaying(const std::vector<int32_t> &sessionsID)
{
    return a2dpInstance_->OffloadStopPlaying(activeA2dpDevice_, sessionsID);
}

void AudioA2dpListener::OnConnectionStateChanged(const BluetoothRemoteDevice &device, int state)
{
    AUDIO_INFO_LOG("AudioA2dpListener OnConnectionStateChanged: state: %{public}d", state);
    // Record connection state and device for hdi start time to check
    AudioA2dpManager::SetConnectionState(state);
    if (state == static_cast<int>(BTConnectState::CONNECTED)) {
        MediaBluetoothDeviceManager::SetMediaStack(device, BluetoothDeviceAction::CONNECT_ACTION);
    }
    if (state == static_cast<int>(BTConnectState::DISCONNECTED)) {
        MediaBluetoothDeviceManager::SetMediaStack(device, BluetoothDeviceAction::DISCONNECT_ACTION);
    }
}

void AudioA2dpListener::OnConfigurationChanged(const BluetoothRemoteDevice &device, const A2dpCodecInfo &codecInfo,
    int error)
{
    AUDIO_INFO_LOG("OnConfigurationChanged: sampleRate: %{public}d, channels: %{public}d, format: %{public}d",
        codecInfo.sampleRate, codecInfo.channelMode, codecInfo.bitsPerSample);
    AudioStreamInfo streamInfo = {};
    if (!GetAudioStreamInfo(codecInfo, streamInfo)) {
        AUDIO_ERR_LOG("OnConfigurationChanged: Unsupported a2dp codec info");
        return;
    }
    MediaBluetoothDeviceManager::UpdateA2dpDeviceConfiguration(device, streamInfo);
}

void AudioA2dpListener::OnPlayingStatusChanged(const BluetoothRemoteDevice &device, int playingState, int error)
{
    AUDIO_INFO_LOG("OnPlayingStatusChanged, state: %{public}d, error: %{public}d", playingState, error);
}

void AudioA2dpListener::OnMediaStackChanged(const BluetoothRemoteDevice &device, int action)
{
    AUDIO_INFO_LOG("OnMediaStackChanged, action: %{public}d", action);
    MediaBluetoothDeviceManager::SetMediaStack(device, action);
}

void AudioHfpManager::RegisterBluetoothScoListener()
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    std::lock_guard<std::mutex> hfpLock(g_hfpInstanceLock);
    hfpInstance_ = HandsFreeAudioGateway::GetProfile();
    CHECK_AND_RETURN_LOG(hfpInstance_ != nullptr, "Failed to obtain HFP AG profile instance");

    hfpInstance_->RegisterObserver(hfpListener_);
}

void AudioHfpManager::UnregisterBluetoothScoListener()
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    std::lock_guard<std::mutex> hfpLock(g_hfpInstanceLock);
    CHECK_AND_RETURN_LOG(hfpInstance_ != nullptr, "HFP AG profile instance unavailable");

    hfpInstance_->DeregisterObserver(hfpListener_);
    hfpInstance_ = nullptr;
}

int32_t AudioHfpManager::SetActiveHfpDevice(const std::string &macAddress)
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    BluetoothRemoteDevice device;
    if (HfpBluetoothDeviceManager::GetConnectedHfpBluetoothDevice(macAddress, device) != SUCCESS) {
        AUDIO_ERR_LOG("SetActiveHfpDevice failed for the HFP device does not exist.");
        return ERROR;
    }
    if (macAddress != activeHfpDevice_.GetDeviceAddr()) {
        AUDIO_INFO_LOG("Active hfp device is changed, need to DisconnectSco for current activeHfpDevice.");
        int32_t ret = DisconnectSco();
        CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "DisconnectSco failed, result: %{public}d", ret);
    }
    std::lock_guard<std::mutex> hfpLock(g_hfpInstanceLock);
    CHECK_AND_RETURN_RET_LOG(hfpInstance_ != nullptr, ERROR, "HFP AG profile instance unavailable");
    bool res = hfpInstance_->SetActiveDevice(device);
    CHECK_AND_RETURN_RET_LOG(res == true, ERROR, "SetActiveHfpDevice failed, result: %{public}d", res);
    activeHfpDevice_ = device;
    return SUCCESS;
}

std::string AudioHfpManager::GetActiveHfpDevice()
{
    std::lock_guard<std::mutex> hfpLock(g_hfpInstanceLock);
    CHECK_AND_RETURN_RET_LOG(hfpInstance_ != nullptr, "", "HFP AG profile instance unavailable");
    BluetoothRemoteDevice device = hfpInstance_->GetActiveDevice();
    return device.GetDeviceAddr();
}

int32_t AudioHfpManager::ConnectScoWithAudioScene(AudioScene scene)
{
    AUDIO_INFO_LOG("Entered %{public}s,\
        new audioScene is %{public}d, last audioScene is %{public}d", __func__, scene, scene_);
    std::lock_guard<std::mutex> sceneLock(g_audioSceneLock);
    if (scene_ == scene) {
        AUDIO_DEBUG_LOG("Current scene is not changed, ignore connectSco operation.");
        return SUCCESS;
    }
    std::lock_guard<std::mutex> hfpLock(g_hfpInstanceLock);
    CHECK_AND_RETURN_RET_LOG(hfpInstance_ != nullptr, ERROR, "HFP AG profile instance unavailable");
    if (scene == AUDIO_SCENE_RINGING && !hfpInstance_->IsInbandRingingEnabled()) {
        AUDIO_INFO_LOG("The inbarding switch is off, ignore the ring scene.");
        return SUCCESS;
    }
    int32_t ret;
    int8_t lastScoCategory = GetScoCategoryFromScene(scene_);
    if (lastScoCategory != ScoCategory::SCO_DEFAULT) {
        ret = hfpInstance_->DisconnectSco(static_cast<uint8_t>(lastScoCategory));
        CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR,
            "ConnectScoWithAudioScene failed as the last SCO failed to be disconnected, result: %{public}d", ret);
    }
    int8_t newScoCategory = GetScoCategoryFromScene(scene);
    if (newScoCategory != ScoCategory::SCO_DEFAULT) {
        AUDIO_INFO_LOG("Entered to connectSco.");
        ret = hfpInstance_->ConnectSco(static_cast<uint8_t>(newScoCategory));
        CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "ConnectScoWithAudioScene failed, result: %{public}d", ret);
    }
    scene_ = scene;
    return SUCCESS;
}

int32_t AudioHfpManager::DisconnectSco()
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    std::lock_guard<std::mutex> sceneLock(g_audioSceneLock);
    int8_t currentScoCategory = GetScoCategoryFromScene(scene_);
    if (currentScoCategory == ScoCategory::SCO_DEFAULT) {
        AUDIO_INFO_LOG("Current audioScene is not need to disconnect sco.");
        return SUCCESS;
    }
    std::lock_guard<std::mutex> hfpLock(g_hfpInstanceLock);
    CHECK_AND_RETURN_RET_LOG(hfpInstance_ != nullptr, ERROR, "HFP AG profile instance unavailable");
    int32_t ret = hfpInstance_->DisconnectSco(static_cast<uint8_t>(currentScoCategory));
    CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "DisconnectSco failed, result: %{public}d", ret);
    scene_ = AUDIO_SCENE_DEFAULT;
    return SUCCESS;
}

int8_t AudioHfpManager::GetScoCategoryFromScene(AudioScene scene)
{
    switch (scene) {
        case AUDIO_SCENE_PHONE_CALL:
            return ScoCategory::SCO_CALLULAR;
        case AUDIO_SCENE_PHONE_CHAT:
        case AUDIO_SCENE_RINGING:
            return ScoCategory::SCO_VIRTUAL;
        default:
            return ScoCategory::SCO_DEFAULT;
    }
}

void AudioHfpManager::DisconnectBluetoothHfpSink()
{
    int connectionState = static_cast<int>(BTConnectState::DISCONNECTED);
    hfpListener_->OnConnectionStateChanged(activeHfpDevice_, connectionState);
    HfpBluetoothDeviceManager::ClearAllHfpBluetoothDevice();
}

void AudioHfpListener::OnScoStateChanged(const BluetoothRemoteDevice &device, int state)
{
    AUDIO_INFO_LOG("Entered %{public}s [%{public}d]", __func__, state);
    HfpScoConnectState scoState = static_cast<HfpScoConnectState>(state);
    if (scoState == HfpScoConnectState::SCO_CONNECTED || scoState == HfpScoConnectState::SCO_DISCONNECTED) {
        bool isConnected = (scoState == HfpScoConnectState::SCO_CONNECTED) ? true : false;
        HfpBluetoothDeviceManager::OnScoStateChanged(device, isConnected);
    }
}

void AudioHfpListener::OnConnectionStateChanged(const BluetoothRemoteDevice &device, int state)
{
    AUDIO_INFO_LOG("AudioHfpListener OnConnectionStateChanged: state: %{public}d", state);
    if (state == static_cast<int>(BTConnectState::CONNECTED)) {
        HfpBluetoothDeviceManager::SetHfpStack(device, BluetoothDeviceAction::CONNECT_ACTION);
    }
    if (state == static_cast<int>(BTConnectState::DISCONNECTED)) {
        HfpBluetoothDeviceManager::SetHfpStack(device, BluetoothDeviceAction::DISCONNECT_ACTION);
    }
}

void AudioHfpListener::OnHfpStackChanged(const BluetoothRemoteDevice &device, int action)
{
    AUDIO_INFO_LOG("OnHfpStackChanged, action: %{public}d", action);
    HfpBluetoothDeviceManager::SetHfpStack(device, action);
}
} // namespace Bluetooth
} // namespace OHOS
