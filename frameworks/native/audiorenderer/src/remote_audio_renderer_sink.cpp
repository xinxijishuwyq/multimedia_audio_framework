/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <chrono>
#include <cstring>
#include <dlfcn.h>
#include <string>
#include <sstream>
#include <unistd.h>

#include "audio_errors.h"
#include "audio_log.h"
#include "remote_audio_renderer_sink.h"

#define DEBUG_DUMP_FILE

using namespace std;

namespace OHOS {
namespace AudioStandard {
namespace {
const int32_t HALF_FACTOR = 2;
// const int32_t MAX_AUDIO_ADAPTER_NUM = 5;
const float DEFAULT_VOLUME_LEVEL = 1.0f;
const uint32_t AUDIO_CHANNELCOUNT = 2;
const uint32_t AUDIO_SAMPLE_RATE_48K = 48000;
const uint32_t DEEP_BUFFER_RENDER_PERIOD_SIZE = 4096;
const uint32_t INT_32_MAX = 0x7fffffff;
const uint32_t PCM_8_BIT = 8;
const uint32_t PCM_16_BIT = 16;
const uint32_t PCM_24_BIT = 24;
const uint32_t PCM_32_BIT = 32;
const uint32_t INTERNAL_OUTPUT_STREAM_ID = 0;
}

std::map<std::string, RemoteAudioRendererSink *> RemoteAudioRendererSink::allsinks;

RemoteAudioRendererSink::RemoteAudioRendererSink(std::string deviceNetworkId)
    : rendererInited_(false), started_(false), paused_(false), leftVolume_(DEFAULT_VOLUME_LEVEL),
      rightVolume_(DEFAULT_VOLUME_LEVEL),audioAdapter_(nullptr), audioRender_(nullptr)
{
    AUDIO_INFO_LOG("Constract.");
    attr_ = {};
    this->deviceNetworkId_ = deviceNetworkId;
    audioManager_ = GetAudioManager();
#ifdef DEBUG_DUMP_FILE
    pfd = nullptr;
#endif // DEBUG_DUMP_FILE
}

RemoteAudioRendererSink::~RemoteAudioRendererSink()
{
    if (rendererInited_ == true) {
        DeInit();
    } else {
        AUDIO_INFO_LOG("RemoteAudioRendererSink has already DeInit.");
    }
}

RemoteAudioRendererSink *RemoteAudioRendererSink::GetInstance(const char *deviceNetworkId)
{
    AUDIO_INFO_LOG("GetInstance.");
    RemoteAudioRendererSink *audioRenderer_ = nullptr;
    if(deviceNetworkId == nullptr) {
        return audioRenderer_;
    }
    // check if it is in our map
    std::string deviceName = deviceNetworkId;
    if (allsinks.count(deviceName)) {
        return allsinks[deviceName];
    } else {
        audioRenderer_ = new(std::nothrow) RemoteAudioRendererSink(deviceName);
        AUDIO_DEBUG_LOG("new Daudio device sink:[%{public}s]", deviceNetworkId);
        allsinks[deviceName] = audioRenderer_;
    }
    CHECK_AND_RETURN_RET_LOG((audioRenderer_ != nullptr), nullptr, "null audioRenderer!");
    return audioRenderer_;
}

void RemoteAudioRendererSink::DeInit()
{
    AUDIO_INFO_LOG("DeInit.");
    started_ = false;
    rendererInited_ = false;
    if ((audioRender_ != nullptr) && (audioAdapter_ != nullptr)) {
        audioAdapter_->DestroyRender(audioAdapter_, audioRender_);
    }
    audioRender_ = nullptr;

    if ((audioManager_ != nullptr) && (audioAdapter_ != nullptr)) {
        if (routeHandle_ != -1) {
            audioAdapter_->ReleaseAudioRoute(audioAdapter_, routeHandle_);
        }
        audioManager_->UnloadAdapter(audioManager_, audioAdapter_);
    }
    audioAdapter_ = nullptr;
    audioManager_ = nullptr;
#ifdef DEBUG_DUMP_FILE
    if (pfd) {
        fclose(pfd);
        pfd = nullptr;
    }
#endif // DEBUG_DUMP_FILE
    // remove map recorder.
    RemoteAudioRendererSink *temp = allsinks[this->deviceNetworkId_];
    if (temp != nullptr) {
        delete temp;
        temp = nullptr;
        allsinks.erase(this->deviceNetworkId_);
    }
}

void InitAttrs(struct AudioSampleAttributes &attrs)
{
    /* Initialization of audio parameters for playback */
    attrs.channelCount = AUDIO_CHANNELCOUNT;
    attrs.sampleRate = AUDIO_SAMPLE_RATE_48K;
    attrs.interleaved = 0;
    attrs.streamId = INTERNAL_OUTPUT_STREAM_ID;
    attrs.type = AUDIO_IN_MEDIA;
    attrs.period = DEEP_BUFFER_RENDER_PERIOD_SIZE;
    attrs.isBigEndian = false;
    attrs.isSignedData = true;
    attrs.stopThreshold = INT_32_MAX;
    attrs.silenceThreshold = 0;
}

struct AudioManager *RemoteAudioRendererSink::GetAudioManager()
{
    AUDIO_INFO_LOG("RemoteAudioRendererSink: Initialize audio proxy manager");
#ifdef PRODUCT_M40
    char resolvedPath[100] = "/vendor/lib64/libdaudio_client.z.so";
    struct AudioManager *(*GetAudioManagerFuncs)() = nullptr;

