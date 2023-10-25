/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "audio_service_client.h"

#include <fstream>
#include <sstream>

#include "iservice_registry.h"
#include "audio_log.h"
#include "audio_utils.h"
#include "hisysevent.h"
#include "securec.h"
#include "system_ability_definition.h"
#include "unistd.h"
#include "audio_errors.h"
#include "audio_info.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
AudioRendererCallbacks::~AudioRendererCallbacks() = default;
AudioCapturerCallbacks::~AudioCapturerCallbacks() = default;
const uint32_t CHECK_UTIL_SUCCESS = 0;
const uint32_t INIT_TIMEOUT_IN_SEC = 3;
const uint32_t DRAIN_TIMEOUT_IN_SEC = 3;
const uint32_t WRITE_TIMEOUT_IN_SEC = 2;
const uint32_t READ_TIMEOUT_IN_SEC = 5;
const uint32_t DOUBLE_VALUE = 2;
const uint32_t MAX_LENGTH_FACTOR = 5;
const uint32_t T_LENGTH_FACTOR = 4;
const uint32_t MAX_LENGTH_OFFLOAD = 5000;
const uint64_t MIN_BUF_DURATION_IN_USEC = 92880;
const uint32_t LATENCY_THRESHOLD = 35;
const int32_t NO_OF_PREBUF_TIMES = 6;
const int32_t OFFLOAD_HDI_CACHE1 = 100; // ms, should equal with val in hdi_sink.c
const int32_t OFFLOAD_HDI_CACHE2 = 2000; // ms, should equal with val in hdi_sink.c

static const string INNER_CAPTURER_SOURCE = "Speaker.monitor";

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

static int32_t CheckPaStatusIfinvalid(pa_threaded_mainloop *mainLoop, pa_context *context,
    pa_stream *paStream, const int32_t retVal)
{
    return CheckReturnIfinvalid(mainLoop && context &&
        paStream && PA_CONTEXT_IS_GOOD(pa_context_get_state(context)) &&
        PA_STREAM_IS_GOOD(pa_stream_get_state(paStream)), retVal);
}

static int32_t CheckPaStatusIfinvalid(pa_threaded_mainloop *mainLoop, pa_context *context,
    pa_stream *paStream, const int32_t retVal, int32_t &pError)
{
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, -1) < 0) {
        pError = pa_context_errno(context);
        return -1;
    }
    return retVal;
}

AudioStreamParams AudioServiceClient::ConvertFromPAAudioParams(pa_sample_spec paSampleSpec)
{
    AudioStreamParams audioParams;

    audioParams.channels = paSampleSpec.channels;
    audioParams.samplingRate = paSampleSpec.rate;
    audioParams.encoding = ENCODING_PCM;

    switch (paSampleSpec.format) {
        case PA_SAMPLE_U8:
            audioParams.format = SAMPLE_U8;
            break;
        case PA_SAMPLE_S16LE:
            audioParams.format = SAMPLE_S16LE;
            break;
        case PA_SAMPLE_S24LE:
            audioParams.format = SAMPLE_S24LE;
            break;
        case PA_SAMPLE_S32LE:
            audioParams.format = SAMPLE_S32LE;
            break;
        default:
            audioParams.format = INVALID_WIDTH;
            break;
    }

    return audioParams;
}

pa_sample_spec AudioServiceClient::ConvertToPAAudioParams(AudioStreamParams audioParams)
{
    pa_sample_spec paSampleSpec;

    paSampleSpec.channels = audioParams.channels;
    paSampleSpec.rate     = audioParams.samplingRate;

    switch ((AudioSampleFormat)audioParams.format) {
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

static size_t AlignToAudioFrameSize(size_t l, const pa_sample_spec &ss)
{
    size_t fs = pa_frame_size(&ss);
    if (fs == 0) {
        AUDIO_ERR_LOG(" Error: pa_frame_size returned  0");
        return 0;
    }

    return (l / fs) * fs;
}

void AudioServiceClient::PAStreamStartSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamStartSuccessCb: userdata is null");
        return;
    }

    AudioServiceClient *asClient = static_cast<AudioServiceClient *>(userdata);
    pa_threaded_mainloop *mainLoop = static_cast<pa_threaded_mainloop *>(asClient->mainLoop);

    asClient->state_ = RUNNING;
    asClient->breakingWritePa = false;
    asClient->WriteStateChangedSysEvents();
    std::shared_ptr<AudioStreamCallback> streamCb = asClient->streamCallback_.lock();
    if (streamCb != nullptr) {
        streamCb->OnStateChange(asClient->state_, asClient->stateChangeCmdType_);
    }
    asClient->stateChangeCmdType_ = CMD_FROM_CLIENT;
    asClient->streamCmdStatus_ = success;
    pa_threaded_mainloop_signal(mainLoop, 0);
}

void AudioServiceClient::PAStreamStopSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    AUDIO_DEBUG_LOG("PAStreamStopSuccessCb in");
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamStopSuccessCb: userdata is null");
        return;
    }

    AudioServiceClient *asClient = static_cast<AudioServiceClient *>(userdata);
    pa_threaded_mainloop *mainLoop = static_cast<pa_threaded_mainloop *>(asClient->mainLoop);

    asClient->state_ = STOPPED;
    asClient->WriteStateChangedSysEvents();
    std::shared_ptr<AudioStreamCallback> streamCb = asClient->streamCallback_.lock();
    if (streamCb != nullptr) {
        streamCb->OnStateChange(asClient->state_);
    }
    asClient->streamCmdStatus_ = success;
    AUDIO_DEBUG_LOG("PAStreamStopSuccessCb: start signal");
    pa_threaded_mainloop_signal(mainLoop, 0);
    AUDIO_DEBUG_LOG("PAStreamStopSuccessCb out");
}

void AudioServiceClient::PAStreamAsyncStopSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    AUDIO_DEBUG_LOG("PAStreamAsyncStopSuccessCb in");
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamAsyncStopSuccessCb: userdata is null");
        return;
    }

    AudioServiceClient *asClient = static_cast<AudioServiceClient *>(userdata);
    pa_threaded_mainloop *mainLoop = static_cast<pa_threaded_mainloop *>(asClient->mainLoop);
    unique_lock<mutex> lockstopping(asClient->stoppingMutex_);

    if (asClient->offloadEnable) {
        asClient->mAudioSystemMgr->Drain();
    }
    asClient->state_ = STOPPED;
    asClient->WriteStateChangedSysEvents();
    std::shared_ptr<AudioStreamCallback> streamCb = asClient->streamCallback_.lock();
    if (streamCb != nullptr) {
        streamCb->OnStateChange(asClient->state_);
    }
    asClient->streamCmdStatus_ = success;
    AUDIO_DEBUG_LOG("PAStreamAsyncStopSuccessCb: start signal");
    lockstopping.unlock();
    pa_threaded_mainloop_signal(mainLoop, 0);
    asClient->dataCv_.notify_one();
    AUDIO_DEBUG_LOG("PAStreamAsyncStopSuccessCb out");
}

void AudioServiceClient::PAStreamPauseSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamPauseSuccessCb: userdata is null");
        return;
    }

    AudioServiceClient *asClient = static_cast<AudioServiceClient *>(userdata);
    pa_threaded_mainloop *mainLoop = static_cast<pa_threaded_mainloop *>(asClient->mainLoop);

    asClient->state_ = PAUSED;
    asClient->breakingWritePa = false;
    asClient->WriteStateChangedSysEvents();
    std::shared_ptr<AudioStreamCallback> streamCb = asClient->streamCallback_.lock();
    if (streamCb != nullptr) {
        streamCb->OnStateChange(asClient->state_, asClient->stateChangeCmdType_);
    }
    asClient->stateChangeCmdType_ = CMD_FROM_CLIENT;
    asClient->streamCmdStatus_ = success;
    pa_threaded_mainloop_signal(mainLoop, 0);
}

void AudioServiceClient::PAStreamDrainSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamDrainSuccessCb: userdata is null");
        return;
    }

    AudioServiceClient *asClient = (AudioServiceClient *)userdata;
    pa_threaded_mainloop *mainLoop = (pa_threaded_mainloop *)asClient->mainLoop;

    asClient->streamDrainStatus_ = success;
    pa_threaded_mainloop_signal(mainLoop, 0);
}

void AudioServiceClient::PAStreamDrainInStopCb(pa_stream *stream, int32_t success, void *userdata)
{
    AudioServiceClient *asClient = (AudioServiceClient *)userdata;
    pa_operation *operation = pa_stream_cork(asClient->paStream, 1,
        AudioServiceClient::PAStreamAsyncStopSuccessCb, userdata);
    pa_operation_unref(operation);

    asClient->streamDrainStatus_ = success;
}

void AudioServiceClient::PAStreamFlushSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamFlushSuccessCb: userdata is null");
        return;
    }
    AudioServiceClient *asClient = (AudioServiceClient *)userdata;
    pa_threaded_mainloop *mainLoop = (pa_threaded_mainloop *)asClient->mainLoop;

    asClient->streamFlushStatus_ = success;
    pa_threaded_mainloop_signal(mainLoop, 0);
}

void AudioServiceClient::PAStreamSetBufAttrSuccessCb(pa_stream *stream, int32_t success, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamSetBufAttrSuccessCb: userdata is null");
        return;
    }
    AudioServiceClient *asClient = (AudioServiceClient *)userdata;
    pa_threaded_mainloop *mainLoop = (pa_threaded_mainloop *)asClient->mainLoop;

    AUDIO_DEBUG_LOG("SetBufAttr %{public}s", success ? "success" : "faild");

    const pa_buffer_attr *bufferAttr = pa_stream_get_buffer_attr(stream);
    asClient->acache_.totalCacheSizeTgt = bufferAttr->minreq;
    pa_threaded_mainloop_signal(mainLoop, 0);
}

int32_t AudioServiceClient::SetAudioRenderMode(AudioRenderMode renderMode)
{
    AUDIO_DEBUG_LOG("SetAudioRenderMode begin");
    renderMode_ = renderMode;

    if (renderMode_ != RENDER_MODE_CALLBACK) {
        return AUDIO_CLIENT_SUCCESS;
    }

    if (CheckReturnIfinvalid(mainLoop && context && paStream, AUDIO_CLIENT_ERR) < 0) {
        return AUDIO_CLIENT_ERR;
    }

    pa_threaded_mainloop_lock(mainLoop);
    pa_buffer_attr bufferAttr;
    bufferAttr.fragsize = static_cast<uint32_t>(-1);
    bufferAttr.prebuf = AlignToAudioFrameSize(pa_usec_to_bytes(MIN_BUF_DURATION_IN_USEC, &sampleSpec), sampleSpec);
    bufferAttr.maxlength = bufferAttr.prebuf * MAX_LENGTH_FACTOR;
    bufferAttr.tlength = bufferAttr.prebuf * T_LENGTH_FACTOR;
    bufferAttr.minreq = bufferAttr.prebuf;
    pa_operation *operation = pa_stream_set_buffer_attr(paStream, &bufferAttr,
        PAStreamSetBufAttrSuccessCb, (void *)this);
        while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING) {
        pa_threaded_mainloop_wait(mainLoop);
    }
    pa_operation_unref(operation);
    pa_threaded_mainloop_unlock(mainLoop);

    AUDIO_DEBUG_LOG("SetAudioRenderMode end");

    return AUDIO_CLIENT_SUCCESS;
}

AudioRenderMode AudioServiceClient::GetAudioRenderMode()
{
    return renderMode_;
}

int32_t AudioServiceClient::SetAudioCaptureMode(AudioCaptureMode captureMode)
{
    AUDIO_DEBUG_LOG("SetAudioCaptureMode.");
    captureMode_ = captureMode;

    return AUDIO_CLIENT_SUCCESS;
}

AudioCaptureMode AudioServiceClient::GetAudioCaptureMode()
{
    return captureMode_;
}

void AudioServiceClient::PAStreamWriteCb(pa_stream *stream, size_t length, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamWriteCb: userdata is null");
        return;
    }

    auto asClient = static_cast<AudioServiceClient *>(userdata);
    int64_t now = ClockTime::GetCurNano() / AUDIO_US_PER_SECOND;
    AUDIO_DEBUG_LOG("Inside PA write callback cost[%{public}" PRId64 "], sessionID %u",
        (now - asClient->mWriteCbStamp), asClient->sessionID_);
    asClient->mWriteCbStamp = now;
    auto mainLoop = static_cast<pa_threaded_mainloop *>(asClient->mainLoop);
    pa_threaded_mainloop_signal(mainLoop, 0);
}

void AudioServiceClient::PAStreamReadCb(pa_stream *stream, size_t length, void *userdata)
{
    AUDIO_DEBUG_LOG("PAStreamReadCb Inside PA read callback");
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamReadCb: userdata is null");
        return;
    }
    auto asClient = static_cast<AudioServiceClient *>(userdata);
    auto mainLoop = static_cast<pa_threaded_mainloop *>(asClient->mainLoop);
    pa_threaded_mainloop_signal(mainLoop, 0);
}

void AudioServiceClient::PAStreamUnderFlowCb(pa_stream *stream, void *userdata)
{
    Trace trace("PAStreamUnderFlow");
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamUnderFlowCb: userdata is null");
        return;
    }

    AudioServiceClient *asClient = (AudioServiceClient *)userdata;
    asClient->underFlowCount++;
    AUDIO_WARNING_LOG("AudioServiceClient underrun: %{public}d!", asClient->underFlowCount);
}

void AudioServiceClient::PAStreamEventCb(pa_stream *stream, const char *event, pa_proplist *pl, void *userdata)
{
    Trace trace("PAStreamUnderFlow");
    if (!userdata) {
        AUDIO_ERR_LOG("AudioServiceClient::PAEventCb: userdata is null");
        return;
    }

    AudioServiceClient *asClient = (AudioServiceClient *)userdata;
    if (strcmp(event, "signal_mainloop")) {
        pa_threaded_mainloop_signal(asClient->mainLoop, 0);
        AUDIO_DEBUG_LOG("AudioServiceClient::PAEventCb receive event signal_mainloop");
    }
}

void AudioServiceClient::PAStreamLatencyUpdateCb(pa_stream *stream, void *userdata)
{
    pa_threaded_mainloop *mainLoop = (pa_threaded_mainloop *)userdata;
    pa_threaded_mainloop_signal(mainLoop, 0);
}

void AudioServiceClient::PAStreamMovedCb(pa_stream *stream, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamMovedCb: userdata is null");
        return;
    }

    // get stream informations.
    uint32_t deviceIndex = pa_stream_get_device_index(stream); // pa_context_get_sink_info_by_index

    // Return 1 if the sink or source this stream is connected to has been suspended.
    // This will return 0 if not, and a negative value on error.
    int res = pa_stream_is_suspended(stream);
    AUDIO_DEBUG_LOG("PAstream moved to index:[%{public}d] suspended:[%{public}d]",
        deviceIndex, res);
}

void AudioServiceClient::PAStreamStateCb(pa_stream *stream, void *userdata)
{
    if (!userdata) {
        AUDIO_ERR_LOG("PAStreamStateCb: userdata is null");
        return;
    }

    AudioServiceClient *asClient = (AudioServiceClient *)userdata;
    pa_threaded_mainloop *mainLoop = (pa_threaded_mainloop *)asClient->mainLoop;

    if (asClient->mAudioRendererCallbacks)
        asClient->mAudioRendererCallbacks->OnStreamStateChangeCb();

    AUDIO_INFO_LOG("Current Stream State: %{public}d", pa_stream_get_state(stream));

    switch (pa_stream_get_state(stream)) {
        case PA_STREAM_READY:
        case PA_STREAM_FAILED:
        case PA_STREAM_TERMINATED:
            pa_threaded_mainloop_signal(mainLoop, 0);
            break;

        case PA_STREAM_UNCONNECTED:
        case PA_STREAM_CREATING:
        default:
            break;
    }
}

