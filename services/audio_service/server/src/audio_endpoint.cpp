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

#include "audio_endpoint.h"

#include <atomic>
#include <cinttypes>
#include <condition_variable>
#include <thread>
#include <vector>
#include <mutex>

#include "securec.h"

#include "audio_errors.h"
#include "audio_log.h"
#include "audio_utils.h"
#include "fast_audio_renderer_sink.h"
#include "i_audio_capturer_source.h"
#include "linear_pos_time_model.h"
#include "remote_fast_audio_renderer_sink.h"
#include "remote_fast_audio_capturer_source.h"

// DUMP_PROCESS_FILE // define it for dump file
namespace OHOS {
namespace AudioStandard {
namespace {
    static constexpr int32_t VOLUME_SHIFT_NUMBER = 16; // 1 >> 16 = 65536, max volume
    static constexpr int64_t MAX_SPAN_DURATION_IN_NANO = 100000000; // 100ms
    static constexpr int32_t SLEEP_TIME_IN_DEFAULT = 400; // 400ms
    static constexpr int64_t DELTA_TO_REAL_READ_START_TIME = 0; // 0ms
}

class AudioEndpointInner : public AudioEndpoint {
public:
    explicit AudioEndpointInner(EndpointType type);
    ~AudioEndpointInner();

    bool Config(const DeviceInfo &deviceInfo);
    bool StartDevice();
    bool StopDevice();

    // when audio process start.
    int32_t OnStart(IAudioProcessStream *processStream) override;
    // when audio process pause.
    int32_t OnPause(IAudioProcessStream *processStream) override;
    // when audio process request update handle info.
    int32_t OnUpdateHandleInfo(IAudioProcessStream *processStream) override;

    /**
     * Call LinkProcessStream when first create process or link other process with this endpoint.
     * Here are cases:
     *   case1: endpointStatus_ = UNLINKED, link not running process; UNLINKED-->IDEL & godown
     *   case2: endpointStatus_ = UNLINKED, link running process; UNLINKED-->IDEL & godown
     *   case3: endpointStatus_ = IDEL, link not running process; IDEL-->IDEL
     *   case4: endpointStatus_ = IDEL, link running process; IDEL-->STARTING-->RUNNING
     *   case5: endpointStatus_ = RUNNING; RUNNING-->RUNNING
    */
    int32_t LinkProcessStream(IAudioProcessStream *processStream) override;
    int32_t UnlinkProcessStream(IAudioProcessStream *processStream) override;

    int32_t GetPreferBufferInfo(uint32_t &totalSizeInframe, uint32_t &spanSizeInframe) override;

    void Dump(std::stringstream &dumpStringStream) override;

private:
    bool ConfigInputPoint(const DeviceInfo &deviceInfo);
    int32_t PrepareDeviceBuffer(const DeviceInfo &deviceInfo);
    int32_t GetAdapterBufferInfo(const DeviceInfo &deviceInfo);
    void ReSyncPosition();
    void RecordReSyncPosition();
    void InitAudiobuffer(bool resetReadWritePos);
    void ProcessData(const std::vector<AudioStreamData> &srcDataList, const AudioStreamData &dstData);
    int64_t GetPredictNextReadTime(uint64_t posInFrame);
    int64_t GetPredictNextWriteTime(uint64_t posInFrame);
    bool PrepareNextLoop(uint64_t curWritePos, int64_t &wakeUpTime);
    bool RecordPrepareNextLoop(uint64_t curReadPos, int64_t &wakeUpTime);

    /**
     * @brief Get the current read position in frame and the read-time with it.
     *
     * @param frames the read position in frame
     * @param nanoTime the time in nanosecond when device-sink start read the buffer
    */
    bool GetDeviceHandleInfo(uint64_t &frames, int64_t &nanoTime);
    int32_t GetProcLastWriteDoneInfo(const std::shared_ptr<OHAudioBuffer> processBuffer, uint64_t curWriteFrame,
        uint64_t &proHandleFrame, int64_t &proHandleTime);

    bool IsAnyProcessRunning();
    bool CheckAllBufferReady(int64_t checkTime, uint64_t curWritePos);
    bool ProcessToEndpointDataHandle(uint64_t curWritePos);

    std::string GetStatusStr(EndpointStatus status);

    int32_t WriteToSpecialProcBuf(const std::shared_ptr<OHAudioBuffer> &procBuf, const BufferDesc &readBuf);
    void WriteToProcessBuffers(const BufferDesc &readBuf);
    int32_t ReadFromEndpoint(uint64_t curReadPos);
    bool KeepWorkloopRunning();

    void EndpointWorkLoopFuc();
    void RecordEndpointWorkLoopFuc();

private:
    static constexpr int64_t ONE_MILLISECOND_DURATION = 1000000; // 1ms
    static constexpr int64_t WRITE_TO_HDI_AHEAD_TIME = -1000000; // ahead 1ms
    enum ThreadStatus : uint32_t {
        WAITTING = 0,
        SLEEPING,
        INRUNNING
    };
    // SamplingRate EncodingType SampleFormat Channel
    DeviceInfo deviceInfo_;
    AudioStreamInfo dstStreamInfo_;
    EndpointType endpointType_;

    std::mutex listLock_;
    std::vector<IAudioProcessStream *> processList_;
    std::vector<std::shared_ptr<OHAudioBuffer>> processBufferList_;

    FastAudioRendererSink *fastSink_ = nullptr;
    IMmapAudioCapturerSource *fastSource_ = nullptr;

    LinearPosTimeModel readTimeModel_;
    LinearPosTimeModel writeTimeModel_;

    int64_t spanDuration_ = 0; // nano second
    int64_t serverAheadReadTime_ = 0;
    int dstBufferFd_ = -1; // -1: invalid fd.
    uint32_t dstTotalSizeInframe_ = 0;
    uint32_t dstSpanSizeInframe_ = 0;
    uint32_t dstByteSizePerFrame_ = 0;
    std::shared_ptr<OHAudioBuffer> dstAudioBuffer_ = nullptr;

    std::atomic<EndpointStatus> endpointStatus_ = INVALID;

    std::atomic<ThreadStatus> threadStatus_ = WAITTING;
    std::thread endpointWorkThread_;
    std::mutex loopThreadLock_;
    std::condition_variable workThreadCV_;
    bool isThreadEnd_ = false;
    int64_t lastHandleProcessTime_ = 0;

