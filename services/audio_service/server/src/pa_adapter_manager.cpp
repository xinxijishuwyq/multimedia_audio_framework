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
const uint64_t BUF_LENGTH_IN_MSEC = 20;
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

int32_t PaAdapterManager::CreateRender(AudioProcessConfig processConfig, std::shared_ptr<IRendererStream> &stream)
{
    AUDIO_DEBUG_LOG("Create renderer start");
    if (context_ == nullptr) {
        AUDIO_INFO_LOG("Context is null, start to create context");
        int32_t ret = InitPaContext();
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Failed to init pa context");
    }

    pa_stream *paStream = InitPaStream(processConfig);
    std::shared_ptr<IRendererStream> rendererStream = CreateRendererStream(processConfig, paStream);
    CHECK_AND_RETURN_RET_LOG(rendererStream != nullptr, ERR_DEVICE_INIT, "Failed to init pa stream");
    stream = rendererStream;
    return SUCCESS;
}

int32_t PaAdapterManager::ReleaseRender(uint32_t streamIndex)
{
    AUDIO_DEBUG_LOG("Enter ReleaseRender");
    auto it = rendererStreamMap_.find(streamIndex);
    if (it == rendererStreamMap_.end()) {
        AUDIO_WARNING_LOG("No matching stream");
        return SUCCESS;
    }

    if (rendererStreamMap_[streamIndex]->Release() < 0) {
        AUDIO_WARNING_LOG("Release stream %{public}d failed", streamIndex);
        return ERR_OPERATION_FAILED;
    }
    rendererStreamMap_[streamIndex] = nullptr;
    rendererStreamMap_.erase(streamIndex);

    AUDIO_INFO_LOG("rendererStreamMap_.size() : %{public}zu", rendererStreamMap_.size());
    if (rendererStreamMap_.size() == 0) {
        AUDIO_INFO_LOG("Release the last stream");
        if (ResetPaContext() < 0) {
            AUDIO_ERR_LOG("Release pa context falied");
            return ERR_OPERATION_FAILED;
        }
    }
    return SUCCESS;
}

int32_t PaAdapterManager::CreateCapturer(AudioProcessConfig processConfig, std::shared_ptr<ICapturerStream> &stream)
{
    AUDIO_DEBUG_LOG("Create capturer start");
    if (context_ == nullptr) {
        AUDIO_INFO_LOG("Context is null, start to create context");
        int32_t ret = InitPaContext();
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Failed to init pa context");
    }
    pa_stream *paStream = InitPaStream(processConfig);
    std::shared_ptr<ICapturerStream> capturerStream = CreateCapturerStream(processConfig, paStream);
    CHECK_AND_RETURN_RET_LOG(capturerStream != nullptr, ERR_DEVICE_INIT, "Failed to init pa stream");
    stream = capturerStream;
    return SUCCESS;
}

int32_t PaAdapterManager::ReleaseCapturer(uint32_t streamIndex)
{
    AUDIO_DEBUG_LOG("Enter ReleaseCapturer");
    auto it = capturerStreamMap_.find(streamIndex);
    if (it == capturerStreamMap_.end()) {
        AUDIO_WARNING_LOG("No matching stream");
        return SUCCESS;
    }

    if (capturerStreamMap_[streamIndex]->Release() < 0) {
        AUDIO_WARNING_LOG("Release stream %{public}d failed", streamIndex);
        return ERR_OPERATION_FAILED;
    }

    capturerStreamMap_[streamIndex] = nullptr;
    capturerStreamMap_.erase(streamIndex);
    if (capturerStreamMap_.size() == 0) {
        AUDIO_INFO_LOG("Release the last stream");
        if (ResetPaContext() < 0) {
            AUDIO_ERR_LOG("Release pa context falied");
            return ERR_OPERATION_FAILED;
        }
    }
    return SUCCESS;
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
    return SUCCESS;
}

