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

#include "remote_fast_audio_renderer_sink.h"

#include <cinttypes>
#include <dlfcn.h>
#include <map>
#include <sstream>
#include "securec.h"

#include "audio_errors.h"
#include "audio_log.h"
#include "audio_utils.h"
#include "audio_info.h"
#include "audio_manager.h"
#include "ashmem.h"
#include "i_audio_renderer_sink.h"
#include "i_audio_device_adapter.h"
#include "i_audio_device_manager.h"

namespace OHOS {
namespace AudioStandard {
namespace {
const int32_t HALF_FACTOR = 2;
const uint32_t AUDIO_CHANNELCOUNT = 2;
const uint32_t AUDIO_SAMPLE_RATE_48K = 48000;
const uint32_t DEEP_BUFFER_RENDER_PERIOD_SIZE = 3840;
const uint32_t INT_32_MAX = 0x7fffffff;
const uint32_t PCM_8_BIT = 8;
const uint32_t PCM_16_BIT = 16;
const uint32_t PCM_24_BIT = 24;
const uint32_t PCM_32_BIT = 32;
const uint32_t REMOTE_FAST_OUTPUT_STREAM_ID = 37; // 13 + 3 * 8
const int64_t SECOND_TO_NANOSECOND = 1000000000;
const int32_t INVALID_FD = -1;
}
class RemoteFastAudioRendererSinkInner : public RemoteFastAudioRendererSink, public IAudioDeviceAdapterCallback,
    public std::enable_shared_from_this<RemoteFastAudioRendererSinkInner> {
public:
    explicit RemoteFastAudioRendererSinkInner(const std::string &deviceNetworkId);
    ~RemoteFastAudioRendererSinkInner();

    int32_t Init(IAudioSinkAttr attr) override;
    bool IsInited(void) override;
    void DeInit(void) override;

    int32_t Start(void) override;
    int32_t Stop(void) override;
    int32_t Flush(void) override;
    int32_t Reset(void) override;
    int32_t Pause(void) override;
    int32_t Resume(void) override;

    int32_t RenderFrame(char &data, uint64_t len, uint64_t &writeLen) override;
    int32_t SetVolume(float left, float right) override;
    int32_t GetVolume(float &left, float &right) override;
    int32_t SetVoiceVolume(float volume) override;
    int32_t GetTransactionId(uint64_t *transactionId) override;
    int32_t GetLatency(uint32_t *latency) override;
    int32_t SetAudioScene(AudioScene audioScene, DeviceType activeDevice) override;
    int32_t SetOutputRoute(DeviceType deviceType) override;
    void SetAudioParameter(const AudioParamKey key, const std::string& condition, const std::string& value) override;
    std::string GetAudioParameter(const AudioParamKey key, const std::string& condition) override;
    void SetAudioMonoState(bool audioMono) override;
    void SetAudioBalanceValue(float audioBalance) override;
    void RegisterParameterCallback(IAudioSinkCallback* callback) override;

    int32_t GetMmapBufferInfo(int &fd, uint32_t &totalSizeInframe, uint32_t &spanSizeInframe,
        uint32_t &byteSizePerFrame) override;
    int32_t GetMmapHandlePosition(uint64_t &frames, int64_t &timeSec, int64_t &timeNanoSec) override;

    void OnAudioParamChange(const std::string &adapterName, const AudioParamKey key, const std::string &condition,
        const std::string &value) override;

    std::string GetNetworkId();
    IAudioSinkCallback* GetParamCallback();

private:
    int32_t CreateRender(const struct AudioPort &renderPort);
    void InitAttrs(struct AudioSampleAttributes &attrs);
    AudioFormat ConverToHdiFormat(AudioSampleFormat format);
    int32_t PrepareMmapBuffer();
    uint32_t PcmFormatToBits(AudioSampleFormat format);
    void ClearRender();

private:
    std::atomic<bool> rendererInited_ = false;
    std::atomic<bool> isRenderCreated_ = false;
    std::atomic<bool> started_ = false;
    std::atomic<bool> paused_ = false;
    float leftVolume_ = 0;
    float rightVolume_ = 0;
    std::shared_ptr<IAudioDeviceManager> audioManager_ = nullptr;
    std::shared_ptr<IAudioDeviceAdapter> audioAdapter_ = nullptr;
    IAudioSinkCallback *callback_ = nullptr;
    struct AudioRender *audioRender_ = nullptr;
    struct AudioPort audioPort_;
    IAudioSinkAttr attr_ = {};
    std::string deviceNetworkId_;

    uint32_t bufferTotalFrameSize_ = 0;
    int32_t bufferFd_ = INVALID_FD;
    uint32_t frameSizeInByte_ = 1;
    uint32_t eachReadFrameSize_ = 0;

#ifdef DEBUG_DIRECT_USE_HDI
    sptr<Ashmem> ashmemSink_ = nullptr;
    size_t bufferSize_ = 0;
#endif
};

std::map<std::string, RemoteFastAudioRendererSinkInner *> allRFSinks;
IMmapAudioRendererSink *RemoteFastAudioRendererSink::GetInstance(const std::string &deviceNetworkId)
{
    AUDIO_INFO_LOG("GetInstance.");
    RemoteFastAudioRendererSinkInner *audioRenderer = nullptr;
    // check if it is in our map
    if (allRFSinks.count(deviceNetworkId)) {
        return allRFSinks[deviceNetworkId];
    } else {
        audioRenderer = new(std::nothrow) RemoteFastAudioRendererSinkInner(deviceNetworkId);
        AUDIO_DEBUG_LOG("new Daudio device sink:[%{public}s]", deviceNetworkId.c_str());
        allRFSinks[deviceNetworkId] = audioRenderer;
    }
    CHECK_AND_RETURN_RET_LOG((audioRenderer != nullptr), nullptr, "null audioRenderer!");
    return audioRenderer;
}

RemoteFastAudioRendererSinkInner::RemoteFastAudioRendererSinkInner(const std::string &deviceNetworkId)
    : deviceNetworkId_(deviceNetworkId)
{
    AUDIO_INFO_LOG("RemoteFastAudioRendererSinkInner Constract.");
}

RemoteFastAudioRendererSinkInner::~RemoteFastAudioRendererSinkInner()
{
    if (rendererInited_.load()) {
        RemoteFastAudioRendererSinkInner::DeInit();
    }
    AUDIO_INFO_LOG("RemoteFastAudioRendererSink end.");
}

bool RemoteFastAudioRendererSinkInner::IsInited()
{
    return rendererInited_.load();
}

void RemoteFastAudioRendererSinkInner::ClearRender()
{
    AUDIO_INFO_LOG("Clear remote fast audio render enter.");
    rendererInited_.store(false);
    isRenderCreated_.store(false);
    started_.store(false);
    paused_.store(false);

#ifdef DEBUG_DIRECT_USE_HDI
    if (ashmemSink_ != nullptr) {
        ashmemSink_->UnmapAshmem();
        ashmemSink_->CloseAshmem();
        ashmemSink_ = nullptr;
        AUDIO_INFO_LOG("ClearRender: UnInit sink ashmem OK.");
    }
#endif // DEBUG_DIRECT_USE_HDI
    if (bufferFd_ != INVALID_FD) {
        close(bufferFd_);
        bufferFd_ = INVALID_FD;
    }

    if (audioAdapter_ != nullptr) {
        audioAdapter_->DestroyRender(audioRender_);
        audioAdapter_->Release();
    }
    audioRender_ = nullptr;
    audioAdapter_ = nullptr;

    if (audioManager_ != nullptr) {
        audioManager_->UnloadAdapter(attr_.adapterName);
    }
    audioManager_ = nullptr;

    AudioDeviceManagerFactory::GetInstance().DestoryDeviceManager(REMOTE_DEV_MGR);
    AUDIO_INFO_LOG("Clear remote audio render end.");
}

void RemoteFastAudioRendererSinkInner::DeInit()
{
    AUDIO_INFO_LOG("DeInit.");
    ClearRender();

    RemoteFastAudioRendererSinkInner *temp = allRFSinks[this->deviceNetworkId_];
    if (temp != nullptr) {
        delete temp;
        temp = nullptr;
        allRFSinks.erase(this->deviceNetworkId_);
    }
}

int32_t RemoteFastAudioRendererSinkInner::Init(IAudioSinkAttr attr)
{
    AUDIO_INFO_LOG("Init start.");
    attr_ = attr;
    audioManager_ = AudioDeviceManagerFactory::GetInstance().CreatDeviceManager(REMOTE_DEV_MGR);
    CHECK_AND_RETURN_RET_LOG(audioManager_ != nullptr, ERR_NOT_STARTED, "Init audio manager fail.");

    struct AudioAdapterDescriptor *desc = audioManager_->GetTargetAdapterDesc(attr_.adapterName, true);
    CHECK_AND_RETURN_RET_LOG(desc != nullptr, ERR_NOT_STARTED, "Get target adapters descriptor fail.");
    for (uint32_t port = 0; port < desc->portNum; port++) {
        if (desc->ports[port].portId == PIN_OUT_SPEAKER) {
            audioPort_ = desc->ports[port];
            break;
        }
        if (port == (desc->portNum - 1)) {
            AUDIO_ERR_LOG("Not found the audio spk port.");
            return ERR_INVALID_INDEX;
        }
    }

    // The two adapterName params are equal when getting remote fast sink attr from policy server
    attr_.adapterName = desc->adapterName;
    audioAdapter_ = audioManager_->LoadAdapters(attr_.adapterName, true);
    CHECK_AND_RETURN_RET_LOG(audioAdapter_ != nullptr, ERR_NOT_STARTED, "Load audio device adapter failed.");

    int32_t ret = audioAdapter_->Init();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Audio adapter init fail, ret %{publid}d.", ret);

    ret = CreateRender(audioPort_);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Create render fail, audio port %{public}d, ret %{public}d.",
        audioPort_.portId, ret);

