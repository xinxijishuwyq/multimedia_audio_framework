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
#undef LOG_TAG
#define LOG_TAG "AudioContainerClientBase"

#include "audio_container_client_base.h"
#include "audio_renderer_write_callback_stub.h"
#include <fstream>
#include "audio_log.h"
#include "securec.h"
#include "unistd.h"
#include "system_ability_definition.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {

static const int32_t MAX_VOLUME_LEVEL = 15;
static const int32_t CONST_FACTOR = 100;

static float VolumeToDb(int32_t volumeLevel)
{
    float value = static_cast<float>(volumeLevel) / MAX_VOLUME_LEVEL;
    float roundValue = static_cast<int>(value * CONST_FACTOR);

    return static_cast<float>(roundValue) / CONST_FACTOR;
}

void AudioContainerClientBase::InitializeClientGa()
{
    mVolumeFactor = 1.0f;
    mStreamType = STREAM_MUSIC;
    mAudioSystemMgr = nullptr;
    renderRate = RENDER_RATE_NORMAL;
    renderMode_ = RENDER_MODE_NORMAL;
    eAudioClientType = AUDIO_SERVICE_CLIENT_PLAYBACK;
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

    internalRdBufIndex_ = 0;
    internalRdBufLen_ = 0;
}

void AudioContainerClientBase::ResetPAAudioClientGa()
{
    lock_guard<mutex> lock(ctrlMutex_);
    mAudioRendererCallbacks = nullptr;
    mAudioCapturerCallbacks = nullptr;
    internalReadBuffer_ = nullptr;

    internalRdBufIndex_ = 0;
    internalRdBufLen_ = 0;
}

AudioContainerClientBase::AudioContainerClientBase(const sptr<IRemoteObject> &impl)
    :IRemoteProxy<IAudioContainerService>(impl)
{
}

AudioContainerClientBase::~AudioContainerClientBase()
{
    ResetPAAudioClientGa();
}

int32_t AudioContainerClientBase::InitializeGa(ASClientType eClientType)
{
    AUDIO_DEBUG_LOG("AudioContainerClientBase::InitializeGa");
    InitializeClientGa();
    eAudioClientType = eClientType;

    mMarkReached = false;
    mTotalBytesWritten = 0;
    mFramePeriodWritten = 0;
    mTotalBytesRead = 0;
    mFramePeriodRead = 0;

    mAudioSystemMgr = AudioSystemManager::GetInstance();

    return AUDIO_CLIENT_SUCCESS;
}

const std::string AudioContainerClientBase::GetStreamNameGa(AudioStreamType audioType)
{
    std::string name;
    switch (audioType) {
        case STREAM_VOICE_ASSISTANT:
            name = "voice_assistant";
            break;
        case STREAM_VOICE_CALL:
            name = "voice_call";
            break;
        case STREAM_SYSTEM:
            name = "system";
            break;
        case STREAM_RING:
            name = "ring";
            break;
        case STREAM_MUSIC:
            name = "music";
            break;
        case STREAM_ALARM:
            name = "alarm";
            break;
        case STREAM_NOTIFICATION:
            name = "notification";
            break;
        case STREAM_BLUETOOTH_SCO:
            name = "bluetooth_sco";
            break;
        case STREAM_DTMF:
            name = "dtmf";
            break;
        case STREAM_TTS:
            name = "tts";
            break;
        case STREAM_ACCESSIBILITY:
            name = "accessibility";
            break;
        case STREAM_WAKEUP:
            name = "wakeup";
            break;
        default:
            name = "unknown";
    }

    const std::string streamName = name;
    return streamName;
}