int32_t PaAdapterManager::InitPaContext()
{
    AUDIO_DEBUG_LOG("Enter InitPaContext");
    int error = ERROR;
    mainLoop_ = pa_threaded_mainloop_new();
    CHECK_AND_RETURN_RET_LOG(mainLoop_ != nullptr, ERR_DEVICE_INIT, "Failed to init pa mainLoop");
    api_ = pa_threaded_mainloop_get_api(mainLoop_);
    if (api_ == nullptr) {
        pa_threaded_mainloop_free(mainLoop_);
        AUDIO_ERR_LOG("Get api from mainLoop failed");
        return ERR_DEVICE_INIT;
    }

    std::stringstream ss;
    ss << "app-pid<" << getpid() << ">-uid<" << getuid() << ">";
    std::string packageName = "";
    ss >> packageName;

    context_ = pa_context_new(api_, packageName.c_str());
    if (context_ == nullptr) {
        pa_threaded_mainloop_free(mainLoop_);
        AUDIO_ERR_LOG("New context failed");
        return ERR_DEVICE_INIT;
    }

    pa_context_set_state_callback(context_, PAContextStateCb, mainLoop_);
    if (pa_context_connect(context_, nullptr, PA_CONTEXT_NOFAIL, nullptr) < 0) {
        error = pa_context_errno(context_);
        AUDIO_ERR_LOG("Context connect error: %{public}s", pa_strerror(error));
        return ERR_DEVICE_INIT;
    }
    isContextConnected_ = true;
    HandleMainLoopStart();

    pa_threaded_mainloop_unlock(mainLoop_);
    return SUCCESS;
}

int32_t PaAdapterManager::HandleMainLoopStart()
{
    int error = ERROR;
    pa_threaded_mainloop_lock(mainLoop_);
    if (pa_threaded_mainloop_start(mainLoop_) < 0) {
        pa_threaded_mainloop_unlock(mainLoop_);
        return ERR_DEVICE_INIT;
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
            return ERR_DEVICE_INIT;
        }
        pa_threaded_mainloop_wait(mainLoop_);
    }
    return SUCCESS;
}

