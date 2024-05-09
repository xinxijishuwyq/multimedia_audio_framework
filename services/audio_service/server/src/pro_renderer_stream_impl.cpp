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
#define LOG_TAG "ProRendererStream"

#include "pro_renderer_stream_impl.h"
#include "audio_errors.h"
#include "audio_log.h"
#include "audio_utils.h"
#include "audio_common_converter.h"

namespace OHOS {
namespace AudioStandard {
constexpr uint64_t AUDIO_NS_PER_S = 1000000000;
constexpr uint32_t DOUBLE_VALUE = 2;
constexpr float AUDIO_SAMPLE_32BIT_VALUE = 2147483647.f;
const std::string DUMP_DIRECT_STREAM_FILE = "dump_direct_audio_stream.pcm";

ProRendererStreamImpl::ProRendererStreamImpl(AudioProcessConfig processConfig, bool isDirect)
    : isDirect_(isDirect),
      isNeedResample_(false),
      isBlock_(false),
      privacyType_(0),
      renderRate_(0),
      streamIndex_(static_cast<uint32_t>(-1)),
      currentRate_(1),
      desSamplingRate_(0),
      desFormat_(AudioSampleFormat::SAMPLE_S32LE),
      byteSizePerFrame_(0),
      spanSizeInFrame_(0),
      totalBytesWritten_(0),
      minBufferSize_(0),
      abortFlag_(0),
      powerVolumeFactor_(1.f),
      status_(ProStreamStatus::RELEASED),
      resample_(nullptr),
      processConfig_(processConfig),
      dumpFile_(nullptr)
{
    AUDIO_DEBUG_LOG("ProRendererStreamImpl constructor");
}

ProRendererStreamImpl::~ProRendererStreamImpl()
{
    AUDIO_DEBUG_LOG("~ProRendererStreamImpl");
    status_ = ProStreamStatus::RELEASED;
    DumpFileUtil::CloseDumpFile(dumpFile_);
}

AudioSamplingRate ProRendererStreamImpl::GetDirectSampleRate(AudioSamplingRate sampleRate) const noexcept
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

int32_t ProRendererStreamImpl::InitParams()
{
    if (status_ != ProStreamStatus::RELEASED) {
        return ERR_ILLEGAL_STATE;
    }
    AudioStreamInfo streamInfo = processConfig_.streamInfo;
    AUDIO_INFO_LOG("sampleSpec: channels: %{public}u, formats: %{public}d, rate: %{public}d", streamInfo.channels,
                   streamInfo.format, streamInfo.samplingRate);
    currentRate_ = streamInfo.samplingRate;
    desSamplingRate_ = GetDirectSampleRate(streamInfo.samplingRate);
    desFormat_ = AudioSampleFormat::SAMPLE_S32LE;
    if (!isDirect_) {
        desFormat_ = AudioSampleFormat::SAMPLE_S16LE;
    }
    spanSizeInFrame_ = (streamInfo.samplingRate * 20) / 1000;
    byteSizePerFrame_ = GetSamplePerFrame(streamInfo.format) * streamInfo.channels;
    minBufferSize_ = spanSizeInFrame_ * byteSizePerFrame_;
    int32_t frameSize = spanSizeInFrame_ * streamInfo.channels;
    if (streamInfo.format != desFormat_) { // todo
        isNeedResample_ = true;
        resample_ = std::make_shared<AudioResample>(streamInfo.channels, streamInfo.samplingRate, desSamplingRate_, 2);
        resampleSrcBuffer.resize(frameSize, 0.f);
        resampleDesBuffer.resize((frameSize * desSamplingRate_) / streamInfo.samplingRate, 0.f);
        resample_->ProcessFloadResample(resampleSrcBuffer, resampleDesBuffer);
    }
    uint32_t bufferSize = (GetSamplePerFrame(desFormat_) * frameSize * desSamplingRate_) / streamInfo.samplingRate;
    sinkBuffer_.resize(4, std::vector<float>(bufferSize, 0));
    for (int32_t i = 0; i < 4; i++) {
        writeQueue_.emplace(i);
    }
    status_ = ProStreamStatus::INITIALIZED;
    return SUCCESS;
}

int32_t ProRendererStreamImpl::Start()
{
    isBlock_ = false;
    AUDIO_INFO_LOG("Enter ProRendererStreamImpl::Start");
    if (status_ != ProStreamStatus::INITIALIZED) {
        return ERR_ILLEGAL_STATE;
    }
    status_ = ProStreamStatus::RUNNING;
    std::shared_ptr<IStatusCallback> statusCallback = statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_STARTED);
    }
    DumpFileUtil::OpenDumpFile(DUMP_SERVER_PARA, DUMP_DIRECT_STREAM_FILE, dumpFile_);
    return SUCCESS;
}