void AudioServiceClient::PAContextStateCb(pa_context *context, void *userdata)
{
    pa_threaded_mainloop *mainLoop = (pa_threaded_mainloop *)userdata;
    AUDIO_INFO_LOG("Current Context State: %{public}d", pa_context_get_state(context));

    switch (pa_context_get_state(context)) {
        case PA_CONTEXT_READY:
            AudioSystemManager::GetInstance()->RequestThreadPriority(gettid());
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

AudioServiceClient::AudioServiceClient()
    : AppExecFwk::EventHandler(AppExecFwk::EventRunner::Create("AudioServiceClientRunner"))
{
    isMainLoopStarted_ = false;
    isContextConnected_ = false;
    isStreamConnected_ = false;
    isInnerCapturerStream_ = false;

    sinkDevices.clear();
    sourceDevices.clear();
    sinkInputs.clear();
    sourceOutputs.clear();
    clientInfo.clear();

    mVolumeFactor = 1.0f;
    mPowerVolumeFactor = 1.0f;
    mStreamType = STREAM_MUSIC;
    mAudioSystemMgr = nullptr;

    streamIndex = 0;
    sessionID_ = 0;
    volumeChannels = STEREO;
    streamInfoUpdated = false;
    firstFrame_ = true;

    renderRate = RENDER_RATE_NORMAL;
    renderMode_ = RENDER_MODE_NORMAL;

    captureMode_ = CAPTURE_MODE_NORMAL;

    eAudioClientType = AUDIO_SERVICE_CLIENT_PLAYBACK;
    effectSceneName = "SCENE_MUSIC";
    effectMode = EFFECT_DEFAULT;

    mFrameSize = 0;
    mFrameMarkPosition = 0;
    mMarkReached = false;
    mFramePeriodNumber = 0;

    mTotalBytesWritten = 0;
    mFramePeriodWritten = 0;
    mTotalBytesRead = 0;
    mFramePeriodRead = 0;
    mRenderPositionCb = nullptr;
    mRenderPeriodPositionCb = nullptr;

    mAudioRendererCallbacks = nullptr;
    mAudioCapturerCallbacks = nullptr;
    internalReadBuffer_ = nullptr;
    mainLoop = nullptr;
    paStream = nullptr;
    context  = nullptr;
    api = nullptr;

    internalRdBufIndex_ = 0;
    internalRdBufLen_ = 0;
    streamCmdStatus_ = 0;
    streamDrainStatus_ = 0;
    streamFlushStatus_ = 0;
    underFlowCount = 0;

    acache_.readIndex = 0;
    acache_.writeIndex = 0;
    acache_.isFull = false;
    acache_.totalCacheSize = 0;
    acache_.totalCacheSizeTgt = 0;
    acache_.buffer = nullptr;

    setBufferSize_ = 0;
    PAStreamCorkSuccessCb = PAStreamStopSuccessCb;
    rendererSampleRate = 0;

    mPrivacyType = PRIVACY_TYPE_PUBLIC;
    mStreamUsage = STREAM_USAGE_UNKNOWN;
    streamClass_ = IAudioStream::StreamClass::PA_STREAM;
}

void AudioServiceClient::ResetPAAudioClient()
{
    AUDIO_INFO_LOG("Enter ResetPAAudioClient");
    lock_guard<mutex> lock(ctrlMutex_);
    if (mainLoop && (isMainLoopStarted_ == true))
        pa_threaded_mainloop_stop(mainLoop);

    if (paStream) {
        pa_stream_set_state_callback(paStream, nullptr, nullptr);
        pa_stream_set_write_callback(paStream, nullptr, nullptr);
        pa_stream_set_read_callback(paStream, nullptr, nullptr);
        pa_stream_set_latency_update_callback(paStream, nullptr, nullptr);
        pa_stream_set_underflow_callback(paStream, nullptr, nullptr);

        if (isStreamConnected_ == true) {
            pa_stream_disconnect(paStream);
            pa_stream_unref(paStream);
            isStreamConnected_  = false;
            paStream = nullptr;
        }
    }

    if (context) {
        pa_context_set_state_callback(context, nullptr, nullptr);
        if (isContextConnected_ == true) {
            pa_threaded_mainloop_lock(mainLoop);
            pa_context_disconnect(context);
            pa_context_unref(context);
            pa_threaded_mainloop_unlock(mainLoop);
            isContextConnected_ = false;
            context = nullptr;
        }
    }

    if (mainLoop) {
        pa_threaded_mainloop_free(mainLoop);
        isMainLoopStarted_  = false;
        mainLoop = nullptr;
    }

    for (auto &thread : mPositionCBThreads) {
        if (thread && thread->joinable()) {
            thread->join();
        }
    }

    for (auto &thread : mPeriodPositionCBThreads) {
        if (thread && thread->joinable()) {
            thread->join();
        }
    }

    if (appCookiePath.compare("")) {
        remove(appCookiePath.c_str());
        appCookiePath = "";
    }

    sinkDevices.clear();
    sourceDevices.clear();
    sinkInputs.clear();
    sourceOutputs.clear();
    clientInfo.clear();

    mAudioRendererCallbacks = nullptr;
    mAudioCapturerCallbacks = nullptr;
    internalReadBuffer_      = nullptr;

    api      = nullptr;

    internalRdBufIndex_ = 0;
    internalRdBufLen_   = 0;
    underFlowCount     = 0;

    acache_.buffer = nullptr;
    acache_.readIndex = 0;
    acache_.writeIndex = 0;
    acache_.isFull = false;
    acache_.totalCacheSize = 0;
    acache_.totalCacheSizeTgt = 0;

    setBufferSize_ = 0;
    PAStreamCorkSuccessCb = nullptr;
}

AudioServiceClient::~AudioServiceClient()
{
    lock_guard<mutex> lockdata(dataMutex_);
    AUDIO_INFO_LOG("start ~AudioServiceClient");
    ResetPAAudioClient();
    StopTimer();
}

void AudioServiceClient::SetEnv()
{
    AUDIO_INFO_LOG("SetEnv called");

    int ret = setenv("HOME", PA_HOME_DIR, 1);
    AUDIO_DEBUG_LOG("set env HOME: %{public}d", ret);

    ret = setenv("PULSE_RUNTIME_PATH", PA_RUNTIME_DIR, 1);
    AUDIO_DEBUG_LOG("set env PULSE_RUNTIME_DIR: %{public}d", ret);

    ret = setenv("PULSE_STATE_PATH", PA_STATE_DIR, 1);
    AUDIO_DEBUG_LOG("set env PULSE_STATE_PATH: %{public}d", ret);
}

void AudioServiceClient::SetApplicationCachePath(const std::string cachePath)
{
    AUDIO_DEBUG_LOG("SetApplicationCachePath in");

    char realPath[PATH_MAX] = {0x00};

    if ((strlen(cachePath.c_str()) >= PATH_MAX) || (realpath(cachePath.c_str(), realPath) == nullptr)) {
        AUDIO_ERR_LOG("Invalid cache path. err = %{public}d", errno);
        return;
    }

    cachePath_ = realPath;
}

bool AudioServiceClient::CheckRecordingCreate(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid)
{
    return AudioPolicyManager::GetInstance().CheckRecordingCreate(appTokenId, appFullTokenId, appUid);
}

bool AudioServiceClient::CheckRecordingStateChange(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid,
    AudioPermissionState state)
{
    return AudioPolicyManager::GetInstance().CheckRecordingStateChange(appTokenId, appFullTokenId, appUid, state);
}

int32_t AudioServiceClient::Initialize(ASClientType eClientType)
{
    int error = PA_ERR_INTERNAL;
    eAudioClientType = eClientType;

    mMarkReached = false;
    mTotalBytesWritten = 0;
    mFramePeriodWritten = 0;
    mTotalBytesRead = 0;
    mFramePeriodRead = 0;

    SetEnv();

    mAudioSystemMgr = AudioSystemManager::GetInstance();
    lock_guard<mutex> lockdata(dataMutex_);
    mainLoop = pa_threaded_mainloop_new();
    if (mainLoop == nullptr)
        return AUDIO_CLIENT_INIT_ERR;
    api = pa_threaded_mainloop_get_api(mainLoop);
    if (api == nullptr) {
        ResetPAAudioClient();
        return AUDIO_CLIENT_INIT_ERR;
    }
    stringstream ss;
    string packageName = "";
    ss << "app-pid<" << getpid() << ">-uid<" << getuid() << ">";
    ss >> packageName;
    AUDIO_DEBUG_LOG("AudioServiceClient:Initialize [%{public}s]", packageName.c_str());
    context = pa_context_new(api, packageName.c_str());
    if (context == nullptr) {
        ResetPAAudioClient();
        return AUDIO_CLIENT_INIT_ERR;
    }

    pa_context_set_state_callback(context, PAContextStateCb, mainLoop);

    if (pa_context_connect(context, nullptr, PA_CONTEXT_NOFAIL, nullptr) < 0) {
        error = pa_context_errno(context);
        AUDIO_ERR_LOG("context connect error: %{public}s", pa_strerror(error));
        ResetPAAudioClient();
        return AUDIO_CLIENT_INIT_ERR;
    }

    isContextConnected_ = true;
    pa_threaded_mainloop_lock(mainLoop);

    if (pa_threaded_mainloop_start(mainLoop) < 0) {
        pa_threaded_mainloop_unlock(mainLoop);
        ResetPAAudioClient();
        return AUDIO_CLIENT_INIT_ERR;
    }

    isMainLoopStarted_ = true;
    while (true) {
        pa_context_state_t state = pa_context_get_state(context);
        if (state == PA_CONTEXT_READY)
            break;

        if (!PA_CONTEXT_IS_GOOD(state)) {
            error = pa_context_errno(context);
            AUDIO_ERR_LOG("context bad state error: %{public}s", pa_strerror(error));
            pa_threaded_mainloop_unlock(mainLoop);
            ResetPAAudioClient();
            return AUDIO_CLIENT_INIT_ERR;
        }

        StartTimer(INIT_TIMEOUT_IN_SEC);
        pa_threaded_mainloop_wait(mainLoop);
        StopTimer();
        if (IsTimeOut()) {
            AUDIO_ERR_LOG("Initialize timeout");
            return AUDIO_CLIENT_INIT_ERR;
        }
    }

    if (appCookiePath.compare("")) {
        remove(appCookiePath.c_str());
        appCookiePath = "";
    }

    pa_threaded_mainloop_unlock(mainLoop);
    return AUDIO_CLIENT_SUCCESS;
}

const std::string AudioServiceClient::GetStreamName(AudioStreamType audioType)
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

std::pair<const int32_t, const std::string> AudioServiceClient::GetDeviceNameForConnect()
{
    string deviceName;
    if (eAudioClientType == AUDIO_SERVICE_CLIENT_PLAYBACK) {
        const std::string selectDevice =  AudioSystemManager::GetInstance()->GetSelectedDeviceInfo(clientUid_,
            clientPid_,
            mStreamType);
        deviceName = (selectDevice.empty() ? "" : selectDevice);
        return {AUDIO_CLIENT_SUCCESS, deviceName};
    }

    if (eAudioClientType == AUDIO_SERVICE_CLIENT_RECORD) {
        if (isInnerCapturerStream_) {
            return {AUDIO_CLIENT_SUCCESS, INNER_CAPTURER_SOURCE};
        }

        if (isWakeupCapturerStream_) {
            int32_t no = AudioPolicyManager::GetInstance().SetWakeUpAudioCapturer({});
            if (no < 0) {
                AUDIO_ERR_LOG("SetWakeUpAudioCapturer Error! ErrorCode: %{public}d", no);
                return {no, ""};
            }

            if (no >= WAKEUP_LIMIT) {
                AUDIO_ERR_LOG("SetWakeUpAudioCapturer Error! client no>=WAKEUP_LIMIT no=: %{public}d", no);
                return {AUDIO_CLIENT_CREATE_STREAM_ERR, ""};
            }

            if (no < WAKEUP_LIMIT) {
                deviceName = WAKEUP_NAMES[no];
                return {AUDIO_CLIENT_SUCCESS, deviceName};
            }
        }
    }
    return {AUDIO_CLIENT_SUCCESS, deviceName};
}

int32_t AudioServiceClient::ConnectStreamToPA()
{
    AUDIO_DEBUG_LOG("Enter AudioServiceClient::ConnectStreamToPA");
    int error, result;

    if (CheckReturnIfinvalid(mainLoop && context && paStream, AUDIO_CLIENT_ERR) < 0) {
        return AUDIO_CLIENT_ERR;
    }
    int32_t latency_in_msec = AudioSystemManager::GetInstance()->GetAudioLatencyFromXml();
    if (latency_in_msec < 0) {
        AUDIO_ERR_LOG("Get audio latency failed.");
        return AUDIO_CLIENT_CREATE_STREAM_ERR;
    }
    sinkLatencyInMsec_ = AudioSystemManager::GetInstance()->GetSinkLatencyFromXml();

    auto [errorCode, deviceNameS] = GetDeviceNameForConnect();
    if (errorCode != AUDIO_CLIENT_SUCCESS) {
        return errorCode;
    }

    const char* deviceName = deviceNameS.empty() ? nullptr : deviceNameS.c_str();

    pa_threaded_mainloop_lock(mainLoop);

    pa_buffer_attr bufferAttr;
    bufferAttr.fragsize = static_cast<uint32_t>(-1);
    if (latency_in_msec <= LATENCY_THRESHOLD) {
        bufferAttr.prebuf = AlignToAudioFrameSize(pa_usec_to_bytes(latency_in_msec * PA_USEC_PER_MSEC, &sampleSpec),
                                                  sampleSpec);
        bufferAttr.maxlength =  NO_OF_PREBUF_TIMES * bufferAttr.prebuf;
        bufferAttr.tlength = static_cast<uint32_t>(-1);
    } else {
        bufferAttr.prebuf = pa_usec_to_bytes(latency_in_msec * PA_USEC_PER_MSEC, &sampleSpec);
        bufferAttr.maxlength = pa_usec_to_bytes(latency_in_msec * PA_USEC_PER_MSEC * MAX_LENGTH_FACTOR, &sampleSpec);
        bufferAttr.tlength = pa_usec_to_bytes(latency_in_msec * PA_USEC_PER_MSEC * T_LENGTH_FACTOR, &sampleSpec);
    }
    bufferAttr.minreq = bufferAttr.prebuf;

    if (eAudioClientType == AUDIO_SERVICE_CLIENT_PLAYBACK) {
        result = pa_stream_connect_playback(paStream, deviceName, &bufferAttr,
            (pa_stream_flags_t)(PA_STREAM_ADJUST_LATENCY | PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_START_CORKED |
            PA_STREAM_VARIABLE_RATE), nullptr, nullptr);
        preBuf_ = make_unique<uint8_t[]>(bufferAttr.maxlength);
        if (preBuf_ == nullptr) {
            AUDIO_ERR_LOG("Allocate memory for buffer failed.");
            pa_threaded_mainloop_unlock(mainLoop);
            return AUDIO_CLIENT_INIT_ERR;
        }
        if (memset_s(preBuf_.get(), bufferAttr.maxlength, 0, bufferAttr.maxlength) != 0) {
            AUDIO_ERR_LOG("memset_s for buffer failed.");
            pa_threaded_mainloop_unlock(mainLoop);
            return AUDIO_CLIENT_INIT_ERR;
        }
    } else {
        AUDIO_DEBUG_LOG("pa_stream_connect_record connect to:%{public}s",
            deviceName ? deviceName : "nullptr");
        result = pa_stream_connect_record(paStream, deviceName, nullptr,
                                          (pa_stream_flags_t)(PA_STREAM_INTERPOLATE_TIMING
                                          | PA_STREAM_ADJUST_LATENCY
                                          | PA_STREAM_START_CORKED
                                          | PA_STREAM_AUTO_TIMING_UPDATE));
    }
    if (result < 0) {
        error = pa_context_errno(context);
        AUDIO_ERR_LOG("connection to stream error: %{public}d", error);
        pa_threaded_mainloop_unlock(mainLoop);
        ResetPAAudioClient();
        return AUDIO_CLIENT_CREATE_STREAM_ERR;
    }

    while (true) {
        pa_stream_state_t state = pa_stream_get_state(paStream);
        if (state == PA_STREAM_READY)
            break;

        if (!PA_STREAM_IS_GOOD(state)) {
            error = pa_context_errno(context);
            pa_threaded_mainloop_unlock(mainLoop);
            AUDIO_ERR_LOG("connection to stream error: %{public}d", error);
            ResetPAAudioClient();
            return AUDIO_CLIENT_CREATE_STREAM_ERR;
        }

        pa_threaded_mainloop_wait(mainLoop);
    }

    isStreamConnected_ = true;
    pa_threaded_mainloop_unlock(mainLoop);
    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::InitializeAudioCache()
{
    AUDIO_DEBUG_LOG("Initializing internal audio cache");

    if (CheckReturnIfinvalid(mainLoop && context && paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        return AUDIO_CLIENT_PA_ERR;
    }

    const pa_buffer_attr *bufferAttr = pa_stream_get_buffer_attr(paStream);
    if (bufferAttr == nullptr) {
        AUDIO_ERR_LOG("pa stream get buffer attribute returned null");
        return AUDIO_CLIENT_INIT_ERR;
    }

    acache_.buffer = make_unique<uint8_t[]>(max((uint32_t)bufferAttr->minreq,
        (uint32_t)pa_usec_to_bytes(200 * PA_USEC_PER_MSEC, &sampleSpec)));
    if (acache_.buffer == nullptr) {
        AUDIO_ERR_LOG("Allocate memory for buffer failed");
        return AUDIO_CLIENT_INIT_ERR;
    }

    acache_.readIndex = 0;
    acache_.writeIndex = 0;
    acache_.totalCacheSize = bufferAttr->minreq;
    acache_.totalCacheSizeTgt = bufferAttr->minreq;
    acache_.isFull = false;
    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::CreateStream(AudioStreamParams audioParams, AudioStreamType audioType)
{
    AUDIO_INFO_LOG("Enter AudioServiceClient::CreateStream");
    int error;
    lock_guard<mutex> lockdata(dataMutex_);
    if (CheckReturnIfinvalid(mainLoop && context, AUDIO_CLIENT_ERR) < 0) {
        return AUDIO_CLIENT_ERR;
    }

    if (eAudioClientType == AUDIO_SERVICE_CLIENT_CONTROLLER) {
        return AUDIO_CLIENT_INVALID_PARAMS_ERR;
    }
    pa_threaded_mainloop_lock(mainLoop);
    mStreamType = audioType;
    const std::string streamName = GetStreamName(audioType);

    auto timenow = chrono::system_clock::to_time_t(chrono::system_clock::now());
    const std::string streamStartTime = ctime(&timenow);

    sampleSpec = ConvertToPAAudioParams(audioParams);
    mFrameSize = pa_frame_size(&sampleSpec);
    InitializebufferAttrOffload();
    mChannelLayout = audioParams.channelLayout;

    pa_proplist *propList = pa_proplist_new();
    if (propList == nullptr) {
        AUDIO_ERR_LOG("pa_proplist_new failed");
        ResetPAAudioClient();
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_CREATE_STREAM_ERR;
    }

    // for remote audio device router filter.
    pa_proplist_sets(propList, "stream.client.uid", std::to_string(clientUid_).c_str());
    pa_proplist_sets(propList, "stream.client.pid", std::to_string(clientPid_).c_str());

    pa_proplist_sets(propList, "stream.type", streamName.c_str());
    pa_proplist_sets(propList, "stream.volumeFactor", std::to_string(mVolumeFactor).c_str());
    pa_proplist_sets(propList, "stream.powerVolumeFactor", std::to_string(mPowerVolumeFactor).c_str());
    sessionID_ = pa_context_get_index(context);
    pa_proplist_sets(propList, "stream.sessionID", std::to_string(sessionID_).c_str());
    pa_proplist_sets(propList, "stream.startTime", streamStartTime.c_str());
    offloadEnable = false;
    pa_proplist_sets(propList, "stream.offload.statePolicy", "0");

    if (eAudioClientType == AUDIO_SERVICE_CLIENT_RECORD) {
        pa_proplist_sets(propList, "stream.isInnerCapturer", std::to_string(isInnerCapturerStream_).c_str());
        pa_proplist_sets(propList, "stream.isWakeupCapturer", std::to_string(isWakeupCapturerStream_).c_str());
    } else if (eAudioClientType == AUDIO_SERVICE_CLIENT_PLAYBACK) {
        pa_proplist_sets(propList, "stream.privacyType", std::to_string(mPrivacyType).c_str());
        pa_proplist_sets(propList, "stream.usage", std::to_string(mStreamUsage).c_str());
    }

    AUDIO_DEBUG_LOG("Creating stream of channels %{public}d", audioParams.channels);
    if (audioParams.channelLayout == 0) {
        audioParams.channelLayout = defaultChCountToLayoutMap_[audioParams.channels];
    }
    pa_proplist_sets(propList, "stream.channelLayout", std::to_string(audioParams.channelLayout).c_str());
    pa_channel_map map;
    pa_channel_map_init(&map);
    map.channels = audioParams.channels;
    uint32_t channelsInLayout = ConvertChLayoutToPaChMap(audioParams.channelLayout, map);
    if (channelsInLayout != audioParams.channels || channelsInLayout == 0) {
        AUDIO_ERR_LOG("Invalid channel Layout");
        return AUDIO_CLIENT_CREATE_STREAM_ERR;
    }
    paStream = pa_stream_new_with_proplist(context, streamName.c_str(), &sampleSpec, &map, propList);
    if (!paStream) {
        error = pa_context_errno(context);
        AUDIO_ERR_LOG("create stream Failed, error: %{public}d", error);
        pa_proplist_free(propList);
        pa_threaded_mainloop_unlock(mainLoop);
        ResetPAAudioClient();
        AUDIO_ERR_LOG("pa_stream_new_with_proplist failed, error: %{public}d", error);
        return AUDIO_CLIENT_CREATE_STREAM_ERR;
    }

    pa_proplist_free(propList);
    pa_stream_set_state_callback(paStream, PAStreamStateCb, (void *)this);
    pa_stream_set_moved_callback(paStream, PAStreamMovedCb, (void *)this); // used to notify sink/source moved
    pa_stream_set_write_callback(paStream, PAStreamWriteCb, (void *)this);
    pa_stream_set_read_callback(paStream, PAStreamReadCb, (void *)this);
    pa_stream_set_latency_update_callback(paStream, PAStreamLatencyUpdateCb, mainLoop);
    pa_stream_set_underflow_callback(paStream, PAStreamUnderFlowCb, (void *)this);
    pa_stream_set_event_callback(paStream, PAStreamEventCb, (void *)this);

    pa_threaded_mainloop_unlock(mainLoop);

    error = ConnectStreamToPA();
    streamInfoUpdated = false;
    if (error < 0) {
        AUDIO_ERR_LOG("Create Stream Failed");
        ResetPAAudioClient();
        return AUDIO_CLIENT_CREATE_STREAM_ERR;
    }

    if (eAudioClientType == AUDIO_SERVICE_CLIENT_PLAYBACK) {
        error = InitializeAudioCache();
        if (error < 0) {
            AUDIO_ERR_LOG("Initialize audio cache failed");
            ResetPAAudioClient();
            return AUDIO_CLIENT_CREATE_STREAM_ERR;
        }

        if (SetStreamRenderRate(renderRate) != AUDIO_CLIENT_SUCCESS) {
            AUDIO_ERR_LOG("Set render rate failed");
        }

        effectSceneName = IAudioStream::GetEffectSceneName(audioType);
        if (SetStreamAudioEffectMode(effectMode) != AUDIO_CLIENT_SUCCESS) {
            AUDIO_ERR_LOG("Set audio effect mode failed");
        }
    }

    state_ = PREPARED;
    WriteStateChangedSysEvents();
    std::shared_ptr<AudioStreamCallback> streamCb = streamCallback_.lock();
    if (streamCb != nullptr) {
        streamCb->OnStateChange(state_);
    }
    return AUDIO_CLIENT_SUCCESS;
}

uint32_t AudioServiceClient::GetUnderflowCount()
{
    return underFlowCount;
}

int32_t AudioServiceClient::GetSessionID(uint32_t &sessionID) const
{
    AUDIO_DEBUG_LOG("GetSessionID sessionID: %{public}d", sessionID_);
    if (sessionID_ == PA_INVALID_INDEX || sessionID_ == 0) {
        return AUDIO_CLIENT_ERR;
    }
    sessionID = sessionID_;
    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::StartStream(StateChangeCmdType cmdType)
{
    AUDIO_INFO_LOG("Enter AudioServiceClient::StartStream");
    int error;
    lock_guard<mutex> lockdata(dataMutex_);
    unique_lock<mutex> stoppinglock(stoppingMutex_);

    // wait 1 second, timeout return error
    if (!dataCv_.wait_for(stoppinglock, 1s, [this] {return state_ != STOPPING;})) {
        AUDIO_ERR_LOG("StartStream: wait stopping timeout");
        return AUDIO_CLIENT_START_STREAM_ERR;
    }
    stoppinglock.unlock();
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        return AUDIO_CLIENT_PA_ERR;
    }
    pa_operation *operation = nullptr;

    pa_threaded_mainloop_lock(mainLoop);

    pa_stream_state_t state = pa_stream_get_state(paStream);
    if (state != PA_STREAM_READY) {
        error = pa_context_errno(context);
        pa_threaded_mainloop_unlock(mainLoop);
        AUDIO_ERR_LOG("Stream Start Failed, error: %{public}d", error);
        ResetPAAudioClient();
        return AUDIO_CLIENT_START_STREAM_ERR;
    }

    streamCmdStatus_ = 0;
    stateChangeCmdType_ = cmdType;
    operation = pa_stream_cork(paStream, 0, PAStreamStartSuccessCb, (void *)this);

    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING) {
        pa_threaded_mainloop_wait(mainLoop);
    }
    pa_operation_unref(operation);
    pa_threaded_mainloop_unlock(mainLoop);

    if (!streamCmdStatus_) {
        AUDIO_ERR_LOG("Stream Start Failed");
        ResetPAAudioClient();
        return AUDIO_CLIENT_START_STREAM_ERR;
    } else {
        return AUDIO_CLIENT_SUCCESS;
    }
}

int32_t AudioServiceClient::PauseStream(StateChangeCmdType cmdType)
{
    AUDIO_INFO_LOG("Enter AudioServiceClient::PauseStream");
    breakingWritePa = true;
    pa_threaded_mainloop_signal(mainLoop, 0);
    lock_guard<mutex> lockdata(dataMutex_);
    lock_guard<mutex> lockctrl(ctrlMutex_);
    PAStreamCorkSuccessCb = PAStreamPauseSuccessCb;
    stateChangeCmdType_ = cmdType;

    int32_t ret = CorkStream();
    breakingWritePa = false;
    if (ret) {
        return ret;
    }

    if (!streamCmdStatus_) {
        AUDIO_ERR_LOG("Stream Pause Failed");
        return AUDIO_CLIENT_ERR;
    } else {
        return AUDIO_CLIENT_SUCCESS;
    }
}

int32_t AudioServiceClient::StopStream()
{
    AUDIO_INFO_LOG("Enter AudioServiceClient::StopStream");
    lock_guard<mutex> lockdata(dataMutex_);
    lock_guard<mutex> lockctrl(ctrlMutex_);
    if (eAudioClientType == AUDIO_SERVICE_CLIENT_PLAYBACK) {
        state_ = STOPPING;
        DrainAudioCache();

        if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
            return AUDIO_CLIENT_PA_ERR;
        }

        if (!offloadEnable) {
            pa_threaded_mainloop_lock(mainLoop);

            streamDrainStatus_ = 0;
            pa_operation *operation = pa_stream_drain(paStream, PAStreamDrainInStopCb, (void *)this);

            if (operation == nullptr) {
                pa_threaded_mainloop_unlock(mainLoop);
                AUDIO_ERR_LOG("pa_stream_drain operation is null");
                return AUDIO_CLIENT_ERR;
            }

            pa_operation_unref(operation);
            pa_threaded_mainloop_unlock(mainLoop);
        } else {
            PAStreamCorkSuccessCb = PAStreamStopSuccessCb;
            int32_t ret = CorkStream();
            if (ret) {
                return ret;
            }
        }
        return AUDIO_CLIENT_SUCCESS;
    } else {
        PAStreamCorkSuccessCb = PAStreamStopSuccessCb;
        int32_t ret = CorkStream();
        if (ret) {
            return ret;
        }

        if (!streamCmdStatus_) {
            AUDIO_ERR_LOG("Stream Stop Failed");
            return AUDIO_CLIENT_ERR;
        } else {
            if (internalRdBufLen_) {
                (void)pa_stream_drop(paStream);
                internalReadBuffer_ = nullptr;
                internalRdBufLen_ = 0;
                internalRdBufIndex_ = 0;
            }
            return AUDIO_CLIENT_SUCCESS;
        }
    }
}

int32_t AudioServiceClient::CorkStream()
{
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        return AUDIO_CLIENT_PA_ERR;
    }

    pa_operation *operation = nullptr;

    pa_threaded_mainloop_lock(mainLoop);
    pa_stream_state_t state = pa_stream_get_state(paStream);
    if (state != PA_STREAM_READY) {
        int32_t error = pa_context_errno(context);
        pa_threaded_mainloop_unlock(mainLoop);
        AUDIO_ERR_LOG("Stream Stop Failed : %{public}d", error);
        return AUDIO_CLIENT_ERR;
    }

    streamCmdStatus_ = 0;
    operation = pa_stream_cork(paStream, 1, PAStreamCorkSuccessCb, (void *)this);

    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING) {
        pa_threaded_mainloop_wait(mainLoop);
    }
    pa_operation_unref(operation);
    if (InitializePAProbListOffload() != AUDIO_CLIENT_SUCCESS) {
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_ERR;
    }
    pa_threaded_mainloop_unlock(mainLoop);

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::FlushStream()
{
    AUDIO_INFO_LOG("Enter AudioServiceClient::FlushStream");
    lock_guard<mutex> lock(dataMutex_);
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        return AUDIO_CLIENT_PA_ERR;
    }

    pa_operation *operation = nullptr;
    pa_threaded_mainloop_lock(mainLoop);

    pa_stream_state_t state = pa_stream_get_state(paStream);
    if (state != PA_STREAM_READY) {
        int error = pa_context_errno(context);
        pa_threaded_mainloop_unlock(mainLoop);
        AUDIO_ERR_LOG("Stream Flush Failed, error: %{public}d", error);
        return AUDIO_CLIENT_ERR;
    }

    streamFlushStatus_ = 0;
    operation = pa_stream_flush(paStream, PAStreamFlushSuccessCb, (void *)this);
    if (operation == nullptr) {
        AUDIO_ERR_LOG("Stream Flush Operation Failed");
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_ERR;
    }

    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING) {
        pa_threaded_mainloop_wait(mainLoop);
    }
    pa_operation_unref(operation);
    pa_threaded_mainloop_unlock(mainLoop);

    if (!streamFlushStatus_) {
        AUDIO_ERR_LOG("Stream Flush Failed");
        return AUDIO_CLIENT_ERR;
    } else {
        acache_.readIndex = 0;
        acache_.writeIndex = 0;
        acache_.isFull = false;
        return AUDIO_CLIENT_SUCCESS;
    }
}