    bool isDeviceRunningInIdel_ = true; // will call start sink when linked.
    bool needReSyncPosition_ = true;
#ifdef DUMP_PROCESS_FILE
    FILE *dcp_ = nullptr;
    FILE *dump_hdi_ = nullptr;
#endif
};

std::shared_ptr<AudioEndpoint> AudioEndpoint::GetInstance(EndpointType type, const DeviceInfo &deviceInfo)
{
    std::shared_ptr<AudioEndpointInner> audioEndpoint = std::make_shared<AudioEndpointInner>(type);
    CHECK_AND_RETURN_RET_LOG(audioEndpoint != nullptr, nullptr, "Create AudioEndpoint failed.");

    if (!audioEndpoint->Config(deviceInfo)) {
        AUDIO_ERR_LOG("Config AudioEndpoint failed.");
        audioEndpoint = nullptr;
    }
    return audioEndpoint;
}

AudioEndpointInner::AudioEndpointInner(EndpointType type) : endpointType_(type)
{
    AUDIO_INFO_LOG("AudioEndpoint type:%{public}d", endpointType_);
}

AudioEndpointInner::~AudioEndpointInner()
{
    // Wait for thread end and then clear other data to avoid using any cleared data in thread.
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    if (endpointWorkThread_.joinable()) {
        AUDIO_INFO_LOG("AudioEndpoint join work thread start");
        isThreadEnd_ = true;
        endpointWorkThread_.join();
        AUDIO_INFO_LOG("AudioEndpoint join work thread end");
    }

    if (fastSink_ != nullptr) {
        fastSink_->DeInit();
        fastSink_ = nullptr;
    }

    if (fastSource_ != nullptr) {
        fastSource_->DeInit();
        fastSource_ = nullptr;
    }

    endpointStatus_.store(INVALID);

    if (dstAudioBuffer_ != nullptr) {
        AUDIO_INFO_LOG("Set device buffer null");
        dstAudioBuffer_ = nullptr;
    }
#ifdef DUMP_PROCESS_FILE
    if (dcp_) {
        fclose(dcp_);
        dcp_ = nullptr;
    }
    if (dump_hdi_) {
        fclose(dump_hdi_);
        dump_hdi_ = nullptr;
    }
#endif
    AUDIO_INFO_LOG("~AudioEndpoint()");
}

void AudioEndpointInner::Dump(std::stringstream &dumpStringStream)
{
    // dump endpoint stream info
    dumpStringStream << std::endl << "Endpoint stream info:" << std::endl;
    dumpStringStream << " samplingRate:" << dstStreamInfo_.samplingRate << std::endl;
    dumpStringStream << " channels:" << dstStreamInfo_.channels << std::endl;
    dumpStringStream << " format:" << dstStreamInfo_.format << std::endl;

    // dump status info
    dumpStringStream << " Current endpoint status:" << GetStatusStr(endpointStatus_) << std::endl;
    if (dstAudioBuffer_ != nullptr) {
        dumpStringStream << " Currend hdi read position:" << dstAudioBuffer_->GetCurReadFrame() << std::endl;
        dumpStringStream << " Currend hdi write position:" << dstAudioBuffer_->GetCurWriteFrame() << std::endl;
    }

    // dump linked process info
    std::lock_guard<std::mutex> lock(listLock_);
    dumpStringStream << processBufferList_.size() << " linked process:" << std::endl;
    for (auto item : processBufferList_) {
        dumpStringStream << " process read position:" << item->GetCurReadFrame() << std::endl;
        dumpStringStream << " process write position:" << item->GetCurWriteFrame() << std::endl << std::endl;
    }
    dumpStringStream << std::endl;
}

bool AudioEndpointInner::ConfigInputPoint(const DeviceInfo &deviceInfo)
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    fastSource_ = RemoteFastAudioCapturerSource::GetInstance(deviceInfo.networkId);
    IAudioSourceAttr attr = {};
    attr.adapterName = "remote";
    attr.sampleRate = dstStreamInfo_.samplingRate;
    attr.channel = dstStreamInfo_.channels;
    attr.format = dstStreamInfo_.format;
    attr.deviceNetworkId = deviceInfo.networkId.c_str();

    int32_t err = fastSource_->Init(attr);
    if (err != SUCCESS || !fastSource_->IsInited()) {
        AUDIO_ERR_LOG("%{public}s init remote fast fail, err %{public}d.", __func__, err);
        fastSource_ = nullptr;
        return false;
    }
    if (PrepareDeviceBuffer(deviceInfo) != SUCCESS) {
        fastSource_->DeInit();
        fastSource_ = nullptr;
        return false;
    }

    float initVolume = 1.0; // init volume to 1.0, right and left
    err = fastSource_->SetVolume(initVolume, initVolume);
    CHECK_AND_BREAK_LOG(err == SUCCESS, "%{public}s remote fast set volume fail, err %{public}d.", __func__, err);

    bool ret = writeTimeModel_.ConfigSampleRate(dstStreamInfo_.samplingRate);
    CHECK_AND_RETURN_RET_LOG(ret != false, false, "Config LinearPosTimeModel failed.");

    endpointStatus_ = UNLINKED;
    endpointWorkThread_ = std::thread(&AudioEndpointInner::RecordEndpointWorkLoopFuc, this);
    pthread_setname_np(endpointWorkThread_.native_handle(), "AudioEndpointLoop");
#ifdef DUMP_PROCESS_FILE
    dump_hdi_ = fopen("/data/data/server-capture-hdi.pcm", "a+");
    if (dump_hdi_ == nullptr) {
        AUDIO_ERR_LOG("Error opening pcm test file!");
    }
#endif
    return true;
}

bool AudioEndpointInner::Config(const DeviceInfo &deviceInfo)
{
    AUDIO_INFO_LOG("%{public}s enter, deviceRole %{public}d.", __func__, deviceInfo.deviceRole);
    deviceInfo_ = deviceInfo;
    dstStreamInfo_ = deviceInfo.audioStreamInfo;
    if (deviceInfo.deviceRole == INPUT_DEVICE) {
        return ConfigInputPoint(deviceInfo);
    }

    fastSink_ = deviceInfo.networkId == REMOTE_NETWORK_ID ?
        RemoteFastAudioRendererSink::GetInstance(deviceInfo.networkId) : FastAudioRendererSink::GetInstance();
    IAudioSinkAttr attr = {};
    attr.adapterName = "primary";
    attr.sampleRate = dstStreamInfo_.samplingRate; // 48000hz
    attr.channel = dstStreamInfo_.channels; // STEREO = 2
    attr.format = dstStreamInfo_.format; // SAMPLE_S16LE = 1
    attr.sampleFmt = dstStreamInfo_.format;
    attr.deviceNetworkId = deviceInfo.networkId.c_str();

    fastSink_->Init(attr);
    if (!fastSink_->IsInited()) {
        fastSink_ = nullptr;
        return false;
    }
    if (PrepareDeviceBuffer(deviceInfo) != SUCCESS) {
        fastSink_->DeInit();
        fastSink_ = nullptr;
        return false;
    }

    float initVolume = 1.0; // init volume to 1.0
    fastSink_->SetVolume(initVolume, initVolume);

    bool ret = readTimeModel_.ConfigSampleRate(dstStreamInfo_.samplingRate);
    CHECK_AND_RETURN_RET_LOG(ret != false, false, "Config LinearPosTimeModel failed.");

    endpointStatus_ = UNLINKED;
    endpointWorkThread_ = std::thread(&AudioEndpointInner::EndpointWorkLoopFuc, this);
    pthread_setname_np(endpointWorkThread_.native_handle(), "AudioEndpointLoop");

#ifdef DUMP_PROCESS_FILE
    dcp_ = fopen("/data/data/server-read-client.pcm", "a+");
    dump_hdi_ = fopen("/data/data/server-hdi.pcm", "a+");
    if (dcp_ == nullptr || dump_hdi_ == nullptr) {
        AUDIO_ERR_LOG("Error opening pcm test file!");
    }
#endif
    return true;
}

int32_t AudioEndpointInner::GetAdapterBufferInfo(const DeviceInfo &deviceInfo)
{
    int32_t ret = 0;
    AUDIO_INFO_LOG("%{public}s enter, deviceRole %{public}d.", __func__, deviceInfo.deviceRole);
    if (deviceInfo.deviceRole == INPUT_DEVICE) {
        CHECK_AND_RETURN_RET_LOG(fastSource_ != nullptr, ERR_INVALID_HANDLE,
            "%{public}s fast source is null.", __func__);
        ret = fastSource_->GetMmapBufferInfo(dstBufferFd_, dstTotalSizeInframe_, dstSpanSizeInframe_,
        dstByteSizePerFrame_);
    } else {
        CHECK_AND_RETURN_RET_LOG(fastSink_ != nullptr, ERR_INVALID_HANDLE, "%{public}s fast sink is null.", __func__);
        ret = fastSink_->GetMmapBufferInfo(dstBufferFd_, dstTotalSizeInframe_, dstSpanSizeInframe_,
        dstByteSizePerFrame_);
    }

    if (ret != SUCCESS || dstBufferFd_ == -1 || dstTotalSizeInframe_ == 0 || dstSpanSizeInframe_ == 0 ||
        dstByteSizePerFrame_ == 0) {
        AUDIO_ERR_LOG("%{public}s get mmap buffer info fail, ret %{public}d, dstBufferFd %{public}d, \
            dstTotalSizeInframe %{public}d, dstSpanSizeInframe %{public}d, dstByteSizePerFrame %{public}d.",
            __func__, ret, dstBufferFd_, dstTotalSizeInframe_, dstSpanSizeInframe_, dstByteSizePerFrame_);
        return ERR_ILLEGAL_STATE;
    }
    AUDIO_INFO_LOG("%{public}s end, fd %{public}d.", __func__, dstBufferFd_);
    return SUCCESS;
}

int32_t AudioEndpointInner::PrepareDeviceBuffer(const DeviceInfo &deviceInfo)
{
    AUDIO_INFO_LOG("%{public}s enter, deviceRole %{public}d.", __func__, deviceInfo.deviceRole);
    if (dstAudioBuffer_ != nullptr) {
        AUDIO_INFO_LOG("%{public}s endpoint buffer is preapred, fd:%{public}d", __func__, dstBufferFd_);
        return SUCCESS;
    }

    int32_t ret = GetAdapterBufferInfo(deviceInfo);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED,
        "%{public}s get adapter buffer Info fail, ret %{public}d.", __func__, ret);

