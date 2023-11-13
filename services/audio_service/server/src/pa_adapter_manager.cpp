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

#include "pa_adapter_manager.h"
#include <sstream>
#include "audio_log.h"
#include "audio_errors.h"
#include "pa_renderer_stream_impl.h"
#include "pa_capturer_stream_impl.h"
#include "audio_utils.h"

namespace OHOS {
namespace AudioStandard {
const uint32_t CHECK_UTIL_SUCCESS = 0;
static const std::unordered_map<AudioStreamType, std::string> STREAM_TYPE_ENUM_STRING_MAP = {
    {STREAM_VOICE_CALL, "voice_call"},
    {STREAM_MUSIC, "music"},
    {STREAM_RING, "ring"},
    {STREAM_MEDIA, "media"},
    {STREAM_VOICE_ASSISTANT, "voice_assistant"},
    {STREAM_SYSTEM, "system"},
    {STREAM_ALARM, "alarm"},
    {STREAM_NOTIFICATION, "notification"},
    {STREAM_BLUETOOTH_SCO, "bluetooth_sco"},
    {STREAM_ENFORCED_AUDIBLE, "enforced_audible"},
    {STREAM_DTMF, "dtmf"},
    {STREAM_TTS, "tts"},
    {STREAM_ACCESSIBILITY, "accessibility"},
    {STREAM_RECORDING, "recording"},
    {STREAM_MOVIE, "movie"},
    {STREAM_GAME, "game"},
    {STREAM_SPEECH, "speech"},
    {STREAM_SYSTEM_ENFORCED, "system_enforced"},
    {STREAM_ULTRASONIC, "ultrasonic"},
    {STREAM_WAKEUP, "wakeup"},
    {STREAM_VOICE_MESSAGE, "voice_message"},
    {STREAM_NAVIGATION, "navigation"}
};

static int32_t CheckReturnIfinvalid(bool expr, const int32_t retVal)
{
    do {
        if (!(expr)) {
            return retVal;
        }
    } while (false);
    return CHECK_UTIL_SUCCESS;
}

PaAdapterManager::PaAdapterManager(ManagerType type)
{
    AUDIO_DEBUG_LOG("Constructor PaAdapterManager");
    mainLoop_ = nullptr;
    api_ = nullptr;;
    context_ = nullptr;
    isContextConnected_ = false;
    isMainLoopStarted_ = false;
    managerType_ = type;
}

int32_t PaAdapterManager::CreateRender(AudioStreamParams params, AudioStreamType audioType,
    std::shared_ptr<IRendererStream> &stream)
{
    AUDIO_DEBUG_LOG("Create renderer start");
    if (context_ == nullptr) {
        AUDIO_INFO_LOG("Context is null, start to create context");
        int32_t ret = InitPaContext();
        CHECK_AND_RETURN_RET_LOG(ret == PA_ADAPTER_SUCCESS, ret, "Failed to init pa context");
    }

    pa_stream *paStream = InitPaStream(params, audioType);
    std::shared_ptr<IRendererStream> rendererStream = CreateRendererStream(params, paStream);
    CHECK_AND_RETURN_RET_LOG(rendererStream != nullptr, PA_ADAPTER_INIT_ERR, "Failed to init pa stream");
    stream = rendererStream;
    return PA_ADAPTER_SUCCESS;
}

int32_t PaAdapterManager::ReleaseRender(uint32_t streamIndex)
{
    AUDIO_DEBUG_LOG("Enter ReleaseRender");
    auto it = rendererStreamMap_.find(streamIndex);
    if (it == rendererStreamMap_.end()) {
        AUDIO_WARNING_LOG("No matching stream");
        return PA_ADAPTER_SUCCESS;
    }

    if (rendererStreamMap_[streamIndex]->Release() < 0) {
        AUDIO_WARNING_LOG("Release stream %{public}d failed", streamIndex);
        return PA_ADAPTER_ERR;
    }
    rendererStreamMap_[streamIndex] = nullptr;
    rendererStreamMap_.erase(streamIndex);

    if (rendererStreamMap_.size() == 0) {
        AUDIO_INFO_LOG("Release the last stream");
        if (ResetPaContext() < 0) {
            AUDIO_ERR_LOG("Release pa context falied");
            return PA_ADAPTER_ERR;
        }
    }
    return PA_ADAPTER_SUCCESS;
}

int32_t PaAdapterManager::CreateCapturer(AudioStreamParams params, AudioStreamType audioType,
    std::shared_ptr<ICapturerStream> &stream)
{
    AUDIO_DEBUG_LOG("Create capturer start");
    if (context_ == nullptr) {
        AUDIO_INFO_LOG("Context is null, start to create context");
        int32_t ret = InitPaContext();
        CHECK_AND_RETURN_RET_LOG(ret == PA_ADAPTER_SUCCESS, ret, "Failed to init pa context");
    }
    pa_stream *paStream = InitPaStream(params, audioType);
    std::shared_ptr<ICapturerStream> capturerStream = CreateCapturerStream(params, paStream);
    CHECK_AND_RETURN_RET_LOG(capturerStream != nullptr, PA_ADAPTER_INIT_ERR, "Failed to init pa stream");
    stream = capturerStream;
    return PA_ADAPTER_SUCCESS;
}

int32_t PaAdapterManager::ReleaseCapturer(uint32_t streamIndex)
{
    AUDIO_DEBUG_LOG("Enter ReleaseCapturer");
    auto it = capturerStreamMap_.find(streamIndex);
    if (it == capturerStreamMap_.end()) {
        AUDIO_WARNING_LOG("No matching stream");
        return PA_ADAPTER_SUCCESS;
    }

    if (capturerStreamMap_[streamIndex]->Release() < 0) {
        AUDIO_WARNING_LOG("Release stream %{public}d failed", streamIndex);
        return PA_ADAPTER_ERR;
    }

    capturerStreamMap_[streamIndex] = nullptr;
    capturerStreamMap_.erase(streamIndex);
    if (capturerStreamMap_.size() == 0) {
        AUDIO_INFO_LOG("Release the last stream");
        if (ResetPaContext() < 0) {
            AUDIO_ERR_LOG("Release pa context falied");
            return PA_ADAPTER_ERR;
        }
    }
    return PA_ADAPTER_SUCCESS;
}

int32_t PaAdapterManager::ResetPaContext()
{
    AUDIO_DEBUG_LOG("Enter ResetPaContext");
    if (context_) {
        pa_context_set_state_callback(context_, nullptr, nullptr);
        if (isContextConnected_ == true) {
            pa_threaded_mainloop_lock(mainLoop_);
            pa_context_disconnect(context_);
            pa_context_unref(context_);
            pa_threaded_mainloop_unlock(mainLoop_);
            isContextConnected_ = false;
            context_ = nullptr;
        }
    }

    if (mainLoop_) {
        pa_threaded_mainloop_free(mainLoop_);
        isMainLoopStarted_  = false;
        mainLoop_ = nullptr;
    }

    api_ = nullptr;
    return PA_ADAPTER_SUCCESS;
}

int32_t PaAdapterManager::InitPaContext()
{
    AUDIO_DEBUG_LOG("Enter InitPaContext");
    int error = PA_ADAPTER_ERR;
    mainLoop_ = pa_threaded_mainloop_new();
    CHECK_AND_RETURN_RET_LOG(mainLoop_ != nullptr, PA_ADAPTER_INIT_ERR, "Failed to init pa mainLoop");
    api_ = pa_threaded_mainloop_get_api(mainLoop_);
    if (api_ == nullptr) {
        pa_threaded_mainloop_free(mainLoop_);
        AUDIO_ERR_LOG("Get api from mainLoop failed");
        return PA_ADAPTER_INIT_ERR;
    }

    std::stringstream ss;
    ss << "app-pid<" << getpid() << ">-uid<" << getuid() << ">";
    std::string packageName = "";
    ss >> packageName;

    context_ = pa_context_new(api_, packageName.c_str());
    if (context_ == nullptr) {
        pa_threaded_mainloop_free(mainLoop_);
        AUDIO_ERR_LOG("New context failed");
        return PA_ADAPTER_INIT_ERR;
    }

    pa_context_set_state_callback(context_, PAContextStateCb, mainLoop_);
    if (pa_context_connect(context_, nullptr, PA_CONTEXT_NOFAIL, nullptr) < 0) {
        error = pa_context_errno(context_);
        AUDIO_ERR_LOG("Context connect error: %{public}s", pa_strerror(error));
        return PA_ADAPTER_INIT_ERR;
    }
    isContextConnected_ = true;
    HandleMainLoopStart();

    pa_threaded_mainloop_unlock(mainLoop_);
    return PA_ADAPTER_SUCCESS;
}

int32_t PaAdapterManager::HandleMainLoopStart()
{
    int error = PA_ADAPTER_ERR;
    pa_threaded_mainloop_lock(mainLoop_);
    if (pa_threaded_mainloop_start(mainLoop_) < 0) {
        pa_threaded_mainloop_unlock(mainLoop_);
        return PA_ADAPTER_INIT_ERR;
    }
    isMainLoopStarted_ = true;

    while (true) {
        pa_context_state_t state = pa_context_get_state(context_);
        if (state == PA_CONTEXT_READY) {
            AUDIO_INFO_LOG("pa context is ready");
            break;
        }

        if (!PA_CONTEXT_IS_GOOD(state)) {
            error = pa_context_errno(context_);
            AUDIO_ERR_LOG("Context bad state error: %{public}s", pa_strerror(error));
            pa_threaded_mainloop_unlock(mainLoop_);
            ResetPaContext();
            return PA_ADAPTER_INIT_ERR;
        }
        pa_threaded_mainloop_wait(mainLoop_);
    }
    return PA_ADAPTER_SUCCESS;
}

pa_stream *PaAdapterManager::InitPaStream(AudioStreamParams params, AudioStreamType audioType)
{
    AUDIO_DEBUG_LOG("Enter InitPaStream");
    int32_t error = PA_ADAPTER_ERR;
    if (CheckReturnIfinvalid(mainLoop_ && context_, PA_ADAPTER_ERR) < 0) {
        return nullptr;
    }
    pa_threaded_mainloop_lock(mainLoop_);

    // Use struct to save spec size
    pa_sample_spec sampleSpec = ConvertToPAAudioParams(params);
    pa_proplist *propList = pa_proplist_new();
    if (propList == nullptr) {
        AUDIO_ERR_LOG("pa_proplist_new failed");
        pa_threaded_mainloop_unlock(mainLoop_);
        return nullptr;
    }
    pa_proplist_sets(propList, "stream.type", "STREAM_MUSIC");

    // Set streamName and sessionID after pa_stream_new
    const std::string streamName = GetStreamName(audioType);

    int32_t sessionID_ = 1;
    pa_proplist_sets(propList, "stream.sessionID", std::to_string(sessionID_).c_str());
    auto timenow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const std::string streamStartTime = ctime(&timenow);
    pa_proplist_sets(propList, "stream.startTime", streamStartTime.c_str());
    pa_proplist_sets(propList, "scene.type", "SCENE_MUSIC");

    StreamUsage streamUsage = STREAM_USAGE_MUSIC;
    pa_proplist_sets(propList, "stream.usage", std::to_string(streamUsage).c_str());
    float mVolumeFactor = 1.0f;
    float mPowerVolumeFactor = 1.0f;
    pa_proplist_sets(propList, "stream.volumeFactor", std::to_string(mVolumeFactor).c_str());
    pa_proplist_sets(propList, "stream.powerVolumeFactor", std::to_string(mPowerVolumeFactor).c_str());
    AUDIO_INFO_LOG("Creating stream of channels %{public}d", params.channels);

    pa_stream *paStream = pa_stream_new_with_proplist(context_, streamName.c_str(), &sampleSpec, nullptr, propList);
    if (!paStream) {
        error = pa_context_errno(context_);
        pa_proplist_free(propList);
        pa_threaded_mainloop_unlock(mainLoop_);
        AUDIO_ERR_LOG("pa_stream_new_with_proplist failed, error: %{public}d", error);
        return nullptr;
    }

    pa_proplist_free(propList);
    pa_stream_set_state_callback(paStream, PAStreamStateCb, (void *)this);
    pa_threaded_mainloop_unlock(mainLoop_);

    int32_t ret = ConnectStreamToPA(paStream, sampleSpec);
    if (ret < 0) {
        AUDIO_ERR_LOG("ConnectStreamToPA Failed");
        return nullptr;
    }
    return paStream;
}

std::shared_ptr<IRendererStream> PaAdapterManager::CreateRendererStream(AudioStreamParams params, pa_stream *paStream)
{
    std::shared_ptr<PaRendererStreamImpl> rendererStream = std::make_shared<PaRendererStreamImpl>(paStream, params, mainLoop_);
    if (rendererStream == nullptr) {
        AUDIO_ERR_LOG("Create rendererStream Failed");
        return nullptr;
    }
    uint32_t streamIndex = pa_stream_get_index(paStream);
    AUDIO_DEBUG_LOG("PaStream index is %{public}u", streamIndex);
    rendererStreamMap_[streamIndex] = rendererStream;
    return rendererStream;
}

std::shared_ptr<ICapturerStream> PaAdapterManager::CreateCapturerStream(AudioStreamParams params, pa_stream *paStream)
{
    std::shared_ptr<PaCapturerStreamImpl> capturerStream = std::make_shared<PaCapturerStreamImpl>(paStream, params, mainLoop_);
    if (capturerStream == nullptr) {
        AUDIO_ERR_LOG("Create capturerStream Failed");
        return nullptr;
    }
    uint32_t streamIndex = pa_stream_get_index(paStream);
    AUDIO_DEBUG_LOG("PaStream index is %{public}u", streamIndex);
    capturerStreamMap_[streamIndex] = capturerStream;
    return capturerStream;
}

int32_t PaAdapterManager::ConnectStreamToPA(pa_stream *paStream, pa_sample_spec sampleSpec)
{
    AUDIO_DEBUG_LOG("Enter PaAdapterManager::ConnectStreamToPA");
    int error, result;

    if (CheckReturnIfinvalid(mainLoop_ && context_ && paStream, PA_ADAPTER_ERR) < 0) {
        return PA_ADAPTER_ERR;
    }

    pa_threaded_mainloop_lock(mainLoop_);
    if (managerType_ == PLAYBACK) {
        uint32_t tlength = 0;
        GetSysPara("multimedia.audio.tlength", tlength);
        uint32_t maxlength = 0;
        GetSysPara("multimedia.audio.maxlength", maxlength);
        uint32_t prebuf = 0;
        GetSysPara("multimedia.audio.prebuf", prebuf);
        pa_buffer_attr bufferAttr;
        bufferAttr.fragsize = static_cast<uint32_t>(-1);
        bufferAttr.prebuf = pa_usec_to_bytes(20 * PA_USEC_PER_MSEC * prebuf, &sampleSpec);
        bufferAttr.maxlength = pa_usec_to_bytes(20 * PA_USEC_PER_MSEC * maxlength, &sampleSpec);
        bufferAttr.tlength = pa_usec_to_bytes(20 * PA_USEC_PER_MSEC * tlength, &sampleSpec);
        bufferAttr.minreq = pa_usec_to_bytes(20 * PA_USEC_PER_MSEC, &sampleSpec);
        AUDIO_INFO_LOG("bufferAttr, maxLength: %{public}u, tlength: %{public}u, prebuf: %{public}u",
            maxlength, tlength, prebuf);

        result = pa_stream_connect_playback(paStream, nullptr, &bufferAttr,
            (pa_stream_flags_t)(PA_STREAM_ADJUST_LATENCY | PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_START_CORKED |
            PA_STREAM_VARIABLE_RATE), nullptr, nullptr);

        if (result < 0) {
            error = pa_context_errno(context_);
            AUDIO_ERR_LOG("connection to stream error: %{public}d", error);
            pa_threaded_mainloop_unlock(mainLoop_);
            return PA_ADAPTER_CREATE_STREAM_ERR;
        }
    } else {
        uint32_t fragsize = 0;
        GetSysPara("multimedia.audio.fragsize", fragsize);
        uint32_t maxlength = 0;
        GetSysPara("multimedia.audio.maxlength", maxlength);

        pa_buffer_attr bufferAttr;
        bufferAttr.maxlength = pa_usec_to_bytes(20 * PA_USEC_PER_MSEC * maxlength, &sampleSpec);
        bufferAttr.fragsize = pa_usec_to_bytes(20 * PA_USEC_PER_MSEC * fragsize, &sampleSpec);
        AUDIO_INFO_LOG("bufferAttr, maxLength: %{public}u, fragsize: %{public}u",
            maxlength, fragsize);

        result = pa_stream_connect_record(paStream, nullptr, &bufferAttr,
            (pa_stream_flags_t)(PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_START_CORKED |
            PA_STREAM_VARIABLE_RATE));
        // PA_STREAM_ADJUST_LATENCY exist, return peek length from server;

        if (result < 0) {
            error = pa_context_errno(context_);
            AUDIO_ERR_LOG("connection to stream error: %{public}d", error);
            pa_threaded_mainloop_unlock(mainLoop_);
            return PA_ADAPTER_CREATE_STREAM_ERR;
        }
    }

    while (true) {
        pa_stream_state_t state = pa_stream_get_state(paStream);
        if (state == PA_STREAM_READY) {
            break;
        }
        if (!PA_STREAM_IS_GOOD(state)) {

            error = pa_context_errno(context_);
            pa_threaded_mainloop_unlock(mainLoop_);
            AUDIO_ERR_LOG("connection to stream error: %{public}d", error);
            return PA_ADAPTER_CREATE_STREAM_ERR;
        }
        pa_threaded_mainloop_wait(mainLoop_);
    }
    pa_threaded_mainloop_unlock(mainLoop_);
    return PA_ADAPTER_SUCCESS;
}

void PaAdapterManager::PAContextStateCb(pa_context *context, void *userdata)
{
    pa_threaded_mainloop *mainLoop = (pa_threaded_mainloop *)userdata;
    AUDIO_INFO_LOG("Current Context State: %{public}d", pa_context_get_state(context));

    switch (pa_context_get_state(context)) {
        case PA_CONTEXT_READY:
            pa_threaded_mainloop_signal(mainLoop, 0);
            break;
        case PA_CONTEXT_TERMINATED:
        case PA_CONTEXT_FAILED:
            pa_threaded_mainloop_signal(mainLoop, 0);
            break;

        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
        default:
            break;
    }
}

void PaAdapterManager::PAStreamStateCb(pa_stream *stream, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamStateCb: userdata is null");
        return;
    }
    PaAdapterManager *adapterManger = (PaAdapterManager *)userdata;
    AUDIO_INFO_LOG("Current Stream State: %{public}d", pa_stream_get_state(stream));
    switch (pa_stream_get_state(stream)) {
        case PA_STREAM_READY:
        case PA_STREAM_FAILED:
        case PA_STREAM_TERMINATED:
            pa_threaded_mainloop_signal(adapterManger->mainLoop_, 0);
            break;
        case PA_STREAM_UNCONNECTED:
        case PA_STREAM_CREATING:
        default:
            break;
    }
}

const std::string PaAdapterManager::GetStreamName(AudioStreamType audioType)
{
    std::string name = "unknown";
    if (STREAM_TYPE_ENUM_STRING_MAP.find(audioType) != STREAM_TYPE_ENUM_STRING_MAP.end()) {
        name = STREAM_TYPE_ENUM_STRING_MAP.at(audioType);
    } else {
        AUDIO_ERR_LOG("GetStreamName: Invalid stream type [%{public}d], return unknown", audioType);
    }
    const std::string streamName = name;
    return streamName;
}

pa_sample_spec PaAdapterManager::ConvertToPAAudioParams(AudioStreamParams params)
{
    pa_sample_spec paSampleSpec;
    paSampleSpec.channels = params.channels;
    paSampleSpec.rate = params.samplingRate;
    switch ((AudioSampleFormat)params.format) {
        case SAMPLE_U8:
            paSampleSpec.format = (pa_sample_format_t)PA_SAMPLE_U8;
            break;
        case SAMPLE_S16LE:
            paSampleSpec.format = (pa_sample_format_t)PA_SAMPLE_S16LE;
            break;
        case SAMPLE_S24LE:
            paSampleSpec.format = (pa_sample_format_t)PA_SAMPLE_S24LE;
            break;
        case SAMPLE_S32LE:
            paSampleSpec.format = (pa_sample_format_t)PA_SAMPLE_S32LE;
            break;
        default:
            paSampleSpec.format = (pa_sample_format_t)PA_SAMPLE_INVALID;
            break;
    }
    return paSampleSpec;
}


int32_t PaAdapterManager::GetInfo()
{
    AUDIO_INFO_LOG("pa_context_get_state(),: %{public}d, pa_context_errno(): %{public}d", pa_context_get_state(context_), pa_context_errno(context_));
    return 0;
}
} // namespace AudioStandard
} // namespace OHOS
