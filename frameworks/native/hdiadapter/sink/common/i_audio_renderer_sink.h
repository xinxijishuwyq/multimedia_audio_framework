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

#ifndef I_AUDIO_RENDERER_SINK_H
#define I_AUDIO_RENDERER_SINK_H

#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {
typedef struct {
    const char *adapterName;
    uint32_t openMicSpeaker;
    AudioSampleFormat format;
    uint32_t sampleFmt;
    uint32_t sampleRate;
    uint32_t channel;
    float volume;
    const char *filePath;
    const char *deviceNetworkId;
    int32_t deviceType;
} IAudioSinkAttr;

class IAudioSinkCallback {
public:
    virtual void OnAudioParameterChange(std::string netWorkId, const AudioParamKey key, const std::string& condition,
        const std::string& value) = 0;
};

class IAudioRendererSink {
public:
    static IAudioRendererSink *GetInstance(const char *devceClass, const char *deviceNetworkId);

    virtual ~IAudioRendererSink() = default;

    virtual int32_t Init(IAudioSinkAttr attr) = 0;
    virtual bool IsInited(void) = 0;
    virtual void DeInit(void) = 0;

    virtual int32_t Flush(void) = 0;
    virtual int32_t Pause(void) = 0;
    virtual int32_t Reset(void) = 0;
    virtual int32_t Resume(void) = 0;
    virtual int32_t Start(void) = 0;
    virtual int32_t Stop(void) = 0;

    virtual int32_t RenderFrame(char &data, uint64_t len, uint64_t &writeLen) = 0;
    virtual int32_t GetLatency(uint32_t *latency) = 0;

    virtual int32_t SetVolume(float left, float right) = 0;
    virtual int32_t GetVolume(float &left, float &right) = 0;
    virtual int32_t SetVoiceVolume(float volume) = 0;

    virtual int32_t GetTransactionId(uint64_t *transactionId) = 0;

    virtual int32_t SetAudioScene(AudioScene audioScene, DeviceType activeDevice) = 0;
    virtual int32_t SetOutputRoute(DeviceType deviceType) = 0;

    virtual void SetAudioParameter(const AudioParamKey key, const std::string& condition, const std::string& value) = 0;
    virtual std::string GetAudioParameter(const AudioParamKey key, const std::string& condition) = 0;
    virtual void RegisterParameterCallback(IAudioSinkCallback* callback) = 0;

    virtual void SetAudioMonoState(bool audioMono) = 0;
    virtual void SetAudioBalanceValue(float audioBalance) = 0;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif // I_AUDIO_RENDERER_SINK_H