    // spanDuration_ may be less than the correct time of dstSpanSizeInframe_.
    spanDuration_ = dstSpanSizeInframe_ * AUDIO_NS_PER_SECOND / dstStreamInfo_.samplingRate;
    int64_t temp = spanDuration_ / 4 * 3; // 3/4 spanDuration
    serverAheadReadTime_ = temp < ONE_MILLISECOND_DURATION ? ONE_MILLISECOND_DURATION : temp; // at least 1ms ahead.
    AUDIO_INFO_LOG("%{public}s spanDuration %{public}" PRIu64" ns, serverAheadReadTime %{public}" PRIu64" ns.",
        __func__, spanDuration_, serverAheadReadTime_);

    if (spanDuration_ <= 0 || spanDuration_ >= MAX_SPAN_DURATION_IN_NANO) {
        AUDIO_ERR_LOG("%{public}s mmap span info error, spanDuration %{public}" PRIu64".", __func__, spanDuration_);
        return ERR_INVALID_PARAM;
    }
    dstAudioBuffer_ = OHAudioBuffer::CreateFromRemote(dstTotalSizeInframe_, dstSpanSizeInframe_, dstByteSizePerFrame_,
        dstBufferFd_);
    if (dstAudioBuffer_ == nullptr || dstAudioBuffer_->GetBufferHolder() != AudioBufferHolder::AUDIO_SERVER_ONLY) {
        AUDIO_ERR_LOG("%{public}s create buffer from remote fail.", __func__);
        return ERR_ILLEGAL_STATE;
    }
    dstAudioBuffer_->GetStreamStatus()->store(StreamStatus::STREAM_IDEL);

    // clear data buffer
    ret = memset_s(dstAudioBuffer_->GetDataBase(), dstAudioBuffer_->GetDataSize(), 0, dstAudioBuffer_->GetDataSize());
    CHECK_AND_BREAK_LOG(ret == EOK, "%{public}s memset buffer fail, ret %{public}d, fd %{public}d.",
        __func__, ret, dstBufferFd_);
    InitAudiobuffer(true);

    AUDIO_INFO_LOG("%{public}s end, fd %{public}d.", __func__, dstBufferFd_);
    return SUCCESS;
}

void AudioEndpointInner::InitAudiobuffer(bool resetReadWritePos)
{
    CHECK_AND_RETURN_LOG((dstAudioBuffer_ != nullptr), "%{public}s: dst audio buffer is null.", __func__);
    if (resetReadWritePos) {
        dstAudioBuffer_->ResetCurReadWritePos(0, 0);
    }

    uint32_t spanCount = dstAudioBuffer_->GetSpanCount();
    for (uint32_t i = 0; i < spanCount; i++) {
        SpanInfo *spanInfo = dstAudioBuffer_->GetSpanInfoByIndex(i);
        if (spanInfo == nullptr) {
            AUDIO_ERR_LOG("InitAudiobuffer failed.");
            return;
        }
        if (deviceInfo_.deviceRole == INPUT_DEVICE) {
            spanInfo->spanStatus = SPAN_WRITE_DONE;
        } else {
            spanInfo->spanStatus = SPAN_READ_DONE;
        }
        spanInfo->offsetInFrame = 0;

        spanInfo->readStartTime = 0;
        spanInfo->readDoneTime = 0;

        spanInfo->writeStartTime = 0;
        spanInfo->writeDoneTime = 0;

        spanInfo->volumeStart = 1 << VOLUME_SHIFT_NUMBER; // 65536 for initialize
        spanInfo->volumeEnd = 1 << VOLUME_SHIFT_NUMBER; // 65536 for initialize
        spanInfo->isMute = false;
    }
    return;
}

int32_t AudioEndpointInner::GetPreferBufferInfo(uint32_t &totalSizeInframe, uint32_t &spanSizeInframe)
{
    totalSizeInframe = dstTotalSizeInframe_;
    spanSizeInframe = dstSpanSizeInframe_;
    return SUCCESS;
}

bool AudioEndpointInner::IsAnyProcessRunning()
{
    std::lock_guard<std::mutex> lock(listLock_);
    bool isRunning = false;
    for (size_t i = 0; i < processBufferList_.size(); i++) {
        if (processBufferList_[i]->GetStreamStatus()->load() == STREAM_RUNNING) {
            isRunning = true;
            break;
        }
    }
    return isRunning;
}

void AudioEndpointInner::RecordReSyncPosition()
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    uint64_t curHdiWritePos = 0;
    int64_t writeTime = 0;
    CHECK_AND_RETURN_LOG(GetDeviceHandleInfo(curHdiWritePos, writeTime),
        "%{public}s get device handle info fail.", __func__);
    AUDIO_INFO_LOG("%{public}s get capturer info, curHdiWritePos %{public}" PRIu64", writeTime %{public}" PRId64".",
        __func__, curHdiWritePos, writeTime);
    int64_t temp = ClockTime::GetCurNano() - writeTime;
    if (temp > spanDuration_) {
        AUDIO_WARNING_LOG("%{public}s GetDeviceHandleInfo cost long time %{public}" PRIu64".", __func__, temp);
    }

    writeTimeModel_.ResetFrameStamp(curHdiWritePos, writeTime);
    uint64_t nextDstReadPos = curHdiWritePos;
    uint64_t nextDstWritePos = curHdiWritePos;
    InitAudiobuffer(false);
    int32_t ret = dstAudioBuffer_->ResetCurReadWritePos(nextDstReadPos, nextDstWritePos);
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("ResetCurReadWritePos failed.");
        return;
    }

    SpanInfo *nextReadSapn = dstAudioBuffer_->GetSpanInfo(nextDstReadPos);
    CHECK_AND_RETURN_LOG(nextReadSapn != nullptr, "GetSpanInfo failed.");
    nextReadSapn->offsetInFrame = nextDstReadPos;
    nextReadSapn->spanStatus = SpanStatus::SPAN_WRITE_DONE;
    AUDIO_INFO_LOG("%{public}s end.", __func__);
}