int32_t AudioContainerClientBase::InitializeAudioCacheGa()
{
    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::CreateStreamGa(AudioStreamParams audioParams, AudioStreamType audioType)
{
    int32_t error;

    if (eAudioClientType == AUDIO_SERVICE_CLIENT_CONTROLLER) {
        return AUDIO_CLIENT_INVALID_PARAMS_ERR;
    }

    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR,
        "WriteInterfaceToken failed");

    data.WriteInt32(static_cast<int32_t>(audioParams.samplingRate));
    data.WriteInt32(static_cast<int32_t>(audioParams.encoding));
    data.WriteInt32(static_cast<int32_t>(audioParams.format));
    data.WriteInt32(static_cast<int32_t>(audioParams.channels));
    data.WriteInt32(static_cast<int32_t>(audioType));

    error = Remote()->SendRequest(CMD_CREATE_AUDIOSTREAM, data, reply, option);

    mFrameSize = (static_cast<uint32_t>(audioParams.channels)) * (static_cast<uint32_t>(audioParams.format));
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_CREATE_STREAM_ERR,
        "CreateRemoteAudioRenderer() failed, error: %{public}d", error);

    audioTrackId = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(audioTrackId >= 0, AUDIO_CLIENT_CREATE_STREAM_ERR,
        "CreateRemoteAudioRenderer() remote failed");
    state_ = PREPARED;
    std::shared_ptr<AudioStreamCallback> streamCb = streamCallback_.lock();
    if (streamCb != nullptr) {
        streamCb->OnStateChange(state_);
    }

    AUDIO_INFO_LOG("Created Stream SUCCESS TrackId %{public}d", audioTrackId);
    return audioTrackId;
}

int32_t AudioContainerClientBase::StartStreamGa(const int32_t &trackId)
{
    lock_guard<mutex> lock(ctrlMutex_);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_START_AUDIOSTREAM, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_START_STREAM_ERR,
        "StartStreamGa() failed, error: %{public}d", error);
    AUDIO_INFO_LOG("Start Stream SUCCESS");
    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::PauseStreamGa(const int32_t &trackId)
{
    lock_guard<mutex> lock(ctrlMutex_);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_PAUSE_AUDIOSTREAM, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "PauseStreamGa() failed, error: %{public}d", error);

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::StopStreamGa(const int32_t &trackId)
{
    lock_guard<mutex> lock(ctrlMutex_);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_STOP_AUDIOSTREAM, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "StopStreamGa() failed, error: %{public}d", error);

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::FlushStreamGa(const int32_t &trackId)
{
    lock_guard<mutex> lock(dataMutex_);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_FLUSH_AUDIOSTREAM, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "FlushStreamGa() failed, error: %{public}d", error);

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::DrainStreamGa(const int32_t &trackId)
{
    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::GetSessionIDGa(uint32_t &sessionID, const int32_t &trackId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_GET_AUDIO_SESSIONID, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "GetSessinIDGa() failed, error: %{public}d", error);
    sessionID = reply.ReadInt32();
    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::SetAudioRenderModeGa(AudioRenderMode renderMode, const int32_t &trackId)
{
    renderMode_ = renderMode;
    CHECK_AND_RETURN_RET(renderMode_ == RENDER_MODE_CALLBACK, AUDIO_CLIENT_SUCCESS);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");
    data.WriteInt32(static_cast<int32_t>(renderMode));
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_SET_AUDIORENDERER_MODE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "SetAudioRenderModeGa() failed, error: %{public}d", error);

    return AUDIO_CLIENT_SUCCESS;
}

AudioRenderMode AudioContainerClientBase::GetAudioRenderModeGa()
{
    return renderMode_;
}