int32_t ProRendererStreamImpl::Pause()
{
    AUDIO_INFO_LOG("Enter ProRendererStreamImpl::Pause");
    if (status_ == ProStreamStatus::RUNNING) {
        status_ = ProStreamStatus::PAUSED;
    }
    isBlock_ = true;
    std::shared_ptr<IStatusCallback> statusCallback = statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_PAUSED);
    }
    return SUCCESS;
}

int32_t ProRendererStreamImpl::Flush()
{
    AUDIO_INFO_LOG("Enter ProRendererStreamImpl::Flush");
    status_ = ProStreamStatus::FLUSHED;
    std::shared_ptr<IStatusCallback> statusCallback = statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_FLUSHED);
    }
    isBlock_ = true;
    return SUCCESS;
}

int32_t ProRendererStreamImpl::Drain()
{
    AUDIO_INFO_LOG("Enter ProRendererStreamImpl::Drain");
    std::shared_ptr<IStatusCallback> statusCallback = statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_DRAINED);
    }
    isBlock_ = true;
    status_ = ProStreamStatus::DRAIN;
    return SUCCESS;
}

int32_t ProRendererStreamImpl::Stop()
{
    AUDIO_INFO_LOG("Enter ProRendererStreamImpl::Stop");
    status_ = ProStreamStatus::STOPED;
    std::shared_ptr<IStatusCallback> statusCallback = statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_STOPPED);
    }
    isBlock_ = true;
    return SUCCESS;
}

int32_t ProRendererStreamImpl::Release()
{
    AUDIO_INFO_LOG("Enter ProRendererStreamImpl::Release");
    status_ = ProStreamStatus::RELEASED;
    std::shared_ptr<IStatusCallback> statusCallback = statusCallback_.lock();
    if (statusCallback != nullptr) {
        statusCallback->OnStatusUpdate(OPERATION_RELEASED);
    }
    isBlock_ = true;
    return SUCCESS;
}

int32_t ProRendererStreamImpl::GetStreamFramesWritten(uint64_t &framesWritten)
{
    CHECK_AND_RETURN_RET_LOG(byteSizePerFrame_ != 0, ERR_ILLEGAL_STATE, "Error frame size");
    framesWritten = totalBytesWritten_ / byteSizePerFrame_;
    return SUCCESS;
}

int32_t ProRendererStreamImpl::GetCurrentTimeStamp(uint64_t &timestamp)
{
    int64_t timeSec = 0;
    int64_t timeNsec = 0;
    uint64_t framePosition;
    int32_t ret = GetAudioTime(framePosition, timeSec, timeNsec);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "GetBufferSize error.");
    timestamp = timeSec * AUDIO_NS_PER_S + timeNsec;
    return SUCCESS;
}
int32_t ProRendererStreamImpl::GetCurrentPosition(uint64_t &framePosition, uint64_t &timestamp)
{
    int64_t timeSec = 0;
    int64_t timeNsec = 0;
    int32_t ret = GetAudioTime(framePosition, timeSec, timeNsec);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "GetBufferSize error.");
    timestamp = timeSec * AUDIO_NS_PER_S + timeNsec;
    return SUCCESS;
}

int32_t ProRendererStreamImpl::GetLatency(uint64_t &latency)
{
    uint64_t framePos;
    GetStreamFramesWritten(framePos);
    latency = ((framePos / byteSizePerFrame_) * AUDIO_NS_PER_S) / processConfig_.streamInfo.samplingRate;
    return SUCCESS;
}