void AudioEndpointInner::ReSyncPosition()
{
    uint64_t curHdiReadPos = 0;
    int64_t readTime = 0;
    if (!GetDeviceHandleInfo(curHdiReadPos, readTime)) {
        AUDIO_ERR_LOG("ReSyncPosition call GetDeviceHandleInfo failed.");
        return;
    }
    int64_t curTime = ClockTime::GetCurNano();
    int64_t temp = curTime - readTime;
    if (temp > spanDuration_) {
        AUDIO_ERR_LOG("GetDeviceHandleInfo may cost long time.");
    }

    readTimeModel_.ResetFrameStamp(curHdiReadPos, readTime);
    uint64_t nextDstWritePos = curHdiReadPos + dstSpanSizeInframe_;
    InitAudiobuffer(false);
    int32_t ret = dstAudioBuffer_->ResetCurReadWritePos(nextDstWritePos, nextDstWritePos);
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("ResetCurReadWritePos failed.");
        return;
    }

    SpanInfo *nextWriteSapn = dstAudioBuffer_->GetSpanInfo(nextDstWritePos);
    CHECK_AND_RETURN_LOG(nextWriteSapn != nullptr, "GetSpanInfo failed.");
    nextWriteSapn->offsetInFrame = nextDstWritePos;
    nextWriteSapn->spanStatus = SpanStatus::SPAN_READ_DONE;
    return;
}

bool AudioEndpointInner::StartDevice()
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    // how to modify the status while unlinked and started?
    if (endpointStatus_ != IDEL) {
        AUDIO_ERR_LOG("Endpoint status is not IDEL");
        return false;
    }
    endpointStatus_ = STARTING;
    if (deviceInfo_.deviceRole == INPUT_DEVICE) {
        if (fastSource_ == nullptr || fastSource_->Start() != SUCCESS) {
            AUDIO_ERR_LOG("Source start failed.");
            return false;
        }
    } else {
        if (fastSink_ == nullptr || fastSink_->Start() != SUCCESS) {
            AUDIO_ERR_LOG("Sink start failed.");
            return false;
        }
    }

    std::unique_lock<std::mutex> lock(loopThreadLock_);
    needReSyncPosition_ = true;
    endpointStatus_ = IsAnyProcessRunning() ? RUNNING : IDEL;
    workThreadCV_.notify_all();
    AUDIO_INFO_LOG("StartDevice out, status is %{public}s", GetStatusStr(endpointStatus_).c_str());
    return true;
}

bool AudioEndpointInner::StopDevice()
{
    AUDIO_INFO_LOG("StopDevice with status:%{public}s", GetStatusStr(endpointStatus_).c_str());
    // todo
    endpointStatus_ = STOPPING;
    // Clear data buffer to avoid noise in some case.
    if (dstAudioBuffer_ != nullptr) {
        int32_t ret = memset_s(dstAudioBuffer_->GetDataBase(), dstAudioBuffer_->GetDataSize(), 0,
            dstAudioBuffer_->GetDataSize());
        AUDIO_INFO_LOG("StopDevice clear buffer ret:%{public}d", ret);
    }
    if (deviceInfo_.deviceRole == INPUT_DEVICE) {
        if (fastSource_ == nullptr || fastSource_->Stop() != SUCCESS) {
            AUDIO_ERR_LOG("Source stop failed.");
            return false;
        }
    } else {
        if (fastSink_ == nullptr || fastSink_->Stop() != SUCCESS) {
            AUDIO_ERR_LOG("Sink stop failed.");
            return false;
        }
    }
    endpointStatus_ = STOPPED;
    return true;
}

int32_t AudioEndpointInner::OnStart(IAudioProcessStream *processStream)
{
    AUDIO_INFO_LOG("OnStart endpoint status:%{public}s", GetStatusStr(endpointStatus_).c_str());
    if (endpointStatus_ == RUNNING) {
        AUDIO_INFO_LOG("OnStart find endpoint already in RUNNING.");
        return SUCCESS;
    }
    if (endpointStatus_ == IDEL && !isDeviceRunningInIdel_) {
        // call sink start
        StartDevice();
        endpointStatus_ = RUNNING;
    }
    return SUCCESS;
}

int32_t AudioEndpointInner::OnPause(IAudioProcessStream *processStream)
{
    AUDIO_INFO_LOG("OnPause endpoint status:%{public}s", GetStatusStr(endpointStatus_).c_str());
    if (endpointStatus_ == RUNNING) {
        endpointStatus_ = IsAnyProcessRunning() ? RUNNING : IDEL;
    }
    if (endpointStatus_ == IDEL && !isDeviceRunningInIdel_) {
        // call sink stop when no process running?
        AUDIO_INFO_LOG("OnPause status is IDEL, call stop");
    }
    // todo
    return SUCCESS;
}

int32_t AudioEndpointInner::GetProcLastWriteDoneInfo(const std::shared_ptr<OHAudioBuffer> processBuffer,
    uint64_t curWriteFrame, uint64_t &proHandleFrame, int64_t &proHandleTime)
{
    CHECK_AND_RETURN_RET_LOG(processBuffer != nullptr, ERR_INVALID_HANDLE, "Process found but buffer is null");
    uint64_t curReadFrame = processBuffer->GetCurReadFrame();
    SpanInfo *curWriteSpan = processBuffer->GetSpanInfo(curWriteFrame);
    CHECK_AND_RETURN_RET_LOG(curWriteSpan != nullptr, ERR_INVALID_HANDLE,
        "%{public}s curWriteSpan of curWriteFrame %{public}" PRIu64" is null", __func__, curWriteFrame);
    if (curWriteSpan->spanStatus == SpanStatus::SPAN_WRITE_DONE || curWriteFrame < dstSpanSizeInframe_ ||
        curWriteFrame < curReadFrame) {
        proHandleFrame = curWriteFrame;
        proHandleTime = curWriteSpan->writeDoneTime;
    } else {
        int32_t ret = GetProcLastWriteDoneInfo(processBuffer, curWriteFrame - dstSpanSizeInframe_,
            proHandleFrame, proHandleTime);
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret,
            "%{public}s get process last write done info fail, ret %{public}d.", __func__, ret);
    }

    AUDIO_INFO_LOG("%{public}s end, curWriteFrame %{public}" PRIu64", proHandleFrame %{public}" PRIu64", "
        "proHandleTime %{public}" PRId64".", __func__, curWriteFrame, proHandleFrame, proHandleTime);
    return SUCCESS;
}