int32_t AudioContainerClientBase::SaveWriteCallbackGa(const std::weak_ptr<AudioRendererWriteCallback> &callback,
    const int32_t &trackId)
{
    lock_guard<mutex> lock(dataMutex_);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    CHECK_AND_RETURN_RET_LOG(callback.lock() != nullptr, AUDIO_CLIENT_INIT_ERR,
        "callback.lock() == nullpt");
    writeCallback_ = callback;
    // construct writecallback stub for remote
    sptr<AudioRendererWriteCallbackStub> writeCallbackStub = new(std::nothrow) AudioRendererWriteCallbackStub();
    CHECK_AND_RETURN_RET_LOG(writeCallbackStub != nullptr, AUDIO_CLIENT_ERR,
        "writeCallbackStub == nullpt");
    writeCallbackStub->SetOnRendererWriteCallback(callback);
    sptr<IRemoteObject> object = writeCallbackStub->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AUDIO_CLIENT_ERR,
        "writeCallbackStub->AsObject is nullpt");

    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");

    data.WriteRemoteObject(object);
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_WRITE_RENDERER_CALLBACK, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "SaveWriteCallbackGa() failed, error: %{public}d", error);

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::SetStreamVolumeGa(uint32_t sessionID, uint32_t volume)
{
    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::WriteStreamInnerGa(const uint8_t *buffer, size_t &length, const int32_t &trackId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");

    data.WriteInt32(static_cast<int32_t>(length));
    data.WriteBuffer(static_cast<const void *>(buffer), length);
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_WRITE_AUDIOSTREAM, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "WriteStreamInnerGa() failed, error: %{public}d", error);
    HandleRenderPositionCallbacksGa(length);
    return AUDIO_CLIENT_SUCCESS;
}

void AudioContainerClientBase::HandleRenderPositionCallbacksGa(size_t bytesWritten)
{
    mTotalBytesWritten += bytesWritten;
    if (mFrameSize == 0) {
        return;
    }

    uint64_t writtenFrameNumber = mTotalBytesWritten / mFrameSize;
    if (!mMarkReached && mRenderPositionCb) {
        if (writtenFrameNumber >= mFrameMarkPosition) {
            mRenderPositionCb->OnMarkReached(mFrameMarkPosition);
            mPositionCBThreads.emplace_back(std::make_unique<std::thread>(&RendererPositionCallback::OnMarkReached,
                mRenderPositionCb, mFrameMarkPosition));
            mMarkReached = true;
        }
    }

    if (mRenderPeriodPositionCb) {
        mFramePeriodWritten += (bytesWritten / mFrameSize);
        if (mFramePeriodWritten >= mFramePeriodNumber) {
            mRenderPeriodPositionCb->OnPeriodReached(mFramePeriodNumber);
            mFramePeriodWritten %= mFramePeriodNumber;
            mPeriodPositionCBThreads.emplace_back(std::make_unique<std::thread>(
                &RendererPeriodPositionCallback::OnPeriodReached, mRenderPeriodPositionCb, mFramePeriodNumber));
        }
    }
}

size_t AudioContainerClientBase::WriteStreamInCbGa(const StreamBuffer &stream, int32_t &pError, const int32_t &trackId)
{
    lock_guard<mutex> lock(dataMutex_);

    const uint8_t *buffer = stream.buffer;
    size_t length = stream.bufferLen;
    pError = WriteStreamInnerGa(buffer, length, trackId);

    return (stream.bufferLen - length);
}

size_t AudioContainerClientBase::WriteStreamGa(const StreamBuffer &stream, int32_t &pError, const int32_t &trackId)
{
    lock_guard<mutex> lock(dataMutex_);
    int error = 0;
    const uint8_t *inputBuffer = stream.buffer;
    size_t inputLength = stream.bufferLen;
    CHECK_AND_RETURN_RET_LOG(inputBuffer != nullptr, 0, "WriteStreamGa inputBuffer is null");
    error = WriteStreamInnerGa(inputBuffer, inputLength, trackId);

    pError = error;
    return inputLength;
}

