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

#ifndef PA_ADAPTER_MANAGER_H
#define PA_ADAPTER_MANAGER_H

#include <map>
#include <mutex>
#include <pulse/pulseaudio.h>
#include <pulse/thread-mainloop.h>
#include "audio_timer.h"
#include "i_stream_manager.h"

namespace OHOS {
namespace AudioStandard {
class PaAdapterManager : public IStreamManager{
public:
    PaAdapterManager(ManagerType type);

    int32_t CreateRender(AudioStreamParams params, AudioStreamType audioType,
        std::shared_ptr<IRendererStream> &stream) override;
    int32_t ReleaseRender(uint32_t streamIndex_) override;
    int32_t CreateCapturer(AudioStreamParams params, AudioStreamType audioType,
        std::shared_ptr<ICapturerStream> &stream) override;
    int32_t ReleaseCapturer(uint32_t streamIndex_) override;
    int32_t GetInfo() override;

private:
    // audio channel index
    static const uint8_t CHANNEL1_IDX = 0;
    static const uint8_t CHANNEL2_IDX = 1;
    static const uint8_t CHANNEL3_IDX = 2;
    static const uint8_t CHANNEL4_IDX = 3;
    static const uint8_t CHANNEL5_IDX = 4;
    static const uint8_t CHANNEL6_IDX = 5;
    static const uint8_t CHANNEL7_IDX = 6;
    static const uint8_t CHANNEL8_IDX = 7;

    // Error code used
    static const int32_t PA_ADAPTER_SUCCESS = 0;
    static const int32_t PA_ADAPTER_ERR = -1;
    static const int32_t PA_ADAPTER_INVALID_PARAMS_ERR = -2;
    static const int32_t PA_ADAPTER_INIT_ERR = -3;
    static const int32_t PA_ADAPTER_CREATE_STREAM_ERR = -4;
    static const int32_t PA_ADAPTER_START_STREAM_ERR = -5;
    static const int32_t PA_ADAPTER_READ_STREAM_ERR = -6;
    static const int32_t PA_ADAPTER_WRITE_STREAM_ERR = -7;
    static const int32_t PA_ADAPTER_PA_ERR = -8;
    static const int32_t PA_ADAPTER_PERMISSION_ERR = -9;

    int32_t ResetPaContext();
    int32_t InitPaContext();
    int32_t HandleMainLoopStart();
    pa_stream *InitPaStream(AudioStreamParams params, AudioStreamType audioType);
    std::shared_ptr<IRendererStream> CreateRendererStream(AudioStreamParams params, pa_stream *paStream);
    std::shared_ptr<ICapturerStream> CreateCapturerStream(AudioStreamParams params, pa_stream *paStream);
    int32_t ConnectStreamToPA(pa_stream *paStream, pa_sample_spec sampleSpec);

    // Callbacks to be implemented
    static void PAStreamStateCb(pa_stream *stream, void *userdata);
    static void PAContextStateCb(pa_context *context, void *userdata);
    const std::string GetStreamName(AudioStreamType audioType);
    pa_sample_spec ConvertToPAAudioParams(AudioStreamParams audioParams);
    uint32_t ConvertChLayoutToPaChMap(const uint64_t &channelLayout, pa_channel_map &paMap);

    pa_threaded_mainloop *mainLoop_;
    pa_mainloop_api *api_;
    pa_context *context_;
    std::mutex listLock_;
    std::map<int32_t, std::shared_ptr<IRendererStream>> rendererStreamMap_;
    std::map<int32_t, std::shared_ptr<ICapturerStream>> capturerStreamMap_;
    bool isContextConnected_;
    bool isMainLoopStarted_;
    ManagerType managerType_ = PLAYBACK;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // PA_ADAPTER_MANAGER_H
