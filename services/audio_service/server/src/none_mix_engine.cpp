/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#undef LOG_TAG
#define LOG_TAG "NoneMixEngine"

#include "none_mix_engine.h"
#include "audio_common_converter.h"
#include "audio_utils.h"
#include "audio_errors.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
constexpr int32_t DELTA_TIME = 4000000; // 4ms
constexpr int32_t PERIOD_NS = 20000000; // 20ms
constexpr int32_t MAX_ERROR_COUNT = 5;
constexpr int16_t STEREO_CHANNEL_COUNT = 2;
constexpr int16_t HDI_STEREO_CHANNEL_LAYOUT = 3;
constexpr int16_t HDI_MONO_CHANNEL_LAYOUT = 4;
const std::string THREAD_NAME = "noneMixThread";
const std::string VOIP_SINK_NAME = "voip";
const std::string DIRECT_SINK_NAME = "direct";
const char *SINK_ADAPTER_NAME = "primary";

NoneMixEngine::NoneMixEngine(DeviceInfo type, bool isVoip)
    : isVoip_(isVoip),
      isStart_(false),
      isPause_(false),
      device_(type),
      failedCount_(0),
      writeCount_(0),
      fwkSyncTime_(0),
      stream_(nullptr)
{
    AUDIO_INFO_LOG("Constructor");
}

NoneMixEngine::~NoneMixEngine()
{
    writeCount_ = 0;
    failedCount_ = 0;
    fwkSyncTime_ = 0;
    if (playbackThread_) {
        playbackThread_->Stop();
        playbackThread_ = nullptr;
    }
    if (renderSink_ && renderSink_->IsInited()) {
        renderSink_->Stop();
        renderSink_->DeInit();
    }
    isStart_ = false;
}

int32_t NoneMixEngine::Start()
{
    AUDIO_INFO_LOG("Enter in");
    int32_t ret = SUCCESS;
    fwkSyncTime_ = ClockTime::GetCurNano();
    writeCount_ = 0;
    failedCount_ = 0;
    if (!playbackThread_) {
        playbackThread_ = std::make_unique<AudioThreadTask>(THREAD_NAME);
        playbackThread_->RegisterJob([this] { this->MixStreams(); });
    }
    if (!isStart_) {
        ret = renderSink_->Start();
        isStart_ = true;
    }
    isPause_ = false;
    if (!playbackThread_->CheckThreadIsRunning()) {
        playbackThread_->Start();
    }
    return ret;
}

int32_t NoneMixEngine::Stop()
{
    AUDIO_INFO_LOG("Enter");
    int32_t ret = SUCCESS;
    writeCount_ = 0;
    failedCount_ = 0;
    if (playbackThread_) {
        playbackThread_->Stop();
        playbackThread_ = nullptr;
    }
    if (renderSink_ && renderSink_->IsInited()) {
        ret = renderSink_->Stop();
    }
    isStart_ = false;
    return ret;
}

void NoneMixEngine::PauseAsync()
{
    if (playbackThread_) {
        playbackThread_->PauseAsync();
    }
    isPause_ = true;
}

int32_t NoneMixEngine::Pause()
{
    AUDIO_INFO_LOG("Enter");
    int32_t ret = SUCCESS;
    writeCount_ = 0;
    failedCount_ = 0;
    if (playbackThread_) {
        playbackThread_->Pause();
    }
    isPause_ = true;
    return ret;
}

int32_t NoneMixEngine::Flush()
{
    AUDIO_INFO_LOG("Enter");
    return SUCCESS;
}

void NoneMixEngine::MixStreams()
{
    if (stream_ == nullptr) {
        StandbySleep();
        return;
    }

    if (failedCount_ >= MAX_ERROR_COUNT) {
        AUDIO_WARNING_LOG("failed count is overflow.");
        PauseAsync();
        return;
    }
    std::vector<char> audioBuffer;
    int32_t index = -1;
    int32_t result = stream_->Peek(&audioBuffer, index);
    writeCount_++;
    if (index < 0) {
        AUDIO_WARNING_LOG("peek buffer failed.result:%{public}d,buffer size:%{public}d", result, index);
        stream_->ReturnIndex(index);
        failedCount_++;
        StandbySleep();
        return;
    }
    failedCount_ = 0;
    uint64_t written = 0;
    renderSink_->RenderFrame(*audioBuffer.data(), audioBuffer.size(), written);
    stream_->ReturnIndex(index);
    StandbySleep();
}