int32_t AudioContainerClientBase::UpdateReadBufferGa(uint8_t *buffer, size_t &length, size_t &readSize)
{
    size_t l = (internalRdBufLen_ < length) ? internalRdBufLen_ : length;
    errno_t err = memcpy_s(buffer, length, static_cast<const uint8_t*>(internalReadBuffer_) + internalRdBufIndex_, l);
    CHECK_AND_RETURN_RET(err == 0, AUDIO_CLIENT_READ_STREAM_ERR);

    length -= l;
    internalRdBufIndex_ += l;
    internalRdBufLen_ -= l;
    readSize += l;

    if (!internalRdBufLen_) {
        internalReadBuffer_ = nullptr;
        internalRdBufLen_ = 0;
        internalRdBufIndex_ = 0;
    }

    return AUDIO_CLIENT_SUCCESS;
}

void AudioContainerClientBase::HandleCapturePositionCallbacksGa(size_t bytesRead)
{
    mTotalBytesRead += bytesRead;
    CHECK_AND_RETURN_LOG(mFrameSize != 0, "capturerPeriodPositionCb not set");

    uint64_t readFrameNumber = mTotalBytesRead / mFrameSize;
    if (!mMarkReached && mCapturePositionCb) {
        if (readFrameNumber >= mFrameMarkPosition) {
            mCapturePositionCb->OnMarkReached(mFrameMarkPosition);
            mPositionCBThreads.emplace_back(std::make_unique<std::thread>(&CapturerPositionCallback::OnMarkReached,
                mCapturePositionCb, mFrameMarkPosition));
            mMarkReached = true;
        }
    }

    if (mCapturePeriodPositionCb) {
        mFramePeriodRead += (bytesRead / mFrameSize);
        if (mFramePeriodRead >= mFramePeriodNumber) {
            mCapturePeriodPositionCb->OnPeriodReached(mFramePeriodNumber);
            mFramePeriodRead %= mFramePeriodNumber;
            mPeriodPositionCBThreads.emplace_back(std::make_unique<std::thread>(
                &CapturerPeriodPositionCallback::OnPeriodReached, mCapturePeriodPositionCb, mFramePeriodNumber));
        }
    }
}

int32_t AudioContainerClientBase::ReadStreamGa(StreamBuffer &stream, bool isBlocking, const int32_t &trackId)
{
    lock_guard<mutex> lock(dataMutex_);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");
    data.WriteInt32(static_cast<int32_t>(stream.bufferLen));
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_READ_STREAM, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "ReadStreamGa() failed, error: %{public}d", error);
    reply.ReadUint32(stream.bufferLen);
    size_t readSize = static_cast<size_t>(stream.bufferLen);

    const uint8_t *buffer = reply.ReadBuffer(readSize);
    memcpy_s(stream.buffer, readSize, buffer, readSize);
    return readSize;
}

int32_t AudioContainerClientBase::ReleaseStreamGa(const int32_t &trackId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");

    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_RELEASE_AUDIOSTREAM, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "ReleaseStreamGa() failed, error: %{public}d", error);

    ResetPAAudioClientGa();
    state_ = RELEASED;

    std::shared_ptr<AudioStreamCallback> streamCb = streamCallback_.lock();
    if (streamCb != nullptr) {
        streamCb->OnStateChange(state_);
    }

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::GetMinimumBufferSizeGa(size_t &minBufferSize, const int32_t &trackId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_GET_MINIMUM_BUFFERSIZE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "GetMinimumBufferSizeGa() failed, error: %{public}d", error);

    minBufferSize = static_cast<size_t>(reply.ReadInt64());

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::GetMinimumFrameCountGa(uint32_t &frameCount, const int32_t &trackId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_GET_MINIMUM_FRAMECOUNT, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "GetMinimumFrameCountGa() failed, error: %{public}d", error);

    reply.ReadUint32(frameCount);

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::SetBufferSizeInMsecGa(int32_t bufferSizeInMsec)
{
    return AUDIO_CLIENT_SUCCESS;
}

uint32_t AudioContainerClientBase::GetSamplingRateGa() const
{
    return DEFAULT_SAMPLING_RATE;
}

