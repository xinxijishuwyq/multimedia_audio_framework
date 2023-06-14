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
#include "fast_audio_renderer_sink.h"

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
const uint32_t INTERNAL_OUTPUT_STREAM_ID = 0;
const int64_t SECOND_TO_NANOSECOND = 1000000000;
const int32_t INVALID_FD = -1;
}
class RemoteFastAudioRendererSinkInner : public RemoteFastAudioRendererSink {
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

    std::string GetNetworkId();
    IAudioSinkCallback* GetParamCallback();

private:
    int32_t InitAudioManager();
    int32_t GetTargetAdapterPort(struct AudioAdapterDescriptor *descs, int32_t size, const char *networkId);
    int32_t CreateRender(const struct AudioPort &renderPort);
    void InitAttrs(struct AudioSampleAttributes &attrs);
    AudioFormat ConverToHdiFormat(AudioSampleFormat format);
    int32_t PrepareMmapBuffer();
    uint32_t PcmFormatToBits(AudioSampleFormat format);

private:
    std::atomic<bool> rendererInited_ = false;
    std::atomic<bool> isRenderCreated_ = false;

    std::atomic<bool> started_ = false;
    std::atomic<bool> paused_ = false;
    std::atomic<bool> paramCallbackRegistered_ = false;
    float leftVolume_ = 0;
    float rightVolume_ = 0;
    struct AudioManager *audioManager_ = nullptr;
    struct AudioAdapter *audioAdapter_ = nullptr;
    struct AudioRender *audioRender_ = nullptr;
    IAudioSinkCallback *callback_ = nullptr;
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

void RemoteFastAudioRendererSinkInner::DeInit()
{
    AUDIO_INFO_LOG("DeInit.");
    started_.store(false);
    rendererInited_.store(false);
#ifdef DEBUG_DIRECT_USE_HDI
    if (ashmemSink_ != nullptr) {
        ashmemSink_->UnmapAshmem();
        ashmemSink_->CloseAshmem();
        ashmemSink_ = nullptr;
        AUDIO_INFO_LOG("%{public}s: UnInit sink ashmem OK,", __func__);
    }
#endif // DEBUG_DIRECT_USE_HDI
    if (bufferFd_ != INVALID_FD) {
        close(bufferFd_);
        bufferFd_ = INVALID_FD;
    }
    if ((audioRender_ != nullptr) && (audioAdapter_ != nullptr)) {
        audioAdapter_->DestroyRender(audioAdapter_, audioRender_);
        audioRender_ = nullptr;
    }

    if ((audioManager_ != nullptr) && (audioAdapter_ != nullptr)) {
        audioManager_->UnloadAdapter(audioManager_, audioAdapter_);
    }
    audioAdapter_ = nullptr;
    audioManager_ = nullptr;
    // remove map recorder.
    RemoteFastAudioRendererSinkInner *temp = allRFSinks[this->deviceNetworkId_];
    if (temp != nullptr) {
        delete temp;
        temp = nullptr;
        allRFSinks.erase(this->deviceNetworkId_);
    }
}

int32_t RemoteFastAudioRendererSinkInner::Init(IAudioSinkAttr attr)
{
    AUDIO_INFO_LOG("RemoteAudioRendererSink: Init start.");
    attr_ = attr;

    int32_t ret = InitAudioManager();
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("Init audio manager Fail, ret: %{public}d.", ret);
        return ERR_INVALID_HANDLE;
    }

    int32_t size = 0;
    struct AudioAdapterDescriptor *descs = nullptr;
    ret = audioManager_->GetAllAdapters(audioManager_, &descs, &size);
    if (size == 0 || descs == nullptr || ret != SUCCESS) {
        AUDIO_ERR_LOG("Get adapters Fail, ret: %{public}d.", ret);
        return ERR_INVALID_HANDLE;
    }
    AUDIO_INFO_LOG("Get [%{publid}d]adapters", size);
    int32_t targetIdx = GetTargetAdapterPort(descs, size, attr_.deviceNetworkId);
    CHECK_AND_RETURN_RET_LOG((targetIdx >= 0), ERR_INVALID_INDEX, "can not find target adapter.");

    struct AudioAdapterDescriptor *desc = &descs[targetIdx];