int32_t ProRendererStreamImpl::SetRate(int32_t rate)
{
    uint32_t currentRate = processConfig_.streamInfo.samplingRate;
    switch (rate) {
    case RENDER_RATE_NORMAL:
        break;
    case RENDER_RATE_DOUBLE:
        currentRate *= DOUBLE_VALUE;
        break;
    case RENDER_RATE_HALF:
        currentRate /= DOUBLE_VALUE;
        break;
    default:
        return ERR_INVALID_PARAM;
    }
    renderRate_ = rate;
    return SUCCESS;
}

int32_t ProRendererStreamImpl::SetLowPowerVolume(float volume)
{
    powerVolumeFactor_ = volume; // todo power Volume Factor
    return SUCCESS;
}

int32_t ProRendererStreamImpl::GetLowPowerVolume(float &powerVolume)
{
    powerVolume = powerVolumeFactor_;
    return SUCCESS;
}

int32_t ProRendererStreamImpl::SetAudioEffectMode(int32_t effectMode)
{
    return SUCCESS;
}
int32_t ProRendererStreamImpl::GetAudioEffectMode(int32_t &effectMode)
{
    return SUCCESS;
}

int32_t ProRendererStreamImpl::SetPrivacyType(int32_t privacyType)
{
    privacyType_ = privacyType;
    return SUCCESS;
}

int32_t ProRendererStreamImpl::GetPrivacyType(int32_t &privacyType)
{
    privacyType = privacyType_;
    return SUCCESS;
}

void ProRendererStreamImpl::RegisterStatusCallback(const std::weak_ptr<IStatusCallback> &callback)
{
    AUDIO_DEBUG_LOG("RegisterStatusCallback in");
    statusCallback_ = callback;
}
void ProRendererStreamImpl::RegisterWriteCallback(const std::weak_ptr<IWriteCallback> &callback)
{
    AUDIO_DEBUG_LOG("RegisterWriteCallback in");
    writeCallback_ = callback;
}

BufferDesc ProRendererStreamImpl::DequeueBuffer(size_t length)
{
    BufferDesc bufferDesc = {nullptr, 0, 0};
    if (status_ != ProStreamStatus::RUNNING) {
        return bufferDesc;
    }
    bufferDesc.buffer = reinterpret_cast<uint8_t *>(sinkBuffer_[0].data());
    bufferDesc.bufLength = sinkBuffer_[0].size();
    return bufferDesc;
}

int32_t ProRendererStreamImpl::EnqueueBuffer(const BufferDesc &bufferDesc)
{
    std::lock_guard lock(enqueueMutex);
    int32_t writeIndex = PopWriteBufferIndex();
    if (writeIndex < 0) {
        return ERR_WRITE_BUFFER;
    }
    uint64_t duration;
    if (isNeedResample_) {
        ConvertSrcToFloat(bufferDesc.buffer, bufferDesc.bufLength);
        auto start = std::chrono::steady_clock::now();
        resample_->ProcessFloadResample(resampleSrcBuffer, resampleDesBuffer);
        auto end = std::chrono::steady_clock::now();
        ConvertFloatToDes(writeIndex);
        duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        AUDIO_INFO_LOG("resample cost:%{public}" PRIu64 "", duration);
    } else {
        memcpy_s(sinkBuffer_[writeIndex].data(), sinkBuffer_[writeIndex].size(), bufferDesc.buffer,
                 bufferDesc.bufLength);
    }
    readQueue_.emplace(writeIndex);
    // DumpFileUtil::WriteDumpFile(dumpFile_, sinkBuffer_.data(), sinkBuffer_.size());
    AUDIO_INFO_LOG("buffer length:%{public}" PRIu64 ",sink buffer length:%{public}" PRIu64 "", bufferDesc.bufLength,
                   sinkBuffer_.size());
    totalBytesWritten_ += bufferDesc.bufLength;
    return SUCCESS;
}

int32_t ProRendererStreamImpl::GetMinimumBufferSize(size_t &minBufferSize) const
{
    minBufferSize = minBufferSize_;
    return SUCCESS;
}

void ProRendererStreamImpl::GetByteSizePerFrame(size_t &byteSizePerFrame) const
{
    byteSizePerFrame = byteSizePerFrame_;
}

void ProRendererStreamImpl::GetSpanSizePerFrame(size_t &spanSizeInFrame) const
{
    spanSizeInFrame = spanSizeInFrame_;
}