uint8_t AudioContainerClientBase::GetChannelCountGa() const
{
    return DEFAULT_CHANNEL_COUNT;
}

uint8_t AudioContainerClientBase::GetSampleSizeGa() const
{
    return DEFAULT_SAMPLE_SIZE;
}

int32_t AudioContainerClientBase::GetAudioStreamParamsGa(AudioStreamParams &audioParams, const int32_t &trackId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_GET_AUDIOSTREAM_PARAMS, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "GetAudioStreamParamsGa() failed, error: %{public}d", error);

    reply.ReadUint32(audioParams.samplingRate);
    reply.ReadUint8(audioParams.encoding);
    reply.ReadUint8(audioParams.format);
    reply.ReadUint8(audioParams.channels);
    return AUDIO_CLIENT_SUCCESS;
}

float AudioContainerClientBase::GetStreamVolumeGa()
{
    return mVolumeFactor;
}

uint32_t AudioContainerClientBase::GetStreamVolumeGa(uint32_t sessionID)
{
    return DEFAULT_STREAM_VOLUME;
}

int32_t AudioContainerClientBase::SetStreamVolumeGa(float volume, const int32_t &trackId)
{
    lock_guard<mutex> lock(ctrlMutex_);
    CHECK_AND_RETURN_RET((volume >= MIN_STREAM_VOLUME_LEVEL) && (volume <= MAX_STREAM_VOLUME_LEVEL),
        AUDIO_CLIENT_INVALID_PARAMS_ERR);
    mVolumeFactor = volume;
    int32_t systemVolumeLevel = mAudioSystemMgr->GetVolume(static_cast<AudioVolumeType>(mStreamType));
    float systemVolumeDb = VolumeToDb(systemVolumeLevel);
    float vol = systemVolumeDb * mVolumeFactor;

    AudioRingerMode ringerMode = mAudioSystemMgr->GetRingerMode();
    if ((mStreamType == STREAM_RING) && (ringerMode != RINGER_MODE_NORMAL)) {
        vol = MIN_STREAM_VOLUME_LEVEL;
    }

    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");
    data.WriteFloat(vol);
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_SET_STREAM_VOLUME, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "SetStreamVolumeGa() failed, error: %{public}d", error);

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::GetCurrentTimeStampGa(uint64_t &timeStamp, const int32_t &trackId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_GET_CURRENT_TIMESTAMP, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "GetCurrentTimeStampGa() failed, error: %{public}d", error);

    reply.ReadUint64(timeStamp);

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::GetAudioLatencyGa(uint64_t &latency, const int32_t &trackId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_GET_AUDIO_LATENCY, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE, AUDIO_CLIENT_ERR,
        "GetAudioLatencyGa() failed, error: %{public}d", error);

    reply.ReadUint64(latency);

    return AUDIO_CLIENT_SUCCESS;
}

void AudioContainerClientBase::RegisterAudioRendererCallbackGa(const AudioRendererCallback &cb)
{
    AUDIO_DEBUG_LOG("Registering audio render callbacks");
    mAudioRendererCallbacks = (AudioRendererCallback *)&cb;
}

void AudioContainerClientBase::RegisterAudioCapturerCallbackGa(const AudioCapturerCallback &cb)
{
    AUDIO_DEBUG_LOG("Registering audio recorder callbacks");
    mAudioCapturerCallbacks = (AudioCapturerCallback *)&cb;
}

void AudioContainerClientBase::SetRendererPositionCallbackGa(int64_t markPosition,
    const std::shared_ptr<RendererPositionCallback> &callback)
{
    mFrameMarkPosition = markPosition;
    mRenderPositionCb = callback;
}

void AudioContainerClientBase::UnsetRendererPositionCallbackGa()
{
    mRenderPositionCb = nullptr;
    mMarkReached = false;
    mFrameMarkPosition = 0;
}