    void *handle_ = dlopen(resolvedPath, 1);
    if (handle_ == nullptr) {
        AUDIO_ERR_LOG("Open so Fail");
        return nullptr;
    }
    AUDIO_INFO_LOG("dlopen successful");

    GetAudioManagerFuncs = (struct AudioManager *(*)())(dlsym(handle_, "GetAudioManagerFuncs"));
    if (GetAudioManagerFuncs == nullptr) {
        return nullptr;
    }
    AUDIO_INFO_LOG("GetAudioManagerFuncs done");

    struct AudioManager *audioManager = GetAudioManagerFuncs();
    if (audioManager == nullptr) {
        return nullptr;
    }
    AUDIO_INFO_LOG("daudio manager created");
#else
    struct AudioManager *audioManager = GetAudioManagerFuncs();
#endif // PRODUCT_M40
    return audioManager;
}

uint32_t PcmFormatToBits(enum AudioFormat format)
{
    switch (format) {
        case AUDIO_FORMAT_PCM_8_BIT:
            return PCM_8_BIT;
        case AUDIO_FORMAT_PCM_16_BIT:
            return PCM_16_BIT;
        case AUDIO_FORMAT_PCM_24_BIT:
            return PCM_24_BIT;
        case AUDIO_FORMAT_PCM_32_BIT:
            return PCM_32_BIT;
        default:
            return PCM_24_BIT;
    }
}

int32_t RemoteAudioRendererSink::CreateRender(struct AudioPort &renderPort)
{
    int32_t ret;
    struct AudioSampleAttributes param;
    InitAttrs(param);
    param.sampleRate = attr_.sampleRate;
    param.channelCount = attr_.channel;
    param.format = attr_.format;
    param.frameSize = PcmFormatToBits(param.format) * param.channelCount / PCM_8_BIT;
    param.startThreshold = DEEP_BUFFER_RENDER_PERIOD_SIZE / (param.frameSize);
    AUDIO_INFO_LOG("RemoteAudioRendererSink Create render format: %{public}d", param.format);
    struct AudioDeviceDescriptor deviceDesc;
    deviceDesc.portId = renderPort.portId;
    deviceDesc.pins = PIN_OUT_SPEAKER;
    deviceDesc.desc = nullptr;
    ret = audioAdapter_->CreateRender(audioAdapter_, &deviceDesc, &param, &audioRender_);
    if (ret != 0 || audioRender_ == nullptr) {
        AUDIO_ERR_LOG("AudioDeviceCreateRender failed");
        audioManager_->UnloadAdapter(audioManager_, audioAdapter_);
        return ERR_NOT_STARTED;
    }

    return 0;
}

inline std::string printRemoteAttr(RemoteAudioSinkAttr attr_){
    std::stringstream value;
    value << "adapterName[" << attr_.adapterName << "] ";
    value << "openMicSpeaker[" << attr_.openMicSpeaker << "] ";
    value << "format[" << static_cast<int32_t>(attr_.format) << "] ";
    value << "sampleFmt[" << attr_.sampleFmt << "] ";
    value << "sampleRate[" << attr_.sampleRate << "] ";
    value << "channel[" << attr_.channel << "] ";
    value << "volume[" << attr_.volume << "] ";
    value << "filePath[" << attr_.filePath << "] ";
    value << "deviceNetworkId[" << attr_.deviceNetworkId << "]";
    value << "device_type[" << attr_.device_type << "]";
    return value.str();
}

int32_t RemoteAudioRendererSink::Init(RemoteAudioSinkAttr &attr)
{
    AUDIO_INFO_LOG("RemoteAudioRendererSink: Init start.");
    attr_ = attr;
    adapterNameCase_ = attr_.adapterName;  // Set sound card information
    openSpeaker_ = attr_.openMicSpeaker;

    if (audioManager_ == nullptr) {
        AUDIO_ERR_LOG("Init audio manager Fail");
        return ERR_NOT_STARTED;
    }

    int32_t size = 0;
    struct AudioAdapterDescriptor *descs = nullptr;
    int32_t ret = audioManager_->GetAllAdapters(audioManager_, &descs, &size);
    if (size == 0 || descs == nullptr || ret != 0) {
        AUDIO_ERR_LOG("Get adapters Fail");
        return ERR_NOT_STARTED;
    }
    int32_t targetIdx = -1;
    AUDIO_INFO_LOG("Get [%{publid}d]adapters", size);
    for (int32_t index = 0; index < size; index++) {
        struct AudioAdapterDescriptor *desc = &descs[index];
        if (desc == nullptr || desc->adapterName == nullptr) {
            continue;
        }
        if (strcmp(desc->adapterName, attr_.deviceNetworkId)) {
            AUDIO_INFO_LOG("not target adapter");
            continue;
        }
        targetIdx = index;
        for (uint32_t port = 0; port < desc->portNum; port++) {
            // Only find out the port of out in the sound card
            if (desc->ports[port].portId == PIN_OUT_SPEAKER) {
                audioPort_ = desc->ports[port];
                break;
            }
        }
    }

    struct AudioAdapterDescriptor *desc = &descs[targetIdx];

    if (audioManager_->LoadAdapter(audioManager_, desc, &audioAdapter_) != 0) {
        AUDIO_ERR_LOG("Load Adapter Fail");
        return ERR_NOT_STARTED;
    }
    if (audioAdapter_ == nullptr) {
        AUDIO_ERR_LOG("Load audio device failed");
        return ERR_NOT_STARTED;
    }

    ret = audioAdapter_->InitAllPorts(audioAdapter_);
    if (ret != 0) {
        AUDIO_ERR_LOG("InitAllPorts failed");
        return ERR_NOT_STARTED;
    }

    if (CreateRender(audioPort_) != 0) {
        AUDIO_ERR_LOG("Create render failed, Audio Port: %{public}d", audioPort_.portId);
        return ERR_NOT_STARTED;
    }

    rendererInited_ = true;

#ifdef DEBUG_DUMP_FILE
    AUDIO_INFO_LOG("dump RemoteAudioSinkAttr:%{public}s", printRemoteAttr(attr_).c_str());
    std::string fileName = attr_.filePath;
    std::string filePath = "/data/local/tmp/remote_test_001.pcm";
    const char *g_audioOutTestFilePath = filePath.c_str();
    pfd = fopen(g_audioOutTestFilePath, "a+"); // here will not create a file if not exit.
    AUDIO_ERR_LOG("init dump file[%{public}s]", g_audioOutTestFilePath);
    if (pfd == nullptr) {
        AUDIO_ERR_LOG("Error opening remote pcm file[%{public}s]", g_audioOutTestFilePath);
    }
#endif // DEBUG_DUMP_FILE

    return SUCCESS;
}

inline int64_t GetNowTimeMs()
{
    std::chrono::milliseconds nowMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    return nowMs.count();
}

int32_t RemoteAudioRendererSink::RenderFrame(char &data, uint64_t len, uint64_t &writeLen)
{
    int64_t start = GetNowTimeMs();
    int32_t ret;
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("Audio Render Handle is nullptr!");
        return ERR_INVALID_HANDLE;
    }

