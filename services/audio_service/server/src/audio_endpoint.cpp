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
#include <condition_variable>
#include <thread>
#include <vector>
#include <mutex>
#include <inttypes.h>

#include "audio_errors.h"
#include "audio_log.h"
#include "securec.h"

#include "audio_utils.h"
#include "fast_audio_renderer_sink.h"
#include "linear_pos_time_model.h"

// #define DUMP_PROCESS_FILE // open to enable dump file
namespace OHOS {
namespace AudioStandard {
namespace {
    static constexpr int64_t MAX_SPAN_DURATION_IN_NANO = 100000000; // 100ms
    static constexpr int32_t SLEEP_TIME_IN_DEFAULT = 400; // 400ms
    static constexpr int64_t DELTA_TO_REAL_READ_START_TIME = 4000000; // 4ms
}
class AudioEndpointInner : public AudioEndpoint {
public:
    AudioEndpointInner(EndpointType type);
    ~AudioEndpointInner();

    bool Config(AudioStreamInfo streamInfo);
    int32_t PrepareDeviceBuffer();

    bool StartDevice();

    // when audio process start.
    int32_t OnStart(IAudioProcessStream *processStream) override;
    // when audio process pause.
    int32_t OnPause(IAudioProcessStream *processStream) override;

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
private:
    static constexpr int64_t ONE_MILLISECOND_DURATION = 1000000; // 1ms
    static constexpr int64_t WRITE_TO_HDI_AHEAD_TIME = -1000000; // ahead 1ms
    enum ThreadStatus : uint32_t {
        WAITTING = 0,
        SLEEPING,
        INRUNNING
    };
    // SamplingRate EncodingType SampleFormat Channel
    AudioStreamInfo dstStreamInfo_;
    EndpointType endpointType_;

    std::mutex listLock_;
    std::vector<IAudioProcessStream *> processList_;
    std::vector<std::shared_ptr<OHAudioBuffer>> processBufferList_;

    FastAudioRendererSink *fastSink_ = nullptr;

    LinearPosTimeModel readTimeModel_;

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

    bool isDeviceRunningInIdel_ = false;
    bool needReSyncPosition_ = true;
    void ReSyncPosition();

    void InitAudiobuffer(bool resetReadWritePos);
    void ProcessData(const std::vector<AudioStreamData> &srcDataList, const AudioStreamData &dstData);
    int64_t GetPredictNextReadTime(uint64_t posInFrame);
    bool PrepareNextLoop(uint64_t curWritePos, int64_t &wakeUpTime);

    /**
     * @brief Get the current read position in frame and the read-time with it.
     *
     * @param frames the read position in frame
     * @param nanoTime the time in nanosecond when device-sink start read the buffer
    */
    bool GetDeviceHandleInfo(uint64_t &frames, int64_t &nanoTime);

    bool IsAnyProcessRunning();
    bool CheckAllBufferReady(int64_t checkTime);

    std::string GetStatusStr(EndpointStatus status);

