/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License") override;
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

#ifndef REMOTE_FAST_AUDIO_CAPTURER_SOURCE
#define REMOTE_FAST_AUDIO_CAPTURER_SOURCE

#include <map>

#include "ashmem.h"

#include "audio_info.h"
#include "audio_manager.h"
#include "i_audio_capturer_source.h"

namespace OHOS {
namespace AudioStandard {
class RemoteFastAudioCapturerSource : public IMmapAudioCapturerSource {
public:
    static IMmapAudioCapturerSource *GetInstance(const std::string& deviceNetworkId);

    int32_t Init(IAudioSourceAttr &attr) override;
    bool IsInited(void) override;
    void DeInit(void) override;

    int32_t Start(void) override;
    int32_t Stop(void) override;
    int32_t Flush(void) override;
    int32_t Reset(void) override;
    int32_t Pause(void) override;
    int32_t Resume(void) override;
    int32_t CaptureFrame(char *frame, uint64_t requestBytes, uint64_t &replyBytes) override;
    int32_t SetVolume(float left, float right) override;
    int32_t GetVolume(float &left, float &right) override;
    int32_t SetMute(bool isMute) override;
    int32_t GetMute(bool &isMute) override;
    int32_t SetAudioScene(AudioScene audioScene, DeviceType activeDevice) override;
    int32_t SetInputRoute(DeviceType inputDevice) override;
    uint64_t GetTransactionId() override;
    void RegisterWakeupCloseCallback(IAudioSourceCallback* callback) override;
    void RegisterAudioCapturerSourceCallback(IAudioSourceCallback* callback) override;
    int32_t GetMmapBufferInfo(int &fd, uint32_t &totalSizeInframe, uint32_t &spanSizeInframe,
        uint32_t &byteSizePerFrame) override;
    int32_t GetMmapHandlePosition(uint64_t &frames, int64_t &timeSec, int64_t &timeNanoSec) override;

private:
    explicit RemoteFastAudioCapturerSource(const std::string& deviceNetworkId);
    ~RemoteFastAudioCapturerSource();

    int32_t InitAudioManager();
    int32_t GetTargetAdapterPort(struct AudioAdapterDescriptor *descs, int32_t size, const char *networkId);
    int32_t CreateCapture(const struct AudioPort &capturePort);
    void InitAttrs(struct AudioSampleAttributes &attrs);
    AudioFormat ConverToHdiFormat(AudioSampleFormat format);
    int32_t InitAshmem(const struct AudioSampleAttributes &attrs);
    AudioCategory GetAudioCategory(AudioScene audioScene);
    int32_t SetInputPortPin(DeviceType inputDevice, AudioRouteNode &source);
    uint32_t PcmFormatToBits(AudioSampleFormat format);

private:
    static constexpr int32_t INVALID_FD = -1;
    static constexpr int32_t INVALID_INDEX = -1;
    static constexpr int32_t HALF_FACTOR = 2;
    static constexpr uint32_t CAPTURE_INTERLEAVED = 1;
    static constexpr uint32_t AUDIO_SAMPLE_RATE_48K = 48000;
    static constexpr uint32_t DEEP_BUFFER_CAPTURER_PERIOD_SIZE = 3840;
    static constexpr uint32_t INT_32_MAX = 0x7fffffff;
    static constexpr uint32_t REMOTE_FAST_INPUT_STREAM_ID = 38; // 14 + 3 * 8
    static constexpr int32_t EVENT_DES_SIZE = 60;
    static constexpr int64_t SECOND_TO_NANOSECOND = 1000000000;
    static constexpr int64_t CAPTURE_FIRST_FRIME_WAIT_NANO = 20000000; // 20ms
    static constexpr int64_t CAPTURE_RESYNC_SLEEP_NANO = 2000000; // 2ms
    static constexpr  uint32_t PCM_8_BIT = 8;
    static constexpr  uint32_t PCM_16_BIT = 16;
    static constexpr  uint32_t PCM_24_BIT = 24;
    static constexpr  uint32_t PCM_32_BIT = 32;
#ifdef FEATURE_DISTRIBUTE_AUDIO
    static constexpr uint32_t PARAM_VALUE_LENTH = 20;
#endif
    static std::map<std::string, RemoteFastAudioCapturerSource *> allRFSources_;

    std::atomic<bool> micMuteState_ = false;
    std::atomic<bool> capturerInited_ = false;
    std::atomic<bool> isCapturerCreated_ = false;
    std::atomic<bool> started_ = false;
    std::atomic<bool> paused_ = false;
    float leftVolume_ = 0;
    float rightVolume_ = 0;
    int32_t routeHandle_ = -1;
    int32_t bufferFd_ = -1;
    uint32_t bufferTotalFrameSize_ = 0;
    uint32_t eachReadFrameSize_ = 0;
    struct AudioManager *audioManager_ = nullptr;
    struct AudioAdapter *audioAdapter_ = nullptr;
    struct AudioCapture *audioCapture_ = nullptr;
    struct AudioPort audioPort_;
    IAudioSourceAttr attr_ = {};
    std::string deviceNetworkId_;

#ifdef DEBUG_DIRECT_USE_HDI
    sptr<Ashmem> ashmemSource_ = nullptr;
    int32_t ashmemLen_ = 0;
    int32_t lenPerRead_ = 0;
    const char *audioFilePath = "/data/local/tmp/remote_fast_audio_capture.pcm";
    FILE *pfd_ = nullptr;
#endif // DEBUG_DIRECT_USE_HDI
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif  // REMOTE_FAST_AUDIO_CAPTURER_SOURCE