    ret = audioRender_->RenderFrame(audioRender_, (void*)&data, len, &writeLen);
    if (ret != 0) {
        AUDIO_ERR_LOG("RenderFrame failed ret: %{public}x", ret);
        return ERR_WRITE_FAILED;
    }
    writeLen = len;
#ifdef DEBUG_DUMP_FILE
    if (pfd != nullptr) {
        size_t writeResult = fwrite((void*)&data, 1, len, pfd);
        if (writeResult != len) {
            AUDIO_ERR_LOG("Failed to write the file.");
        }
    }
#endif // DEBUG_DUMP_FILE

    int64_t cost = GetNowTimeMs() - start;
    AUDIO_DEBUG_LOG("RenderFrame len[%{public}zu] cost[%{public}zu]ms", (size_t)len, (size_t)cost);
    return SUCCESS;
}

int32_t RemoteAudioRendererSink::Start(void)
{
    AUDIO_INFO_LOG("Start.");
    int32_t ret;

    if (!started_) {
        ret = audioRender_->control.Start(reinterpret_cast<AudioHandle>(audioRender_));
        if (!ret) {
            started_ = true;
            return SUCCESS;
        } else {
            AUDIO_ERR_LOG("RemoteAudioRendererSink::Start failed!");
            return ERR_NOT_STARTED;
        }
    }
    started_ = true;
    return SUCCESS;
}

int32_t RemoteAudioRendererSink::SetVolume(float left, float right)
{
    int32_t ret;
    float volume;

    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::SetVolume failed audioRender_ null");
        return ERR_INVALID_HANDLE;
    }

