/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License") = 0;
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

#ifndef I_AUDIO_CAPTURER_SOURCE_H
#define I_AUDIO_CAPTURER_SOURCE_H

#include <cstdint>
#include "audio_info.h"
#include "audio_types.h"

namespace OHOS {
namespace AudioStandard {
typedef struct {
    const char *adapterName;
    uint32_t open_mic_speaker;
    AudioFormat format;
    uint32_t sampleFmt;
    uint32_t sampleRate;
    uint32_t channel;
    float volume;
    uint32_t bufferSize;
    bool isBigEndian;
    const char *filePath;
    const char *deviceNetworkId;
    int32_t device_type;
} IAudioSourceAttr;

class IAudioCapturerSource {
public:
    static IAudioCapturerSource *GetInstance(const char *devceClass, const char *deviceNetworkId);

    virtual int32_t Init(IAudioSourceAttr &atrr) = 0;
    virtual bool IsInited(void) = 0;
    virtual void DeInit(void) = 0;

    virtual int32_t Start(void) = 0;
    virtual int32_t Stop(void) = 0;
    virtual int32_t Flush(void) = 0;
    virtual int32_t Reset(void) = 0;
    virtual int32_t Pause(void) = 0;
    virtual int32_t Resume(void) = 0;
    virtual int32_t CaptureFrame(char *frame, uint64_t requestBytes, uint64_t &replyBytes) = 0;
    virtual int32_t SetVolume(float left, float right) = 0;
    virtual int32_t GetVolume(float &left, float &right) = 0;
    virtual int32_t SetMute(bool isMute) = 0;
    virtual int32_t GetMute(bool &isMute) = 0;
    virtual int32_t SetAudioScene(AudioScene audioScene, DeviceType activeDevice) = 0;
    virtual int32_t SetInputRoute(DeviceType deviceType) = 0;
    virtual uint64_t GetTransactionId() = 0;

    virtual ~IAudioCapturerSource() = default;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif  // AUDIO_CAPTURER_SOURCE_H
