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

#ifndef I_POLICY_PROVIDER_H
#define I_POLICY_PROVIDER_H

#include <memory>
#include <vector>

#include "audio_info.h"
#include "audio_shared_memory.h"

namespace OHOS {
namespace AudioStandard {
namespace {
    static const std::vector<std::pair<AudioVolumeType, DeviceType>> g_volumeIndexVector = {
        {STREAM_VOICE_CALL, DEVICE_TYPE_SPEAKER},
        {STREAM_VOICE_CALL, DEVICE_TYPE_WIRED_HEADSET},
        {STREAM_VOICE_CALL, DEVICE_TYPE_USB_HEADSET},
        {STREAM_VOICE_CALL, DEVICE_TYPE_WIRED_HEADPHONES},
        {STREAM_VOICE_CALL, DEVICE_TYPE_BLUETOOTH_A2DP},
        {STREAM_RING, DEVICE_TYPE_SPEAKER},
        {STREAM_RING, DEVICE_TYPE_WIRED_HEADSET},
        {STREAM_RING, DEVICE_TYPE_USB_HEADSET},
        {STREAM_RING, DEVICE_TYPE_WIRED_HEADPHONES},
        {STREAM_RING, DEVICE_TYPE_BLUETOOTH_A2DP},
        {STREAM_MUSIC, DEVICE_TYPE_SPEAKER},
        {STREAM_MUSIC, DEVICE_TYPE_WIRED_HEADSET},
        {STREAM_MUSIC, DEVICE_TYPE_USB_HEADSET},
        {STREAM_MUSIC, DEVICE_TYPE_WIRED_HEADPHONES},
        {STREAM_MUSIC, DEVICE_TYPE_BLUETOOTH_A2DP},
        {STREAM_VOICE_ASSISTANT, DEVICE_TYPE_SPEAKER},
        {STREAM_VOICE_ASSISTANT, DEVICE_TYPE_WIRED_HEADSET},
        {STREAM_VOICE_ASSISTANT, DEVICE_TYPE_USB_HEADSET},
        {STREAM_VOICE_ASSISTANT, DEVICE_TYPE_WIRED_HEADPHONES},
        {STREAM_VOICE_ASSISTANT, DEVICE_TYPE_BLUETOOTH_A2DP},
        {STREAM_ALARM, DEVICE_TYPE_SPEAKER},
        {STREAM_ALARM, DEVICE_TYPE_WIRED_HEADSET},
        {STREAM_ALARM, DEVICE_TYPE_USB_HEADSET},
        {STREAM_ALARM, DEVICE_TYPE_WIRED_HEADPHONES},
        {STREAM_ALARM, DEVICE_TYPE_BLUETOOTH_A2DP},
        {STREAM_ACCESSIBILITY, DEVICE_TYPE_SPEAKER},
        {STREAM_ACCESSIBILITY, DEVICE_TYPE_WIRED_HEADSET},
        {STREAM_ACCESSIBILITY, DEVICE_TYPE_USB_HEADSET},
        {STREAM_ACCESSIBILITY, DEVICE_TYPE_WIRED_HEADPHONES},
        {STREAM_ACCESSIBILITY, DEVICE_TYPE_BLUETOOTH_A2DP},
        {STREAM_ULTRASONIC, DEVICE_TYPE_SPEAKER},
        {STREAM_ULTRASONIC, DEVICE_TYPE_WIRED_HEADSET},
        {STREAM_ULTRASONIC, DEVICE_TYPE_USB_HEADSET},
        {STREAM_ULTRASONIC, DEVICE_TYPE_WIRED_HEADPHONES},
        {STREAM_ULTRASONIC, DEVICE_TYPE_BLUETOOTH_A2DP},
        {STREAM_ALL, DEVICE_TYPE_SPEAKER},
        {STREAM_ALL, DEVICE_TYPE_WIRED_HEADSET},
        {STREAM_ALL, DEVICE_TYPE_USB_HEADSET},
        {STREAM_ALL, DEVICE_TYPE_WIRED_HEADPHONES},
        {STREAM_ALL, DEVICE_TYPE_BLUETOOTH_A2DP}};
}
class IPolicyProvider {
public:
    virtual int32_t GetProcessDeviceInfo(const AudioProcessConfig &config, DeviceInfo &deviceInfo) = 0;

    virtual int32_t InitSharedVolume(std::shared_ptr<AudioSharedMemory> &buffer) = 0;

    virtual int32_t SetWakeUpAudioCapturerFromAudioServer(const AudioProcessConfig &config) = 0;

    virtual int32_t NotifyCapturerAdded(AudioCapturerInfo capturerInfo, AudioStreamInfo streamInfo,
        uint32_t sessionId) = 0;

    virtual int32_t NotifyWakeUpCapturerRemoved() = 0;

    virtual bool IsAbsVolumeSupported() = 0;

    virtual ~IPolicyProvider() = default;

    static bool GetVolumeIndex(AudioVolumeType streamType, DeviceType deviceType, size_t &index)
    {
        bool isFind = false;
        for (size_t tempIndex = 0; tempIndex < g_volumeIndexVector.size(); tempIndex++) {
            if (g_volumeIndexVector[tempIndex].first == streamType &&
                g_volumeIndexVector[tempIndex].second == deviceType) {
                isFind = true;
                index = tempIndex;
                break;
            }
        }
        return isFind;
    };
    static size_t GetVolumeVectorSize()
    {
        return g_volumeIndexVector.size();
    };
};
} // namespace AudioStandard
} // namespace OHOS
#endif // I_POLICY_PROVIDER_H