    leftVolume_ = left;
    rightVolume_ = right;
    if ((leftVolume_ == 0) && (rightVolume_ != 0)) {
        volume = rightVolume_;
    } else if ((leftVolume_ != 0) && (rightVolume_ == 0)) {
        volume = leftVolume_;
    } else {
        volume = (leftVolume_ + rightVolume_) / HALF_FACTOR;
    }

    ret = audioRender_->volume.SetVolume(reinterpret_cast<AudioHandle>(audioRender_), volume);
    if (ret) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::Set volume failed!");
    }

    return ret;
}

int32_t RemoteAudioRendererSink::GetVolume(float &left, float &right)
{
    left = leftVolume_;
    right = rightVolume_;
    return SUCCESS;
}

int32_t RemoteAudioRendererSink::GetLatency(uint32_t *latency)
{
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink: GetLatency failed audio render null");
        return ERR_INVALID_HANDLE;
    }

    if (!latency) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink: GetLatency failed latency null");
        return ERR_INVALID_PARAM;
    }

    uint32_t hdiLatency;
    if (audioRender_->GetLatency(audioRender_, &hdiLatency) == 0) {
        *latency = hdiLatency;
        return SUCCESS;
    } else {
        AUDIO_ERR_LOG("RemoteAudioRendererSink: GetLatency failed.");
        return ERR_OPERATION_FAILED;
    }
    *latency = 21; // 4096 bytes ~ 21ms.
    return SUCCESS;
}

static AudioCategory GetAudioCategory(AudioScene audioScene)
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
    AUDIO_DEBUG_LOG("RemoteAudioRendererSink: Audio category returned is: %{public}d", audioCategory);

    return audioCategory;
}

static int32_t SetOutputPortPin(DeviceType outputDevice, AudioRouteNode &sink)
{
    int32_t ret = SUCCESS;

    switch (outputDevice) {
        case DEVICE_TYPE_SPEAKER:
            sink.ext.device.type = PIN_OUT_SPEAKER;
            sink.ext.device.desc = "pin_out_speaker";
            break;
        case DEVICE_TYPE_WIRED_HEADSET:
            sink.ext.device.type = PIN_OUT_HEADSET;
            sink.ext.device.desc = "pin_out_headset";
            break;
        case DEVICE_TYPE_USB_HEADSET:
            sink.ext.device.type = PIN_OUT_USB_EXT;
            sink.ext.device.desc = "pin_out_usb_ext";
            break;
        default:
            ret = ERR_NOT_SUPPORTED;
            break;
    }

    return ret;
}