int32_t AudioEndpointInner::OnUpdateHandleInfo(IAudioProcessStream *processStream)
{
    Trace trace("AudioEndpoint::OnUpdateHandleInfo");
    bool isFind = false;
    std::lock_guard<std::mutex> lock(listLock_);
    auto processItr = processList_.begin();
    while (processItr != processList_.end()) {
        if (*processItr != processStream) {
            processItr++;
            continue;
        }
        std::shared_ptr<OHAudioBuffer> processBuffer = (*processItr)->GetStreamBuffer();
        CHECK_AND_RETURN_RET_LOG(processBuffer != nullptr, ERR_OPERATION_FAILED, "Process found but buffer is null");
        uint64_t proHandleFrame = 0;
        int64_t proHandleTime = 0;
        if (deviceInfo_.deviceRole == INPUT_DEVICE) {
            uint64_t curWriteFrame = processBuffer->GetCurWriteFrame();
            int32_t ret = GetProcLastWriteDoneInfo(processBuffer, curWriteFrame, proHandleFrame, proHandleTime);
            CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret,
                "%{public}s get process last write done info fail, ret %{public}d.", __func__, ret);
        } else {
            proHandleFrame = processBuffer->GetCurReadFrame();
            proHandleTime = lastHandleProcessTime_;
        }
        processBuffer->SetHandleInfo(proHandleFrame, proHandleTime);
        AUDIO_INFO_LOG("%{public}s set process buf handle pos %{public}" PRIu64", handle time %{public}" PRId64", "
            "deviceRole %{public}d.", __func__, proHandleFrame, proHandleTime, deviceInfo_.deviceRole);
        isFind = true;
        break;
    }
    if (!isFind) {
        AUDIO_ERR_LOG("Can not find any process to UpdateHandleInfo");
        return ERR_OPERATION_FAILED;
    }
    return SUCCESS;
}

int32_t AudioEndpointInner::LinkProcessStream(IAudioProcessStream *processStream)
{
    CHECK_AND_RETURN_RET_LOG(processStream != nullptr, ERR_INVALID_PARAM, "IAudioProcessStream is null");
    std::shared_ptr<OHAudioBuffer> processBuffer = processStream->GetStreamBuffer();
    CHECK_AND_RETURN_RET_LOG(processBuffer != nullptr, ERR_INVALID_PARAM, "processBuffer is null");

    CHECK_AND_RETURN_RET_LOG(processList_.size() < MAX_LINKED_PROCESS, ERR_OPERATION_FAILED, "reach link limit.");

    AUDIO_INFO_LOG("LinkProcessStream endpoint status:%{public}s.", GetStatusStr(endpointStatus_).c_str());

    bool needEndpointRunning = processBuffer->GetStreamStatus()->load() == STREAM_RUNNING;

    if (endpointStatus_ == STARTING) {
        AUDIO_INFO_LOG("LinkProcessStream wait start begin.");
        std::unique_lock<std::mutex> lock(loopThreadLock_);
        workThreadCV_.wait(lock, [this] {
            return endpointStatus_ != STARTING;
        });
        AUDIO_INFO_LOG("LinkProcessStream wait start end.");
    }

    if (endpointStatus_ == RUNNING) {
        std::lock_guard<std::mutex> lock(listLock_);
        processList_.push_back(processStream);
        processBufferList_.push_back(processBuffer);
        AUDIO_INFO_LOG("LinkProcessStream success.");
        return SUCCESS;
    }

    if (endpointStatus_ == UNLINKED) {
        endpointStatus_ = IDEL; // handle push_back in IDEL
        if (isDeviceRunningInIdel_) {
            StartDevice();
        }
    }

    if (endpointStatus_ == IDEL) {
        {
            std::lock_guard<std::mutex> lock(listLock_);
            processList_.push_back(processStream);
            processBufferList_.push_back(processBuffer);
        }
        if (!needEndpointRunning) {
            AUDIO_INFO_LOG("LinkProcessStream success, process stream status is not running.");
            return SUCCESS;
        }
        // needEndpointRunning = true
        if (isDeviceRunningInIdel_) {
            endpointStatus_ = IsAnyProcessRunning() ? RUNNING : IDEL;
        } else {
            // needEndpointRunning = true & isDeviceRunningInIdel_ = false
            // KeepWorkloopRunning will wait on IDEL
            StartDevice();
        }
        AUDIO_INFO_LOG("LinkProcessStream success.");
        return SUCCESS;
    }

    return SUCCESS;
}

int32_t AudioEndpointInner::UnlinkProcessStream(IAudioProcessStream *processStream)
{
    AUDIO_INFO_LOG("UnlinkProcessStream in status:%{public}s.", GetStatusStr(endpointStatus_).c_str());
    CHECK_AND_RETURN_RET_LOG(processStream != nullptr, ERR_INVALID_PARAM, "IAudioProcessStream is null");
    std::shared_ptr<OHAudioBuffer> processBuffer = processStream->GetStreamBuffer();
    CHECK_AND_RETURN_RET_LOG(processBuffer != nullptr, ERR_INVALID_PARAM, "processBuffer is null");

    bool isFind = false;
    std::lock_guard<std::mutex> lock(listLock_);
    auto processItr = processList_.begin();
    auto bufferItr = processBufferList_.begin();
    while (processItr != processList_.end()) {
        if (*processItr == processStream && *bufferItr == processBuffer) {
            processList_.erase(processItr);
            processBufferList_.erase(bufferItr);
            isFind = true;
            break;
        } else {
            processItr++;
            bufferItr++;
        }
    }
    if (processList_.size() == 0) {
        StopDevice();
        endpointStatus_ = UNLINKED;
    }

    AUDIO_INFO_LOG("UnlinkProcessStream end, %{public}s the process.", (isFind ? "find and remove" : "not find"));
    return SUCCESS;
}

bool AudioEndpointInner::CheckAllBufferReady(int64_t checkTime, uint64_t curWritePos)
{
    bool isAllReady = true;
    std::lock_guard<std::mutex> lock(listLock_);
    for (size_t i = 0; i < processBufferList_.size(); i++) {
        std::shared_ptr<OHAudioBuffer> tempBuffer = processBufferList_[i];
        uint64_t eachCurReadPos = processBufferList_[i]->GetCurReadFrame();
        lastHandleProcessTime_ = checkTime;
        processBufferList_[i]->SetHandleInfo(eachCurReadPos, lastHandleProcessTime_); // update info for each process
        if (tempBuffer->GetStreamStatus()->load() != StreamStatus::STREAM_RUNNING) {
            continue; // process not running
        }
        uint64_t curRead = tempBuffer->GetCurReadFrame();
        SpanInfo *curReadSpan = tempBuffer->GetSpanInfo(curRead);
        if (curReadSpan == nullptr || curReadSpan->spanStatus != SpanStatus::SPAN_WRITE_DONE) {
            AUDIO_WARNING_LOG("Find one process not ready"); // print uid of the process?
            isAllReady = false;
            break;
        }
    }

    if (!isAllReady) {
        Trace trace("AudioEndpoint::WaitAllProcessReady");
        int64_t tempWakeupTime = readTimeModel_.GetTimeOfPos(curWritePos) + WRITE_TO_HDI_AHEAD_TIME;
        if (tempWakeupTime - ClockTime::GetCurNano() < ONE_MILLISECOND_DURATION) {
            ClockTime::RelativeSleep(ONE_MILLISECOND_DURATION);
        } else {
            ClockTime::AbsoluteSleep(tempWakeupTime); // sleep to hdi read time ahead 1ms.
        }
    }
    return isAllReady;
}