int32_t AudioServiceClient::DrainStream()
{
    AUDIO_INFO_LOG("Enter AudioServiceClient::DrainStream");
    int32_t error;

    if (eAudioClientType != AUDIO_SERVICE_CLIENT_PLAYBACK) {
        AUDIO_ERR_LOG("Drain is not supported");
        return AUDIO_CLIENT_ERR;
    }

    lock_guard<mutex> lock(dataMutex_);

    error = DrainAudioCache();
    if (error != AUDIO_CLIENT_SUCCESS) {
        AUDIO_ERR_LOG("Audio cache drain failed");
        return AUDIO_CLIENT_ERR;
    }
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        return AUDIO_CLIENT_PA_ERR;
    }

    pa_operation *operation = nullptr;

    pa_threaded_mainloop_lock(mainLoop);

    pa_stream_state_t state = pa_stream_get_state(paStream);
    if (state != PA_STREAM_READY) {
        error = pa_context_errno(context);
        pa_threaded_mainloop_unlock(mainLoop);
        AUDIO_ERR_LOG("Stream Drain Failed");
        return AUDIO_CLIENT_ERR;
    }

    streamDrainStatus_ = 0;
    operation = pa_stream_drain(paStream, PAStreamDrainSuccessCb, (void *)this);

    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING) {
        StartTimer(DRAIN_TIMEOUT_IN_SEC);
        pa_threaded_mainloop_wait(mainLoop);
        StopTimer();
        if (IsTimeOut()) {
            AUDIO_ERR_LOG("Drain timeout");
            return AUDIO_CLIENT_ERR;
        }
    }
    pa_operation_unref(operation);
    pa_threaded_mainloop_unlock(mainLoop);

    if (!streamDrainStatus_) {
        AUDIO_ERR_LOG("Stream Drain Failed");
        return AUDIO_CLIENT_ERR;
    } else {
        return AUDIO_CLIENT_SUCCESS;
    }
}

