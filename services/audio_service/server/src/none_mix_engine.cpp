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

#include "audio_common_converter.h"
#include "audio_errors.h"
#include "audio_log.h"
#include "audio_utils.h"
#include "none_mix_engine.h"

namespace OHOS {
namespace AudioStandard {
constexpr int32_t DELTA_TIME = 4000000; // 4ms
constexpr int32_t PERIOD_NS = 20000000; // 20ms
constexpr int32_t FADING_MS = 20; // 20ms
constexpr int32_t MAX_ERROR_COUNT = 5;
constexpr int16_t STEREO_CHANNEL_COUNT = 2;
constexpr int16_t HDI_STEREO_CHANNEL_LAYOUT = 3;
constexpr int16_t HDI_MONO_CHANNEL_LAYOUT = 4;
const std::string THREAD_NAME = "noneMixThread";
const std::string VOIP_SINK_NAME = "voip";
const std::string DIRECT_SINK_NAME = "direct";
const char *SINK_ADAPTER_NAME = "primary";

NoneMixEngine::NoneMixEngine(DeviceInfo type, bool isVoip)
    : isVoip_(isVoip), isStart_(false), isPause_(false), device_(type), failedCount_(0), writeCount_(0),
      fwkSyncTime_(0), stream_(nullptr), startFadein_(false), startFadeout_(false), uChannel_(0),
      uFormat_(sizeof(int32_t)), uSampleRate_(0)
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
    startFadein_ = false;
    startFadeout_ = false;
}

int32_t NoneMixEngine::Start()
{
    AUDIO_INFO_LOG("Enter in");
    int32_t ret = SUCCESS;
    fwkSyncTime_ = static_cast<uint64_t>(ClockTime::GetCurNano());
    writeCount_ = 0;
    failedCount_ = 0;
    if (!playbackThread_) {
        playbackThread_ = std::make_unique<AudioThreadTask>(THREAD_NAME);
        playbackThread_->RegisterJob([this] { this->MixStreams(); });
    }
    if (!isStart_) {
        startFadeout_ = false;
        startFadein_ = true;
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
        startFadein_ = false;
        startFadeout_ = true;
        // wait until fadeout complete
        std::unique_lock fadingLock(fadingMutex_);
        cvFading_.wait_for(
            fadingLock, std::chrono::milliseconds(FADING_MS), [this] { return (!(startFadein_ || startFadeout_)); });
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
        // wait until fadeout complete
        startFadein_ = false;
        startFadeout_ = true;
        std::unique_lock fadingLock(fadingMutex_);
        cvFading_.wait_for(
            fadingLock, std::chrono::milliseconds(FADING_MS), [this] { return (!(startFadein_ || startFadeout_)); });
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
        startFadein_ = false;
        startFadeout_ = true;
        // wait until fadeout complete
        std::unique_lock fadingLock(fadingMutex_);
        cvFading_.wait_for(
            fadingLock, std::chrono::milliseconds(FADING_MS), [this] { return (!(startFadein_ || startFadeout_)); });
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

void NoneMixEngine::DoFadeinOut(bool isFadeOut, char *pBuffer, size_t bufferSize)
{
    CHECK_AND_RETURN_LOG(pBuffer != nullptr && bufferSize > 0 && uChannel_ > 0, "buffer is null.");
    int32_t *dstPtr = reinterpret_cast<int32_t *>(pBuffer);
    size_t dataLength = bufferSize / (static_cast<uint32_t>(uFormat_) * uChannel_);
    float fadeStep = 1.0f / dataLength;
    for (size_t i = 0; i < dataLength; i++) {
        float fadeFactor;
        if (isFadeOut) {
            fadeFactor = 1.0f - ((i + 1) * fadeStep);
        } else {
            fadeFactor = (i + 1) * fadeStep;
        }
        for (uint32_t j = 0; j < uChannel_; j++) {
            dstPtr[i * uChannel_ + j] *= fadeFactor;
        }
    }
    if (isFadeOut) {
        startFadeout_.store(false);
    } else {
        startFadein_.store(false);
    }
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
    int32_t appUid = stream_->GetAudioProcessConfig().appInfo.appUid;
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
    // fade in or fade out
    if (startFadeout_ || startFadein_) {
        DoFadeinOut(startFadeout_, audioBuffer.data(), audioBuffer.size());
        cvFading_.notify_all();
    }

    renderSink_->RenderFrame(*audioBuffer.data(), audioBuffer.size(), written);
    stream_->ReturnIndex(index);
    renderSink_->UpdateAppsUid({appUid});
    StandbySleep();
}

int32_t NoneMixEngine::AddRenderer(const std::shared_ptr<IRendererStream> &stream)
{
    AUDIO_INFO_LOG("Enter add");
    if (!stream_) {
        stream_ = stream;
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
    int64_t writeTime = static_cast<int64_t>(fwkSyncTime_) + static_cast<int64_t>(writeCount_) * PERIOD_NS + DELTA_TIME;
    ClockTime::AbsoluteSleep(writeTime);
}

AudioSamplingRate NoneMixEngine::GetDirectSampleRate(AudioSamplingRate sampleRate)
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
    AUDIO_INFO_LOG("GetDirectSampleRate: sampleRate: %{public}d, result: %{public}d", sampleRate, result);
    return result;
}

AudioSamplingRate NoneMixEngine::GetDirectVoipSampleRate(AudioSamplingRate sampleRate)
{
    AudioSamplingRate result = sampleRate;
    if (sampleRate <= AudioSamplingRate::SAMPLE_RATE_16000) {
        result = AudioSamplingRate::SAMPLE_RATE_16000;
    } else {
        result = AudioSamplingRate::SAMPLE_RATE_48000;
    }
    AUDIO_INFO_LOG("GetDirectVoipSampleRate: sampleRate: %{public}d, result: %{public}d", sampleRate, result);
    return result;
}

HdiAdapterFormat NoneMixEngine::GetDirectDeviceFormate(AudioSampleFormat format)
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

int32_t NoneMixEngine::GetDirectFormatByteSize(HdiAdapterFormat format)
{
    switch (format) {
        case HdiAdapterFormat::SAMPLE_S16:
            return sizeof(int16_t);
        case HdiAdapterFormat::SAMPLE_S32:
        case HdiAdapterFormat::SAMPLE_F32:
            return sizeof(int32_t);
        default:
            return sizeof(int32_t);
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
    attr.sampleRate = isVoip_ ?
        GetDirectVoipSampleRate(streamInfo.samplingRate) : GetDirectSampleRate(streamInfo.samplingRate);
    attr.channel = streamInfo.channels >= STEREO_CHANNEL_COUNT ? STEREO_CHANNEL_COUNT : 1;
    attr.format = GetDirectDeviceFormate(streamInfo.format);
    attr.channelLayout = streamInfo.channels >= STEREO_CHANNEL_COUNT ?
        HDI_STEREO_CHANNEL_LAYOUT : HDI_MONO_CHANNEL_LAYOUT;
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
    uChannel_ = attr.channel;
    uSampleRate_ = attr.sampleRate;
    uFormat_ = GetDirectFormatByteSize(attr.format);

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