void AudioEndpointInner::ProcessData(const std::vector<AudioStreamData> &srcDataList, const AudioStreamData &dstData)
{
    Trace trace("AudioEndpoint::ProcessData");
    size_t srcListSize = srcDataList.size();

    for (size_t i = 0; i < srcListSize; i++) {
        if (srcDataList[i].streamInfo.format != SAMPLE_S16LE || srcDataList[i].streamInfo.channels != STEREO ||
            srcDataList[i].bufferDesc.bufLength != dstData.bufferDesc.bufLength ||
            srcDataList[i].bufferDesc.dataLength != dstData.bufferDesc.dataLength) {
            AUDIO_ERR_LOG("ProcessData failed, streamInfo are different");
            return;
        }
    }

    // Assum using the same format and same size
    if (dstData.streamInfo.format != SAMPLE_S16LE || dstData.streamInfo.channels != STEREO) {
        AUDIO_ERR_LOG("ProcessData failed, streamInfo are not support");
        return;
    }

    size_t dataLength = dstData.bufferDesc.dataLength;
    dataLength /= 2; // SAMPLE_S16LE--> 2 byte
    int16_t *dstPtr = reinterpret_cast<int16_t *>(dstData.bufferDesc.buffer);
    for (size_t offset = 0; dataLength > 0; dataLength--) {
        int32_t sum = 0;
        for (size_t i = 0; i < srcListSize; i++) {
            int32_t vol = srcDataList[i].volumeStart; // change to modify volume of each channel
            int16_t *srcPtr = reinterpret_cast<int16_t *>(srcDataList[i].bufferDesc.buffer) + offset;
            sum += (*srcPtr * static_cast<int64_t>(vol)) >> VOLUME_SHIFT_NUMBER; // 1/65536
        }
        offset++;
        *dstPtr++ = sum > INT16_MAX ? INT16_MAX : (sum < INT16_MIN ? INT16_MIN : sum);
    }
}

bool AudioEndpointInner::ProcessToEndpointDataHandle(uint64_t curWritePos)
{
    std::lock_guard<std::mutex> lock(listLock_);

    std::vector<AudioStreamData> audioDataList;
    for (size_t i = 0; i < processBufferList_.size(); i++) {
        uint64_t curRead = processBufferList_[i]->GetCurReadFrame();
        SpanInfo *curReadSpan = processBufferList_[i]->GetSpanInfo(curRead);
        if (curReadSpan == nullptr) {
            AUDIO_ERR_LOG("GetSpanInfo failed, can not get client curReadSpan");
            continue;
        }
        AudioStreamData streamData;
        streamData.volumeStart = curReadSpan->volumeStart;
        streamData.volumeEnd = curReadSpan->volumeEnd;
        streamData.streamInfo = processList_[i]->GetStreamInfo();
        SpanStatus targetStatus = SpanStatus::SPAN_WRITE_DONE;
        if (curReadSpan->spanStatus.compare_exchange_strong(targetStatus, SpanStatus::SPAN_READING)) {
            processBufferList_[i]->GetReadbuffer(curRead, streamData.bufferDesc); // check return?
            audioDataList.push_back(streamData);
            curReadSpan->readStartTime = ClockTime::GetCurNano();
#ifdef DUMP_PROCESS_FILE
            if (dcp_ != nullptr) {
                fwrite(static_cast<void *>(streamData.bufferDesc.buffer), 1, streamData.bufferDesc.bufLength, dcp_);
            }
#endif
        }
    }

    AudioStreamData dstStreamData;
    dstStreamData.streamInfo = dstStreamInfo_;
    int32_t ret = dstAudioBuffer_->GetWriteBuffer(curWritePos, dstStreamData.bufferDesc);
    CHECK_AND_RETURN_RET_LOG(((ret == SUCCESS && dstStreamData.bufferDesc.buffer != nullptr)), false,
        "GetWriteBuffer failed, ret:%{public}d", ret);

    SpanInfo *curWriteSpan = dstAudioBuffer_->GetSpanInfo(curWritePos);
    CHECK_AND_RETURN_RET_LOG(curWriteSpan != nullptr, false, "GetSpanInfo failed, can not get curWriteSpan");

    dstStreamData.volumeStart = curWriteSpan->volumeStart;
    dstStreamData.volumeEnd = curWriteSpan->volumeEnd;
    curWriteSpan->writeStartTime = ClockTime::GetCurNano();
    // do write work
    if (audioDataList.size() == 0) {
        memset_s(dstStreamData.bufferDesc.buffer, dstStreamData.bufferDesc.bufLength, 0,
            dstStreamData.bufferDesc.bufLength);
    } else {
        ProcessData(audioDataList, dstStreamData);
    }
    curWriteSpan->writeDoneTime = ClockTime::GetCurNano(); // debug server time cost

#ifdef DUMP_PROCESS_FILE
    if (dump_hdi_ != nullptr) {
        fwrite(static_cast<void *>(dstStreamData.bufferDesc.buffer), 1, dstStreamData.bufferDesc.bufLength, dump_hdi_);
    }
#endif
    return true;
}

int64_t AudioEndpointInner::GetPredictNextReadTime(uint64_t posInFrame)
{
    Trace trace("AudioEndpoint::GetPredictNextRead");
    uint64_t handleSpanCnt = posInFrame / dstSpanSizeInframe_;
    uint32_t startPeriodCnt = 20; // sync each time when start
    uint32_t oneBigPeriodCnt = 40; // 200ms
    if (handleSpanCnt < startPeriodCnt || handleSpanCnt % oneBigPeriodCnt == 0) {
        // todo sleep random little time but less than nextHdiReadTime - 2ms
        uint64_t readFrame = 0;
        int64_t readtime = 0;
        if (GetDeviceHandleInfo(readFrame, readtime)) {
            readTimeModel_.UpdataFrameStamp(readFrame, readtime);
        }
    }

    int64_t nextHdiReadTime = readTimeModel_.GetTimeOfPos(posInFrame);
    return nextHdiReadTime;
}

int64_t AudioEndpointInner::GetPredictNextWriteTime(uint64_t posInFrame)
{
    uint64_t handleSpanCnt = posInFrame / dstSpanSizeInframe_;
    uint32_t startPeriodCnt = 20;
    uint32_t oneBigPeriodCnt = 40;
    if (handleSpanCnt < startPeriodCnt || handleSpanCnt % oneBigPeriodCnt == 0) {
        // todo sleep random little time but less than nextHdiReadTime - 2ms
        uint64_t writeFrame = 0;
        int64_t writeTime = 0;
        if (GetDeviceHandleInfo(writeFrame, writeTime)) {
            writeTimeModel_.UpdataFrameStamp(writeFrame, writeTime);
        }
    }
    int64_t nextHdiWriteTime = writeTimeModel_.GetTimeOfPos(posInFrame);
    AUDIO_INFO_LOG("%{public}s end, posInFrame %{public}" PRIu64", nextHdiWriteTime %{public}" PRIu64"",
        __func__, posInFrame, nextHdiWriteTime);
    return nextHdiWriteTime;
}