void ProRendererStreamImpl::SetStreamIndex(uint32_t index)
{
    AUDIO_INFO_LOG("Using index/sessionId %{public}d", index);
    streamIndex_ = index;
}
uint32_t ProRendererStreamImpl::GetStreamIndex()
{
    return streamIndex_;
}

void ProRendererStreamImpl::AbortCallback(int32_t abortTimes)
{
    abortFlag_ += abortTimes;
}

// offload
int32_t ProRendererStreamImpl::SetOffloadMode(int32_t state, bool isAppBack)
{
    return SUCCESS;
}
int32_t ProRendererStreamImpl::UnsetOffloadMode()
{
    return SUCCESS;
}
int32_t ProRendererStreamImpl::GetOffloadApproximatelyCacheTime(uint64_t &timestamp, uint64_t &paWriteIndex,
                                                                uint64_t &cacheTimeDsp, uint64_t &cacheTimePa)
{
    return SUCCESS;
}
int32_t ProRendererStreamImpl::OffloadSetVolume(float volume)
{
    return SUCCESS;
}

size_t ProRendererStreamImpl::GetWritableSize()
{
    return writeQueue_.size() * sinkBuffer_[0].size();
}
// offload end
int32_t ProRendererStreamImpl::UpdateSpatializationState(bool spatializationEnabled, bool headTrackingEnabled)
{
    return SUCCESS;
}

AudioProcessConfig ProRendererStreamImpl::GetAudioProcessConfig() const noexcept
{
    return processConfig_;
}

bool ProRendererStreamImpl::GetAudioTime(uint64_t &framePos, int64_t &sec, int64_t &nanoSec)
{
    GetStreamFramesWritten(framePos);
    int64_t time = handleTimeModel_.GetTimeOfPos(framePos);
    int64_t deltaTime = 20000000; // note: 20ms
    time += deltaTime;
    sec = time / AUDIO_NS_PER_S;
    nanoSec = time % AUDIO_NS_PER_S;
    return true;
}

int32_t ProRendererStreamImpl::Peek(std::vector<char> *audioBuffer)
{
    int32_t result = SUCCESS;
    if (isBlock_) {
        return ERR_WRITE_BUFFER;
    }
    if (!readQueue_.empty()) {
        PopSinkBuffer(audioBuffer);
        return result;
    }
    std::shared_ptr<IWriteCallback> writeCallback = writeCallback_.lock();
    if (writeCallback != nullptr) {
        result = writeCallback->OnWriteData(sinkBuffer_[0].size());
        if (result != SUCCESS) {
            AUDIO_ERR_LOG("Write callback failed,result:%{public}d", result);
            return result;
        }
        PopSinkBuffer(audioBuffer);
    } else {
        AUDIO_ERR_LOG("Write callback is nullptr");
        result = ERR_WRITE_BUFFER;
    }
    return result;
}

int32_t ProRendererStreamImpl::PopWriteBufferIndex()
{
    int32_t writeIndex = -1;
    if (!writeQueue_.empty()) {
        writeIndex = writeQueue_.front();
        writeQueue_.pop_front();
    }
    return writeIndex;
}

void ProRendererStreamImpl::PopSinkBuffer(std::vector<char> *audioBuffer)
{
    if (!readQueue_.empty()) {
        int32_t readIndex = readQueue_.front();
        readQueue_.pop_front();
        *audioBuffer = sinkBuffer_[readIndex];
        writeQueue_.push_back(readIndex);
    } else {
        audioBuffer = nullptr;
    }
}

uint32_t ProRendererStreamImpl::GetSamplePerFrame(AudioSampleFormat format) const noexcept
{
    uint32_t audioPerSampleLength = 2; // 2 byte
    switch (format) {
    case AudioSampleFormat::SAMPLE_U8:
        audioPerSampleLength = 1;
        break;
    case AudioSampleFormat::SAMPLE_S16LE:
        audioPerSampleLength = 2; // 2 byte
        break;
    case AudioSampleFormat::SAMPLE_S24LE:
        audioPerSampleLength = 3; // 3 byte
        break;
    case AudioSampleFormat::SAMPLE_S32LE:
    case AudioSampleFormat::SAMPLE_F32LE:
        audioPerSampleLength = 4; // 4 byte
        break;
    default:
        break;
    }
    return audioPerSampleLength;
}