pa_stream *PaAdapterManager::InitPaStream(AudioProcessConfig processConfig)
{
    AUDIO_DEBUG_LOG("Enter InitPaStream");
    int32_t error = ERROR;
    if (CheckReturnIfinvalid(mainLoop_ && context_, ERR_ILLEGAL_STATE) < 0) {
        return nullptr;
    }
    pa_threaded_mainloop_lock(mainLoop_);

    // Use struct to save spec size
    pa_sample_spec sampleSpec = ConvertToPAAudioParams(processConfig);
    pa_proplist *propList = pa_proplist_new();
    if (propList == nullptr) {
        AUDIO_ERR_LOG("pa_proplist_new failed");
        pa_threaded_mainloop_unlock(mainLoop_);
        return nullptr;
    }
    const std::string streamName = GetStreamName(processConfig.streamType);
    pa_channel_map map;
    CHECK_AND_RETURN_RET_LOG(SetPaProplist(propList, map, processConfig, streamName) == 0, nullptr,
        "set pa proplist failed");

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

int32_t PaAdapterManager::SetPaProplist(pa_proplist *propList, pa_channel_map &map, AudioProcessConfig &processConfig,
    const std::string &streamName)
{
    // for remote audio device router filter
    pa_proplist_sets(propList, "stream.client.uid", std::to_string(processConfig.appInfo.appUid).c_str());
    pa_proplist_sets(propList, "stream.client.pid", std::to_string(processConfig.appInfo.appPid).c_str());
    pa_proplist_sets(propList, "stream.type", streamName.c_str());
    pa_proplist_sets(propList, "media.name", streamName.c_str());
    const std::string effectSceneName = GetEffectSceneName(processConfig.streamType);
    pa_proplist_sets(propList, "scene.type", effectSceneName.c_str());
    float mVolumeFactor = 1.0f;
    float mPowerVolumeFactor = 1.0f;
    pa_proplist_sets(propList, "stream.volumeFactor", std::to_string(mVolumeFactor).c_str());
    pa_proplist_sets(propList, "stream.powerVolumeFactor", std::to_string(mPowerVolumeFactor).c_str());
    auto timenow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const std::string streamStartTime = ctime(&timenow);
    pa_proplist_sets(propList, "stream.startTime", streamStartTime.c_str());

    if (processConfig.audioMode == AUDIO_MODE_PLAYBACK) {
        AudioPrivacyType privacyType = PRIVACY_TYPE_PUBLIC;
        pa_proplist_sets(propList, "stream.privacyType", std::to_string(privacyType).c_str());
        pa_proplist_sets(propList, "stream.usage", std::to_string(processConfig.rendererInfo.streamUsage).c_str());
    } else if (processConfig.audioMode == AUDIO_MODE_RECORD) {
        pa_proplist_sets(propList, "stream.isInnerCapturer", std::to_string(processConfig.isInnerCapturer).c_str());
        pa_proplist_sets(propList, "stream.isWakeupCapturer", std::to_string(processConfig.isWakeupCapturer).c_str());
        pa_proplist_sets(propList, "stream.capturerSource",
            std::to_string(processConfig.capturerInfo.sourceType).c_str());
    }

    AUDIO_INFO_LOG("Creating stream of channels %{public}d", processConfig.streamInfo.channels);
    if (processConfig.streamInfo.channelLayout == 0) {
        processConfig.streamInfo.channelLayout = defaultChCountToLayoutMap[processConfig.streamInfo.channels];
    }
    pa_proplist_sets(propList, "stream.channelLayout", std::to_string(processConfig.streamInfo.channelLayout).c_str());

    pa_channel_map_init(&map);
    map.channels = processConfig.streamInfo.channels;
    uint32_t channelsInLayout = ConvertChLayoutToPaChMap(processConfig.streamInfo.channelLayout, map);
    if (channelsInLayout != processConfig.streamInfo.channels || channelsInLayout == 0) {
        AUDIO_ERR_LOG("Invalid channel Layout");
        return ERR_INVALID_PARAM;
    }
    return SUCCESS;
}

std::shared_ptr<IRendererStream> PaAdapterManager::CreateRendererStream(AudioProcessConfig processConfig,
    pa_stream *paStream)
{
    std::shared_ptr<PaRendererStreamImpl> rendererStream =
        std::make_shared<PaRendererStreamImpl>(paStream, processConfig, mainLoop_);
    if (rendererStream == nullptr) {
        AUDIO_ERR_LOG("Create rendererStream Failed");
        return nullptr;
    }
    uint32_t streamIndex = pa_stream_get_index(paStream);
    AUDIO_DEBUG_LOG("PaStream index is %{public}u", streamIndex);
    rendererStreamMap_[streamIndex] = rendererStream;
    return rendererStream;
}

std::shared_ptr<ICapturerStream> PaAdapterManager::CreateCapturerStream(AudioProcessConfig processConfig,
    pa_stream *paStream)
{
    std::shared_ptr<PaCapturerStreamImpl> capturerStream =
        std::make_shared<PaCapturerStreamImpl>(paStream,processConfig, mainLoop_);
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

    if (CheckReturnIfinvalid(mainLoop_ && context_ && paStream, ERROR) < 0) {
        return ERR_ILLEGAL_STATE;
    }

    pa_threaded_mainloop_lock(mainLoop_);
    if (managerType_ == PLAYBACK) {
        uint32_t tlength = 3;
        // GetSysPara("multimedia.audio.tlength", tlength);
        uint32_t maxlength = 4;
        // GetSysPara("multimedia.audio.maxlength", maxlength);
        uint32_t prebuf = 1;
        // GetSysPara("multimedia.audio.prebuf", prebuf);
        pa_buffer_attr bufferAttr;
        bufferAttr.fragsize = static_cast<uint32_t>(-1);
        bufferAttr.prebuf = pa_usec_to_bytes(BUF_LENGTH_IN_MSEC * PA_USEC_PER_MSEC * prebuf, &sampleSpec);
        bufferAttr.maxlength = pa_usec_to_bytes(BUF_LENGTH_IN_MSEC * PA_USEC_PER_MSEC * maxlength, &sampleSpec);
        bufferAttr.tlength = pa_usec_to_bytes(BUF_LENGTH_IN_MSEC * PA_USEC_PER_MSEC * tlength, &sampleSpec);
        bufferAttr.minreq = pa_usec_to_bytes(BUF_LENGTH_IN_MSEC * PA_USEC_PER_MSEC, &sampleSpec);
        AUDIO_INFO_LOG("bufferAttr, maxLength: %{public}u, tlength: %{public}u, prebuf: %{public}u",
            maxlength, tlength, prebuf);

        result = pa_stream_connect_playback(paStream, nullptr, &bufferAttr,
            (pa_stream_flags_t)(PA_STREAM_ADJUST_LATENCY | PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_START_CORKED |
            PA_STREAM_VARIABLE_RATE), nullptr, nullptr);

        if (result < 0) {
            error = pa_context_errno(context_);
            AUDIO_ERR_LOG("connection to stream error: %{public}d", error);
            pa_threaded_mainloop_unlock(mainLoop_);
            return ERR_INVALID_OPERATION;
        }
    } else {
        uint32_t fragsize = 0;
        GetSysPara("multimedia.audio.fragsize", fragsize);
        uint32_t maxlength = 0;
        GetSysPara("multimedia.audio.maxlength", maxlength);

        pa_buffer_attr bufferAttr;
        bufferAttr.maxlength = pa_usec_to_bytes(BUF_LENGTH_IN_MSEC * PA_USEC_PER_MSEC * maxlength, &sampleSpec);
        bufferAttr.fragsize = pa_usec_to_bytes(BUF_LENGTH_IN_MSEC * PA_USEC_PER_MSEC * fragsize, &sampleSpec);
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
            return ERR_INVALID_OPERATION;
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
            return ERR_INVALID_OPERATION;
        }
        pa_threaded_mainloop_wait(mainLoop_);
    }
    UpdateStreamIndexToPropList(paStream);
    pa_threaded_mainloop_unlock(mainLoop_);
    return SUCCESS;
}