int32_t AudioServiceClient::PaWriteStream(const uint8_t *buffer, size_t &length)
{
    int error = 0;
    if (firstFrame_) {
        AudioSystemManager::GetInstance()->RequestThreadPriority(gettid());
        firstFrame_ = false;
    }

    if ((lastOffloadUpdateFinishTime != 0) &&
        (chrono::system_clock::to_time_t(chrono::system_clock::now()) > lastOffloadUpdateFinishTime)) {
            AUDIO_INFO_LOG("PaWriteStream switching curTime %ld, switchTime %ld",
                chrono::system_clock::to_time_t(chrono::system_clock::now()), lastOffloadUpdateFinishTime);
        
        error = UpdatePolicyOffload(offloadNextStateTargetPolicy);
        lastOffloadUpdateFinishTime = 0;
    }
    if (error != AUDIO_CLIENT_SUCCESS) {
        return error;
    }

    while (length > 0) {
        size_t writableSize;
        size_t origWritableSize = 0;

        Trace trace1("PaWriteStream Wait");
        while (!(writableSize = pa_stream_writable_size(paStream))) {
            AUDIO_DEBUG_LOG("PaWriteStream: wait");
            StartTimer(WRITE_TIMEOUT_IN_SEC);
            if (!breakingWritePa && state_ == RUNNING) {
                pa_threaded_mainloop_wait(mainLoop);
            }
            pa_threaded_mainloop_wait(mainLoop);
            StopTimer();
            if (IsTimeOut()) {
                AUDIO_ERR_LOG("Write timeout");
                if (!offloadEnable) {
                    error = AUDIO_CLIENT_WRITE_STREAM_ERR;
                    return error;
                }
            }
            if (breakingWritePa || state_ != RUNNING) {
                AUDIO_WARNING_LOG("AudioServiceClient::PaWriteStream: breakingWritePa(%d) or state_(%d) not running",
                    breakingWritePa, state_);
                AUDIO_WARNING_LOG("Writable size: %{public}zu, bytes to write: %{public}zu, return val: %{public}d",
                    writableSize, length, error);
                return AUDIO_CLIENT_WRITE_STREAM_ERR;
            }
        }

        AUDIO_DEBUG_LOG("Write stream: writable size = %{public}zu, length = %{public}zu", writableSize, length);
        origWritableSize = writableSize;
        if (writableSize > length) {
            writableSize = length;
        }

        writableSize = AlignToAudioFrameSize(writableSize, sampleSpec);
        if (writableSize == 0) {
            AUDIO_ERR_LOG("Align to frame size failed");
            error = AUDIO_CLIENT_WRITE_STREAM_ERR;
            break;
        }

        Trace trace2("PaWriteStream Write:" + std::to_string(writableSize) + "/" + std::to_string(origWritableSize));
        error = pa_stream_write(paStream, (void *)buffer, writableSize, nullptr, 0LL,
                                PA_SEEK_RELATIVE);
        if (error < 0) {
            AUDIO_ERR_LOG("Write stream failed");
            error = AUDIO_CLIENT_WRITE_STREAM_ERR;
            break;
        }

        AUDIO_DEBUG_LOG("Writable size: %{public}zu, bytes to write: %{public}zu, return val: %{public}d",
                        writableSize, length, error);
        buffer = buffer + writableSize;
        length -= writableSize;

        HandleRenderPositionCallbacks(writableSize);
    }

    return error;
}

void AudioServiceClient::HandleRenderPositionCallbacks(size_t bytesWritten)
{
    mTotalBytesWritten += bytesWritten;
    if (mFrameSize == 0) {
        AUDIO_ERR_LOG("HandleRenderPositionCallbacks: capturePeriodPositionCb not set");
        return;
    }

    uint64_t writtenFrameNumber = mTotalBytesWritten / mFrameSize;
    AUDIO_DEBUG_LOG("frame size: %{public}d", mFrameSize);

    {
        std::lock_guard<std::mutex> lock(rendererMarkReachedMutex_);
        if (!mMarkReached) {
            AUDIO_DEBUG_LOG("frame mark position: %{public}" PRIu64 ", Total frames written: %{public}" PRIu64,
                static_cast<uint64_t>(mFrameMarkPosition), static_cast<uint64_t>(writtenFrameNumber));
            if (writtenFrameNumber >= mFrameMarkPosition) {
                AUDIO_DEBUG_LOG("audio service client OnMarkReached");
                SendRenderMarkReachedRequestEvent(mFrameMarkPosition);
                mMarkReached = true;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(rendererPeriodReachedMutex_);
        mFramePeriodWritten += (bytesWritten / mFrameSize);
        AUDIO_DEBUG_LOG("frame period number: %{public}" PRIu64 ", Total frames written: %{public}" PRIu64,
            static_cast<uint64_t>(mFramePeriodNumber), static_cast<uint64_t>(writtenFrameNumber));
        if (mFramePeriodWritten >= mFramePeriodNumber) {
            mFramePeriodWritten %= mFramePeriodNumber;
            AUDIO_DEBUG_LOG("OnPeriodReached, remaining frames: %{public}" PRIu64,
                static_cast<uint64_t>(mFramePeriodWritten));
            SendRenderPeriodReachedRequestEvent(mFramePeriodNumber);
        }
    }
}

int32_t AudioServiceClient::DrainAudioCache()
{
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        return AUDIO_CLIENT_PA_ERR;
    }

    pa_threaded_mainloop_lock(mainLoop);

    int32_t error = 0;
    if (acache_.buffer == nullptr) {
        AUDIO_ERR_LOG("Drain cache failed");
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_ERR;
    }

    size_t length = acache_.writeIndex - acache_.readIndex;
    const uint8_t *buffer = acache_.buffer.get();

    error = PaWriteStream(buffer, length);

    acache_.readIndex = 0;
    acache_.writeIndex = 0;

    pa_threaded_mainloop_unlock(mainLoop);
    return error;
}

size_t AudioServiceClient::WriteToAudioCache(const StreamBuffer &stream)
{
    if (stream.buffer == nullptr) {
        return 0;
    }

    const uint8_t *inputBuffer = stream.buffer;
    uint8_t *cacheBuffer = acache_.buffer.get() + acache_.writeIndex;

    size_t inputLen = stream.bufferLen;
    if (acache_.totalCacheSize != acache_.totalCacheSizeTgt) {
        uint32_t tgt = acache_.totalCacheSizeTgt;
        if (tgt < acache_.totalCacheSize && tgt <acache_.writeIndex) {
            tgt = acache_.writeIndex;
        }
        acache_.totalCacheSize = tgt;
    }
    while (inputLen > 0) {
        size_t writableSize = acache_.totalCacheSize - acache_.writeIndex;

        if (writableSize > inputLen) {
            writableSize = inputLen;
        }

        if (writableSize == 0) {
            break;
        }

        if (memcpy_s(cacheBuffer, acache_.totalCacheSize, inputBuffer, writableSize)) {
            break;
        }

        inputBuffer = inputBuffer + writableSize;
        cacheBuffer = cacheBuffer + writableSize;
        inputLen -= writableSize;
        acache_.writeIndex += writableSize;
    }

    if ((acache_.writeIndex - acache_.readIndex) == acache_.totalCacheSize) {
        acache_.isFull = true;
    }

    return (stream.bufferLen - inputLen);
}

size_t AudioServiceClient::WriteStreamInCb(const StreamBuffer &stream, int32_t &pError)
{
    lock_guard<mutex> lock(dataMutex_);
    int error = 0;
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, 0, pError) < 0) {
        return 0;
    }

    pa_threaded_mainloop_lock(mainLoop);

    const uint8_t *buffer = stream.buffer;
    size_t length = stream.bufferLen;
    error = PaWriteStream(buffer, length);
    pa_threaded_mainloop_unlock(mainLoop);
    pError = error;
    return (stream.bufferLen - length);
}

size_t AudioServiceClient::WriteStream(const StreamBuffer &stream, int32_t &pError)
{
    lock_guard<mutex> lock(dataMutex_);
    int error = 0;
    size_t cachedLen = WriteToAudioCache(stream);

    if (!acache_.isFull) {
        pError = error;
        return cachedLen;
    }
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, 0, pError) < 0) {
        return 0;
    }
    pa_threaded_mainloop_lock(mainLoop);

    if (acache_.buffer == nullptr) {
        AUDIO_ERR_LOG("Buffer is null");
        pa_threaded_mainloop_unlock(mainLoop);
        pError = AUDIO_CLIENT_WRITE_STREAM_ERR;
        return cachedLen;
    }

    const uint8_t *buffer = acache_.buffer.get();
    size_t length = acache_.totalCacheSize;

    error = PaWriteStream(buffer, length);
    const size_t written = acache_.totalCacheSize - length;
    if (!error && written > 0) {
        acache_.readIndex += written;
        acache_.isFull = false;
    }
    if (acache_.totalCacheSize < length) {
        AUDIO_ERR_LOG("WriteStream totalCacheSize(%u) < length(%lu)", acache_.totalCacheSize, length);
    }

    if (!error && (length >= 0) && !acache_.isFull) {
        uint8_t *cacheBuffer = acache_.buffer.get();
        uint32_t offset = acache_.readIndex;
        if (acache_.writeIndex > acache_.readIndex) {
            uint32_t size = (acache_.writeIndex - acache_.readIndex);
            auto* func = memcpy_s;
            if(offset < size) { // overlop
                func = memmove_s;
            }
            if (func(cacheBuffer, acache_.totalCacheSize, cacheBuffer + offset, size)) {
                AUDIO_ERR_LOG("Update cache failed, offset %u, size %u", offset, size);
                pa_threaded_mainloop_unlock(mainLoop);
                pError = AUDIO_CLIENT_WRITE_STREAM_ERR;
                return cachedLen;
            }
            AUDIO_INFO_LOG("rearranging the audio cache");
            acache_.readIndex = 0;
            acache_.writeIndex = size;
        } else {
            acache_.readIndex = 0;
            acache_.writeIndex = 0;
        }

        if (cachedLen < stream.bufferLen) {
            StreamBuffer str;
            str.buffer = stream.buffer + cachedLen;
            str.bufferLen = stream.bufferLen - cachedLen;
            AUDIO_DEBUG_LOG("writing pending data to audio cache: %{public}d", str.bufferLen);
            cachedLen += WriteToAudioCache(str);
        }
    }

    pa_threaded_mainloop_unlock(mainLoop);
    pError = error;
    return cachedLen;
}

int32_t AudioServiceClient::UpdateReadBuffer(uint8_t *buffer, size_t &length, size_t &readSize)
{
    size_t l = (internalRdBufLen_ < length) ? internalRdBufLen_ : length;
    if (memcpy_s(buffer, length, (const uint8_t*)internalReadBuffer_ + internalRdBufIndex_, l)) {
        AUDIO_ERR_LOG("Update read buffer failed");
        return AUDIO_CLIENT_READ_STREAM_ERR;
    }

    length -= l;
    internalRdBufIndex_ += l;
    internalRdBufLen_ -= l;
    readSize += l;

    if (!internalRdBufLen_) {
        int retVal = pa_stream_drop(paStream);
        internalReadBuffer_ = nullptr;
        internalRdBufLen_ = 0;
        internalRdBufIndex_ = 0;
        if (retVal < 0) {
            AUDIO_ERR_LOG("pa_stream_drop failed, retVal: %{public}d", retVal);
            return AUDIO_CLIENT_READ_STREAM_ERR;
        }
    }

    return 0;
}

int32_t AudioServiceClient::RenderPrebuf(uint32_t writeLen)
{
    Trace trace("RenderPrebuf");
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        return AUDIO_CLIENT_PA_ERR;
    }
    const pa_buffer_attr *bufferAttr = pa_stream_get_buffer_attr(paStream);
    if (bufferAttr == nullptr) {
        AUDIO_ERR_LOG("pa_stream_get_buffer_attr returned nullptr");
        return AUDIO_CLIENT_ERR;
    }

    if (bufferAttr->maxlength <= writeLen) {
        return AUDIO_CLIENT_SUCCESS;
    }
    size_t diff = bufferAttr->maxlength - writeLen;

    int32_t writeError;
    StreamBuffer prebufStream;
    prebufStream.buffer = preBuf_.get();
    if (writeLen == 0) {
        return AUDIO_CLIENT_SUCCESS;
    } else if (writeLen > diff) {
        prebufStream.bufferLen = diff;
    } else {
        prebufStream.bufferLen = writeLen;
    }

    size_t bytesWritten {0};
    AUDIO_INFO_LOG("RenderPrebuf start");
    while (true) {
        bytesWritten += WriteStream(prebufStream, writeError);
        if (writeError) {
            AUDIO_ERR_LOG("RenderPrebuf failed: %{public}d", writeError);
            return AUDIO_CLIENT_ERR;
        }

        if (diff <= bytesWritten) {
            break;
        }

        if ((diff - bytesWritten) < writeLen) {
            prebufStream.bufferLen = diff - bytesWritten;
        }
    }

    return AUDIO_CLIENT_SUCCESS;
}

void AudioServiceClient::OnTimeOut()
{
    AUDIO_ERR_LOG("Inside timeout callback with pid:%{public}d uid:%{public}d", clientPid_, clientUid_);
    if (mainLoop == nullptr) {
        AUDIO_ERR_LOG("OnTimeOut failed: mainLoop == nullptr");
        return;
    }
    pa_threaded_mainloop_signal(mainLoop, 0);
}

void AudioServiceClient::SetClientID(int32_t clientPid, int32_t clientUid)
{
    AUDIO_DEBUG_LOG("Set client PID: %{public}d, UID: %{public}d", clientPid, clientUid);
    clientPid_ = clientPid;
    clientUid_ = clientUid;
}

IAudioStream::StreamClass AudioServiceClient::GetStreamClass()
{
    return streamClass_;
}

void AudioServiceClient::GetStreamSwitchInfo(SwitchInfo& info)
{
    info.cachePath = cachePath_;
    info.rendererSampleRate = rendererSampleRate;
    info.underFlowCount = underFlowCount;
    info.effectMode = effectMode;
    info.renderMode = renderMode_;
    info.captureMode = captureMode_;
    info.renderRate = renderRate;
    info.clientPid = clientPid_;
    info.clientUid = clientUid_;
    info.volume = mVolumeFactor;

    info.frameMarkPosition = mFrameMarkPosition;
    info.renderPositionCb = mRenderPositionCb;
    info.capturePositionCb = mCapturePositionCb;

    info.framePeriodNumber = mFramePeriodNumber;
    info.renderPeriodPositionCb = mRenderPeriodPositionCb;
    info.capturePeriodPositionCb = mCapturePeriodPositionCb;

    info.rendererWriteCallback = writeCallback_;
}