int32_t NoneMixEngine::AddRenderer(const std::shared_ptr<IRendererStream> &stream)
{
    AUDIO_INFO_LOG("Enter add");
    bool isNeedInit = false;
    if (!stream_) {
        stream_ = stream;
        isNeedInit = true;
        AudioProcessConfig config = stream->GetAudioProcessConfig();
        return InitSink(config.streamInfo);
    } else if (stream->GetStreamIndex() != stream_->GetStreamIndex()) {
        return ERROR_UNSUPPORTED;
    }
    return SUCCESS;
}

void NoneMixEngine::RemoveRenderer(const std::shared_ptr<IRendererStream> &stream)
{
    AUDIO_INFO_LOG("step in remove");
    if (stream->GetStreamIndex() == stream_->GetStreamIndex()) {
        Stop();
        renderSink_->DeInit();
        stream_ = nullptr;
    }
}

bool NoneMixEngine::IsPlaybackEngineRunning() const noexcept
{
    return isStart_ && !isPause_;
}

void NoneMixEngine::StandbySleep()
{
    int64_t writeTime = fwkSyncTime_ + writeCount_ * PERIOD_NS + DELTA_TIME;
    ClockTime::AbsoluteSleep(writeTime);
}

static AudioSamplingRate GetDirectSampleRate(AudioSamplingRate sampleRate)
{
    AudioSamplingRate result = sampleRate;
    switch (sampleRate) {
        case AudioSamplingRate::SAMPLE_RATE_44100:
            result = AudioSamplingRate::SAMPLE_RATE_48000;
            break;
        case AudioSamplingRate::SAMPLE_RATE_88200:
            result = AudioSamplingRate::SAMPLE_RATE_96000;
            break;
        case AudioSamplingRate::SAMPLE_RATE_176400:
            result = AudioSamplingRate::SAMPLE_RATE_192000;
            break;
        default:
            break;
    }
    return result;
}

HdiAdapterFormat GetDirectDeviceFormate(AudioSampleFormat format)
{
    switch (format) {
        case AudioSampleFormat::SAMPLE_U8:
        case AudioSampleFormat::SAMPLE_S16LE:
            return HdiAdapterFormat::SAMPLE_S16;
        case AudioSampleFormat::SAMPLE_S24LE:
        case AudioSampleFormat::SAMPLE_S32LE:
            return HdiAdapterFormat::SAMPLE_S32;
        case AudioSampleFormat::SAMPLE_F32LE:
            return HdiAdapterFormat::SAMPLE_F32;
        default:
            return HdiAdapterFormat::SAMPLE_S16;
    }
}

int32_t NoneMixEngine::InitSink(const AudioStreamInfo &streamInfo)
{
    std::string sinkName = DIRECT_SINK_NAME;
    if (isVoip_) {
        sinkName = VOIP_SINK_NAME;
    }
    renderSink_ = AudioRendererSink::GetInstance(sinkName);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = GetDirectSampleRate(streamInfo.samplingRate);
    attr.channel = streamInfo.channels >= STEREO_CHANNEL_COUNT ? STEREO_CHANNEL_COUNT : 1;
    attr.format = GetDirectDeviceFormate(streamInfo.format);
    attr.channelLayout =
        streamInfo.channels >= STEREO_CHANNEL_COUNT ? HDI_STEREO_CHANNEL_LAYOUT : HDI_MONO_CHANNEL_LAYOUT;
    attr.deviceType = device_.deviceType;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    AUDIO_INFO_LOG("sink name:%{public}s,device:%{public}d,sample rate:%{public}d,format:%{public}d", sinkName.c_str(),
        attr.deviceType, attr.sampleRate, attr.format);
    int32_t ret = renderSink_->Init(attr);
    if (ret != SUCCESS) {
        return ret;
    }
    float volume = 1.0f;
    ret = renderSink_->SetVolume(volume, volume);
    return ret;
}

int32_t NoneMixEngine::SwitchSink(const AudioStreamInfo &streamInfo, bool isVoip)
{
    Stop();
    renderSink_->DeInit();
    isVoip_ = isVoip;
    return InitSink(streamInfo);
}
} // namespace AudioStandard
} // namespace OHOS