bool AudioEndpointInner::RecordPrepareNextLoop(uint64_t curReadPos, int64_t &wakeUpTime)
{
    uint64_t nextHandlePos = curReadPos + dstSpanSizeInframe_;
    int64_t nextHdiWriteTime = GetPredictNextWriteTime(nextHandlePos);
    wakeUpTime = nextHdiWriteTime + serverAheadReadTime_;

    int32_t ret = dstAudioBuffer_->SetCurWriteFrame(nextHandlePos);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "%{public}s set dst buffer write frame fail, ret %{public}d.",
        __func__, ret);
    ret = dstAudioBuffer_->SetCurReadFrame(nextHandlePos);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "%{public}s set dst buffer read frame fail, ret %{public}d.",
        __func__, ret);

    AUDIO_INFO_LOG("%{public}s end, dstAudioBuffer curReadPos %{public}" PRIu64", nextHandlePos %{public}" PRIu64", "
        "wakeUpTime %{public}" PRId64"", __func__, curReadPos, nextHandlePos, wakeUpTime);
    return true;
}

bool AudioEndpointInner::PrepareNextLoop(uint64_t curWritePos, int64_t &wakeUpTime)
{
    Trace prepareTrace("AudioEndpoint::PrepareNextLoop");
    uint64_t nextHandlePos = curWritePos + dstSpanSizeInframe_;
    int64_t nextHdiReadTime = GetPredictNextReadTime(nextHandlePos);
    wakeUpTime = nextHdiReadTime - serverAheadReadTime_;

    SpanInfo *nextWriteSpan = dstAudioBuffer_->GetSpanInfo(nextHandlePos);
    if (nextWriteSpan == nullptr) {
        AUDIO_ERR_LOG("GetSpanInfo failed, can not get next write span");
        return false;
    }

    int32_t ret1 = dstAudioBuffer_->SetCurWriteFrame(nextHandlePos);
    int32_t ret2 = dstAudioBuffer_->SetCurReadFrame(nextHandlePos);
    if (ret1 != SUCCESS || ret2 != SUCCESS) {
        AUDIO_ERR_LOG("SetCurWriteFrame or SetCurReadFrame failed, ret1:%{public}d ret2:%{public}d", ret1, ret2);
        return false;
    }
    // handl each process buffer info
    int64_t curReadDoneTime = ClockTime::GetCurNano();
    for (size_t i = 0; i < processBufferList_.size(); i++) {
        uint64_t eachCurReadPos = processBufferList_[i]->GetCurReadFrame();
        SpanInfo *tempSpan = processBufferList_[i]->GetSpanInfo(eachCurReadPos);
        if (tempSpan == nullptr) {
            AUDIO_ERR_LOG("GetSpanInfo failed, can not get process read span");
            return false;
        }
        SpanStatus targetStatus = SpanStatus::SPAN_READING;
        if (tempSpan->spanStatus.compare_exchange_strong(targetStatus, SpanStatus::SPAN_READ_DONE)) {
            tempSpan->readDoneTime = curReadDoneTime;
            BufferDesc bufferReadDone = { nullptr, 0, 0};
            processBufferList_[i]->GetReadbuffer(eachCurReadPos, bufferReadDone);
            if (bufferReadDone.buffer != nullptr && bufferReadDone.bufLength != 0) {
                memset_s(bufferReadDone.buffer, bufferReadDone.bufLength, 0, bufferReadDone.bufLength);
            }
            processBufferList_[i]->SetCurReadFrame(eachCurReadPos + dstSpanSizeInframe_); // use client span size
        } else if (processBufferList_[i]->GetStreamStatus()->load() == StreamStatus::STREAM_RUNNING) {
            AUDIO_WARNING_LOG("Current span not ready:%{public}d", targetStatus);
        }
    }
    return true;
}

bool AudioEndpointInner::GetDeviceHandleInfo(uint64_t &frames, int64_t &nanoTime)
{
    Trace trace("AudioEndpoint::GetDeviceHandleInfo");
    int64_t timeSec = 0;
    int64_t timeNanoSec = 0;
    int32_t ret = 0;
    if (deviceInfo_.deviceRole == INPUT_DEVICE) {
        if (fastSource_ == nullptr || !fastSource_->IsInited()) {
            AUDIO_ERR_LOG("Source start failed.");
            return false;
        }
        // GetMmapHandlePosition will call using ipc.
        ret = fastSource_->GetMmapHandlePosition(frames, timeSec, timeNanoSec);
    } else {
        if (fastSink_ == nullptr || !fastSink_->IsInited()) {
            AUDIO_ERR_LOG("GetDeviceHandleInfo failed: sink is not inited.");
            return false;
        }
        // GetMmapHandlePosition will call using ipc.
        ret = fastSink_->GetMmapHandlePosition(frames, timeSec, timeNanoSec);
    }
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("Call adapter GetMmapHandlePosition failed: %{public}d", ret);
        return false;
    }

    nanoTime = timeNanoSec + timeSec * AUDIO_NS_PER_SECOND;
    nanoTime += DELTA_TO_REAL_READ_START_TIME; // global delay in server
    return true;
}

std::string AudioEndpointInner::GetStatusStr(EndpointStatus status)
{
    switch (status) {
        case INVALID:
            return "INVALID";
        case UNLINKED:
            return "UNLINKED";
        case IDEL:
            return "IDEL";
        case STARTING:
            return "STARTING";
        case RUNNING:
            return "RUNNING";
        case STOPPING:
            return "STOPPING";
        case STOPPED:
            return "STOPPED";
        default:
            break;
    }
    return "NO_SUCH_STATUS";
}

bool AudioEndpointInner::KeepWorkloopRunning()
{
    EndpointStatus targetStatus = INVALID;
    switch (endpointStatus_.load()) {
        case RUNNING:
            return true;
        case IDEL:
            if (isDeviceRunningInIdel_) {
                return true;
            } else {
                targetStatus = STARTING;
            }
            break;
        case UNLINKED:
            targetStatus = IDEL;
            break;
        case STARTING:
            targetStatus = RUNNING;
            break;
        case STOPPING:
            targetStatus = STOPPED;
            break;
        default:
            break;
    }

    // when return false, EndpointWorkLoopFuc will continue loop immediately. Wait to avoid a inifity loop.
    std::unique_lock<std::mutex> lock(loopThreadLock_);
    AUDIO_INFO_LOG("Status is %{public}s now, wait for %{public}s...", GetStatusStr(endpointStatus_).c_str(),
        GetStatusStr(targetStatus).c_str());
    threadStatus_ = WAITTING;
    workThreadCV_.wait_for(lock, std::chrono::milliseconds(SLEEP_TIME_IN_DEFAULT));
    AUDIO_INFO_LOG("Wait end. Cur is %{public}s now, target is %{public}s...", GetStatusStr(endpointStatus_).c_str(),
        GetStatusStr(targetStatus).c_str());

    return false;
}

