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

#ifndef AUDIO_PROCESS_IN_SERVER_H
#define AUDIO_PROCESS_IN_SERVER_H

#include "audio_process_stub.h"
#include "i_audio_process_stream.h"
#include "i_process_status_listener.h"

namespace OHOS {
namespace AudioStandard {
class AudioProcessInServer : public AudioProcessStub, public IAudioProcessStream {
public:
    static sptr<AudioProcessInServer> Create(const AudioProcessConfig &processConfig);
    virtual ~AudioProcessInServer();

    // override for AudioProcess
    int32_t ResolveBuffer(std::shared_ptr<OHAudioBuffer> &buffer) override;

    int32_t Start() override;

    int32_t Pause(bool isFlush) override;

    int32_t Resume() override;

    int32_t Stop() override;

    int32_t RequestHandleInfo() override;

    int32_t Release() override;

    // override for IAudioProcessStream, used in endpoint
    std::shared_ptr<OHAudioBuffer> GetStreamBuffer() override;
    AudioStreamInfo GetStreamInfo() override;

    int32_t ConfigProcessBuffer(uint32_t &totalSizeInframe, uint32_t &spanSizeInframe);

    int32_t AddProcessStatusListener(std::shared_ptr<IProcessStatusListener> listener);
    int32_t RemoveProcessStatusListener(std::shared_ptr<IProcessStatusListener> listener);
private:
    explicit AudioProcessInServer(const AudioProcessConfig &processConfig);
    int32_t InitBufferStatus();
    AudioProcessConfig processConfig_;

    bool isInited_ = false;
    std::atomic<StreamStatus> *streamStatus_ = nullptr;

    uint32_t totalSizeInframe_ = 0;
    uint32_t spanSizeInframe_ = 0;
    uint32_t byteSizePerFrame_ = 0;
    bool isBufferConfiged_ = false;
    std::shared_ptr<OHAudioBuffer> processBuffer_ = nullptr;
    std::vector<std::shared_ptr<IProcessStatusListener>> listenerList_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_PROCESS_IN_SERVER_H