int32_t RemoteAudioRendererSink::OpenOutput(DeviceType outputDevice)
{
    AudioRouteNode source = {};
    AudioRouteNode sink = {};

    int32_t ret = SetOutputPortPin(outputDevice, sink);
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink: OpenOutput FAILED: %{public}d", ret);
        return ret;
    }

    source.portId = 0;
    source.role = AUDIO_PORT_SOURCE_ROLE;
    source.type = AUDIO_PORT_MIX_TYPE;
    source.ext.mix.moduleId = 0;
    source.ext.mix.streamId = INTERNAL_OUTPUT_STREAM_ID;

    sink.portId = audioPort_.portId;
    sink.role = AUDIO_PORT_SINK_ROLE;
    sink.type = AUDIO_PORT_DEVICE_TYPE;
    sink.ext.device.moduleId = 0;

    AudioRoute route = {
        .sourcesNum = 1,
        .sources = &source,
        .sinksNum = 1,
        .sinks = &sink,
    };

    ret = audioAdapter_->UpdateAudioRoute(audioAdapter_, &route, &routeHandle_);
    AUDIO_DEBUG_LOG("RemoteAudioRendererSink: UpdateAudioRoute returns: %{public}d", ret);
    if (ret != 0) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink: UpdateAudioRoute failed");
        return ERR_OPERATION_FAILED;
    }

    return SUCCESS;
}

int32_t RemoteAudioRendererSink::SetAudioScene(AudioScene audioScene)
{
    AUDIO_INFO_LOG("RemoteAudioRendererSink::SetAudioScene in");
    CHECK_AND_RETURN_RET_LOG(audioScene >= AUDIO_SCENE_DEFAULT && audioScene <= AUDIO_SCENE_PHONE_CHAT,
                             ERR_INVALID_PARAM, "invalid audioScene");
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::SetAudioScene failed audio render handle is null!");
        return ERR_INVALID_HANDLE;
    }

	int32_t ret = OpenOutput(DEVICE_TYPE_SPEAKER);
	if (ret < 0) {
		AUDIO_ERR_LOG("RemoteAudioRendererSink: Update route FAILED: %{public}d", ret);
	}
	struct AudioSceneDescriptor scene;
	scene.scene.id = GetAudioCategory(audioScene);
	scene.desc.pins = PIN_OUT_SPEAKER;
	if (audioRender_->scene.SelectScene == nullptr) {
		AUDIO_ERR_LOG("RemoteAudioRendererSink: Select scene nullptr");
		return ERR_OPERATION_FAILED;
	}

	AUDIO_INFO_LOG("RemoteAudioRendererSink::SelectScene start");
	ret = audioRender_->scene.SelectScene((AudioHandle)audioRender_, &scene);
	AUDIO_INFO_LOG("RemoteAudioRendererSink::SelectScene over");
	if (ret < 0) {
		AUDIO_ERR_LOG("RemoteAudioRendererSink: Select scene FAILED: %{public}d", ret);
		return ERR_OPERATION_FAILED;
	}


    AUDIO_INFO_LOG("RemoteAudioRendererSink::Select audio scene SUCCESS: %{public}d", audioScene);
    return SUCCESS;
}

int32_t RemoteAudioRendererSink::Stop(void)
{
    AUDIO_INFO_LOG("Stop.");
    int32_t ret;

    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::Stop failed audioRender_ null");
        return ERR_INVALID_HANDLE;
    }

    if (started_) {
        ret = audioRender_->control.Stop(reinterpret_cast<AudioHandle>(audioRender_));
        if (!ret) {
            started_ = false;
            return SUCCESS;
        } else {
            AUDIO_ERR_LOG("RemoteAudioRendererSink::Stop failed!");
            return ERR_OPERATION_FAILED;
        }
    }
    started_ = false;
    return SUCCESS;
}

int32_t RemoteAudioRendererSink::Pause(void)
{
    AUDIO_INFO_LOG("Pause.");
    int32_t ret;

    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::Pause failed audioRender_ null");
        return ERR_INVALID_HANDLE;
    }

    if (!started_) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::Pause invalid state!");
        return ERR_OPERATION_FAILED;
    }

    if (!paused_) {
        ret = audioRender_->control.Pause(reinterpret_cast<AudioHandle>(audioRender_));
        if (!ret) {
            paused_ = true;
            return SUCCESS;
        } else {
            AUDIO_ERR_LOG("RemoteAudioRendererSink::Pause failed!");
            return ERR_OPERATION_FAILED;
        }
    }
    return SUCCESS;
}