int32_t AudioEndpointInner::WriteToSpecialProcBuf(const std::shared_ptr<OHAudioBuffer> &procBuf,
    const BufferDesc &readBuf)
{
    CHECK_AND_RETURN_RET_LOG(procBuf != nullptr, ERR_INVALID_HANDLE, "%{public}s process buffer is null.", __func__);
    uint64_t curWritePos = procBuf->GetCurWriteFrame();
    SpanInfo *curWriteSpan = procBuf->GetSpanInfo(curWritePos);
    CHECK_AND_RETURN_RET_LOG(curWriteSpan != nullptr, ERR_INVALID_HANDLE,
        "%{public}s get write span info of procBuf fail.", __func__);

    AUDIO_DEBUG_LOG("%{public}s process buffer write start, curWritePos %{public}" PRIu64".", __func__, curWritePos);
    curWriteSpan->spanStatus.store(SpanStatus::SPAN_WRITTING);
    curWriteSpan->writeStartTime = ClockTime::GetCurNano();

    BufferDesc writeBuf;
    int32_t ret = procBuf->GetWriteBuffer(curWritePos, writeBuf);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "%{public}s get write buffer fail, ret %{public}d.", __func__, ret);
    ret = memcpy_s(static_cast<void *>(writeBuf.buffer), writeBuf.bufLength,
        static_cast<void *>(readBuf.buffer), readBuf.bufLength);
    CHECK_AND_RETURN_RET_LOG(ret == EOK, ERR_WRITE_FAILED, "%{public}s memcpy data to process buffer fail, "
        "curWritePos %{public}" PRIu64", ret %{public}d.", __func__, curWritePos, ret);

    curWriteSpan->writeDoneTime = ClockTime::GetCurNano();
    procBuf->SetHandleInfo(curWritePos, curWriteSpan->writeDoneTime);
    ret = procBuf->SetCurWriteFrame(curWritePos + dstSpanSizeInframe_);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "%{public}s set procBuf next write frame fail, ret %{public}d.",
        __func__, ret);
    curWriteSpan->spanStatus.store(SpanStatus::SPAN_WRITE_DONE);
    return SUCCESS;
}

void AudioEndpointInner::WriteToProcessBuffers(const BufferDesc &readBuf)
{
    std::lock_guard<std::mutex> lock(listLock_);
    for (size_t i = 0; i < processBufferList_.size(); i++) {
        if (processBufferList_[i] == nullptr) {
            AUDIO_ERR_LOG("%{public}s process buffer %{public}zu is null.", __func__, i);
            continue;
        }
        if (processBufferList_[i]->GetStreamStatus()->load() != STREAM_RUNNING) {
            AUDIO_WARNING_LOG("%{public}s process buffer %{public}zu not running, stream status %{public}d.",
                __func__, i, processBufferList_[i]->GetStreamStatus()->load());
            continue;
        }

        int32_t ret = WriteToSpecialProcBuf(processBufferList_[i], readBuf);
        if (ret != SUCCESS) {
            AUDIO_ERR_LOG("%{public}s endpoint write to process buffer %{public}zu fail, ret %{public}d.",
                __func__, i, ret);
            continue;
        }
        AUDIO_DEBUG_LOG("%{public}s endpoint process buffer %{public}zu write success.", __func__, i);
    }
}

int32_t AudioEndpointInner::ReadFromEndpoint(uint64_t curReadPos)
{
    AUDIO_DEBUG_LOG("%{public}s enter, dstAudioBuffer curReadPos %{public}" PRIu64".", __func__, curReadPos);
    CHECK_AND_RETURN_RET_LOG(dstAudioBuffer_ != nullptr, ERR_INVALID_HANDLE,
        "%{public}s dst audio buffer is null.", __func__);
    SpanInfo *curReadSpan = dstAudioBuffer_->GetSpanInfo(curReadPos);
    CHECK_AND_RETURN_RET_LOG(curReadSpan != nullptr, ERR_INVALID_HANDLE,
        "%{public}s get source read span info of source adapter fail.", __func__);
    curReadSpan->readStartTime = ClockTime::GetCurNano();
    curReadSpan->spanStatus.store(SpanStatus::SPAN_READING);
    BufferDesc readBuf;
    int32_t ret = dstAudioBuffer_->GetReadbuffer(curReadPos, readBuf);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "%{public}s get read buffer fail, ret %{public}d.", __func__, ret);
#ifdef DUMP_PROCESS_FILE
    if (dump_hdi_ != nullptr) {
        fwrite(static_cast<void *>(readBuf.buffer), 1, readBuf.bufLength, dump_hdi_);
    }
#endif

    WriteToProcessBuffers(readBuf);
    curReadSpan->readDoneTime = ClockTime::GetCurNano();
    curReadSpan->spanStatus.store(SpanStatus::SPAN_READ_DONE);
    return SUCCESS;
}

void AudioEndpointInner::RecordEndpointWorkLoopFuc()
{
    int64_t curTime = 0;
    uint64_t curReadPos = 0;
    int64_t wakeUpTime = ClockTime::GetCurNano();
    AUDIO_INFO_LOG("Record endpoint work loop fuc start.");
    while (!isThreadEnd_) {
        if (!KeepWorkloopRunning()) {
            continue;
        }
        threadStatus_ = INRUNNING;
        if (needReSyncPosition_) {
            RecordReSyncPosition();
            wakeUpTime = ClockTime::GetCurNano();
            needReSyncPosition_ = false;
            continue;
        }
        curTime = ClockTime::GetCurNano();
        Trace loopTrace("Record_loop_trace");
        if (curTime - wakeUpTime > ONE_MILLISECOND_DURATION) {
            AUDIO_WARNING_LOG("%{public}s Wake up too late!", __func__);
        }

        curReadPos = dstAudioBuffer_->GetCurReadFrame();
        if (ReadFromEndpoint(curReadPos) != SUCCESS) {
            AUDIO_ERR_LOG("%{public}s read from endpoint to process service fail.", __func__);
            break;
        }

        if (!RecordPrepareNextLoop(curReadPos, wakeUpTime)) {
            AUDIO_ERR_LOG("PrepareNextLoop failed!");
            break;
        }

        loopTrace.End();
        threadStatus_ = SLEEPING;
        ClockTime::AbsoluteSleep(wakeUpTime);
    }
    AUDIO_INFO_LOG("Record endpoint work loop fuc end");
}

void AudioEndpointInner::EndpointWorkLoopFuc()
{
    int64_t curTime = 0;
    uint64_t curWritePos = 0;
    int64_t wakeUpTime = ClockTime::GetCurNano();
    AUDIO_INFO_LOG("Endpoint work loop fuc start");
    int32_t ret = 0;
    while (!isThreadEnd_) {
        if (!KeepWorkloopRunning()) {
            continue;
        }
        ret = 0;
        threadStatus_ = INRUNNING;
        curTime = ClockTime::GetCurNano();
        Trace loopTrace("AudioEndpoint::loop_trace");
        if (needReSyncPosition_) {
            ReSyncPosition();
            wakeUpTime = curTime;
            needReSyncPosition_ = false;
            continue;
        }
        if (curTime - wakeUpTime > ONE_MILLISECOND_DURATION) {
            AUDIO_WARNING_LOG("Wake up too late!");
        }

        // First, wake up at client may-write-done time, and check if all process write done.
        // If not, do another sleep to the possible latest write time.
        curWritePos = dstAudioBuffer_->GetCurWriteFrame();
        if (!CheckAllBufferReady(wakeUpTime, curWritePos)) {
            curTime = ClockTime::GetCurNano();
        }

        // then do mix & write to hdi buffer and prepare next loop
        if (!ProcessToEndpointDataHandle(curWritePos)) {
            AUDIO_ERR_LOG("ProcessToEndpointDataHandle failed!");
            break;
        }

        // prepare info of next loop
        if (!PrepareNextLoop(curWritePos, wakeUpTime)) {
            AUDIO_ERR_LOG("PrepareNextLoop failed!");
            break;
        }

        loopTrace.End();
        // start sleep
        threadStatus_ = SLEEPING;
        ClockTime::AbsoluteSleep(wakeUpTime);
    }
    AUDIO_INFO_LOG("Endpoint work loop fuc end, ret %{public}d", ret);
}
} // namespace AudioStandard
} // namespace OHOS