void AudioServiceClient::HandleCapturePositionCallbacks(size_t bytesRead)
{
    mTotalBytesRead += bytesRead;
    if (mFrameSize == 0) {
        AUDIO_ERR_LOG("HandleCapturePositionCallbacks: capturePeriodPositionCb not set");
        return;
    }

    uint64_t readFrameNumber = mTotalBytesRead / mFrameSize;
    AUDIO_DEBUG_LOG("frame size: %{public}d", mFrameSize);
    {
        std::lock_guard<std::mutex> lock(capturerMarkReachedMutex_);
        if (!mMarkReached) {
            AUDIO_DEBUG_LOG("frame mark position: %{public}" PRIu64 ", Total frames read: %{public}" PRIu64,
                static_cast<uint64_t>(mFrameMarkPosition), static_cast<uint64_t>(readFrameNumber));
            if (readFrameNumber >= mFrameMarkPosition) {
                AUDIO_DEBUG_LOG("audio service client capturer OnMarkReached");
                SendCapturerMarkReachedRequestEvent(mFrameMarkPosition);
                mMarkReached = true;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(capturerPeriodReachedMutex_);
        mFramePeriodRead += (bytesRead / mFrameSize);
        AUDIO_DEBUG_LOG("frame period number: %{public}" PRIu64 ", Total frames read: %{public}" PRIu64,
            static_cast<uint64_t>(mFramePeriodNumber), static_cast<uint64_t>(readFrameNumber));
        if (mFramePeriodRead >= mFramePeriodNumber) {
            mFramePeriodRead %= mFramePeriodNumber;
            AUDIO_DEBUG_LOG("audio service client OnPeriodReached, remaining frames: %{public}" PRIu64,
                static_cast<uint64_t>(mFramePeriodRead));
            SendCapturerPeriodReachedRequestEvent(mFramePeriodNumber);
        }
    }
}

int32_t AudioServiceClient::ReadStream(StreamBuffer &stream, bool isBlocking)
{
    uint8_t *buffer = stream.buffer;
    size_t length = stream.bufferLen;
    size_t readSize = 0;
    lock_guard<mutex> lock(dataMutex_);
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        return AUDIO_CLIENT_PA_ERR;
    }

    pa_threaded_mainloop_lock(mainLoop);
    while (length > 0) {
        while (!internalReadBuffer_) {
            int retVal = pa_stream_peek(paStream, &internalReadBuffer_, &internalRdBufLen_);
            if (retVal < 0) {
                AUDIO_ERR_LOG("pa_stream_peek failed, retVal: %{public}d", retVal);
                pa_threaded_mainloop_unlock(mainLoop);
                return AUDIO_CLIENT_READ_STREAM_ERR;
            }

            if (internalRdBufLen_ <= 0) {
                if (isBlocking) {
                    StartTimer(READ_TIMEOUT_IN_SEC);
                    pa_threaded_mainloop_wait(mainLoop);
                    StopTimer();
                    if (IsTimeOut()) {
                        AUDIO_ERR_LOG("Read timeout");
                        pa_threaded_mainloop_unlock(mainLoop);
                        return AUDIO_CLIENT_READ_STREAM_ERR;
                    }
                } else {
                    pa_threaded_mainloop_unlock(mainLoop);
                    HandleCapturePositionCallbacks(readSize);
                    return readSize;
                }
            } else if (!internalReadBuffer_) {
                retVal = pa_stream_drop(paStream);
                if (retVal < 0) {
                    AUDIO_ERR_LOG("pa_stream_drop failed, retVal: %{public}d", retVal);
                    pa_threaded_mainloop_unlock(mainLoop);
                    return AUDIO_CLIENT_READ_STREAM_ERR;
                }
            } else {
                internalRdBufIndex_ = 0;
                AUDIO_DEBUG_LOG("buffer size from PA: %{public}zu", internalRdBufLen_);
            }
        }

        if (UpdateReadBuffer(buffer, length, readSize) != 0) {
            pa_threaded_mainloop_unlock(mainLoop);
            return AUDIO_CLIENT_READ_STREAM_ERR;
        }
        buffer = stream.buffer + readSize;
    }
    pa_threaded_mainloop_unlock(mainLoop);
    HandleCapturePositionCallbacks(readSize);

    return readSize;
}

int32_t AudioServiceClient::ReleaseStream(bool releaseRunner)
{
    state_ = RELEASED;
    lock_guard<mutex> lockdata(dataMutex_);
    WriteStateChangedSysEvents();
    ResetPAAudioClient();

    std::shared_ptr<AudioStreamCallback> streamCb = streamCallback_.lock();
    if (streamCb != nullptr) {
        streamCb->OnStateChange(state_);
    }

    {
        lock_guard<mutex> runnerlock(runnerMutex_);
        if (releaseRunner) {
            AUDIO_INFO_LOG("runner remove");
            SetEventRunner(nullptr);
            runnerReleased_ = true;
        }
    }

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::SetBufferSizeInMsec(int32_t bufferSizeInMsec)
{
    size_t bufferSize =  pa_usec_to_bytes(bufferSizeInMsec * PA_USEC_PER_MSEC, &sampleSpec);
    setBufferSize_ = bufferSize;
    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::GetMinimumBufferSize(size_t &minBufferSize) const
{
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        return AUDIO_CLIENT_PA_ERR;
    }

    const pa_buffer_attr *bufferAttr = pa_stream_get_buffer_attr(paStream);

    if (bufferAttr == nullptr) {
        AUDIO_ERR_LOG("pa_stream_get_buffer_attr returned nullptr");
        return AUDIO_CLIENT_ERR;
    }

    if (eAudioClientType == AUDIO_SERVICE_CLIENT_PLAYBACK) {
        if (renderMode_ == RENDER_MODE_CALLBACK) {
            minBufferSize = (size_t)bufferAttr->minreq;
        } else {
            if (setBufferSize_) {
                minBufferSize = setBufferSize_;
            } else {
                minBufferSize = (size_t)bufferAttr->minreq;
            }
        }
    }

    if (eAudioClientType == AUDIO_SERVICE_CLIENT_RECORD) {
        minBufferSize = (size_t)bufferAttr->fragsize;
    }

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::GetMinimumFrameCount(uint32_t &frameCount) const
{
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        return AUDIO_CLIENT_PA_ERR;
    }

    size_t minBufferSize = 0;

    const pa_buffer_attr *bufferAttr = pa_stream_get_buffer_attr(paStream);

    if (bufferAttr == nullptr) {
        AUDIO_ERR_LOG("pa_stream_get_buffer_attr returned nullptr");
        return AUDIO_CLIENT_ERR;
    }

    if (eAudioClientType == AUDIO_SERVICE_CLIENT_PLAYBACK) {
        if (renderMode_ == RENDER_MODE_CALLBACK) {
            minBufferSize = (size_t)bufferAttr->minreq;
        } else {
            if (setBufferSize_) {
                minBufferSize = setBufferSize_;
            } else {
                minBufferSize = (size_t)bufferAttr->minreq;
            }
        }
    }

    if (eAudioClientType == AUDIO_SERVICE_CLIENT_RECORD) {
        minBufferSize = (size_t)bufferAttr->fragsize;
    }

    uint32_t bytesPerSample = pa_frame_size(&sampleSpec);
    if (bytesPerSample == 0) {
        AUDIO_ERR_LOG("GetMinimumFrameCount Failed");
        return AUDIO_CLIENT_ERR;
    }

    frameCount = minBufferSize / bytesPerSample;
    AUDIO_INFO_LOG("frame count: %{public}d", frameCount);
    return AUDIO_CLIENT_SUCCESS;
}

uint32_t AudioServiceClient::GetSamplingRate() const
{
    return DEFAULT_SAMPLING_RATE;
}

uint8_t AudioServiceClient::GetChannelCount() const
{
    return DEFAULT_CHANNEL_COUNT;
}

uint8_t AudioServiceClient::GetSampleSize() const
{
    return DEFAULT_SAMPLE_SIZE;
}

int32_t AudioServiceClient::GetAudioStreamParams(AudioStreamParams& audioParams) const
{
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        return AUDIO_CLIENT_PA_ERR;
    }

    const pa_sample_spec *paSampleSpec = pa_stream_get_sample_spec(paStream);

    if (!paSampleSpec) {
        AUDIO_ERR_LOG("GetAudioStreamParams Failed");
        return AUDIO_CLIENT_ERR;
    }

    audioParams = ConvertFromPAAudioParams(*paSampleSpec);
    audioParams.channelLayout = mChannelLayout;
    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::GetCurrentTimeStamp(uint64_t &timeStamp)
{
    if (eAudioClientType == AUDIO_SERVICE_CLIENT_PLAYBACK && offloadEnable) {
        uint64_t frames;
        int64_t timeSec, timeNanoSec;
        mAudioSystemMgr->GetPresentationPosition(frames, timeSec, timeNanoSec);
        timeStamp = frames;
        return AUDIO_CLIENT_SUCCESS;
    }
    lock_guard<mutex> lock(dataMutex_);
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        return AUDIO_CLIENT_PA_ERR;
    }
    pa_threaded_mainloop_lock(mainLoop);

    if (eAudioClientType == AUDIO_SERVICE_CLIENT_PLAYBACK) {
        pa_operation *operation = pa_stream_update_timing_info(paStream, NULL, NULL);
        if (operation != nullptr) {
            while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING) {
                pa_threaded_mainloop_wait(mainLoop);
            }
            pa_operation_unref(operation);
        } else {
            AUDIO_ERR_LOG("pa_stream_update_timing_info failed");
        }
    }

    const pa_timing_info *info = pa_stream_get_timing_info(paStream);
    if (info == nullptr) {
        AUDIO_ERR_LOG("pa_stream_get_timing_info failed");
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_ERR;
    }

    if (eAudioClientType == AUDIO_SERVICE_CLIENT_PLAYBACK) {
        timeStamp = pa_bytes_to_usec(info->write_index, &sampleSpec);
    } else if (eAudioClientType == AUDIO_SERVICE_CLIENT_RECORD) {
        if (pa_stream_get_time(paStream, &timeStamp)) {
            AUDIO_ERR_LOG("GetCurrentTimeStamp failed for AUDIO_SERVICE_CLIENT_RECORD");
            pa_threaded_mainloop_unlock(mainLoop);
            return AUDIO_CLIENT_ERR;
        }
        int32_t uid = static_cast<int32_t>(getuid());

        // 1013 is media_service's uid
        int32_t media_service = 1013;
        if (uid == media_service) {
            timeStamp = pa_bytes_to_usec(mTotalBytesRead, &sampleSpec);
        }
    }

    pa_threaded_mainloop_unlock(mainLoop);

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::GetAudioLatency(uint64_t &latency)
{
    lock_guard<mutex> lock(dataMutex_);
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        return AUDIO_CLIENT_PA_ERR;
    }
    pa_usec_t paLatency {0};
    pa_usec_t cacheLatency {0};
    int negative {0};

    // Get PA latency
    if (offloadEnable) {
        uint64_t frames;
        int64_t timeSec, timeNanoSec;
        mAudioSystemMgr->GetPresentationPosition(frames, timeSec, timeNanoSec);
        uint64_t writePos = pa_bytes_to_usec(mTotalBytesWritten, &sampleSpec);
        if (writePos >= frames) {
            paLatency = writePos - frames;
        } else {
            paLatency = 0;
        }
    } else {
        pa_threaded_mainloop_lock(mainLoop);
        while (true) {
            pa_operation *operation = pa_stream_update_timing_info(paStream, NULL, NULL);
            if (operation != nullptr) {
                pa_operation_unref(operation);
            } else {
                AUDIO_ERR_LOG("pa_stream_update_timing_info failed");
            }
            if (pa_stream_get_latency(paStream, &paLatency, &negative) >= 0) {
                if (negative) {
                    latency = 0;
                    pa_threaded_mainloop_unlock(mainLoop);
                    return AUDIO_CLIENT_ERR;
                }
                break;
            }
            AUDIO_INFO_LOG("waiting for audio latency information");
            pa_threaded_mainloop_wait(mainLoop);
        }
        pa_threaded_mainloop_unlock(mainLoop);

    }

    if (eAudioClientType == AUDIO_SERVICE_CLIENT_PLAYBACK) {
        // Get audio write cache latency
        cacheLatency = pa_bytes_to_usec((acache_.totalCacheSize - acache_.writeIndex), &sampleSpec);

        // Total latency will be sum of audio write cache latency + PA latency
        uint64_t fwLatency = paLatency + cacheLatency;
        uint64_t sinkLatency = sinkLatencyInMsec_ * PA_USEC_PER_MSEC;
        if (fwLatency > sinkLatency) {
            latency = fwLatency - sinkLatency;
        } else {
            latency = fwLatency;
        }
        AUDIO_DEBUG_LOG("total latency: %{public}" PRIu64 ", pa latency: %{public}"
            PRIu64 ", cache latency: %{public}" PRIu64, latency, paLatency, cacheLatency);
    } else if (eAudioClientType == AUDIO_SERVICE_CLIENT_RECORD) {
        // Get audio read cache latency
        cacheLatency = pa_bytes_to_usec(internalRdBufLen_, &sampleSpec);

        // Total latency will be sum of audio read cache latency + PA latency
        latency = paLatency + cacheLatency;
        AUDIO_DEBUG_LOG("total latency: %{public}" PRIu64 ", pa latency: %{public}" PRIu64, latency, paLatency);
    }

    return AUDIO_CLIENT_SUCCESS;
}

void AudioServiceClient::RegisterAudioRendererCallbacks(const AudioRendererCallbacks &cb)
{
    AUDIO_DEBUG_LOG("Registering audio render callbacks");
    mAudioRendererCallbacks = (AudioRendererCallbacks *)&cb;
}

void AudioServiceClient::RegisterAudioCapturerCallbacks(const AudioCapturerCallbacks &cb)
{
    AUDIO_DEBUG_LOG("Registering audio record callbacks");
    mAudioCapturerCallbacks = (AudioCapturerCallbacks *)&cb;
}

void AudioServiceClient::SetRendererPositionCallback(int64_t markPosition,
    const std::shared_ptr<RendererPositionCallback> &callback)
{
    std::lock_guard<std::mutex> lock(rendererMarkReachedMutex_);
    AUDIO_INFO_LOG("Registering render frame position callback mark position: %{public}" PRIu64, markPosition);
    mFrameMarkPosition = markPosition;
    SendSetRenderMarkReachedRequestEvent(callback);
    mMarkReached = false;
}

void AudioServiceClient::UnsetRendererPositionCallback()
{
    AUDIO_INFO_LOG("Unregistering render frame position callback");
    std::lock_guard<std::mutex> lock(rendererMarkReachedMutex_);
    SendUnsetRenderMarkReachedRequestEvent();
    mMarkReached = false;
    mFrameMarkPosition = 0;
}

void AudioServiceClient::SetRendererPeriodPositionCallback(int64_t periodPosition,
    const std::shared_ptr<RendererPeriodPositionCallback> &callback)
{
    std::lock_guard<std::mutex> lock(rendererPeriodReachedMutex_);
    AUDIO_INFO_LOG("Registering render period position callback");
    mFramePeriodNumber = periodPosition;
    if ((mFrameSize != 0) && (mFramePeriodNumber != 0)) {
        mFramePeriodWritten = (mTotalBytesWritten / mFrameSize) % mFramePeriodNumber;
    } else {
        AUDIO_ERR_LOG("SetRendererPeriodPositionCallback failed");
        return;
    }
    SendSetRenderPeriodReachedRequestEvent(callback);
}

void AudioServiceClient::UnsetRendererPeriodPositionCallback()
{
    AUDIO_INFO_LOG("Unregistering render period position callback");
    std::lock_guard<std::mutex> lock(rendererPeriodReachedMutex_);
    SendUnsetRenderPeriodReachedRequestEvent();
    mFramePeriodWritten = 0;
    mFramePeriodNumber = 0;
}

