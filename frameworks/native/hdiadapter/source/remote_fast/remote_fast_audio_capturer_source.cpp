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

#include "remote_fast_audio_capturer_source.h"

#include <cinttypes>
#include <dlfcn.h>
#include <sstream>
#include "securec.h"

#include "audio_errors.h"
#include "audio_log.h"
#include "audio_utils.h"

namespace OHOS {
namespace AudioStandard {
std::map<std::string, RemoteFastAudioCapturerSource *> RemoteFastAudioCapturerSource::allRFSources_;
IMmapAudioCapturerSource *RemoteFastAudioCapturerSource::GetInstance(const std::string& deviceNetworkId)
{
    RemoteFastAudioCapturerSource *rfCapturer = nullptr;
    // check if it is in our map
    if (allRFSources_.count(deviceNetworkId)) {
        return allRFSources_[deviceNetworkId];
    } else {
        rfCapturer = new(std::nothrow) RemoteFastAudioCapturerSource(deviceNetworkId);
        AUDIO_DEBUG_LOG("new Daudio device source: [%{public}s]", deviceNetworkId.c_str());
        allRFSources_[deviceNetworkId] = rfCapturer;
    }
    CHECK_AND_RETURN_RET_LOG((rfCapturer != nullptr), nullptr, "Remote fast audio capturer is null!");
    return rfCapturer;
}

RemoteFastAudioCapturerSource::RemoteFastAudioCapturerSource(const std::string& deviceNetworkId)
    : deviceNetworkId_(deviceNetworkId)
{
    AUDIO_INFO_LOG("RemoteFastAudioCapturerSource Constract.");
}

RemoteFastAudioCapturerSource::~RemoteFastAudioCapturerSource()
{
    if (capturerInited_.load()) {
        RemoteFastAudioCapturerSource::DeInit();
    }
    AUDIO_INFO_LOG("~RemoteFastAudioCapturerSource end.");
}

bool RemoteFastAudioCapturerSource::IsInited()
{
    return capturerInited_.load();
}

void RemoteFastAudioCapturerSource::DeInit()
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    started_.store(false);
    capturerInited_.store(false);
#ifdef DEBUG_DIRECT_USE_HDI
    if (pfd_) {
        fclose(pfd_);
        pfd_ = nullptr;
    }
    if (ashmemSource_ != nullptr) {
        ashmemSource_->UnmapAshmem();
        ashmemSource_->CloseAshmem();
        ashmemSource_ = nullptr;
        AUDIO_INFO_LOG("%{public}s: Uninit source ashmem OK.", __func__);
    }
#endif // DEBUG_DIRECT_USE_HDI
    if (bufferFd_ != INVALID_FD) {
        close(bufferFd_);
        bufferFd_ = INVALID_FD;
    }
    if ((audioCapture_ != nullptr) && (audioAdapter_ != nullptr)) {
        audioAdapter_->DestroyCapture(audioAdapter_, audioCapture_);
        audioCapture_ = nullptr;
    }