int32_t RemoteAudioRendererSink::Resume(void)
{
    AUDIO_INFO_LOG("Pause.");
    int32_t ret;

    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::Resume failed audioRender_ null");
        return ERR_INVALID_HANDLE;
    }

    if (!started_) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::Resume invalid state!");
        return ERR_OPERATION_FAILED;
    }

    if (paused_) {
        ret = audioRender_->control.Resume(reinterpret_cast<AudioHandle>(audioRender_));
        if (!ret) {
            paused_ = false;
            return SUCCESS;
        } else {
            AUDIO_ERR_LOG("RemoteAudioRendererSink::Resume failed!");
            return ERR_OPERATION_FAILED;
        }
    }
    return SUCCESS;
}

int32_t RemoteAudioRendererSink::Reset(void)
{
    AUDIO_INFO_LOG("Reset.");
    int32_t ret;

    if (started_ && audioRender_ != nullptr) {
        ret = audioRender_->control.Flush(reinterpret_cast<AudioHandle>(audioRender_));
        if (!ret) {
            return SUCCESS;
        } else {
            AUDIO_ERR_LOG("RemoteAudioRendererSink::Reset failed!");
            return ERR_OPERATION_FAILED;
        }
    }
    return ERR_OPERATION_FAILED;
}

int32_t RemoteAudioRendererSink::Flush(void)
{
    AUDIO_INFO_LOG("Flush.");
    int32_t ret;

    if (started_ && audioRender_ != nullptr) {
        ret = audioRender_->control.Flush(reinterpret_cast<AudioHandle>(audioRender_));
        if (!ret) {
            return SUCCESS;
        } else {
            AUDIO_ERR_LOG("RemoteAudioRendererSink::Flush failed!");
            return ERR_OPERATION_FAILED;
        }
    }
    return ERR_OPERATION_FAILED;
}
} // namespace AudioStandard
} // namespace OHOS