void AudioServiceClient::SetCapturerPositionCallback(int64_t markPosition,
    const std::shared_ptr<CapturerPositionCallback> &callback)
{
    std::lock_guard<std::mutex> lock(capturerMarkReachedMutex_);
    AUDIO_INFO_LOG("Registering capture frame position callback, mark position: %{public}" PRIu64, markPosition);
    mFrameMarkPosition = markPosition;
    SendSetCapturerMarkReachedRequestEvent(callback);
    mMarkReached = false;
}

void AudioServiceClient::UnsetCapturerPositionCallback()
{
    AUDIO_INFO_LOG("Unregistering capture frame position callback");
    std::lock_guard<std::mutex> lock(capturerMarkReachedMutex_);
    SendUnsetCapturerMarkReachedRequestEvent();
    mMarkReached = false;
    mFrameMarkPosition = 0;
}

void AudioServiceClient::SetCapturerPeriodPositionCallback(int64_t periodPosition,
    const std::shared_ptr<CapturerPeriodPositionCallback> &callback)
{
    std::lock_guard<std::mutex> lock(capturerPeriodReachedMutex_);
    AUDIO_INFO_LOG("Registering period position callback");
    mFramePeriodNumber = periodPosition;
    if ((mFrameSize != 0) && (mFramePeriodNumber != 0)) {
        mFramePeriodRead = (mTotalBytesRead / mFrameSize) % mFramePeriodNumber;
    } else {
        AUDIO_ERR_LOG("SetCapturerPeriodPositionCallback failed");
        return;
    }
    SendSetCapturerPeriodReachedRequestEvent(callback);
}

void AudioServiceClient::UnsetCapturerPeriodPositionCallback()
{
    AUDIO_INFO_LOG("Unregistering period position callback");
    std::lock_guard<std::mutex> lock(capturerPeriodReachedMutex_);
    SendUnsetCapturerPeriodReachedRequestEvent();
    mFramePeriodRead = 0;
    mFramePeriodNumber = 0;
}

int32_t AudioServiceClient::SetStreamType(AudioStreamType audioStreamType)
{
    AUDIO_INFO_LOG("SetStreamType: %{public}d", audioStreamType);

    if (context == nullptr) {
        AUDIO_ERR_LOG("context is null");
        return AUDIO_CLIENT_ERR;
    }

    pa_threaded_mainloop_lock(mainLoop);

    mStreamType = audioStreamType;
    const std::string streamName = GetStreamName(audioStreamType);
    effectSceneName = IAudioStream::GetEffectSceneName(audioStreamType);

    pa_proplist *propList = pa_proplist_new();
    if (propList == nullptr) {
        AUDIO_ERR_LOG("pa_proplist_new failed");
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_ERR;
    }

    pa_proplist_sets(propList, "stream.type", streamName.c_str());
    pa_proplist_sets(propList, "media.name", streamName.c_str());
    pa_proplist_sets(propList, "scene.type", effectSceneName.c_str());
    pa_operation *updatePropOperation = pa_stream_proplist_update(paStream, PA_UPDATE_REPLACE, propList,
        nullptr, nullptr);
    pa_proplist_free(propList);
    pa_operation_unref(updatePropOperation);

    pa_threaded_mainloop_unlock(mainLoop);

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::SetStreamVolume(float volume)
{
    lock_guard<mutex> lock(ctrlMutex_);
    AUDIO_INFO_LOG("SetVolume volume: %{public}f", volume);

    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        AUDIO_ERR_LOG("set stream volume: invalid stream state");
        return AUDIO_CLIENT_PA_ERR;
    }

    /* Validate and return INVALID_PARAMS error */
    if ((volume < MIN_STREAM_VOLUME_LEVEL) || (volume > MAX_STREAM_VOLUME_LEVEL)) {
        AUDIO_ERR_LOG("Invalid Volume Input!");
        return AUDIO_CLIENT_INVALID_PARAMS_ERR;
    }

    pa_threaded_mainloop_lock(mainLoop);

    mVolumeFactor = volume;
    pa_proplist *propList = pa_proplist_new();
    if (propList == nullptr) {
        AUDIO_ERR_LOG("pa_proplist_new failed");
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_ERR;
    }

    pa_proplist_sets(propList, "stream.volumeFactor", std::to_string(mVolumeFactor).c_str());
    pa_operation *updatePropOperation = pa_stream_proplist_update(paStream, PA_UPDATE_REPLACE, propList,
        nullptr, nullptr);
    if (updatePropOperation == nullptr) {
        AUDIO_ERR_LOG("pa_stream_proplist_update returned null");
        pa_proplist_free(propList);
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_ERR;
    }

    pa_proplist_free(propList);
    pa_operation_unref(updatePropOperation);

    if (mAudioSystemMgr == nullptr) {
        AUDIO_ERR_LOG("System manager instance is null");
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_ERR;
    }

    if (!streamInfoUpdated) {
        uint32_t idx = pa_stream_get_index(paStream);
        pa_operation *operation = pa_context_get_sink_input_info(context, idx, AudioServiceClient::GetSinkInputInfoCb,
            reinterpret_cast<void *>(this));
        if (operation == nullptr) {
            AUDIO_ERR_LOG("pa_context_get_sink_input_info_list returned null");
            pa_threaded_mainloop_unlock(mainLoop);
            return AUDIO_CLIENT_ERR;
        }

        pa_threaded_mainloop_accept(mainLoop);

        pa_operation_unref(operation);
    } else {
        SetPaVolume(*this);
    }

    pa_threaded_mainloop_unlock(mainLoop);

    return AUDIO_CLIENT_SUCCESS;
}

float AudioServiceClient::GetStreamVolume()
{
    return mVolumeFactor;
}

static void printBufAttr(pa_stream* paStream)
{
    const pa_buffer_attr* bufferAttr = pa_stream_get_buffer_attr(paStream);
    AUDIO_INFO_LOG("pa_stream_get_buffer_attr: minreq    %{public}u", bufferAttr->minreq);
    AUDIO_INFO_LOG("pa_stream_get_buffer_attr: prebuf    %{public}u", bufferAttr->prebuf);
    AUDIO_INFO_LOG("pa_stream_get_buffer_attr: tlength   %{public}u", bufferAttr->tlength);
    AUDIO_INFO_LOG("pa_stream_get_buffer_attr: maxlength %{public}u", bufferAttr->maxlength);
    AUDIO_INFO_LOG("pa_stream_get_buffer_attr: fragsize  %{public}u", bufferAttr->fragsize );
}

int32_t AudioServiceClient::InitializePAProbListOffload()
{
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0){
        AUDIO_ERR_LOG("set offload mode: invalid stream state");
        AUDIO_INFO_LOG("quit SetStreamOffloadMode, err");
        return AUDIO_CLIENT_PA_ERR;
    }

    pa_proplist* propList = pa_proplist_new();
    if (propList == nullptr) {
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_ERR;
    }

    pa_proplist_sets(propList, "stream.offload.statePolicy", "0");

    pa_operation* updatePropOperation = 
        pa_stream_proplist_update(paStream, PA_UPDATE_REPLACE, propList, nullptr, nullptr);
    if (updatePropOperation == nullptr) {
        pa_proplist_free(propList);
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_ERR;
    }

    pa_proplist_free(propList);
    pa_operation_unref(updatePropOperation);
    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::UpdatePAProbListOffload(AudioOffloadType statePolicy)
{
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        AUDIO_ERR_LOG("set offload mode: invalid stream state, quit SetStreamOffloadMode due err");
        return AUDIO_CLIENT_PA_ERR;
    }

    // if possible turn on the buffer immediately(long buffer -> short buffer), turn it at once.
    if ((statePolicy < offloadStatePolicy) || (offloadStatePolicy == OFFLOAD_DEFAULT)) {
        AUDIO_DEBUG_LOG("Update statePolicy immediately: %{public}d -> %{public}d", offloadStatePolicy, statePolicy);
        lastOffloadUpdateFinishTime = 0;
        pa_threaded_mainloop_lock(mainLoop);
        int32_t ret = UpdatePolicyOffload(statePolicy);
        pa_threaded_mainloop_unlock(mainLoop);
        offloadNextStateTargetPolicy = statePolicy; // Fix here if sometimes can't cut into state 3
        return ret;
    } else {    // Otherwise, hdi_sink.c's times detects the stateTarget change and switches after a certain period of time
                // this time is checked the PaWriteStream to check if the switch has been made
        AUDIO_DEBUG_LOG("Update statePolicy in 5 seconds: %{public}d -> %{public}d", offloadStatePolicy, statePolicy);
        lastOffloadUpdateFinishTime = chrono::system_clock::to_time_t(
            chrono::system_clock::now() + std::chrono::seconds(5)); // add 5s latencu to change offload state
        offloadNextStateTargetPolicy = statePolicy;
    }

    return AUDIO_CLIENT_SUCCESS;
}

inline size_t GetFormatSize(const AudioStreamParams& info)
{
    size_t result = 0;
    size_t bitWidthSize = 0;
    switch (info.format) {
        case SAMPLE_U8:
            bitWidthSize = 1; // size is 1
            break;
        case SAMPLE_S16LE:
            bitWidthSize = 2; // size is 2
            break;
        case SAMPLE_S24LE:
            bitWidthSize = 3; // size is 3
            break;
        case SAMPLE_S32LE:
            bitWidthSize = 4; // size is 4
            break;
        default:
            bitWidthSize = 2; // size is 2
            break;
    }
    result = bitWidthSize * info.channels;
    return result;
}

int32_t AudioServiceClient::UpdatePolicyOffload(AudioOffloadType statePolicy)
{
    
    pa_proplist *propList = pa_proplist_new();
    if (propList == nullptr) {
        AUDIO_ERR_LOG("AudioServiceClient::UpdatePolicyOffload, pa_proplist_new failed");
        return AUDIO_CLIENT_ERR;
    }
    if (offloadEnable) {
        pa_proplist_sets(propList, "stream.offload.enable", "1");
    } else {
        pa_proplist_sets(propList, "stream.offload.enable", "0");
    }
    pa_proplist_sets(propList, "stream.offload.statePolicy", std::to_string(statePolicy).c_str());

    pa_operation *updatePropOperation =
        pa_stream_proplist_update(paStream, PA_UPDATE_REPLACE, propList, nullptr, nullptr);
    if (updatePropOperation == nullptr) {
        AUDIO_ERR_LOG("AudioServiceClient::UpdatePolicyOffload pa_stream_proplist_update failed!");
        pa_proplist_free(propList);
        return AUDIO_CLIENT_ERR;
    }
    pa_proplist_free(propList);
    pa_operation_unref(updatePropOperation);

    const uint32_t bufLenMs = statePolicy > 1 ? OFFLOAD_HDI_CACHE2 : OFFLOAD_HDI_CACHE1;
    mAudioSystemMgr->SetBufferSize(bufLenMs);



    offloadStatePolicy = statePolicy;
    UpdatebufferAttrOffload(offloadStatePolicy);

    uint32_t sessionID = 0;
    GetSessionID(sessionID);
    DeviceType deviceType = mAudioSystemMgr->GetActiveInputDevice();
    uint64_t transactionId = mAudioSystemMgr->GetTransactionId(deviceType, OUTPUT_DEVICE);
    HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::AUDIO, "STREAM_CHANGE",
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        "ISOUTPUT", 1,
        "STREAMID", sessionID,
        "UID", clientUid_,
        "PID", clientPid_,
        "TRANSACTIONID", transactionId,
        "STREAMTYPE", mStreamType,
        "STATE", state_,
        "OFFLOADSTATE", statePolicy,
        "DEVICETYPE", deviceType);
    

    return AUDIO_CLIENT_SUCCESS;
}

static size_t MsToAlignedSize(size_t ms, const pa_sample_spec &ss)
{
    return AlignToAudioFrameSize(pa_usec_to_bytes(ms * PA_USEC_PER_MSEC, &ss), ss);
}