void PaAdapterManager::UpdateStreamIndexToPropList(pa_stream *paStream)
{
    uint32_t paStreamIndex = pa_stream_get_index(paStream);
    pa_proplist *propListForUpdate = pa_proplist_new();
    if (propListForUpdate == nullptr) {
        AUDIO_ERR_LOG("pa_proplist_new failed");
        pa_threaded_mainloop_unlock(mainLoop_);
        return;
    }
    pa_proplist_sets(propListForUpdate, "stream.sessionID", std::to_string(paStreamIndex).c_str());
    pa_operation *updatePropOperation = pa_stream_proplist_update(paStream, PA_UPDATE_REPLACE, propListForUpdate,
        nullptr, nullptr);
    pa_proplist_free(propListForUpdate);
    pa_operation_unref(updatePropOperation);
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

pa_sample_spec PaAdapterManager::ConvertToPAAudioParams(AudioProcessConfig processConfig)
{
    pa_sample_spec paSampleSpec;
    paSampleSpec.channels = processConfig.streamInfo.channels;
    paSampleSpec.rate = processConfig.streamInfo.samplingRate;
    switch (processConfig.streamInfo.format) {
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


uint32_t PaAdapterManager::ConvertChLayoutToPaChMap(const uint64_t &channelLayout, pa_channel_map &paMap)
{
    uint32_t channelNum = 0;
    uint64_t mode = (channelLayout & CH_MODE_MASK) >> CH_MODE_OFFSET;
    switch (mode) {
        case 0: {
            for (auto bit = chSetToPaPositionMap.begin(); bit != chSetToPaPositionMap.end(); ++bit) {
                if ((channelLayout & (bit->first)) != 0) {
                    paMap.map[channelNum++] = bit->second;
                }
            }
            break;
        }
        case 1: {
            uint64_t order = (channelLayout & CH_HOA_ORDNUM_MASK) >> CH_HOA_ORDNUM_OFFSET;
            channelNum = (order + 1) * (order + 1);
            for (uint32_t i = 0; i < channelNum; ++i) {
                paMap.map[i] = chSetToPaPositionMap[FRONT_LEFT];
            }
            break;
        }
        default:
            channelNum = 0;
            break;
    }
    return channelNum;
}

const std::string PaAdapterManager::GetEffectSceneName(AudioStreamType audioType)
{
    std::string name;
    switch (audioType) {
        case STREAM_MUSIC:
            name = "SCENE_MUSIC";
            break;
        case STREAM_GAME:
            name = "SCENE_GAME";
            break;
        case STREAM_MOVIE:
            name = "SCENE_MOVIE";
            break;
        case STREAM_SPEECH:
        case STREAM_VOICE_CALL:
        case STREAM_VOICE_ASSISTANT:
            name = "SCENE_SPEECH";
            break;
        case STREAM_RING:
        case STREAM_ALARM:
        case STREAM_NOTIFICATION:
        case STREAM_SYSTEM:
        case STREAM_DTMF:
        case STREAM_SYSTEM_ENFORCED:
            name = "SCENE_RING";
            break;
        default:
            name = "SCENE_OTHERS";
    }

    const std::string sceneName = name;
    return sceneName;
}


int32_t PaAdapterManager::GetInfo()
{
    AUDIO_INFO_LOG("pa_context_get_state(),: %{public}d, pa_context_errno(): %{public}d",
        pa_context_get_state(context_), pa_context_errno(context_));
    return SUCCESS;
}
} // namespace AudioStandard
} // namespace OHOS