    bool KeepWorkloopRunning();
    void EndpointWorkLoopFuc();
#ifdef DUMP_PROCESS_FILE
    FILE *dcp_ = nullptr;
    FILE *dump_hdi_ = nullptr;
#endif
};

std::shared_ptr<AudioEndpoint> AudioEndpoint::GetInstance(EndpointType type, AudioStreamInfo streamInfo)
{
    std::shared_ptr<AudioEndpointInner> audioEndpoint = std::make_shared<AudioEndpointInner>(type);
    CHECK_AND_RETURN_RET_LOG(audioEndpoint != nullptr, nullptr, "Create AudioEndpoint failed.");

    if (!audioEndpoint->Config(streamInfo)) {
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

    endpointStatus_.store(INVALID);

    if (dstAudioBuffer_ != nullptr) {
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

bool AudioEndpointInner::Config(AudioStreamInfo streamInfo)
{
    dstStreamInfo_ = streamInfo;
    fastSink_ = FastAudioRendererSink::GetInstance();
    IAudioSinkAttr attr = {};
    attr.adapterName = "primary";
    attr.sampleRate = dstStreamInfo_.samplingRate; // 48000hz
    attr.channel = dstStreamInfo_.channels; // STEREO = 2
    attr.format = dstStreamInfo_.format; // SAMPLE_S16LE = 1

    fastSink_->Init(attr);
    if (!fastSink_->IsInited()) {
        fastSink_ = nullptr;
        return false;
    }
    if (PrepareDeviceBuffer() != SUCCESS) {
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
    dcp_ = fopen("/data/local/tmp/server-read-client.pcm", "a+");
    dump_hdi_ = fopen("/data/local/tmp/server-hdi.pcm", "a+");
    if (dcp_ == nullptr || dump_hdi_ == nullptr) {
        AUDIO_ERR_LOG("Error opening pcm test file!");
    }
#endif
    return true;
}

int32_t AudioEndpointInner::PrepareDeviceBuffer()
{
    if (dstAudioBuffer_ != nullptr) {
        AUDIO_INFO_LOG("Endpoint buffer is preapred, fd:%{public}d", dstBufferFd_);
        return SUCCESS;
    }

    int32_t ret = fastSink_->GetMmapBufferInfo(dstBufferFd_, dstTotalSizeInframe_, dstSpanSizeInframe_,
        dstByteSizePerFrame_);
    if (ret != SUCCESS || dstBufferFd_ == -1 || dstTotalSizeInframe_ == 0 || dstSpanSizeInframe_ == 0 ||
        dstByteSizePerFrame_ == 0) {
        AUDIO_ERR_LOG("GetMmapBufferInfo failed.");
        return ERR_ILLEGAL_STATE;
    }

    // spanDuration_ may be less than the correct time of dstSpanSizeInframe_.
    spanDuration_ = dstSpanSizeInframe_ * AUDIO_NS_PER_SECOND / dstStreamInfo_.samplingRate;
    int64_t temp = spanDuration_ / 4 * 3; // 3/4 spanDuration
    serverAheadReadTime_ = temp < ONE_MILLISECOND_DURATION ? ONE_MILLISECOND_DURATION : temp; // at least 1ms ahead.
    AUDIO_INFO_LOG("Span duration: %{public}" PRId64"ns, server will wake up and handle %{public}" PRId64"ns ahead.",
        spanDuration_, serverAheadReadTime_);

    if (spanDuration_ <= 0 || spanDuration_ >= MAX_SPAN_DURATION_IN_NANO) {
        AUDIO_ERR_LOG("GetMmapBufferInfo error: invalid span info.");
        return ERR_INVALID_PARAM;
    }
    dstAudioBuffer_ = OHAudioBuffer::CreateFromRemote(dstTotalSizeInframe_, dstSpanSizeInframe_, dstByteSizePerFrame_,
        dstBufferFd_);
    if (dstAudioBuffer_ == nullptr || dstAudioBuffer_->GetBufferHolder() != AudioBufferHolder::AUDIO_SERVER_ONLY) {
        AUDIO_ERR_LOG("GetMmapBufferInfo failed.");
        return ERR_ILLEGAL_STATE;
    }
    dstAudioBuffer_->GetStreamStatus()->store(StreamStatus::STREAM_IDEL);

    // clear data buffer
    ret = memset_s(dstAudioBuffer_->GetDataBase(), dstAudioBuffer_->GetDataSize(), 0, dstAudioBuffer_->GetDataSize());
    AUDIO_INFO_LOG("PrepareDeviceBuffer and clear buffer[fd:%{public}d] success, ret:%{public}d", dstBufferFd_, ret);
    InitAudiobuffer(true);

    return SUCCESS;
}

void AudioEndpointInner::InitAudiobuffer(bool resetReadWritePos)
{
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
        spanInfo->spanStatus = SPAN_READ_DONE;
        spanInfo->offsetInFrame = 0;

        spanInfo->readStartTime = 0;
        spanInfo->readDoneTime = 0;

        spanInfo->writeStartTime = 0;
        spanInfo->writeDoneTime = 0;

        spanInfo->volumeStart = 1 << 16; // 65536 for initialize
        spanInfo->volumeEnd = 1 << 16; // 65536 for initialize
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
    AUDIO_INFO_LOG("StartDevice in.");
    // how to modify the status while unlinked and started?
    if (endpointStatus_ != IDEL) {
        AUDIO_ERR_LOG("Endpoint status is not IDEL");
        return false;
    }
    endpointStatus_ = STARTING;
    if (fastSink_->Start() != SUCCESS) {
        AUDIO_ERR_LOG("Sink start failed.");
        return false;
    }
    std::unique_lock<std::mutex> lock(loopThreadLock_);
    needReSyncPosition_ = true;
    endpointStatus_ = IsAnyProcessRunning() ? RUNNING : IDEL;
    workThreadCV_.notify_all();
    AUDIO_INFO_LOG("StartDevice out, status is %{public}s", GetStatusStr(endpointStatus_).c_str());
    return true;
}

int32_t AudioEndpointInner::OnStart(IAudioProcessStream *stream)
{
    if (endpointStatus_ == IDEL && !isDeviceRunningInIdel_) {
        // call sink start
        StartDevice();
        endpointStatus_ = RUNNING;
    }
    return SUCCESS;
}

int32_t AudioEndpointInner::OnPause(IAudioProcessStream *stream)
{
    // todo
    return SUCCESS;
}

int32_t AudioEndpointInner::LinkProcessStream(IAudioProcessStream *processStream)
{
    CHECK_AND_RETURN_RET_LOG(processStream != nullptr, ERR_INVALID_PARAM, "IAudioProcessStream is null");
    std::shared_ptr<OHAudioBuffer> processBuffer = processStream->GetStreamBuffer();
    CHECK_AND_RETURN_RET_LOG(processBuffer != nullptr, ERR_INVALID_PARAM, "processBuffer is null");

    if (processList_.size() >= MAX_LINKED_PROCESS) {
        AUDIO_ERR_LOG("LinkProcessStream failed, too many process.");
        return ERR_OPERATION_FAILED;
    }

    AUDIO_INFO_LOG("LinkProcessStream success in status:%{public}s.", GetStatusStr(endpointStatus_).c_str());

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
            AUDIO_INFO_LOG("LinkProcessStream success.");
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
    CHECK_AND_RETURN_RET_LOG(processStream != nullptr, ERR_INVALID_PARAM, "IAudioProcessStream is null");
    std::shared_ptr<OHAudioBuffer> processBuffer = processStream->GetStreamBuffer();
    CHECK_AND_RETURN_RET_LOG(processBuffer != nullptr, ERR_INVALID_PARAM, "processBuffer is null");

    // todo
    bool isFind = false;
    std::lock_guard<std::mutex> lock(listLock_);
    auto processItr = processList_.begin();
    auto bufferItr = processBufferList_.begin();
    while (processItr != processList_.end()) {
        if (*processItr == processStream && *bufferItr == processBuffer) {
            processItr = processList_.erase(processItr);
            bufferItr = processBufferList_.erase(bufferItr);
            isFind = true;
            break;
        } else {
            processItr++;
            bufferItr++;
        }
    }

    AUDIO_INFO_LOG("UnlinkProcessStream end, %{public}s the process.", (isFind ? "find and remove" : "not find"));
    return SUCCESS;
}

bool AudioEndpointInner::CheckAllBufferReady(int64_t checkTime)
{
    bool isAllReady = true;
    std::lock_guard<std::mutex> lock(listLock_);
    for (size_t i = 0; i < processBufferList_.size(); i++) {
        std::shared_ptr<OHAudioBuffer> tempBuffer = processBufferList_[i];
        uint64_t eachCurReadPos = processBufferList_[i]->GetCurReadFrame();
        processBufferList_[i]->SetHandleInfo(eachCurReadPos, checkTime); // update info for each process
        if (tempBuffer->GetStreamStatus()->load() != StreamStatus::STREAM_RUNNING) {
            continue; // process not running
        }
        uint64_t curRead = tempBuffer->GetCurReadFrame();
        SpanInfo *curReadSpan = tempBuffer->GetSpanInfo(curRead);
        if (curReadSpan == nullptr || curReadSpan->spanStatus != SpanStatus::SPAN_WRITE_DONE) {
            AUDIO_DEBUG_LOG("Find one process not ready"); // print uid of the process?
            isAllReady = false;
            break;
        }
    }
    return isAllReady;
}

void AudioEndpointInner::ProcessData(const std::vector<AudioStreamData> &srcDataList, const AudioStreamData &dstData)
{
    Trace trace("ProcessData");
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
    int16_t *dstPtr = (int16_t *)(dstData.bufferDesc.buffer);
    for (size_t offset = 0; dataLength > 0; dataLength--) {
        int32_t sum = 0;
        for (size_t i = 0; i < srcListSize; i++) {
            int32_t vol = srcDataList[i].volumeStart; // change to modify volume of each channel
            int16_t *srcPtr = (int16_t *)(srcDataList[i].bufferDesc.buffer) + offset;
            sum += (*srcPtr * (int64_t)vol) >> 16; // 1/65536
        }
        offset++;
        *dstPtr++ = sum > INT16_MAX ? INT16_MAX : (sum < INT16_MIN ? INT16_MIN : sum);
    }
}

int64_t AudioEndpointInner::GetPredictNextReadTime(uint64_t posInFrame)
{
    Trace trace("GetPredictNextRead");
    int64_t nextHdiReadTime = readTimeModel_.GetTimeOfPos(posInFrame);
    uint64_t handleSpanCout = posInFrame / dstSpanSizeInframe_;
    uint32_t startPeriodCount = 20; // sync each time when start
    uint32_t oneBigPeriodCount = 40; // 200ms
    if (handleSpanCout < startPeriodCount || handleSpanCout % oneBigPeriodCount == 0) {
        // todo sleep random little time but less than nextHdiReadTime - 2ms
        uint64_t readFrame = 0;
        int64_t readtime = 0;
        if (GetDeviceHandleInfo(readFrame, readtime)) {
            readTimeModel_.UpdataFrameStamp(readFrame, readtime);
        }
        nextHdiReadTime = readTimeModel_.GetTimeOfPos(posInFrame);
    }

    return nextHdiReadTime;
}

bool AudioEndpointInner::PrepareNextLoop(uint64_t curWritePos, int64_t &wakeUpTime)
{
    Trace prepareTrace("PrepareNextLoop");
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
        if (tempSpan->spanStatus.load() == SpanStatus::SPAN_READING) {
            tempSpan->spanStatus.store(SpanStatus::SPAN_READ_DONE);
            tempSpan->readDoneTime = curReadDoneTime;
            BufferDesc bufferReadDone = { nullptr, 0, 0};
            processBufferList_[i]->GetReadbuffer(eachCurReadPos, bufferReadDone);
            if (bufferReadDone.buffer != nullptr && bufferReadDone.bufLength != 0) {
                memset_s(bufferReadDone.buffer, bufferReadDone.bufLength, 0, bufferReadDone.bufLength);
            }
            processBufferList_[i]->SetCurReadFrame(eachCurReadPos + dstSpanSizeInframe_); // use client span size
        }
    }
    return true;
}

bool AudioEndpointInner::GetDeviceHandleInfo(uint64_t &frames, int64_t &nanoTime)
{
    Trace trace("GetDeviceHandleInfo");
    if (fastSink_ == nullptr || !fastSink_->IsInited()) {
        AUDIO_INFO_LOG("GetDeviceHandleInfo failed: sink is not inited.");
        return false;
    }
    int64_t timeSec = 0;
    int64_t timeNanoSec = 0;
    // GetMmapHandlePosition will call using ipc.
    fastSink_->GetMmapHandlePosition(frames, timeSec, timeNanoSec);
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

void AudioEndpointInner::EndpointWorkLoopFuc()
{
    int64_t curTime = 0;
    uint64_t curWritePos = 0;
    // uint64_t curReadPos = 0;
    // uint64_t nextHandlePos = 0;
    int64_t wakeUpTime = ClockTime::GetCurNano();
    AUDIO_INFO_LOG("EndpointWorkLoopFuc start");
    int32_t ret = 0;
    while (!isThreadEnd_) {
        if (!KeepWorkloopRunning()) {
            continue;
        }
        ret = 0;
        threadStatus_ = INRUNNING;
        curTime = ClockTime::GetCurNano();
        Trace loopTrace("loop_trace");
        if (needReSyncPosition_) {
            ReSyncPosition();
            wakeUpTime = curTime;
            needReSyncPosition_ = false;
            continue;
        }
        if (curTime - wakeUpTime > ONE_MILLISECOND_DURATION) {
            AUDIO_WARNING_LOG("Wake up too late!");
        }

        // get current server write position
        curWritePos = dstAudioBuffer_->GetCurWriteFrame();
        SpanInfo *curWriteSpan = dstAudioBuffer_->GetSpanInfo(curWritePos);
        if (curWriteSpan == nullptr) {
            AUDIO_ERR_LOG("GetSpanInfo failed, can not get curWriteSpan");
            break;
        }

        /**
         * first, wake up at client may-write-done time, and check all process write status
         * if not all process write done, do another sleep to server-buffer latest write time
         * then do mix & write to hdi buffer and prepare next loop
         */
        bool isAllBufferReady = CheckAllBufferReady(wakeUpTime);
        if (!isAllBufferReady) {
            Trace trace("WaitAllReady");
            int64_t tempWakeupTime = readTimeModel_.GetTimeOfPos(curWritePos) + WRITE_TO_HDI_AHEAD_TIME;
            if (tempWakeupTime - curTime < ONE_MILLISECOND_DURATION) {
                ClockTime::RelativeSleep(ONE_MILLISECOND_DURATION);
            } else {
                ClockTime::AbsoluteSleep(tempWakeupTime); // sleep to hdi read time ahead 1ms.
            }
            curTime = ClockTime::GetCurNano();
        }

        std::vector<AudioStreamData> audioDataList;
        {
            std::lock_guard<std::mutex> lock(listLock_);
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
                if (curReadSpan->spanStatus.load() == SpanStatus::SPAN_WRITE_DONE) {
                    processBufferList_[i]->GetReadbuffer(curRead, streamData.bufferDesc); // check return?
                    audioDataList.push_back(streamData);
#ifdef DUMP_PROCESS_FILE
                    if (dcp_ != nullptr) {
                        fwrite((void *)streamData.bufferDesc.buffer, 1, streamData.bufferDesc.bufLength, dcp_);
                    }
#endif
                    curReadSpan->spanStatus.store(SpanStatus::SPAN_READING);
                    curReadSpan->readStartTime = curTime;
                }
            }
        }

        AudioStreamData dstStreamData;
        dstStreamData.streamInfo = dstStreamInfo_;
        dstStreamData.volumeStart = curWriteSpan->volumeStart;
        dstStreamData.volumeEnd = curWriteSpan->volumeEnd;
        ret = dstAudioBuffer_->GetWriteBuffer(curWritePos, dstStreamData.bufferDesc);
        if (ret != SUCCESS || dstStreamData.bufferDesc.buffer == nullptr) {
            AUDIO_ERR_LOG("GetWriteBuffer failed, ret:%{public}d", ret);
            break;
        }
        curWriteSpan->writeStartTime = curTime;
        // do write work
        // call data process module
        if (audioDataList.size() == 0) {
            memset_s(dstStreamData.bufferDesc.buffer, dstStreamData.bufferDesc.bufLength, 0,
                dstStreamData.bufferDesc.bufLength);
        } else {
            ProcessData(audioDataList, dstStreamData); // what if process call unlink and remove buffer?
        }

#ifdef DUMP_PROCESS_FILE
        if (dump_hdi_ != nullptr) {
            fwrite((void *)dstStreamData.bufferDesc.buffer, 1, dstStreamData.bufferDesc.bufLength, dump_hdi_);
        }
#endif
        curTime = ClockTime::GetCurNano();
        curWriteSpan->writeDoneTime = curTime; // debug server time cost

        // prepare info of next loop
        if(!PrepareNextLoop(curWritePos, wakeUpTime)) {
            AUDIO_ERR_LOG("PrepareNextLoop failed!");
            break;
        }

        loopTrace.End();
        // start sleep
        threadStatus_ = SLEEPING;
        ClockTime::AbsoluteSleep(wakeUpTime);
    }
    AUDIO_INFO_LOG("EndpointWorkLoopFuc end");
}
} // namespace AudioStandard
} // namespace OHOS