void ProRendererStreamImpl::ConvertSrcToFloat(uint8_t *buffer, size_t bufLength)
{
    auto streamInfo = processConfig_.streamInfo;
    uint32_t samplePerFrame = GetSamplePerFrame(streamInfo.format);
    if (streamInfo.format == AudioSampleFormat::SAMPLE_F32LE) {
        memcpy_s(resampleSrcBuffer.data(), resampleSrcBuffer.size() * samplePerFrame, buffer, bufLength);
        return;
    }
    AUDIO_INFO_LOG("ConvertSrcToFloat resample buffer,samplePerFrame:%{public}d,size:%{public}" PRIu64 "",
                   samplePerFrame, resampleSrcBuffer.size());
    uint32_t convertValue = samplePerFrame * 8 - 1;
    for (uint32_t i = 0; i < resampleSrcBuffer.size(); i++) {
        int32_t sampleValue = 0;
        if (samplePerFrame == 3) {
            sampleValue = (buffer[i * samplePerFrame + 2] & 0xff) << 24 |
                          (buffer[i * samplePerFrame + 1] & 0xff) << 16 | (buffer[i * samplePerFrame] & 0xff) << 8;
            resampleSrcBuffer[i] = sampleValue * (1.0f / AUDIO_SAMPLE_32BIT_VALUE);
            continue;
        }
        for (uint32_t j = 0; j < samplePerFrame; j++) {
            sampleValue |= (buffer[i * samplePerFrame + j] & 0xff) << (j * 8);
        }
        resampleSrcBuffer[i] = sampleValue * (1.0f / (1U << convertValue));
    }
}

void ProRendererStreamImpl::ConvertFloatToDes(int32_t writeIndex)
{
    uint32_t samplePerFrame = GetSamplePerFrame(desFormat_);
    if (desFormat_ == AudioSampleFormat::SAMPLE_F32LE) {
        memcpy_s(sinkBuffer_[writeIndex].data(), sinkBuffer_[writeIndex].size(), resampleSrcBuffer.data(),
                 resampleSrcBuffer.size() * samplePerFrame);
        return;
    }
    uint32_t convertValue = samplePerFrame * 8 - 1;
    for (uint32_t i = 0; i < resampleDesBuffer.size(); i++) {
        int32_t sampleValue = static_cast<int32_t>(resampleDesBuffer[i] * std::pow(2, convertValue));
        for (uint32_t j = 0; j < samplePerFrame; j++) {
            uint8_t tempValue = (sampleValue >> (8 * j)) & 0xff;
            sinkBuffer_[writeIndex][samplePerFrame * i + j] = tempValue;
        }
    }
}

float ProRendererStreamImpl::GetStreamVolume()
{
    float volume = powerVolumeFactor_;
    Volume vol = {true, 1.0f, 0};
    AudioStreamType streamType = processConfig_.streamType;
    AudioVolumeType volumeType = PolicyHandler::GetInstance().GetVolumeTypeFromStreamType(streamType);
    DeviceInfo deviceInfo;
    bool ret = PolicyHandler::GetInstance().GetProcessDeviceInfo(processConfig_, deviceInfo);
    if (!ret) {
        AUDIO_ERR_LOG("GetProcessDeviceInfo failed.");
        return ERR_DEVICE_INIT;
    }
    if (deviceInfo_.networkId == LOCAL_NETWORK_ID &&
        PolicyHandler::GetInstance().GetSharedVolume(volumeType, processConfig_.deviceType, vol)) {
        volume = vol.isMute ? 0 : static_cast<int32_t>(volume * vol.volumeFloat);
    }
    return volume;
}

void AudioProcessInClientInner::CopyWithVolume(uint8_t *buffer, float volume) const
{
    if (volume >= 1.f) {
        return;
    }
    if (streamInfo.format == AudioSampleFormat::SAMPLE_F32LE) {
        float *tempBuffer = reinterpret_cast<float *>(buffer);
        for (uint32_t i = 0; i < resampleSrcBuffer.size(); i++) {
            resampleSrcBuffer[i] = volume * tempBuffer[i];
        }
        return;
    }
}

int32_t PaRendererStreamImpl::TriggerStartIfNecessary(bool isBlock)
{
    isBlock_ = isBlock;
    return SUCCESS;
}
} // namespace AudioStandard
} // namespace OHOS