int32_t AudioServiceClient::InitializebufferAttrOffload()
{
    pa_buffer_attr bufferAttrOffloadActiveForeground;
    pa_buffer_attr bufferAttrOffloadActiveBackground;
    pa_buffer_attr bufferAttrOffloadInactiveBackground;

    // perbuf tlength maxlength minreq should same to hdi_sink.c
    bufferAttrOffloadActiveForeground.fragsize =static_cast<uint32_t>(-1);
    bufferAttrOffloadActiveForeground.prebuf = MsToAlignedSize(20, sampleSpec); // 20 for config
    bufferAttrOffloadActiveForeground.tlength = MsToAlignedSize(20 * 4, sampleSpec); // 20 * 4 for config
    bufferAttrOffloadActiveForeground.maxlength = MsToAlignedSize(MAX_LENGTH_OFFLOAD, sampleSpec);
    bufferAttrOffloadActiveForeground.minreq = MsToAlignedSize(20, sampleSpec); // 20 for config

    bufferAttrOffloadActiveBackground = bufferAttrOffloadActiveForeground;

    bufferAttrOffloadInactiveBackground.fragsize =static_cast<uint32_t>(-1);
    bufferAttrOffloadInactiveBackground.prebuf = MsToAlignedSize(20, sampleSpec); // 20 for config
    // +20 for requested_latency, otherwise requested_latency will set to 500us, that may cause problem
    bufferAttrOffloadInactiveBackground.tlength = MsToAlignedSize(100 * 3 + 20, sampleSpec); // 100 * 3 + 20 for config
    bufferAttrOffloadInactiveBackground.maxlength = MsToAlignedSize(MAX_LENGTH_OFFLOAD, sampleSpec);
    bufferAttrOffloadInactiveBackground.minreq = MsToAlignedSize(100, sampleSpec); // 100 for config

    bufferAttrStateMap.emplace(OFFLOAD_ACTIVE_FOREGROUND, bufferAttrOffloadActiveForeground);
    bufferAttrStateMap.emplace(OFFLOAD_ACTIVE_BACKGROUND, bufferAttrOffloadActiveBackground);
    bufferAttrStateMap.emplace(OFFLOAD_INACTIVE_BACKGROUND, bufferAttrOffloadInactiveBackground);

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::UpdatebufferAttrOffload(AudioOffloadType statePolicy)
{
    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        AUDIO_ERR_LOG("set offload mode: invalid stream state");
        AUDIO_INFO_LOG("quit SetStreamOffloadMode ,err");
        return AUDIO_CLIENT_PA_ERR;
    }

    pa_buffer_attr* bufferAttr;
    printBufAttr(paStream);
    if (bufferAttrStateMap.find(statePolicy)!=bufferAttrStateMap.end()) {
        bufferAttr = &(bufferAttrStateMap[statePolicy]);
    } else {
        AUDIO_ERR_LOG("impossible bufferAttr branch error in AudioServiceClient::SetStreamOffloadMode");
        return AUDIO_CLIENT_ERR;
    }

    pa_operation* operation =
        pa_stream_set_buffer_attr(paStream, bufferAttr, PAStreamSetBufAttrSuccessCb, (void*)this);
    
    if (operation == nullptr) {
        AUDIO_ERR_LOG("pa_stream_set_buffer_attr returned null");
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_ERR;
    }

    AUDIO_INFO_LOG("after pa_stream_set_buffer_attr");
    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING) {
        pa_threaded_mainloop_wait(mainLoop);
    }

    AUDIO_INFO_LOG("after pa_stream_set_buffer_attr pa_threaded_mainloop_wait");
    pa_operation_unref(operation);
    printBufAttr(paStream);

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::SetStreamOffloadMode(int32_t state, bool isAppBack)
{
    AudioOffloadType statePolicy = OFFLOAD_DEFAULT;
    if (state > 1) {
        statePolicy = OFFLOAD_INACTIVE_BACKGROUND;
    } else if (isAppBack) {
        statePolicy = OFFLOAD_ACTIVE_FOREGROUND;
    } else if (!isAppBack) {
        statePolicy = OFFLOAD_ACTIVE_FOREGROUND;
    }

    if (statePolicy == OFFLOAD_DEFAULT) {
        AUDIO_ERR_LOG("AudioServiceClient::SetStreamOffloadMode, impossible INPUT branch error");
        return AUDIO_CLIENT_ERR;
    }
    
    AUDIO_INFO_LOG("AudioServiceClient::SetStreamOffloadMode, calling set stream "
                    "offloadMode PowerState: %{public}d, isAppBack: %{public}d", state, isAppBack);

    if (offloadNextStateTargetPolicy == statePolicy) {
        AUDIO_INFO_LOG("AudioServiceClient::SetStreamOffloadMode, pa_stream_proplist_update NextStatePolicy not "
                        "change: last(%d) curr(%d)",
            offloadNextStateTargetPolicy, statePolicy);
        return AUDIO_CLIENT_SUCCESS;
    }

    if ((offloadStatePolicy == offloadNextStateTargetPolicy) && (offloadStatePolicy == statePolicy)) {
        AUDIO_INFO_LOG("AudioServiceClient::SetStreamOffloadMode, pa_stream_proplist_update StatePolicy not change :"
                        "last(%d) curr(%d)",
            offloadStatePolicy, statePolicy);
        return AUDIO_CLIENT_SUCCESS;
    }

    AUDIO_INFO_LOG("AudioServiceClient::SetStreamOffloadMode, Update statePolicy before ctrlMutex_");
    lock_guard<mutex> lock(ctrlMutex_);
    AUDIO_INFO_LOG("AudioServiceClient::SetStreamOffloadMode, Update statePolicy after ctrlMutex_");

    offloadEnable = true;
    if (UpdatePAProbListOffload(statePolicy) != AUDIO_CLIENT_SUCCESS) {
        return AUDIO_CLIENT_ERR;
    }

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::UnSetStreamOffloadMode()
{

    lastOffloadUpdateFinishTime = 0;
    offloadEnable = false;
    pa_threaded_mainloop_lock(mainLoop);
    int32_t ret = UpdatePolicyOffload(OFFLOAD_ACTIVE_FOREGROUND);
    pa_threaded_mainloop_unlock(mainLoop);
    offloadNextStateTargetPolicy = OFFLOAD_DEFAULT;
    offloadStatePolicy = OFFLOAD_DEFAULT;
    return ret;
}

void AudioServiceClient::GetSinkInputInfoCb(pa_context *context, const pa_sink_input_info *info, int eol,
    void *userdata)
{
    AUDIO_INFO_LOG("GetSinkInputInfoVolumeCb in");
    AudioServiceClient *thiz = reinterpret_cast<AudioServiceClient *>(userdata);

    if (eol < 0) {
        AUDIO_ERR_LOG("Failed to get sink input information: %{public}s", pa_strerror(pa_context_errno(context)));
        return;
    }

    if (eol) {
        pa_threaded_mainloop_signal(thiz->mainLoop, 1);
        return;
    }

    if (info->proplist == nullptr) {
        AUDIO_ERR_LOG("Invalid prop list for sink input (%{public}d).", info->index);
        return;
    }

    uint32_t sessionID = 0;
    const char *sessionCStr = pa_proplist_gets(info->proplist, "stream.sessionID");
    if (sessionCStr != nullptr) {
        std::stringstream sessionStr;
        sessionStr << sessionCStr;
        sessionStr >> sessionID;
    }

    thiz->cvolume = info->volume;
    thiz->streamIndex = info->index;
    thiz->sessionID_ = sessionID;
    thiz->volumeChannels = info->channel_map.channels;
    thiz->streamInfoUpdated = true;

    SetPaVolume(*thiz);

    return;
}

void AudioServiceClient::GetSinkInputInfoOffloadCb(pa_context* context, const pa_sink_input_info* info, int eol,
    void* userdata)
{

    AudioServiceClient* thiz = reinterpret_cast<AudioServiceClient*>(userdata);

    if (eol < 0) {
        AUDIO_ERR_LOG("Failed to get sink input information: %{public}s", pa_strerror(pa_context_errno(context)));
        return;
    }

    if (eol) {
        pa_threaded_mainloop_signal(thiz->mainLoop, 1);
        return;
    }

    if (info->proplist == nullptr) {
        AUDIO_ERR_LOG("Invalid prop list for sink input (%{public}d).", info->index);
        return;
    }

    thiz->offloadCurrentStatePolicy = static_cast<AudioOffloadType>(atoi(
        pa_proplist_gets(info->proplist, "stream.offload.statePolicy")));
    return;
}

void AudioServiceClient::SetPaVolume(const AudioServiceClient &client)
{
    pa_cvolume cv = client.cvolume;
    AudioVolumeType volumeType = GetVolumeTypeFromStreamType(client.mStreamType);
    int32_t systemVolumeLevel = AudioSystemManager::GetInstance()->GetVolume(volumeType);
    DeviceType deviceType = AudioSystemManager::GetInstance()->GetActiveOutputDevice();
    float systemVolumeDb = AudioPolicyManager::GetInstance().GetSystemVolumeInDb(volumeType,
        systemVolumeLevel, deviceType);
    float vol = systemVolumeDb * client.mVolumeFactor * client.mPowerVolumeFactor;

    uint32_t volume = pa_sw_volume_from_linear(vol);
    pa_cvolume_set(&cv, client.volumeChannels, volume);
    pa_operation_unref(pa_context_set_sink_input_volume(client.context, client.streamIndex, &cv, nullptr, nullptr));

    AUDIO_INFO_LOG("Applied volume %{public}f, systemVolume %{public}f, volumeFactor %{public}f", vol, systemVolumeDb,
        client.mVolumeFactor);
    HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::AUDIO,
        "VOLUME_CHANGE", HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        "ISOUTPUT", 1,
        "STREAMID", client.sessionID_,
        "APP_UID", client.clientUid_,
        "APP_PID", client.clientPid_,
        "STREAMTYPE", client.mStreamType,
        "VOLUME", vol,
        "SYSVOLUME", systemVolumeDb,
        "VOLUMEFACTOR", client.mVolumeFactor,
        "POWERVOLUMEFACTOR", client.mPowerVolumeFactor);
}

AudioVolumeType AudioServiceClient::GetVolumeTypeFromStreamType(AudioStreamType streamType)
{
    switch (streamType) {
        case STREAM_VOICE_CALL:
        case STREAM_VOICE_MESSAGE:
            return STREAM_VOICE_CALL;
        case STREAM_RING:
        case STREAM_SYSTEM:
        case STREAM_NOTIFICATION:
        case STREAM_SYSTEM_ENFORCED:
        case STREAM_DTMF:
            return STREAM_RING;
        case STREAM_MUSIC:
        case STREAM_MEDIA:
        case STREAM_MOVIE:
        case STREAM_GAME:
        case STREAM_SPEECH:
        case STREAM_NAVIGATION:
            return STREAM_MUSIC;
        case STREAM_VOICE_ASSISTANT:
            return STREAM_VOICE_ASSISTANT;
        case STREAM_ALARM:
            return STREAM_ALARM;
        case STREAM_ACCESSIBILITY:
            return STREAM_ACCESSIBILITY;
        case STREAM_ULTRASONIC:
            return STREAM_ULTRASONIC;
        default:
            AUDIO_ERR_LOG("GetVolumeTypeFromStreamType streamType = %{public}d not supported", streamType);
            return streamType;
    }
}

int32_t AudioServiceClient::SetStreamRenderRate(AudioRendererRate audioRendererRate)
{
    AUDIO_INFO_LOG("SetStreamRenderRate in");
    if (!paStream) {
        return AUDIO_CLIENT_SUCCESS;
    }

    uint32_t rate = sampleSpec.rate;
    switch (audioRendererRate) {
        case RENDER_RATE_NORMAL:
            break;
        case RENDER_RATE_DOUBLE:
            rate *= DOUBLE_VALUE;
            break;
        case RENDER_RATE_HALF:
            rate /= DOUBLE_VALUE;
            break;
        default:
            return AUDIO_CLIENT_INVALID_PARAMS_ERR;
    }
    renderRate = audioRendererRate;

    pa_threaded_mainloop_lock(mainLoop);
    pa_operation *operation = pa_stream_update_sample_rate(paStream, rate, nullptr, nullptr);
    if (operation != nullptr) {
        pa_operation_unref(operation);
    } else {
        AUDIO_ERR_LOG("SetStreamRenderRate: operation is nullptr");
    }
    pa_threaded_mainloop_unlock(mainLoop);

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioServiceClient::SetRendererSamplingRate(uint32_t sampleRate)
{
    AUDIO_INFO_LOG("SetStreamRendererSamplingRate %{public}d", sampleRate);
    if (!paStream) {
        return AUDIO_CLIENT_SUCCESS;
    }

    if (sampleRate <= 0) {
        return AUDIO_CLIENT_INVALID_PARAMS_ERR;
    }
    rendererSampleRate = sampleRate;

    pa_threaded_mainloop_lock(mainLoop);
    pa_operation *operation = pa_stream_update_sample_rate(paStream, sampleRate, nullptr, nullptr);
    pa_operation_unref(operation);
    pa_threaded_mainloop_unlock(mainLoop);

    return AUDIO_CLIENT_SUCCESS;
}

uint32_t AudioServiceClient::GetRendererSamplingRate()
{
    if (rendererSampleRate == 0) {
        return sampleSpec.rate;
    }
    return rendererSampleRate;
}

AudioRendererRate AudioServiceClient::GetStreamRenderRate()
{
    return renderRate;
}

void AudioServiceClient::SaveStreamCallback(const std::weak_ptr<AudioStreamCallback> &callback)
{
    streamCallback_ = callback;

    if (state_ != PREPARED) {
        return;
    }

    std::shared_ptr<AudioStreamCallback> streamCb = streamCallback_.lock();
    if (streamCb != nullptr) {
        streamCb->OnStateChange(state_);
    }
}

void AudioServiceClient::WriteStateChangedSysEvents()
{
    uint32_t sessionID = 0;
    uint64_t transactionId = 0;
    DeviceType deviceType = DEVICE_TYPE_INVALID;

    bool isOutput = true;
    if (eAudioClientType == AUDIO_SERVICE_CLIENT_PLAYBACK) {
        deviceType = mAudioSystemMgr->GetActiveOutputDevice();
        transactionId = mAudioSystemMgr->GetTransactionId(deviceType, OUTPUT_DEVICE);
    } else {
        deviceType = mAudioSystemMgr->GetActiveInputDevice();
        transactionId = mAudioSystemMgr->GetTransactionId(deviceType, INPUT_DEVICE);
        isOutput = false;
    }

    GetSessionID(sessionID);

    HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::AUDIO, "STREAM_CHANGE",
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        "ISOUTPUT", isOutput ? 1 : 0,
        "STREAMID", sessionID,
        "UID", clientUid_,
        "PID", clientPid_,
        "TRANSACTIONID", transactionId,
        "STREAMTYPE", mStreamType,
        "STATE", state_,
        "DEVICETYPE", deviceType);
}

int32_t AudioServiceClient::SetStreamLowPowerVolume(float powerVolumeFactor)
{
    lock_guard<mutex> lock(ctrlMutex_);
    AUDIO_INFO_LOG("SetPowerVolumeFactor volume: %{public}f", powerVolumeFactor);

    if (context == nullptr) {
        AUDIO_ERR_LOG("context is null");
        return AUDIO_CLIENT_ERR;
    }

    /* Validate and return INVALID_PARAMS error */
    if ((powerVolumeFactor < MIN_STREAM_VOLUME_LEVEL) || (powerVolumeFactor > MAX_STREAM_VOLUME_LEVEL)) {
        AUDIO_ERR_LOG("Invalid Power Volume Set!");
        return AUDIO_CLIENT_INVALID_PARAMS_ERR;
    }

    pa_threaded_mainloop_lock(mainLoop);

    mPowerVolumeFactor = powerVolumeFactor;
    pa_proplist *propList = pa_proplist_new();
    if (propList == nullptr) {
        AUDIO_ERR_LOG("pa_proplist_new failed");
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_ERR;
    }

    pa_proplist_sets(propList, "stream.powerVolumeFactor", std::to_string(mPowerVolumeFactor).c_str());
    pa_operation *updatePropOperation = pa_stream_proplist_update(paStream, PA_UPDATE_REPLACE, propList,
        nullptr, nullptr);
    pa_proplist_free(propList);
    pa_operation_unref(updatePropOperation);

    if (mAudioSystemMgr == nullptr) {
        AUDIO_ERR_LOG("System manager instance is null");
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_ERR;
    }

    if (!streamInfoUpdated) {
        uint32_t idx = pa_stream_get_index(paStream);
        pa_operation *operation = pa_context_get_sink_input_info(context, idx, AudioServiceClient::GetSinkInputInfoCb,
            reinterpret_cast<void *>(this));
        if (operation == nullptr) {
            AUDIO_ERR_LOG("pa_context_get_sink_input_info_list returned null");
            pa_threaded_mainloop_unlock(mainLoop);
            return AUDIO_CLIENT_ERR;
        }

        pa_threaded_mainloop_accept(mainLoop);

        pa_operation_unref(operation);
    } else {
        SetPaVolume(*this);
    }

    pa_threaded_mainloop_unlock(mainLoop);

    return AUDIO_CLIENT_SUCCESS;
}

float AudioServiceClient::GetStreamLowPowerVolume()
{
    return mPowerVolumeFactor;
}

float AudioServiceClient::GetSingleStreamVol()
{
    int32_t systemVolumeLevel = mAudioSystemMgr->GetVolume(static_cast<AudioVolumeType>(mStreamType));
    DeviceType deviceType = mAudioSystemMgr->GetActiveOutputDevice();
    float systemVolumeDb = AudioPolicyManager::GetInstance().GetSystemVolumeInDb(mStreamType,
        systemVolumeLevel, deviceType);
    return systemVolumeDb * mVolumeFactor * mPowerVolumeFactor;
}

// OnRenderMarkReach by eventHandler
void AudioServiceClient::SendRenderMarkReachedRequestEvent(uint64_t mFrameMarkPosition)
{
    lock_guard<mutex> runnerlock(runnerMutex_);
    if (runnerReleased_) {
        AUDIO_WARNING_LOG("SendRenderMarkReachedRequestEvent runner released");
        return;
    }
    SendEvent(AppExecFwk::InnerEvent::Get(RENDERER_MARK_REACHED_REQUEST, mFrameMarkPosition));
}

void AudioServiceClient::HandleRenderMarkReachedEvent(uint64_t mFrameMarkPosition)
{
    if (mRenderPositionCb) {
        mRenderPositionCb->OnMarkReached(mFrameMarkPosition);
    }
}

// SetRenderMarkReached by eventHandler
void AudioServiceClient::SendSetRenderMarkReachedRequestEvent(
    const std::shared_ptr<RendererPositionCallback> &callback)
{
    lock_guard<mutex> runnerlock(runnerMutex_);
    if (runnerReleased_) {
        AUDIO_WARNING_LOG("SendSetRenderMarkReachedRequestEvent runner released");
        return;
    }
    SendSyncEvent(AppExecFwk::InnerEvent::Get(SET_RENDERER_MARK_REACHED_REQUEST, callback));
}

void AudioServiceClient::HandleSetRenderMarkReachedEvent(const std::shared_ptr<RendererPositionCallback> &callback)
{
    mRenderPositionCb = callback;
}

// UnsetRenderMarkReach by eventHandler
void AudioServiceClient::SendUnsetRenderMarkReachedRequestEvent()
{
    lock_guard<mutex> runnerlock(runnerMutex_);
    if (runnerReleased_) {
        AUDIO_WARNING_LOG("SendUnsetRenderMarkReachedRequestEvent runner released");
        return;
    }
    SendSyncEvent(AppExecFwk::InnerEvent::Get(UNSET_RENDERER_MARK_REACHED_REQUEST));
}

void AudioServiceClient::HandleUnsetRenderMarkReachedEvent()
{
    mRenderPositionCb = nullptr;
}

