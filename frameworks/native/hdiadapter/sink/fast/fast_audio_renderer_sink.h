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

#ifndef FAST_AUDIO_RENDERER_SINK_H
#define FAST_AUDIO_RENDERER_SINK_H

#include "audio_info.h"
#include "i_audio_renderer_sink.h"

namespace OHOS {
namespace AudioStandard {
class FastAudioRendererSink : public IAudioRendererSink {
public:
    static FastAudioRendererSink *GetInstance(void);

    int32_t Init(IAudioSinkAttr attr);
    bool IsInited(void);
    void DeInit(void);

    int32_t Start(void);
    int32_t Stop(void);
    int32_t Flush(void);
    int32_t Reset(void);
    int32_t Pause(void);
    int32_t Resume(void);

    int32_t RenderFrame(char &data, uint64_t len, uint64_t &writeLen);
    int32_t SetVolume(float left, float right);
    int32_t GetVolume(float &left, float &right);
    int32_t SetVoiceVolume(float volume);
    int32_t GetLatency(uint32_t *latency);
    int32_t GetTransactionId(uint64_t *transactionId);
    int32_t SetAudioScene(AudioScene audioScene, DeviceType activeDevice);
    int32_t SetOutputRoute(DeviceType deviceType);

    void SetAudioParameter(const AudioParamKey key, const std::string& condition, const std::string& value);
    std::string GetAudioParameter(const AudioParamKey key, const std::string& condition);
    void RegisterParameterCallback(IAudioSinkCallback* callback);

    void SetAudioMonoState(bool audioMono);
    void SetAudioBalanceValue(float audioBalance);

    uint32_t frameSizeInByte_ = 1;
    uint32_t eachReadFrameSize_ = 0;
    virtual int32_t GetMmapHandlePosition(uint64_t &frames, int64_t &timeSec, int64_t &timeNanoSec) = 0;

    FastAudioRendererSink() = default;
    ~FastAudioRendererSink() = default;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif // FAST_AUDIO_RENDERER_SINK_H