void AudioContainerClientBase::SetRendererPeriodPositionCallbackGa(int64_t periodPosition,
    const std::shared_ptr<RendererPeriodPositionCallback> &callback)
{
    mFramePeriodNumber = periodPosition;
    if (mFrameSize != 0 && mFramePeriodNumber != 0) {
        mFramePeriodWritten = (mTotalBytesWritten / mFrameSize) % mFramePeriodNumber;
    } else {
        return;
    }

    mRenderPeriodPositionCb = callback;
}

void AudioContainerClientBase::UnsetRendererPeriodPositionCallbackGa()
{
    mRenderPeriodPositionCb = nullptr;
    mFramePeriodWritten = 0;
    mFramePeriodNumber = 0;
}

void AudioContainerClientBase::SetCapturerPositionCallbackGa(int64_t markPosition,
    const std::shared_ptr<CapturerPositionCallback> &callback)
{
    mFrameMarkPosition = markPosition;
    mCapturePositionCb = callback;
}

void AudioContainerClientBase::UnsetCapturerPositionCallbackGa()
{
    mCapturePositionCb = nullptr;
    mMarkReached = false;
    mFrameMarkPosition = 0;
}

void AudioContainerClientBase::SetCapturerPeriodPositionCallbackGa(int64_t periodPosition,
    const std::shared_ptr<CapturerPeriodPositionCallback> &callback)
{
    mFramePeriodNumber = periodPosition;
    if (mFrameSize != 0 && mFramePeriodNumber != 0) {
        mFramePeriodRead = (mTotalBytesRead / mFrameSize) % mFramePeriodNumber;
    } else {
        return;
    }

    mCapturePeriodPositionCb = callback;
}

void AudioContainerClientBase::UnsetCapturerPeriodPositionCallbackGa()
{
    mCapturePeriodPositionCb = nullptr;
    mFramePeriodRead = 0;
    mFramePeriodNumber = 0;
}

int32_t AudioContainerClientBase::SetStreamTypeGa(AudioStreamType audioStreamType, const int32_t &trackId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");

    data.WriteInt32(static_cast<int32_t>(audioStreamType));
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_SET_STREAM_TYPE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE,
        "SetStreamTypeGa() failed, error: %{public}d", error);

    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::SetStreamRenderRateGa(AudioRendererRate audioRendererRate, const int32_t &trackId)
{
    renderRate = audioRendererRate;
    uint32_t rate = renderRate;

    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool tmp = data.WriteInterfaceToken(GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(tmp, AUDIO_CLIENT_ERR, "WriteInterfaceToken failed");

    data.WriteUint32(rate);
    data.WriteInt32(trackId);
    int32_t error = Remote()->SendRequest(CMD_SET_STREAM_RENDER_RATE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == ERR_NONE,
        "SetStreamRenderRateGa() failed, error: %{public}d", error);

    return AUDIO_CLIENT_SUCCESS;
}

AudioRendererRate AudioContainerClientBase::GetStreamRenderRateGa()
{
    return renderRate;
}

void AudioContainerClientBase::SaveStreamCallbackGa(const std::weak_ptr<AudioStreamCallback> &callback)
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

int32_t AudioContainerClientBase::SetAudioCaptureMode(AudioCaptureMode captureMode)
{
    return AUDIO_CLIENT_SUCCESS;
}

int32_t AudioContainerClientBase::SaveReadCallback(const std::weak_ptr<AudioCapturerReadCallback> &callback)
{
    return AUDIO_CLIENT_SUCCESS;
}

AudioCaptureMode AudioContainerClientBase::GetAudioCaptureMode()
{
    return AudioCaptureMode::CAPTURE_MODE_NORMAL;
}

void AudioContainerClientBase::SetAppCachePath(const std::string cachePath)
{
    AUDIO_INFO_LOG("cachePath %{public}s", cachePath.c_str());
}
} // namespace AudioStandard
} // namespace OHOS