    rendererInited_.store(true);
    AUDIO_DEBUG_LOG("RemoteFastAudioRendererSink: Init end.");
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::CreateRender(const struct AudioPort &renderPort)
{
    int64_t start = ClockTime::GetCurNano();

    struct AudioSampleAttributes param;
    InitAttrs(param);
    param.sampleRate = attr_.sampleRate;
    param.channelCount = attr_.channel;
    param.format = ConverToHdiFormat(attr_.format);
    param.frameSize = PCM_16_BIT * param.channelCount / PCM_8_BIT;
    param.startThreshold = DEEP_BUFFER_RENDER_PERIOD_SIZE / (param.frameSize);
    AUDIO_INFO_LOG("Create render format: %{public}d", param.format);

    struct AudioDeviceDescriptor deviceDesc;
    deviceDesc.portId = renderPort.portId;
    deviceDesc.pins = PIN_OUT_SPEAKER;
    deviceDesc.desc = nullptr;

    CHECK_AND_RETURN_RET_LOG(audioAdapter_ != nullptr, ERR_INVALID_HANDLE, "CreateRender: Audio adapter is null.");
    int32_t ret = audioAdapter_->CreateRender(&deviceDesc, &param, &audioRender_, shared_from_this());
    if (ret != SUCCESS || audioRender_ == nullptr) {
        AUDIO_ERR_LOG("AudioDeviceCreateRender failed");
        return ret;
    }
    if (param.type == AUDIO_MMAP_NOIRQ) {
        PrepareMmapBuffer();
    }
    isRenderCreated_.store(true);
    int64_t cost = (ClockTime::GetCurNano() - start) / AUDIO_US_PER_SECOND;
    AUDIO_DEBUG_LOG("CreateRender cost[%{public}" PRId64 "]ms", cost);
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::PrepareMmapBuffer()
{
    CHECK_AND_RETURN_RET_LOG(audioRender_ != nullptr, ERR_INVALID_HANDLE, "PrepareMmapBuffer: Audio render is null.");

    int32_t totalBifferInMs = 40; // 5 * (6 + 2 * (1)) = 40ms, the buffer size, not latency.
    frameSizeInByte_ = PcmFormatToBits(attr_.format) * attr_.channel / PCM_8_BIT;
    int32_t reqBufferFrameSize = totalBifferInMs * (attr_.sampleRate / 1000);

    struct AudioMmapBufferDescriptor desc = {0};
    int32_t ret = audioRender_->attr.ReqMmapBuffer((AudioHandle)audioRender_, reqBufferFrameSize, &desc);
    CHECK_AND_RETURN_RET_LOG((ret == SUCCESS), ERR_OPERATION_FAILED,
        "PrepareMmapBuffer require mmap buffer failed, ret:%{public}d.", ret);

    AUDIO_INFO_LOG("AudioMmapBufferDescriptor memoryAddress[%{private}p] memoryFd[%{public}d] totalBufferFrames"
        "[%{public}d] transferFrameSize[%{public}d] isShareable[%{public}d] offset[%{public}d]", desc.memoryAddress,
        desc.memoryFd, desc.totalBufferFrames, desc.transferFrameSize, desc.isShareable, desc.offset);

    bufferFd_ = desc.memoryFd; // fcntl(fd, 1030,3) after dup?
    int32_t periodFrameMaxSize = 1920000; // 192khz * 10s
    if (desc.totalBufferFrames < 0 || desc.transferFrameSize < 0 || desc.transferFrameSize > periodFrameMaxSize) {
        AUDIO_ERR_LOG("ReqMmapBuffer invalid values: totalBufferFrames[%{public}d] transferFrameSize[%{public}d]",
            desc.totalBufferFrames, desc.transferFrameSize);
        return ERR_OPERATION_FAILED;
    }
    bufferTotalFrameSize_ = desc.totalBufferFrames; // 1440 ~ 3840
    eachReadFrameSize_ = desc.transferFrameSize; // 240

    if (frameSizeInByte_ > ULLONG_MAX / bufferTotalFrameSize_) {
        AUDIO_ERR_LOG("BufferSize will overflow!");
        return ERR_OPERATION_FAILED;
    }

#ifdef DEBUG_DIRECT_USE_HDI
    bufferSize_ = bufferTotalFrameSize_ * frameSizeInByte_;
    ashmemSink_ = new Ashmem(bufferFd_, bufferSize_);
    AUDIO_INFO_LOG("PrepareMmapBuffer create ashmem sink OK, ashmemLen %{public}zu.", bufferSize_);
    if (!(ashmemSink_->MapReadAndWriteAshmem())) {
        AUDIO_ERR_LOG("PrepareMmapBuffer map ashmem sink failed.");
        return ERR_OPERATION_FAILED;
    }
#endif // DEBUG_DIRECT_USE_HDI
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::GetMmapBufferInfo(int &fd, uint32_t &totalSizeInframe,
    uint32_t &spanSizeInframe, uint32_t &byteSizePerFrame)
{
    if (bufferFd_ == INVALID_FD) {
        AUDIO_ERR_LOG("buffer fd has been released!");
        return ERR_INVALID_HANDLE;
    }
    fd = bufferFd_;
    totalSizeInframe = bufferTotalFrameSize_;
    spanSizeInframe = eachReadFrameSize_;
    byteSizePerFrame = PcmFormatToBits(attr_.format) * attr_.channel / PCM_8_BIT;
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::GetMmapHandlePosition(uint64_t &frames, int64_t &timeSec,
    int64_t &timeNanoSec)
{
    CHECK_AND_RETURN_RET_LOG(audioRender_ != nullptr, ERR_INVALID_HANDLE,
        "GetMmapHandlePosition: Audio render is null.");

    struct AudioTimeStamp timestamp = {};
    int32_t ret = audioRender_->attr.GetMmapPosition((AudioHandle)audioRender_, &frames, &timestamp);
    if (ret != 0) {
        AUDIO_ERR_LOG("Hdi GetMmapPosition filed, ret:%{public}d!", ret);
        return ERR_OPERATION_FAILED;
    }

    int64_t maxSec = 9223372036; // (9223372036 + 1) * 10^9 > INT64_MAX, seconds should not bigger than it.
    if (timestamp.tvSec < 0 || timestamp.tvSec > maxSec || timestamp.tvNSec < 0 ||
        timestamp.tvNSec > SECOND_TO_NANOSECOND) {
        AUDIO_ERR_LOG("Hdi GetMmapPosition get invaild second:%{public}" PRId64 " or nanosecond:%{public}" PRId64 " !",
            timestamp.tvSec, timestamp.tvNSec);
        return ERR_OPERATION_FAILED;
    }
    timeSec = timestamp.tvSec;
    timeNanoSec = timestamp.tvNSec;

    return SUCCESS;
}

uint32_t RemoteFastAudioRendererSinkInner::PcmFormatToBits(AudioSampleFormat format)
{
    switch (format) {
        case SAMPLE_U8:
            return PCM_8_BIT;
        case SAMPLE_S16LE:
            return PCM_16_BIT;
        case SAMPLE_S24LE:
            return PCM_24_BIT;
        case SAMPLE_S32LE:
            return PCM_32_BIT;
        case SAMPLE_F32LE:
            return PCM_32_BIT;
        default:
            return PCM_24_BIT;
    }
}

AudioFormat RemoteFastAudioRendererSinkInner::ConverToHdiFormat(AudioSampleFormat format)
{
    AudioFormat hdiFormat;
    switch (format) {
        case SAMPLE_U8:
            hdiFormat = AUDIO_FORMAT_TYPE_PCM_8_BIT;
            break;
        case SAMPLE_S16LE:
            hdiFormat = AUDIO_FORMAT_TYPE_PCM_16_BIT;
            break;
        case SAMPLE_S24LE:
            hdiFormat = AUDIO_FORMAT_TYPE_PCM_24_BIT;
            break;
        case SAMPLE_S32LE:
            hdiFormat = AUDIO_FORMAT_TYPE_PCM_32_BIT;
            break;
        default:
            hdiFormat = AUDIO_FORMAT_TYPE_PCM_16_BIT;
            break;
    }

    return hdiFormat;
}

void RemoteFastAudioRendererSinkInner::InitAttrs(struct AudioSampleAttributes &attrs)
{
    /* Initialization of audio parameters for playback */
    attrs.channelCount = AUDIO_CHANNELCOUNT;
    attrs.sampleRate = AUDIO_SAMPLE_RATE_48K;
    attrs.interleaved = 0;
    attrs.streamId = REMOTE_FAST_OUTPUT_STREAM_ID;
    attrs.type = AUDIO_MMAP_NOIRQ;
    attrs.period = DEEP_BUFFER_RENDER_PERIOD_SIZE;
    attrs.isBigEndian = false;
    attrs.isSignedData = true;
    attrs.stopThreshold = INT_32_MAX;
    attrs.silenceThreshold = 0;
}

inline std::string printRemoteAttr(IAudioSinkAttr attr_)
{
    std::stringstream value;
    value << "adapterName[" << attr_.adapterName << "] openMicSpeaker[" << attr_.openMicSpeaker << "] ";
    value << "format[" << static_cast<int32_t>(attr_.format) << "] sampleFmt[" << attr_.sampleFmt << "] ";
    value << "sampleRate[" << attr_.sampleRate << "] channel[" << attr_.channel << "] ";
    value << "volume[" << attr_.volume << "] filePath[" << attr_.filePath << "] ";
    value << "deviceNetworkId[" << attr_.deviceNetworkId << "] device_type[" << attr_.deviceType << "]";
    return value.str();
}

int32_t RemoteFastAudioRendererSinkInner::RenderFrame(char &data, uint64_t len, uint64_t &writeLen)
{
    AUDIO_DEBUG_LOG("RenderFrame is not supported.");
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::Start(void)
{
    AUDIO_INFO_LOG("Start.");
    if (!isRenderCreated_.load()) {
        CHECK_AND_RETURN_RET_LOG(CreateRender(audioPort_) == SUCCESS, ERR_NOT_STARTED,
            "Create render fail, audio port %{public}d", audioPort_.portId);
    }

    if (started_.load()) {
        AUDIO_INFO_LOG("Remote fast render is already started.");
        return SUCCESS;
    }

    CHECK_AND_RETURN_RET_LOG(audioRender_ != nullptr, ERR_INVALID_HANDLE, "Start: Audio render is null.");
    int32_t ret = audioRender_->control.Start(reinterpret_cast<AudioHandle>(audioRender_));
    CHECK_AND_RETURN_RET_LOG(ret == 0, ERR_NOT_STARTED, "Start fail, ret %{public}d.", ret);
    started_.store(true);
    AUDIO_INFO_LOG("Start Ok.");
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::Stop(void)
{
    AUDIO_INFO_LOG("Stop.");
    if (!started_.load()) {
        AUDIO_INFO_LOG("Remote fast render is already stopped.");
        return SUCCESS;
    }

    CHECK_AND_RETURN_RET_LOG(audioRender_ != nullptr, ERR_INVALID_HANDLE, "Stop: Audio render is null.");
    int32_t ret = audioRender_->control.Stop(reinterpret_cast<AudioHandle>(audioRender_));
    CHECK_AND_RETURN_RET_LOG(ret == 0, ERR_OPERATION_FAILED, "Stop fail, ret %{public}d.", ret);
    started_.store(false);
    AUDIO_DEBUG_LOG("Stop ok.");
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::Pause(void)
{
    AUDIO_INFO_LOG("Pause.");
    CHECK_AND_RETURN_RET_LOG(started_.load(), ERR_ILLEGAL_STATE, "Pause invalid state!");

    if (paused_.load()) {
        AUDIO_INFO_LOG("Remote fast render is already paused.");
        return SUCCESS;
    }

    CHECK_AND_RETURN_RET_LOG(audioRender_ != nullptr, ERR_INVALID_HANDLE, "Pause: Audio render is null.");
    int32_t ret = audioRender_->control.Pause(reinterpret_cast<AudioHandle>(audioRender_));
    CHECK_AND_RETURN_RET_LOG(ret == 0, ERR_OPERATION_FAILED, "Pause fail, ret %{public}d.", ret);
    paused_.store(true);
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::Resume(void)
{
    AUDIO_INFO_LOG("Pause.");
    CHECK_AND_RETURN_RET_LOG(started_.load(), ERR_ILLEGAL_STATE, "Resume invalid state!");

    if (!paused_.load()) {
        AUDIO_INFO_LOG("Remote fast render is already resumed.");
        return SUCCESS;
    }

    CHECK_AND_RETURN_RET_LOG(audioRender_ != nullptr, ERR_INVALID_HANDLE, "Resume: Audio render is null.");
    int32_t ret = audioRender_->control.Resume(reinterpret_cast<AudioHandle>(audioRender_));
    CHECK_AND_RETURN_RET_LOG(ret == 0, ERR_OPERATION_FAILED, "Resume fail, ret %{public}d.", ret);
    paused_.store(false);
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::Reset(void)
{
    AUDIO_INFO_LOG("Reset.");
    CHECK_AND_RETURN_RET_LOG(started_.load(), ERR_ILLEGAL_STATE, "Reset invalid state!");

    CHECK_AND_RETURN_RET_LOG(audioRender_ != nullptr, ERR_INVALID_HANDLE, "Reset: Audio render is null.");
    int32_t ret = audioRender_->control.Flush(reinterpret_cast<AudioHandle>(audioRender_));
    CHECK_AND_RETURN_RET_LOG(ret == 0, ERR_OPERATION_FAILED, "Reset fail, ret %{public}d.", ret);
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::Flush(void)
{
    AUDIO_INFO_LOG("Flush.");
    CHECK_AND_RETURN_RET_LOG(started_.load(), ERR_ILLEGAL_STATE, "Flush invalid state!");

    CHECK_AND_RETURN_RET_LOG(audioRender_ != nullptr, ERR_INVALID_HANDLE, "Flush: Audio render is null.");
    int32_t ret = audioRender_->control.Flush(reinterpret_cast<AudioHandle>(audioRender_));
    CHECK_AND_RETURN_RET_LOG(ret == 0, ERR_OPERATION_FAILED, "Flush fail, ret %{public}d.", ret);
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::SetVolume(float left, float right)
{
    CHECK_AND_RETURN_RET_LOG(audioRender_ != nullptr, ERR_INVALID_HANDLE, "SetVolume: Audio render is null.");

    float volume;
    leftVolume_ = left;
    rightVolume_ = right;
    if ((leftVolume_ == 0) && (rightVolume_ != 0)) {
        volume = rightVolume_;
    } else if ((leftVolume_ != 0) && (rightVolume_ == 0)) {
        volume = leftVolume_;
    } else {
        volume = (leftVolume_ + rightVolume_) / HALF_FACTOR;
    }

    int32_t ret = audioRender_->volume.SetVolume(reinterpret_cast<AudioHandle>(audioRender_), volume);
    if (ret) {
        AUDIO_ERR_LOG("Set volume failed!");
    }
    return ret;
}

int32_t RemoteFastAudioRendererSinkInner::GetVolume(float &left, float &right)
{
    left = leftVolume_;
    right = rightVolume_;
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::GetLatency(uint32_t *latency)
{
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("GetLatency failed audio render null");
        return ERR_INVALID_HANDLE;
    }

    if (!latency) {
        AUDIO_ERR_LOG("GetLatency failed latency null");
        return ERR_INVALID_PARAM;
    }

    uint32_t hdiLatency = 0;
    if (audioRender_->GetLatency(audioRender_, &hdiLatency) != 0) {
        AUDIO_ERR_LOG("GetLatency failed.");
        return ERR_OPERATION_FAILED;
    }

    *latency = hdiLatency;
    return SUCCESS;
}

void RemoteFastAudioRendererSinkInner::RegisterParameterCallback(IAudioSinkCallback* callback)
{
    AUDIO_INFO_LOG("register params callback");
    callback_ = callback;

#ifdef FEATURE_DISTRIBUTE_AUDIO
    CHECK_AND_RETURN_LOG(audioAdapter_ != nullptr, "RegisterParameterCallback: Audio adapter is null.");
    int32_t ret = audioAdapter_->RegExtraParamObserver();
    CHECK_AND_RETURN_LOG(ret == SUCCESS, "RegisterParameterCallback failed, ret %{public}d.", ret);
#endif
}

void RemoteFastAudioRendererSinkInner::OnAudioParamChange(const std::string &adapterName, const AudioParamKey key,
    const std::string& condition, const std::string& value)
{
    AUDIO_INFO_LOG("Audio param change event, key:%{public}d, condition:%{public}s, value:%{public}s",
        key, condition.c_str(), value.c_str());
    if (key == AudioParamKey::PARAM_KEY_STATE) {
        ClearRender();
    }

    CHECK_AND_RETURN_LOG(callback_ != nullptr, "Sink audio param callback is null.");
    callback_->OnAudioSinkParamChange(adapterName, key, condition, value);
}

int32_t RemoteFastAudioRendererSinkInner::GetTransactionId(uint64_t *transactionId)
{
    (void)transactionId;
    AUDIO_ERR_LOG("GetTransactionId not supported");
    return ERR_NOT_SUPPORTED;
}

int32_t RemoteFastAudioRendererSinkInner::SetVoiceVolume(float volume)
{
    (void)volume;
    AUDIO_ERR_LOG("SetVoiceVolume not supported");
    return ERR_NOT_SUPPORTED;
}

int32_t RemoteFastAudioRendererSinkInner::SetOutputRoute(DeviceType deviceType)
{
    (void)deviceType;
    AUDIO_ERR_LOG("SetOutputRoute not supported");
    return ERR_NOT_SUPPORTED;
}

void RemoteFastAudioRendererSinkInner::SetAudioMonoState(bool audioMono)
{
    (void)audioMono;
    AUDIO_ERR_LOG("SetAudioMonoState not supported");
    return;
}

void RemoteFastAudioRendererSinkInner::SetAudioBalanceValue(float audioBalance)
{
    (void)audioBalance;
    AUDIO_ERR_LOG("SetAudioBalanceValue not supported");
    return;
}

int32_t RemoteFastAudioRendererSinkInner::SetAudioScene(AudioScene audioScene, DeviceType activeDevice)
{
    AUDIO_INFO_LOG("SetAudioScene not supported");
    return SUCCESS;
}

void RemoteFastAudioRendererSinkInner::SetAudioParameter(const AudioParamKey key, const std::string& condition,
    const std::string& value)
{
    AUDIO_INFO_LOG("SetAudioParameter not support.");
}

std::string RemoteFastAudioRendererSinkInner::GetAudioParameter(const AudioParamKey key, const std::string& condition)
{
    AUDIO_INFO_LOG("GetAudioParameter not support.");
    return "";
}

std::string RemoteFastAudioRendererSinkInner::GetNetworkId()
{
    return deviceNetworkId_;
}

OHOS::AudioStandard::IAudioSinkCallback* RemoteFastAudioRendererSinkInner::GetParamCallback()
{
    return callback_;
}
} // namespace AudioStandard
} // namespace OHOS