    if ((audioManager_ != nullptr) && (audioAdapter_ != nullptr)) {
        if (routeHandle_ != -1) {
            audioAdapter_->ReleaseAudioRoute(audioAdapter_, routeHandle_);
        }
    }
    audioAdapter_ = nullptr;
    audioManager_ = nullptr;
    // remove map recorder.
    RemoteFastAudioCapturerSource *temp = allRFSources_[this->deviceNetworkId_];
    if (temp != nullptr) {
        delete temp;
        temp = nullptr;
        allRFSources_.erase(this->deviceNetworkId_);
    }
}

int32_t RemoteFastAudioCapturerSource::Init(IAudioSourceAttr &attr)
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    attr_ = attr;
    int32_t ret = InitAudioManager();
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("Init audio manager fail, ret %{public}d.", ret);
        return ERR_INVALID_HANDLE;
    }

    int32_t size = 0;
    struct AudioAdapterDescriptor *descs = nullptr;
    ret = audioManager_->GetAllAdapters(audioManager_, &descs, &size);
    if (size == 0 || descs == nullptr || ret != 0) {
        AUDIO_ERR_LOG("Get adapters fail, ret %{public}d.", ret);
        return ERR_INVALID_HANDLE;
    }
    AUDIO_INFO_LOG("Get [%{publid}d] adapters", size);
    int32_t targetIdx = GetTargetAdapterPort(descs, size, attr_.deviceNetworkId);
    CHECK_AND_RETURN_RET_LOG((targetIdx >= 0), ERR_INVALID_INDEX, "can not find target adapter.");

    struct AudioAdapterDescriptor *desc = &descs[targetIdx];
    ret = audioManager_->LoadAdapter(audioManager_, desc, &audioAdapter_);
    if (ret != 0 || audioAdapter_ == nullptr) {
        AUDIO_ERR_LOG("Load audio adapter fail, ret %{public}d.", ret);
        return ERR_INVALID_HANDLE;
    }
    ret = audioAdapter_->InitAllPorts(audioAdapter_);
    if (ret != 0) {
        AUDIO_ERR_LOG("Init audio adapter ports fail, ret %{public}d.", ret);
        return ERR_DEVICE_INIT;
    }

    if (!isCapturerCreated_.load()) {
        CHECK_AND_RETURN_RET_LOG(CreateCapture(audioPort_) == SUCCESS, ERR_NOT_STARTED,
            "Create capture failed, Audio Port: %{public}d.", audioPort_.portId);
    }
    capturerInited_.store(true);

#ifdef DEBUG_DIRECT_USE_HDI
    AUDIO_INFO_LOG("Dump audio source attr: [%{public}s]", printRemoteAttr(attr_).c_str());
    pfd_ = fopen(audioFilePath, "a+"); // here will not create a file if not exit.
    AUDIO_INFO_LOG("Init dump file [%{public}s]", audioFilePath);
    if (pfd_ == nullptr) {
        AUDIO_ERR_LOG("Opening remote pcm file [%{public}s] fail.", audioFilePath);
    }
#endif // DEBUG_DIRECT_USE_HDI

    AUDIO_INFO_LOG("%{public}s end.", __func__);
    return SUCCESS;
}

int32_t RemoteFastAudioCapturerSource::InitAudioManager()
{
    AUDIO_INFO_LOG("%{public}s remote fast capturer enter.", __func__);
#ifdef __aarch64__
    char resolvedPath[100] = "/system/lib64/libdaudio_client.z.so";
#else
    char resolvedPath[100] = "/system/lib/libdaudio_client.z.so";
#endif
    struct AudioManager *(*GetAudioManagerFuncs)() = nullptr;

    void *handle = dlopen(resolvedPath, RTLD_LAZY);
    if (handle == nullptr) {
        AUDIO_ERR_LOG("dlopen %{public}s fail.", resolvedPath);
        return ERR_INVALID_HANDLE;
    }
    AUDIO_INFO_LOG("dlopen %{public}s OK.", resolvedPath);

    dlerror();
    GetAudioManagerFuncs = (struct AudioManager *(*)())(dlsym(handle, "GetAudioManagerFuncs"));
    if (dlerror() != nullptr || GetAudioManagerFuncs == nullptr) {
        AUDIO_ERR_LOG("dlsym GetAudioManagerFuncs fail.");
        return ERR_INVALID_HANDLE;
    }
    AUDIO_INFO_LOG("dlsym GetAudioManagerFuncs OK.");
    audioManager_ = GetAudioManagerFuncs();
    CHECK_AND_RETURN_RET_LOG((audioManager_ != nullptr), ERR_INVALID_HANDLE, "Init daudio manager fail!");

    AUDIO_INFO_LOG("Get daudio manager OK.");
    return SUCCESS;
}