#ifdef __cplusplus
extern "C" {
#endif

using namespace OHOS::AudioStandard;

int32_t FillinRemoteAudioRenderSinkWapper(const char *deviceNetworkId, void **wapper)
{
    RemoteAudioRendererSink *instance = RemoteAudioRendererSink::GetInstance(deviceNetworkId);
    if (instance != nullptr) {
        *wapper = static_cast<void *>(instance);
    } else {
        *wapper = nullptr;
    }

    return SUCCESS;
}

int32_t RemoteAudioRendererSinkInit(void *wapper, RemoteAudioSinkAttr *attr)
{
    int32_t ret;
    RemoteAudioRendererSink *audioRendererSink = static_cast<RemoteAudioRendererSink *>(wapper);
    CHECK_AND_RETURN_RET_LOG(audioRendererSink != nullptr, ERR_INVALID_HANDLE, "null audioRendererSink");
    if (audioRendererSink->rendererInited_)
        return SUCCESS;

    ret = audioRendererSink->Init(*attr);
    return ret;
}

void RemoteAudioRendererSinkDeInit(void *wapper)
{
    RemoteAudioRendererSink *audioRendererSink = static_cast<RemoteAudioRendererSink *>(wapper);
    CHECK_AND_RETURN_LOG(audioRendererSink != nullptr, "null audioRendererSink");
    // remove the sink in allsinks.
    if (audioRendererSink->rendererInited_)
        audioRendererSink->DeInit();
}

int32_t RemoteAudioRendererSinkStop(void *wapper)
{
    int32_t ret;
    RemoteAudioRendererSink *audioRendererSink = static_cast<RemoteAudioRendererSink *>(wapper);
    CHECK_AND_RETURN_RET_LOG(audioRendererSink != nullptr, ERR_INVALID_HANDLE, "null audioRendererSink");
    if (!audioRendererSink->rendererInited_)
        return SUCCESS;

    ret = audioRendererSink->Stop();
    return ret;
}

int32_t RemoteAudioRendererSinkStart(void *wapper)
{
    int32_t ret;
    RemoteAudioRendererSink *audioRendererSink = static_cast<RemoteAudioRendererSink *>(wapper);
    CHECK_AND_RETURN_RET_LOG(audioRendererSink != nullptr, ERR_INVALID_HANDLE, "null audioRendererSink");
    if (!audioRendererSink->rendererInited_) {
        AUDIO_ERR_LOG("audioRenderer Not Inited! Init the renderer first\n");
        return ERR_NOT_STARTED;
    }

    ret = audioRendererSink->Start();
    return ret;
}

int32_t RemoteAudioRendererSinkPause(void *wapper)
{
    RemoteAudioRendererSink *audioRendererSink = static_cast<RemoteAudioRendererSink *>(wapper);
    CHECK_AND_RETURN_RET_LOG(audioRendererSink != nullptr, ERR_INVALID_HANDLE, "null audioRendererSink");
    if (!audioRendererSink->rendererInited_) {
        AUDIO_ERR_LOG("Renderer pause failed");
        return ERR_NOT_STARTED;
    }

    return audioRendererSink->Pause();
}

int32_t RemoteAudioRendererSinkResume(void *wapper)
{
    RemoteAudioRendererSink *audioRendererSink = static_cast<RemoteAudioRendererSink *>(wapper);
    CHECK_AND_RETURN_RET_LOG(audioRendererSink != nullptr, ERR_INVALID_HANDLE, "null audioRendererSink");
    if (!audioRendererSink->rendererInited_) {
        AUDIO_ERR_LOG("Renderer resume failed");
        return ERR_NOT_STARTED;
    }

    return audioRendererSink->Resume();
}

int32_t RemoteAudioRendererRenderFrame(void *wapper, char &data, uint64_t len, uint64_t &writeLen)
{
    int32_t ret;
    RemoteAudioRendererSink *audioRendererSink = static_cast<RemoteAudioRendererSink *>(wapper);
    CHECK_AND_RETURN_RET_LOG(audioRendererSink != nullptr, ERR_INVALID_HANDLE, "null audioRendererSink");
    if (!audioRendererSink->rendererInited_) {
        AUDIO_ERR_LOG("audioRenderer Not Inited! Init the renderer first\n");
        return ERR_NOT_STARTED;
    }

    ret = audioRendererSink->RenderFrame(data, len, writeLen);
    return ret;
}

int32_t RemoteAudioRendererSinkSetVolume(void *wapper, float left, float right)
{
    int32_t ret;
    RemoteAudioRendererSink *audioRendererSink = static_cast<RemoteAudioRendererSink *>(wapper);
    CHECK_AND_RETURN_RET_LOG(audioRendererSink != nullptr, ERR_INVALID_HANDLE, "null audioRendererSink");
    if (!audioRendererSink->rendererInited_) {
        AUDIO_ERR_LOG("audioRenderer Not Inited! Init the renderer first\n");
        return ERR_NOT_STARTED;
    }

    ret = audioRendererSink->SetVolume(left, right);
    return ret;
}

int32_t RemoteAudioRendererSinkGetLatency(void *wapper, uint32_t *latency)
{
    int32_t ret;
    RemoteAudioRendererSink *audioRendererSink = static_cast<RemoteAudioRendererSink *>(wapper);
    CHECK_AND_RETURN_RET_LOG(audioRendererSink != nullptr, ERR_INVALID_HANDLE, "null audioRendererSink");
    if (!audioRendererSink->rendererInited_) {
        AUDIO_ERR_LOG("audioRenderer Not Inited! Init the renderer first\n");
        return ERR_NOT_STARTED;
    }

    if (!latency) {
        AUDIO_ERR_LOG("RemoteAudioRendererSinkGetLatency failed latency null");
        return ERR_INVALID_PARAM;
    }

    ret = audioRendererSink->GetLatency(latency);
    return ret;
}

int32_t RemoteAudioRendererSinkGetTransactionId(uint64_t *transactionId)
{
    // as we can not get wapper initialized, we dont know which sink address we should return.
    *transactionId = (uint16_t)-1;
    return SUCCESS;
}
#ifdef __cplusplus
}
#endif