    if (audioManager_->LoadAdapter(audioManager_, desc, &audioAdapter_) != SUCCESS ||
        audioAdapter_ == nullptr) {
        AUDIO_ERR_LOG("Load adapter failed, ret: %{public}d.", ret);
        return ERR_INVALID_HANDLE;
    }

    ret = audioAdapter_->InitAllPorts(audioAdapter_);
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("InitAllPorts failed, ret: %{public}d.", ret);
        return ERR_NOT_STARTED;
    }

    if (CreateRender(audioPort_) != SUCCESS) {
        AUDIO_ERR_LOG("Create render failed, audio port: %{public}d", audioPort_.portId);
        return ERR_NOT_STARTED;
    }

    AUDIO_INFO_LOG("RemoteAudioRendererSink: Init end.");
    rendererInited_.store(true);
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::InitAudioManager()
{
    AUDIO_INFO_LOG("RemoteFastAudioRendererSinkInner: Initialize audio proxy manager");
#ifdef __aarch64__
    char resolvedPath[100] = "/system/lib64/libdaudio_client.z.so";
#else
    char resolvedPath[100] = "/system/lib/libdaudio_client.z.so";
#endif
    struct AudioManager *(*GetAudioManagerFuncs)() = nullptr;

    void *handle = dlopen(resolvedPath, RTLD_LAZY);
    if (handle == nullptr) {
        AUDIO_ERR_LOG("Open so Fail");
        return ERR_INVALID_HANDLE;
    }
    AUDIO_INFO_LOG("dlopen successful");

    GetAudioManagerFuncs = (struct AudioManager *(*)())(dlsym(handle, "GetAudioManagerFuncs"));
    if (GetAudioManagerFuncs == nullptr) {
        AUDIO_ERR_LOG("dlsym GetAudioManagerFuncs fail.");
        return ERR_INVALID_HANDLE;
    }
    AUDIO_INFO_LOG("dlsym GetAudioManagerFuncs done");
    audioManager_ = GetAudioManagerFuncs();
    CHECK_AND_RETURN_RET_LOG((audioManager_ != nullptr), ERR_INVALID_HANDLE, "Init daudio manager fail!");
    AUDIO_INFO_LOG("Get daudio manager ok");
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::GetTargetAdapterPort(struct AudioAdapterDescriptor *descs, int32_t size,
    const char *networkId)
{
    return 0;
}

void RemoteFastAudioRendererSinkInner::RegisterParameterCallback(IAudioSinkCallback* callback)
{
    AUDIO_INFO_LOG("RemoteFastAudioRendererSink: register params callback");
    callback_ = callback;
    if (paramCallbackRegistered_.load()) {
        return;
    }
    paramCallbackRegistered_.store(true);
}

void RemoteFastAudioRendererSinkInner::SetAudioParameter(const AudioParamKey key, const std::string& condition,
    const std::string& value)
{
    AUDIO_INFO_LOG("RemoteFastAudioRendererSink SetAudioParameter not support.");
}

std::string RemoteFastAudioRendererSinkInner::GetAudioParameter(const AudioParamKey key, const std::string& condition)
{
    AUDIO_INFO_LOG("RemoteFastAudioRendererSink GetAudioParameter not support.");
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
    AUDIO_INFO_LOG("RemoteFastAudioRendererSink Create render format: %{public}d", param.format);
    struct AudioDeviceDescriptor deviceDesc;
    deviceDesc.portId = renderPort.portId;
    deviceDesc.pins = PIN_OUT_SPEAKER;
    deviceDesc.desc = nullptr;
    int32_t ret = audioAdapter_->CreateRender(audioAdapter_, &deviceDesc, &param, &audioRender_);
    if (ret != SUCCESS || audioRender_ == nullptr) {
        AUDIO_ERR_LOG("AudioDeviceCreateRender failed");
        return ERR_NOT_STARTED;
    }
    if (param.type == AUDIO_MMAP_NOIRQ) {
        PrepareMmapBuffer();
    }
    isRenderCreated_.store(true);
    int64_t cost = (ClockTime::GetCurNano() - start) / AUDIO_US_PER_SECOND;
    AUDIO_INFO_LOG("CreateRender cost[%{public}" PRId64 "]ms", cost);
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::PrepareMmapBuffer()
{
    CHECK_AND_RETURN_RET_LOG((audioRender_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio render is null.", __func__);

    int32_t totalBifferInMs = 40; // 5 * (6 + 2 * (1)) = 40ms, the buffer size, not latency.
    frameSizeInByte_ = PcmFormatToBits(attr_.format) * attr_.channel / PCM_8_BIT;
    int32_t reqBufferFrameSize = totalBifferInMs * (attr_.sampleRate / 1000);

    struct AudioMmapBufferDescriptor desc = {0};
    int32_t ret = audioRender_->attr.ReqMmapBuffer((AudioHandle)audioRender_, reqBufferFrameSize, &desc);
    CHECK_AND_RETURN_RET_LOG((ret == SUCCESS), ERR_OPERATION_FAILED,
        "%{public}s require mmap buffer failed, ret:%{public}d.", __func__, ret);

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
    AUDIO_INFO_LOG("%{public}s create ashmem sink OK, ashmemLen %{public}zu.", __func__, bufferSize_);
    if (!(ashmemSink_->MapReadAndWriteAshmem())) {
        AUDIO_ERR_LOG("%{public}s map ashmem sink failed.", __func__);
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
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("Audio render is null!");
        return ERR_INVALID_HANDLE;
    }

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
    attrs.streamId = INTERNAL_OUTPUT_STREAM_ID;
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

int32_t RemoteFastAudioRendererSinkInner::Start(void)
{
    AUDIO_INFO_LOG("Start.");
    if (!isRenderCreated_.load()) {
        if (CreateRender(audioPort_) != 0) {
            AUDIO_ERR_LOG("Create render failed, Audio Port: %{public}d", audioPort_.portId);
            return ERR_NOT_STARTED;
        }
    }

    if (!started_.load()) {
        int32_t ret = audioRender_->control.Start(reinterpret_cast<AudioHandle>(audioRender_));
        if (ret != SUCCESS) {
            AUDIO_ERR_LOG("RemoteFastAudioRendererSink::Start failed!");
            return ERR_NOT_STARTED;
        }
        started_.store(true);
    }
    AUDIO_INFO_LOG("Start Ok.");
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::RenderFrame(char &data, uint64_t len, uint64_t &writeLen)
{
    AUDIO_DEBUG_LOG("RenderFrame is not supported.");
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::SetVolume(float left, float right)
{
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteFastAudioRendererSink::SetVolume failed audioRender_ null");
        return ERR_INVALID_HANDLE;
    }

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
        AUDIO_ERR_LOG("RemoteFastAudioRendererSink::Set volume failed!");
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
        AUDIO_ERR_LOG("RemoteFastAudioRendererSink: GetLatency failed audio render null");
        return ERR_INVALID_HANDLE;
    }

    if (!latency) {
        AUDIO_ERR_LOG("RemoteFastAudioRendererSink: GetLatency failed latency null");
        return ERR_INVALID_PARAM;
    }

    uint32_t hdiLatency = 0;
    if (audioRender_->GetLatency(audioRender_, &hdiLatency) != 0) {
        AUDIO_ERR_LOG("RemoteFastAudioRendererSink: GetLatency failed.");
        return ERR_OPERATION_FAILED;
    }

    *latency = hdiLatency;
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::GetTransactionId(uint64_t *transactionId)
{
    (void)transactionId;
    AUDIO_ERR_LOG("RemoteFastAudioRendererSink: GetTransactionId not supported");
    return ERR_NOT_SUPPORTED;
}

int32_t RemoteFastAudioRendererSinkInner::SetVoiceVolume(float volume)
{
    (void)volume;
    AUDIO_ERR_LOG("RemoteFastAudioRendererSink: SetVoiceVolume not supported");
    return ERR_NOT_SUPPORTED;
}

int32_t RemoteFastAudioRendererSinkInner::SetOutputRoute(DeviceType deviceType)
{
    (void)deviceType;
    AUDIO_ERR_LOG("RemoteFastAudioRendererSink: SetOutputRoute not supported");
    return ERR_NOT_SUPPORTED;
}

void RemoteFastAudioRendererSinkInner::SetAudioMonoState(bool audioMono)
{
    (void)audioMono;
    AUDIO_ERR_LOG("RemoteFastAudioRendererSink: SetAudioMonoState not supported");
    return;
}

void RemoteFastAudioRendererSinkInner::SetAudioBalanceValue(float audioBalance)
{
    (void)audioBalance;
    AUDIO_ERR_LOG("RemoteFastAudioRendererSink: SetAudioBalanceValue not supported");
    return;
}

int32_t RemoteFastAudioRendererSinkInner::SetAudioScene(AudioScene audioScene, DeviceType activeDevice)
{
    AUDIO_INFO_LOG("RemoteFastAudioRendererSink: SetAudioScene not supported");
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::Stop(void)
{
    AUDIO_INFO_LOG("Stop.");
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteFastAudioRendererSink::Stop failed audioRender_ null");
        return ERR_INVALID_HANDLE;
    }

    if (started_.load()) {
        int32_t ret = audioRender_->control.Stop(reinterpret_cast<AudioHandle>(audioRender_));
        if (ret) {
            AUDIO_ERR_LOG("RemoteFastAudioRendererSink::Stop failed!");
            return ERR_OPERATION_FAILED;
        }
        started_.store(false);
    }
    AUDIO_INFO_LOG("Stop ok.");
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::Pause(void)
{
    AUDIO_INFO_LOG("Pause.");
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteFastAudioRendererSink::Pause failed audioRender_ null");
        return ERR_INVALID_HANDLE;
    }

    if (!started_.load()) {
        AUDIO_ERR_LOG("RemoteFastAudioRendererSink::Pause invalid state!");
        return ERR_OPERATION_FAILED;
    }

    if (!paused_.load()) {
        int32_t ret = audioRender_->control.Pause(reinterpret_cast<AudioHandle>(audioRender_));
        if (ret) {
            AUDIO_ERR_LOG("RemoteFastAudioRendererSink::Pause failed!");
            return ERR_OPERATION_FAILED;
        }
        paused_.store(true);
    }
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::Resume(void)
{
    AUDIO_INFO_LOG("Pause.");
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteFastAudioRendererSink::Resume failed audioRender_ null");
        return ERR_INVALID_HANDLE;
    }

    if (!started_.load()) {
        AUDIO_ERR_LOG("RemoteFastAudioRendererSink::Resume invalid state!");
        return ERR_OPERATION_FAILED;
    }

    if (paused_.load()) {
        int32_t ret = audioRender_->control.Resume(reinterpret_cast<AudioHandle>(audioRender_));
        if (ret) {
            AUDIO_ERR_LOG("RemoteFastAudioRendererSink::Resume failed!");
            return ERR_OPERATION_FAILED;
        }
        paused_.store(false);
    }
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::Reset(void)
{
    AUDIO_INFO_LOG("Reset.");
    if (!started_.load() || audioRender_ == nullptr) {
        AUDIO_ERR_LOG("%{public}s remote renderer start state %{public}d.", __func__, started_.load());
        return ERR_OPERATION_FAILED;
    }

    int32_t ret = audioRender_->control.Flush(reinterpret_cast<AudioHandle>(audioRender_));
    if (ret) {
        AUDIO_ERR_LOG("RemoteFastAudioRendererSink::Reset failed, ret %{public}d.", ret);
        return ERR_OPERATION_FAILED;
    }
    return SUCCESS;
}

int32_t RemoteFastAudioRendererSinkInner::Flush(void)
{
    AUDIO_INFO_LOG("Flush.");
    if (!started_.load() || audioRender_ == nullptr) {
        AUDIO_ERR_LOG("%{public}s remote renderer start state %{public}d.", __func__, started_.load());
        return ERR_OPERATION_FAILED;
    }

    int32_t ret = audioRender_->control.Flush(reinterpret_cast<AudioHandle>(audioRender_));
    if (ret) {
        AUDIO_ERR_LOG("RemoteFastAudioRendererSink::Flush failed, ret %{public}d.", ret);
        return ERR_OPERATION_FAILED;
    }
    return SUCCESS;
}
} // namespace AudioStandard
} // namespace OHOS