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

#ifndef AUDIO_BLUETOOTH_MANAGERI_H
#define AUDIO_BLUETOOTH_MANAGERI_H

#include "bluetooth_a2dp_src.h"
#include "bluetooth_a2dp_codec.h"
#include "bluetooth_hfp_ag.h"
#include "idevice_status_observer.h"

namespace OHOS {
namespace Bluetooth {

int32_t RegisterDeviceObserver(AudioStandard::IDeviceStatusObserver &observer);
void UnregisterDeviceObserver();

// Audio bluetooth a2dp feature support
class AudioA2dpListener : public A2dpSourceObserver {
public:
    AudioA2dpListener() = default;
    virtual ~AudioA2dpListener() = default;

    virtual void OnConnectionStateChanged(const BluetoothRemoteDevice &device, int state);
    virtual void OnConfigurationChanged(const BluetoothRemoteDevice &device, const A2dpCodecInfo &info, int error);
    virtual void OnPlayingStatusChanged(const BluetoothRemoteDevice &device, int playingState, int error);

private:
    BLUETOOTH_DISALLOW_COPY_AND_ASSIGN(AudioA2dpListener);
};

class AudioA2dpManager {
public:
    AudioA2dpManager() = default;
    virtual ~AudioA2dpManager() = default;
    static void RegisterBluetoothA2dpListener();
    static void UnregisterBluetoothA2dpListener();
    static void ConnectBluetoothA2dpSink();
    static void DisconnectBluetoothA2dpSink();
    static void SetConnectionState(int state)
    {
        connectionState_ = state;
    }
    static int GetConnectionState()
    {
        return connectionState_;
    }
    static void SetBluetoothSinkLoaded(bool isLoaded)
    {
        bluetoothSinkLoaded_ = isLoaded;
    }
    static bool GetBluetoothSinkLoaded()
    {
        return bluetoothSinkLoaded_;
    }
    static void SetBluetoothRemoteDevice(BluetoothRemoteDevice device)
    {
        bluetoothRemoteDevice_ = device;
    }
    static BluetoothRemoteDevice GetBluetoothRemoteDevice()
    {
        return bluetoothRemoteDevice_;
    }

private:
    static A2dpSource *a2dpInstance_;
    static AudioA2dpListener a2dpListener_;
    static int connectionState_;
    static bool bluetoothSinkLoaded_;
    static BluetoothRemoteDevice bluetoothRemoteDevice_;
};

// Audio bluetooth sco feature support
class AudioHfpListener : public HandsFreeAudioGatewayObserver {
public:
    AudioHfpListener() = default;
    virtual ~AudioHfpListener() = default;

    void OnScoStateChanged(const BluetoothRemoteDevice &device, int state);
    void OnConnectionStateChanged(const BluetoothRemoteDevice &device, int state) {}
    void OnActiveDeviceChanged(const BluetoothRemoteDevice &device) {}
    void OnHfEnhancedDriverSafetyChanged(const BluetoothRemoteDevice &device, int indValue) {}

private:
    BLUETOOTH_DISALLOW_COPY_AND_ASSIGN(AudioHfpListener);
};

class AudioHfpManager {
public:
    AudioHfpManager() = default;
    virtual ~AudioHfpManager() = default;
    static void RegisterBluetoothScoListener();
    static void UnregisterBluetoothScoListener();

private:
    static HandsFreeAudioGateway *hfpInstance_;
    static AudioHfpListener hfpListener_;
};
}
}
#endif  // AUDIO_BLUETOOTH_MANAGERI_H