// OnRenderPeriodReach by eventHandler
void AudioServiceClient::SendRenderPeriodReachedRequestEvent(uint64_t mFramePeriodNumber)
{
    lock_guard<mutex> runnerlock(runnerMutex_);
    if (runnerReleased_) {
        AUDIO_WARNING_LOG("SendRenderPeriodReachedRequestEvent runner released");
        return;
    }
    SendEvent(AppExecFwk::InnerEvent::Get(RENDERER_PERIOD_REACHED_REQUEST, mFramePeriodNumber));
}

void AudioServiceClient::HandleRenderPeriodReachedEvent(uint64_t mFramePeriodNumber)
{
    if (mRenderPeriodPositionCb) {
        mRenderPeriodPositionCb->OnPeriodReached(mFramePeriodNumber);
    }
}

// SetRenderPeriodReach by eventHandler
void AudioServiceClient::SendSetRenderPeriodReachedRequestEvent(
    const std::shared_ptr<RendererPeriodPositionCallback> &callback)
{
    lock_guard<mutex> runnerlock(runnerMutex_);
    if (runnerReleased_) {
        AUDIO_WARNING_LOG("SendSetRenderPeriodReachedRequestEvent runner released");
        return;
    }
    SendSyncEvent(AppExecFwk::InnerEvent::Get(SET_RENDERER_PERIOD_REACHED_REQUEST, callback));
}

void AudioServiceClient::HandleSetRenderPeriodReachedEvent(
    const std::shared_ptr<RendererPeriodPositionCallback> &callback)
{
    if (callback) {
        mRenderPeriodPositionCb = callback;
    }
}

// UnsetRenderPeriodReach by eventHandler
void AudioServiceClient::SendUnsetRenderPeriodReachedRequestEvent()
{
    lock_guard<mutex> runnerlock(runnerMutex_);
    if (runnerReleased_) {
        AUDIO_WARNING_LOG("SendUnsetRenderPeriodReachedRequestEvent runner released");
        return;
    }
    SendSyncEvent(AppExecFwk::InnerEvent::Get(UNSET_RENDERER_PERIOD_REACHED_REQUEST));
}

void AudioServiceClient::HandleUnsetRenderPeriodReachedEvent()
{
    mRenderPeriodPositionCb = nullptr;
}

// OnCapturerMarkReach by eventHandler
void AudioServiceClient::SendCapturerMarkReachedRequestEvent(uint64_t mFrameMarkPosition)
{
    lock_guard<mutex> runnerlock(runnerMutex_);
    if (runnerReleased_) {
        AUDIO_WARNING_LOG("SendCapturerMarkReachedRequestEvent runner released");
        return;
    }
    SendEvent(AppExecFwk::InnerEvent::Get(CAPTURER_MARK_REACHED_REQUEST, mFrameMarkPosition));
}

void AudioServiceClient::HandleCapturerMarkReachedEvent(uint64_t mFrameMarkPosition)
{
    if (mCapturePositionCb) {
        mCapturePositionCb->OnMarkReached(mFrameMarkPosition);
    }
}

// SetCapturerMarkReach by eventHandler
void AudioServiceClient::SendSetCapturerMarkReachedRequestEvent(
    const std::shared_ptr<CapturerPositionCallback> &callback)
{
    lock_guard<mutex> runnerlock(runnerMutex_);
    if (runnerReleased_) {
        AUDIO_WARNING_LOG("SendSetCapturerMarkReachedRequestEvent runner released");
        return;
    }
    SendSyncEvent(AppExecFwk::InnerEvent::Get(SET_CAPTURER_MARK_REACHED_REQUEST, callback));
}

void AudioServiceClient::HandleSetCapturerMarkReachedEvent(const std::shared_ptr<CapturerPositionCallback> &callback)
{
    mCapturePositionCb = callback;
}

// UnsetCapturerMarkReach by eventHandler
void AudioServiceClient::SendUnsetCapturerMarkReachedRequestEvent()
{
    lock_guard<mutex> runnerlock(runnerMutex_);
    if (runnerReleased_) {
        AUDIO_WARNING_LOG("SendUnsetCapturerMarkReachedRequestEvent runner released");
        return;
    }
    SendSyncEvent(AppExecFwk::InnerEvent::Get(UNSET_CAPTURER_MARK_REACHED_REQUEST));
}

void AudioServiceClient::HandleUnsetCapturerMarkReachedEvent()
{
    mCapturePositionCb = nullptr;
}

// OnCapturerPeriodReach by eventHandler
void AudioServiceClient::SendCapturerPeriodReachedRequestEvent(uint64_t mFramePeriodNumber)
{
    lock_guard<mutex> runnerlock(runnerMutex_);
    if (runnerReleased_) {
        AUDIO_WARNING_LOG("SendCapturerPeriodReachedRequestEvent runner released");
        return;
    }
    SendEvent(AppExecFwk::InnerEvent::Get(CAPTURER_PERIOD_REACHED_REQUEST, mFramePeriodNumber));
}

void AudioServiceClient::HandleCapturerPeriodReachedEvent(uint64_t mFramePeriodNumber)
{
    if (mCapturePeriodPositionCb) {
        mCapturePeriodPositionCb->OnPeriodReached(mFramePeriodNumber);
    }
}

// SetCapturerPeriodReach by eventHandler
void AudioServiceClient::SendSetCapturerPeriodReachedRequestEvent(
    const std::shared_ptr<CapturerPeriodPositionCallback> &callback)
{
    lock_guard<mutex> runnerlock(runnerMutex_);
    if (runnerReleased_) {
        AUDIO_WARNING_LOG("SendSetCapturerPeriodReachedRequestEvent runner released");
        return;
    }
    SendSyncEvent(AppExecFwk::InnerEvent::Get(SET_CAPTURER_PERIOD_REACHED_REQUEST, callback));
}

void AudioServiceClient::HandleSetCapturerPeriodReachedEvent(
    const std::shared_ptr<CapturerPeriodPositionCallback> &callback)
{
    mCapturePeriodPositionCb = callback;
}

// UnsetCapturerPeriodReach by eventHandler
void AudioServiceClient::SendUnsetCapturerPeriodReachedRequestEvent()
{
    lock_guard<mutex> runnerlock(runnerMutex_);
    if (runnerReleased_) {
        AUDIO_WARNING_LOG("SendUnsetCapturerPeriodReachedRequestEvent runner released");
        return;
    }
    SendSyncEvent(AppExecFwk::InnerEvent::Get(UNSET_CAPTURER_PERIOD_REACHED_REQUEST));
}

void AudioServiceClient::HandleUnsetCapturerPeriodReachedEvent()
{
    mCapturePeriodPositionCb = nullptr;
}

int32_t AudioServiceClient::SetRendererWriteCallback(const std::shared_ptr<AudioRendererWriteCallback> &callback)
{
    std::lock_guard<std::mutex> lockSet(writeCallbackMutex_);
    if (!callback) {
        AUDIO_ERR_LOG("SetRendererWriteCallback callback is nullptr");
        return ERR_INVALID_PARAM;
    }
    writeCallback_ = callback;
    return SUCCESS;
}

int32_t AudioServiceClient::SetCapturerReadCallback(const std::shared_ptr<AudioCapturerReadCallback> &callback)
{
    if (!callback) {
        AUDIO_ERR_LOG("SetCapturerReadCallback callback is nullptr");
        return ERR_INVALID_PARAM;
    }
    readCallback_ = callback;
    return SUCCESS;
}

void AudioServiceClient::SendWriteBufferRequestEvent()
{
    // send write event to handler
    lock_guard<mutex> runnerlock(runnerMutex_);
    if (runnerReleased_) {
        AUDIO_WARNING_LOG("SendWriteBufferRequestEvent after runner released");
        return;
    }
    SendEvent(AppExecFwk::InnerEvent::Get(WRITE_BUFFER_REQUEST));
}

void AudioServiceClient::SendReadBufferRequestEvent()
{
    // send write event to handler
    lock_guard<mutex> runnerlock(runnerMutex_);
    if (runnerReleased_) {
        AUDIO_WARNING_LOG("SendReadBufferRequestEvent after runner released");
        return;
    }
    SendEvent(AppExecFwk::InnerEvent::Get(READ_BUFFER_REQUEST));
}

void AudioServiceClient::HandleWriteRequestEvent()
{
    std::lock_guard<std::mutex> lockSet(writeCallbackMutex_);
    // do callback to application
    if (writeCallback_) {
        size_t requestSize;
        GetMinimumBufferSize(requestSize);
        writeCallback_->OnWriteData(requestSize);
    }
}

void AudioServiceClient::HandleReadRequestEvent()
{
    // do callback to application
    if (readCallback_) {
        size_t requestSize;
        GetMinimumBufferSize(requestSize);
        readCallback_->OnReadData(requestSize);
    }
}

void AudioServiceClient::ProcessEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    uint32_t eventId = event->GetInnerEventId();
    uint64_t mFrameMarkPosition;
    uint64_t mFramePeriodNumber;
    std::shared_ptr<RendererPositionCallback> renderPositionCb;
    std::shared_ptr<RendererPeriodPositionCallback> renderPeriodPositionCb;
    std::shared_ptr<CapturerPositionCallback> capturePositionCb;
    std::shared_ptr<CapturerPeriodPositionCallback> capturePeriodPositionCb;

    switch (eventId) {
        case WRITE_BUFFER_REQUEST:
            HandleWriteRequestEvent();
            break;
        case READ_BUFFER_REQUEST:
            HandleReadRequestEvent();
            break;

        // RenderMarkReach
        case RENDERER_MARK_REACHED_REQUEST:
            mFrameMarkPosition = event->GetParam();
            HandleRenderMarkReachedEvent(mFrameMarkPosition);
            break;
        case SET_RENDERER_MARK_REACHED_REQUEST:
            renderPositionCb = event->GetSharedObject<RendererPositionCallback>();
            HandleSetRenderMarkReachedEvent(renderPositionCb);
            break;
        case UNSET_RENDERER_MARK_REACHED_REQUEST:
            HandleUnsetRenderMarkReachedEvent();
            break;

        // RenderPeriodReach
        case RENDERER_PERIOD_REACHED_REQUEST:
            mFramePeriodNumber = event->GetParam();
            HandleRenderPeriodReachedEvent(mFramePeriodNumber);
            break;
        case SET_RENDERER_PERIOD_REACHED_REQUEST:
            renderPeriodPositionCb = event->GetSharedObject<RendererPeriodPositionCallback>();
            HandleSetRenderPeriodReachedEvent(renderPeriodPositionCb);
            break;
        case UNSET_RENDERER_PERIOD_REACHED_REQUEST:
            HandleUnsetRenderPeriodReachedEvent();
            break;

        // CapturerMarkReach
        case CAPTURER_MARK_REACHED_REQUEST:
            mFrameMarkPosition = event->GetParam();
            HandleCapturerMarkReachedEvent(mFrameMarkPosition);
            break;
        case SET_CAPTURER_MARK_REACHED_REQUEST:
            capturePositionCb = event->GetSharedObject<CapturerPositionCallback>();
            HandleSetCapturerMarkReachedEvent(capturePositionCb);
            break;
        case UNSET_CAPTURER_MARK_REACHED_REQUEST:
            HandleUnsetCapturerMarkReachedEvent();
            break;

        // CapturerPeriodReach
        case CAPTURER_PERIOD_REACHED_REQUEST:
            mFramePeriodNumber = event->GetParam();
            HandleCapturerPeriodReachedEvent(mFramePeriodNumber);
            break;
        case SET_CAPTURER_PERIOD_REACHED_REQUEST:
            capturePeriodPositionCb = event->GetSharedObject<CapturerPeriodPositionCallback>();
            HandleSetCapturerPeriodReachedEvent(capturePeriodPositionCb);
            break;
        case UNSET_CAPTURER_PERIOD_REACHED_REQUEST:
            HandleUnsetCapturerPeriodReachedEvent();
            break;

        default:
            break;
    }
}

const std::string AudioServiceClient::GetEffectModeName(AudioEffectMode effectMode)
{
    std::string name;
    switch (effectMode) {
        case EFFECT_NONE:
            name = "EFFECT_NONE";
            break;
        default:
            name = "EFFECT_DEFAULT";
    }

    const std::string modeName = name;
    return modeName;
}

AudioEffectMode AudioServiceClient::GetStreamAudioEffectMode()
{
    return effectMode;
}

int64_t AudioServiceClient::GetStreamFramesWritten()
{
    if (mFrameSize == 0) {
        AUDIO_ERR_LOG("Error frame size");
        return ERROR;
    }
    return mTotalBytesWritten / mFrameSize;
}

int64_t AudioServiceClient::GetStreamFramesRead()
{
    if (mFrameSize == 0) {
        AUDIO_ERR_LOG("Error frame size");
        return ERROR;
    }
    return mTotalBytesRead / mFrameSize;
}

int32_t AudioServiceClient::SetStreamAudioEffectMode(AudioEffectMode audioEffectMode)
{
    AUDIO_INFO_LOG("SetStreamAudioEffectMode: %{public}d", audioEffectMode);

    if (CheckPaStatusIfinvalid(mainLoop, context, paStream, AUDIO_CLIENT_PA_ERR) < 0) {
        AUDIO_ERR_LOG("set stream audio effect mode: invalid stream state");
        return AUDIO_CLIENT_PA_ERR;
    }

    pa_threaded_mainloop_lock(mainLoop);

    effectMode = audioEffectMode;
    const std::string effectModeName = GetEffectModeName(audioEffectMode);

    pa_proplist *propList = pa_proplist_new();
    if (propList == nullptr) {
        AUDIO_ERR_LOG("pa_proplist_new failed");
        pa_threaded_mainloop_unlock(mainLoop);
        return AUDIO_CLIENT_ERR;
    }

    pa_proplist_sets(propList, "scene.type", effectSceneName.c_str());
    pa_proplist_sets(propList, "scene.mode", effectModeName.c_str());
    pa_operation *updatePropOperation = pa_stream_proplist_update(paStream, PA_UPDATE_REPLACE, propList,
        nullptr, nullptr);
    pa_proplist_free(propList);
    pa_operation_unref(updatePropOperation);

    pa_threaded_mainloop_unlock(mainLoop);

    return AUDIO_CLIENT_SUCCESS;
}

void AudioServiceClient::SetStreamInnerCapturerState(bool isInnerCapturer)
{
    AUDIO_DEBUG_LOG("SetInnerCapturerState: %{public}d", isInnerCapturer);
    isInnerCapturerStream_ = isInnerCapturer;
}

void AudioServiceClient::SetStreamPrivacyType(AudioPrivacyType privacyType)
{
    AUDIO_DEBUG_LOG("SetInnerCapturerState: %{public}d", privacyType);
    mPrivacyType = privacyType;
}

void AudioServiceClient::SetStreamUsage(StreamUsage usage)
{
    AUDIO_DEBUG_LOG("SetStreamUsage: %{public}d", usage);
    mStreamUsage = usage;
}

void AudioServiceClient::SetWakeupCapturerState(bool isWakeupCapturer)
{
    AUDIO_DEBUG_LOG("SetWakeupCapturerState: %{public}d", isWakeupCapturer);
    isWakeupCapturerStream_ = isWakeupCapturer;
}

uint32_t AudioServiceClient::ConvertChLayoutToPaChMap(const uint64_t &channelLayout, pa_channel_map &paMap)
{
    uint32_t channelNum = 0;
    uint64_t mode = (channelLayout & CH_MODE_MASK) >> CH_MODE_OFFSET;
    switch (mode) {
        case 0: {
            for (auto bit = chSetToPaPositionMap_.begin(); bit != chSetToPaPositionMap_.end(); ++bit) {
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
                paMap.map[i] = chSetToPaPositionMap_[FRONT_LEFT];
            }
            break;
        }
        default:
            channelNum = 0;
            break;
    }
    return channelNum;
}

} // namespace AudioStandard
} // namespace OHOS