int32_t RemoteFastAudioCapturerSource::GetTargetAdapterPort(struct AudioAdapterDescriptor *descs, int32_t size,
    const char *networkId)
{
    int32_t targetIdx = INVALID_INDEX;
    for (int32_t index = 0; index < size; index++) {
        struct AudioAdapterDescriptor *desc = &descs[index];
        if (desc == nullptr || desc->adapterName == nullptr) {
            continue;
        }
        targetIdx = index;
        for (uint32_t port = 0; port < desc->portNum; port++) {
            // Only find out the port of mic
            if (desc->ports[port].portId == PIN_IN_MIC) {
                audioPort_ = desc->ports[port];
                break;
            }
        }
    }
    return targetIdx;
}

int32_t RemoteFastAudioCapturerSource::CreateCapture(const struct AudioPort &capturePort)
{
    CHECK_AND_RETURN_RET_LOG((audioAdapter_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio adapter is null.", __func__);
    struct AudioSampleAttributes captureAttr;
    InitAttrs(captureAttr);
    struct AudioDeviceDescriptor deviceDesc;
    deviceDesc.portId = capturePort.portId;
    deviceDesc.pins = PIN_IN_MIC;
    deviceDesc.desc = nullptr;
    int32_t ret = audioAdapter_->CreateCapture(audioAdapter_, &deviceDesc, &captureAttr, &audioCapture_);
    if (ret != 0 || audioCapture_ == nullptr) {
        AUDIO_ERR_LOG("%{public}s create capture failed.", __func__);
        return ERR_NOT_STARTED;
    }
    if (captureAttr.type == AUDIO_MMAP_NOIRQ) {
        InitAshmem(captureAttr); // The remote fast source first start
    }

    isCapturerCreated_.store(true);
    AUDIO_INFO_LOG("%{public}s end, capture format: %{public}d.", __func__, captureAttr.format);
    return SUCCESS;
}

int32_t RemoteFastAudioCapturerSource::InitAshmem(const struct AudioSampleAttributes &attrs)
{
    CHECK_AND_RETURN_RET_LOG((audioCapture_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio capture is null.", __func__);

    int32_t reqSize = 1440;
    struct AudioMmapBufferDescriptor desc = {0};
    int32_t ret = audioCapture_->attr.ReqMmapBuffer(audioCapture_, reqSize, &desc);
    CHECK_AND_RETURN_RET_LOG((ret == 0), ERR_OPERATION_FAILED,
        "%{public}s require mmap buffer failed, ret %{public}d.", __func__, ret);
    AUDIO_INFO_LOG("%{public}s audio capture mmap buffer info, memoryAddress[%{private}p] memoryFd[%{public}d] "
        "totalBufferFrames [%{public}d] transferFrameSize[%{public}d] isShareable[%{public}d] offset[%{public}d]",
        __func__, desc.memoryAddress, desc.memoryFd, desc.totalBufferFrames, desc.transferFrameSize,
        desc.isShareable, desc.offset);

    bufferFd_ = desc.memoryFd;
    int32_t periodFrameMaxSize = 1920000; // 192khz * 10s
    if (desc.totalBufferFrames < 0 || desc.transferFrameSize < 0 || desc.transferFrameSize > periodFrameMaxSize) {
        AUDIO_ERR_LOG("ReqMmapBuffer invalid values: totalBufferFrames[%{public}d] transferFrameSize[%{public}d]",
            desc.totalBufferFrames, desc.transferFrameSize);
        return ERR_OPERATION_FAILED;
    }
    bufferTotalFrameSize_ = desc.totalBufferFrames;
    eachReadFrameSize_ = desc.transferFrameSize;

#ifdef DEBUG_DIRECT_USE_HDI
    ashmemLen_ = desc.totalBufferFrames * attrs.channelCount * attrs.format;
    ashmemSource_ = new Ashmem(bufferFd_, ashmemLen_);
    AUDIO_INFO_LOG("%{public}s creat ashmem source OK, ashmemLen %{public}d.", __func__, ashmemLen_);
    if (!(ashmemSource_->MapReadAndWriteAshmem())) {
        AUDIO_ERR_LOG("%{public}s map ashmem source failed.", __func__);
        return ERR_OPERATION_FAILED;
    }
    lenPerRead_ = desc.transferFrameSize * attrs.channelCount * attrs.format;
    AUDIO_INFO_LOG("%{public}s map ashmem source OK, lenPerWrite %{public}d.", __func__, lenPerRead_);
#endif
    return SUCCESS;
}

void RemoteFastAudioCapturerSource::InitAttrs(struct AudioSampleAttributes &attrs)
{
    /* Initialization of audio parameters for playback */
    attrs.type = AUDIO_MMAP_NOIRQ;
    attrs.interleaved = CAPTURE_INTERLEAVED;
    attrs.format = ConverToHdiFormat(attr_.format);
    attrs.sampleRate = attr_.sampleRate;
    attrs.channelCount = attr_.channel;
    attrs.period = DEEP_BUFFER_CAPTURER_PERIOD_SIZE;
    attrs.frameSize = attrs.format * attrs.channelCount;
    attrs.isBigEndian = attr_.isBigEndian;
    attrs.isSignedData = true;
    attrs.startThreshold = DEEP_BUFFER_CAPTURER_PERIOD_SIZE / (attrs.frameSize);
    attrs.stopThreshold = INT_32_MAX;
    attrs.silenceThreshold = attr_.bufferSize;
    attrs.streamId = INTERNAL_OUTPUT_STREAM_ID;
}

AudioFormat RemoteFastAudioCapturerSource::ConverToHdiFormat(AudioSampleFormat format)
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

inline std::string printRemoteAttr(IAudioSourceAttr attr_)
{
    std::stringstream value;
    value << "adapterName[" << attr_.adapterName << "] openMicSpeaker[" << attr_.open_mic_speaker << "] ";
    value << "format[" << static_cast<int32_t>(attr_.format) << "] sampleFmt[" << attr_.sampleFmt << "] ";
    value << "sampleRate[" << attr_.sampleRate << "] channel[" << attr_.channel << "] ";
    value << "volume[" << attr_.volume << "] filePath[" << attr_.filePath << "] ";
    value << "deviceNetworkId[" << attr_.deviceNetworkId << "] device_type[" << attr_.deviceType << "]";
    return value.str();
}

int32_t RemoteFastAudioCapturerSource::Start(void)
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    if (!isCapturerCreated_.load()) {
        if (CreateCapture(audioPort_) != SUCCESS) {
            AUDIO_ERR_LOG("Create capture failed, Audio Port: %{public}d.", audioPort_.portId);
            return ERR_NOT_STARTED;
        }
    }

    if (started_.load()) {
        AUDIO_INFO_LOG("Remote fast capturer is already start.");
        return SUCCESS;
    }

    CHECK_AND_RETURN_RET_LOG((audioCapture_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio capture is null.", __func__);
    int32_t ret = audioCapture_->control.Start(reinterpret_cast<AudioHandle>(audioCapture_));
    if (ret != 0) {
        AUDIO_ERR_LOG("Remote fast capturer start fail, ret %{public}d.", ret);
        return ERR_NOT_STARTED;
    }

    ClockTime::RelativeSleep(CAPTURE_FIRST_FRIME_WAIT_NANO);
    uint64_t curHdiWritePos = 0;
    int64_t timeSec = 0;
    int64_t timeNanoSec = 0;
    int64_t writeTime = 0;
    while (true) {
        ret = GetMmapHandlePosition(curHdiWritePos, timeSec, timeNanoSec);
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "%{public}s get source handle info fail.", __func__);
        writeTime = timeNanoSec + timeSec * SECOND_TO_NANOSECOND;
        if (writeTime > 0) {
            break;
        }
        ClockTime::RelativeSleep(CAPTURE_RESYNC_SLEEP_NANO);
    }
    started_.store(true);

    AUDIO_INFO_LOG("%{public}s OK, curHdiWritePos %{public}" PRIu64", writeTime %{public}" PRId64" ns.",
        __func__, curHdiWritePos, writeTime);
    return SUCCESS;
}

int32_t RemoteFastAudioCapturerSource::CaptureFrame(char *frame, uint64_t requestBytes, uint64_t &replyBytes)
{
    AUDIO_DEBUG_LOG("%{public}s capture frame is not supported.", __func__);
    return SUCCESS;
}

int32_t RemoteFastAudioCapturerSource::SetVolume(float left, float right)
{
    AUDIO_INFO_LOG("%{public}s enter, left %{public}f, right %{public}f.", __func__, left, right);
    CHECK_AND_RETURN_RET_LOG((audioCapture_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio capture is null.", __func__);

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

    int32_t ret = audioCapture_->volume.SetVolume(reinterpret_cast<AudioHandle>(audioCapture_), volume);
    if (ret) {
        AUDIO_ERR_LOG("Remote fast capturer set volume fail, ret %{public}d.", ret);
    }
    return ret;
}

int32_t RemoteFastAudioCapturerSource::GetVolume(float &left, float &right)
{
    CHECK_AND_RETURN_RET_LOG((audioCapture_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio capture is null.", __func__);
    float val = 0;
    audioCapture_->volume.GetVolume((AudioHandle)audioCapture_, &val);
    left = val;
    right = val;
    return SUCCESS;
}

int32_t RemoteFastAudioCapturerSource::SetMute(bool isMute)
{
    CHECK_AND_RETURN_RET_LOG((audioCapture_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio capture is null.", __func__);
    int32_t ret = audioCapture_->volume.SetMute((AudioHandle)audioCapture_, isMute);
    if (ret != 0) {
        AUDIO_ERR_LOG("Remote fast capturer set mute fail, ret %{public}d.", ret);
    }

    micMuteState_ = isMute;
    return SUCCESS;
}

int32_t RemoteFastAudioCapturerSource::GetMute(bool &isMute)
{
    CHECK_AND_RETURN_RET_LOG((audioCapture_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio capture is null.", __func__);
    bool isHdiMute = false;
    int32_t ret = audioCapture_->volume.GetMute((AudioHandle)audioCapture_, &isHdiMute);
    if (ret != 0) {
        AUDIO_ERR_LOG("Remote fast capturer get mute fail, ret %{public}d.", ret);
    }

    isMute = micMuteState_;
    return SUCCESS;
}

uint64_t RemoteFastAudioCapturerSource::GetTransactionId()
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    return reinterpret_cast<uint64_t>(audioCapture_);
}

int32_t RemoteFastAudioCapturerSource::SetInputPortPin(DeviceType inputDevice, AudioRouteNode &source)
{
    int32_t ret = SUCCESS;
    switch (inputDevice) {
        case DEVICE_TYPE_MIC:
            source.ext.device.type = PIN_IN_MIC;
            source.ext.device.desc = "pin_in_mic";
            break;
        case DEVICE_TYPE_WIRED_HEADSET:
            source.ext.device.type = PIN_IN_HS_MIC;
            source.ext.device.desc = "pin_in_hs_mic";
            break;
        case DEVICE_TYPE_USB_HEADSET:
            source.ext.device.type = PIN_IN_USB_EXT;
            source.ext.device.desc = "pin_in_usb_ext";
            break;
        default:
            ret = ERR_NOT_SUPPORTED;
            break;
    }
    return ret;
}

int32_t RemoteFastAudioCapturerSource::SetInputRoute(DeviceType inputDevice)
{
    AudioRouteNode source = {};
    AudioRouteNode sink = {};
    int32_t ret = SetInputPortPin(inputDevice, source);
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("%{public}s set input port pin fail, ret %{public}d.", __func__, ret);
        return ret;
    }

    source.portId = audioPort_.portId;
    source.role = AUDIO_PORT_SOURCE_ROLE;
    source.type = AUDIO_PORT_DEVICE_TYPE;
    source.ext.device.moduleId = 0;

    sink.portId = 0;
    sink.role = AUDIO_PORT_SINK_ROLE;
    sink.type = AUDIO_PORT_MIX_TYPE;
    sink.ext.mix.moduleId = 0;
    sink.ext.mix.streamId = INTERNAL_INPUT_STREAM_ID;

    AudioRoute route = {
        .sourcesNum = 1,
        .sources = &source,
        .sinksNum = 1,
        .sinks = &sink,
    };

    CHECK_AND_RETURN_RET_LOG((audioAdapter_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio adapter is null.", __func__);
    ret = audioAdapter_->UpdateAudioRoute(audioAdapter_, &route, &routeHandle_);
    AUDIO_DEBUG_LOG("AudioCapturerSource: UpdateAudioRoute returns: %{public}d", ret);
    if (ret != 0) {
        AUDIO_ERR_LOG("AudioCapturerSource: UpdateAudioRoute failed");
        return ERR_OPERATION_FAILED;
    }
    return SUCCESS;
}

AudioCategory RemoteFastAudioCapturerSource::GetAudioCategory(AudioScene audioScene)
{
    AudioCategory audioCategory;
    switch (audioScene) {
        case AUDIO_SCENE_DEFAULT:
            audioCategory = AUDIO_IN_MEDIA;
            break;
        case AUDIO_SCENE_RINGING:
            audioCategory = AUDIO_IN_RINGTONE;
            break;
        case AUDIO_SCENE_PHONE_CALL:
            audioCategory = AUDIO_IN_CALL;
            break;
        case AUDIO_SCENE_PHONE_CHAT:
            audioCategory = AUDIO_IN_COMMUNICATION;
            break;
        default:
            audioCategory = AUDIO_IN_MEDIA;
            break;
    }
    AUDIO_DEBUG_LOG("RemoteFastAudioCapturerSource: Audio category returned is: %{public}d", audioCategory);

    return audioCategory;
}

int32_t RemoteFastAudioCapturerSource::SetAudioScene(AudioScene audioScene, DeviceType activeDevice)
{
    AUDIO_INFO_LOG("%{public}s enter: scene: %{public}d, device %{public}d.", __func__,
        audioScene, activeDevice);
    CHECK_AND_RETURN_RET_LOG((audioCapture_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio capture is null.", __func__);
    struct AudioSceneDescriptor scene;
    scene.scene.id = GetAudioCategory(audioScene);
    scene.desc.pins = PIN_IN_MIC;
    if (audioCapture_->scene.SelectScene == nullptr) {
        AUDIO_ERR_LOG("AudioCapturerSource: Select scene nullptr");
        return ERR_OPERATION_FAILED;
    }

    AUDIO_INFO_LOG("AudioCapturerSource::SelectScene start");
    int32_t ret = audioCapture_->scene.SelectScene((AudioHandle)audioCapture_, &scene);
    AUDIO_INFO_LOG("AudioCapturerSource::SelectScene over");
    if (ret < 0) {
        AUDIO_ERR_LOG("AudioCapturerSource: Select scene FAILED: %{public}d", ret);
        return ERR_OPERATION_FAILED;
    }
    AUDIO_INFO_LOG("AudioCapturerSource::Select audio scene SUCCESS: %{public}d", audioScene);
    return SUCCESS;
}

uint32_t RemoteFastAudioCapturerSource::PcmFormatToBits(AudioSampleFormat format)
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

int32_t RemoteFastAudioCapturerSource::GetMmapBufferInfo(int &fd, uint32_t &totalSizeInframe,
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

int32_t RemoteFastAudioCapturerSource::GetMmapHandlePosition(uint64_t &frames, int64_t &timeSec, int64_t &timeNanoSec)
{
    CHECK_AND_RETURN_RET_LOG((audioCapture_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio capture is null.", __func__);

    struct AudioTimeStamp timestamp = {};
    int32_t ret = audioCapture_->attr.GetMmapPosition((AudioHandle)audioCapture_, &frames, &timestamp);
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

int32_t RemoteFastAudioCapturerSource::Stop(void)
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    CHECK_AND_RETURN_RET_LOG((audioCapture_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio capture is null.", __func__);

    if (started_.load()) {
        int32_t ret = audioCapture_->control.Stop(reinterpret_cast<AudioHandle>(audioCapture_));
        if (ret != 0) {
            AUDIO_ERR_LOG("Remote fast capturer stop fail, ret %{public}d.", ret);
            return ERR_OPERATION_FAILED;
        }
        started_.store(false);
    }
    return SUCCESS;
}

int32_t RemoteFastAudioCapturerSource::Pause(void)
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    CHECK_AND_RETURN_RET_LOG((audioCapture_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio capture is null.", __func__);

    if (!started_.load()) {
        AUDIO_ERR_LOG("%{public}s: Invalid start state %{public}d.", __func__, started_.load());
        return ERR_OPERATION_FAILED;
    }

    if (!paused_.load()) {
        int32_t ret = audioCapture_->control.Pause(reinterpret_cast<AudioHandle>(audioCapture_));
        if (ret != 0) {
            AUDIO_ERR_LOG("Remote fast capturer pause fail, ret %{public}d.", ret);
            return ERR_OPERATION_FAILED;
        }
        paused_.store(true);
    }
    return SUCCESS;
}

int32_t RemoteFastAudioCapturerSource::Resume(void)
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    CHECK_AND_RETURN_RET_LOG((audioCapture_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio capture is null.", __func__);

    if (!started_.load()) {
        AUDIO_ERR_LOG("%{public}s: Invalid start state %{public}d.", __func__, started_.load());
        return ERR_OPERATION_FAILED;
    }

    if (paused_.load()) {
        int32_t ret = audioCapture_->control.Resume(reinterpret_cast<AudioHandle>(audioCapture_));
        if (ret != 0) {
            AUDIO_ERR_LOG("Remote fast capturer resume fail, ret %{public}d.", ret);
            return ERR_OPERATION_FAILED;
        }
        paused_.store(false);
    }
    return SUCCESS;
}

int32_t RemoteFastAudioCapturerSource::Reset(void)
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    CHECK_AND_RETURN_RET_LOG((audioCapture_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio capture is null.", __func__);
    if (!started_.load()) {
        AUDIO_ERR_LOG("%{public}s: Invalid start state %{public}d.", __func__, started_.load());
        return ERR_OPERATION_FAILED;
    }

    int32_t ret = audioCapture_->control.Flush(reinterpret_cast<AudioHandle>(audioCapture_));
    if (ret != 0) {
        AUDIO_ERR_LOG("Remote fast capturer reset fail, ret %{public}d.", ret);
        return ERR_OPERATION_FAILED;
    }
    return SUCCESS;
}

int32_t RemoteFastAudioCapturerSource::Flush(void)
{
    AUDIO_INFO_LOG("%{public}s enter.", __func__);
    CHECK_AND_RETURN_RET_LOG((audioCapture_ != nullptr), ERR_INVALID_HANDLE,
        "%{public}s: audio capture is null.", __func__);
    if (!started_.load()) {
        AUDIO_ERR_LOG("%{public}s: Invalid start state %{public}d.", __func__, started_.load());
        return ERR_OPERATION_FAILED;
    }

    int32_t ret = audioCapture_->control.Flush(reinterpret_cast<AudioHandle>(audioCapture_));
    if (ret != 0) {
        AUDIO_ERR_LOG("Remote fast capturer flush fail, ret %{public}d.", ret);
        return ERR_OPERATION_FAILED;
    }
    return SUCCESS;
}
} // namespace AudioStandard
} // namesapce OHOS
