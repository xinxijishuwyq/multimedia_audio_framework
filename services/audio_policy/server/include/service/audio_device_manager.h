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
#ifndef ST_AUDIO_DEVICE_MANAGER_H
#define ST_AUDIO_DEVICE_MANAGER_H

#include <list>
#include <string>
#include <memory>
#include <unordered_map>
#include "audio_info.h"
#include "audio_device_info.h"
#include "audio_system_manager.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;

typedef function<bool(const std::unique_ptr<AudioDeviceDescriptor> &desc)> IsPresentFunc;

class AudioDeviceManager {
public:
    static AudioDeviceManager& GetAudioDeviceManager()
    {
        static AudioDeviceManager audioDeviceManager;
        return audioDeviceManager;
    }

    void AddNewDevice(AudioDeviceDescriptor &devDesc);
    void RemoveNewDevice(const AudioDeviceDescriptor &devDesc);
    void OnXmlParsingCompleted(const unordered_map<AudioDevicePrivacyType, list<DevicePrivacyInfo>> &xmlData);
    int32_t GetDeviceUsageFromType(const DeviceType devType) const;
    void ParseDeviceXml();

    vector<unique_ptr<AudioDeviceDescriptor>> GetRemoteRenderDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetRemoteCaptureDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetCommRenderPrivacyDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetCommRenderPublicDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetCommCapturePrivacyDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetCommCapturePublicDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetMediaRenderPrivacyDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetMediaRenderPublicDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetMediaCapturePrivacyDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetMediaCapturePublicDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetCapturePrivacyDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetCapturePublicDevices();

private:
    AudioDeviceManager() {};
    ~AudioDeviceManager() {};
    bool DeviceAttrMatch(const AudioDeviceDescriptor &devDesc, AudioDevicePrivacyType &privacyType,
        DeviceRole &devRole, DeviceUsage &devUsage);
    void FillArrayWhenDeviceAttrMatch(vector<unique_ptr<AudioDeviceDescriptor>> &descArray,
        const AudioDeviceDescriptor &devDesc, AudioDevicePrivacyType privacyType, DeviceRole devRole,
        DeviceUsage devUsage, string logName = "device");
    void RemoveMatchDeviceInArray(vector<unique_ptr<AudioDeviceDescriptor>> &descArray,
        const AudioDeviceDescriptor &devDesc, string logName);

    void AddRemoteRenderDev(const AudioDeviceDescriptor &devDesc);
    void AddRemoteCaptureDev(const AudioDeviceDescriptor &devDesc);

    list<DevicePrivacyInfo> privacyDeviceList;
    list<DevicePrivacyInfo> publicDeviceList;
    // 远程播放设备
    vector<unique_ptr<AudioDeviceDescriptor>> remoteRenderDevices_;
    // 远程录制设备
    vector<unique_ptr<AudioDeviceDescriptor>> remoteCaptureDevices_;
    // 隐私通话设备
    vector<unique_ptr<AudioDeviceDescriptor>> commRenderPrivacyDevices_;
    // 公共通话设备
    vector<unique_ptr<AudioDeviceDescriptor>> commRenderPublicDevices_;
    // 隐私通话录制设备
    vector<unique_ptr<AudioDeviceDescriptor>> commCapturePrivacyDevices_;
    // 公共通话录制设备
    vector<unique_ptr<AudioDeviceDescriptor>> commCapturePublicDevices_;
    // 私有媒体播放设备
    vector<unique_ptr<AudioDeviceDescriptor>> mediaRenderPrivacyDevices_;
    // 公共媒体播放设备
    vector<unique_ptr<AudioDeviceDescriptor>> mediaRenderPublicDevices_;
    // 私有媒体录制设备
    vector<unique_ptr<AudioDeviceDescriptor>> mediaCapturePrivacyDevices_;
    // 公共媒体录制设备
    vector<unique_ptr<AudioDeviceDescriptor>> mediaCapturePublicDevices_;
    // 私有录制设备
    vector<unique_ptr<AudioDeviceDescriptor>> capturePrivacyDevices_;
    // 公共录制设备
    vector<unique_ptr<AudioDeviceDescriptor>> capturePublicDevices_;
    unordered_map<AudioDevicePrivacyType, list<DevicePrivacyInfo>> devicePrivacyMaps_ = {};
};
} // namespace AudioStandard
} // namespace OHOS
#endif //ST_AUDIO_DEVICE_MANAGER_H