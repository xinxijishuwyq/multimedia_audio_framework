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

#include <memory>

#include "ipc_stream_in_server.h"

#include "audio_log.h"
#include "audio_errors.h"

// #include "audio_renderer_in_server.h"
// #include "audio_capturer_in_server.h"

namespace OHOS {
namespace AudioStandard {
StreamListenerHolder::StreamListenerHolder()
{
    AUDIO_INFO_LOG("StreamListenerHolder()");
}

StreamListenerHolder::~StreamListenerHolder()
{
    AUDIO_INFO_LOG("~StreamListenerHolder()");
}

int32_t StreamListenerHolder::RegisterStreamListener(sptr<IpcStreamListener> listener)
{
    std::lock_guard<std::mutex> lock(listenerMutex_);
    // should only be set once
    if (streamListener_ != nullptr) {
        return ERR_INVALID_OPERATION;
    }
    streamListener_ = listener;
    return SUCCESS;
}

int32_t StreamListenerHolder::OnOperationHandled(Operation operation, int64_t result)
{
    std::lock_guard<std::mutex> lock(listenerMutex_);
    CHECK_AND_RETURN_RET_LOG(streamListener_ != nullptr, ERR_OPERATION_FAILED, "stream listrener not set");
    return streamListener_->OnOperationHandled(operation, result);
}

sptr<IpcStreamInServer> IpcStreamInServer::Create(const AudioProcessConfig &config, int32_t &ret)
{
    AudioMode mode = config.audioMode;
    sptr<IpcStreamInServer> streamInServer = new(std::nothrow) IpcStreamInServer(config, mode);
    ret = streamInServer->Config();
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("IpcStreamInServer Config failed: %{public}d", ret); // in plan: add uid.
        streamInServer = nullptr;
    }
    return streamInServer;
}

IpcStreamInServer::IpcStreamInServer(const AudioProcessConfig &config, AudioMode mode) : config_(config), mode_(mode)
{
    AUDIO_INFO_LOG("IpcStreamInServer()"); // in plan: add uid.
}

IpcStreamInServer::~IpcStreamInServer()
{
    AUDIO_INFO_LOG("~IpcStreamInServer()"); // in plan: add uid.
}

int32_t IpcStreamInServer::Config()
{
    streamListenerHolder_ = std::make_shared<StreamListenerHolder>();

    // LYH in plan: pass streamListenerHolder_ to RendererInServer or CapturerInServer
    if (mode_ == AUDIO_MODE_PLAYBACK) {
        return ConfigRenderer();
    }
    if (mode_ == AUDIO_MODE_RECORD) {
        return ConfigCapturer();
    }
    AUDIO_ERR_LOG("Config failed, mode is %{public}d", static_cast<int32_t>(mode_));
    return ERR_OPERATION_FAILED;
}

int32_t IpcStreamInServer::ConfigRenderer()
{
    // LYH in plan: use config_.streamInfo instead of AudioStreamParams
    AudioStreamParams params = {};
    rendererInServer_ = std::make_shared<RendererInServer>(params, config_.streamType);
    CHECK_AND_RETURN_RET_LOG(rendererInServer_ != nullptr, ERR_OPERATION_FAILED, "create RendererInServer failed");
    return SUCCESS;
}

int32_t IpcStreamInServer::ConfigCapturer()
{
    // LYH in plan: use config_.streamInfo instead of AudioStreamParams
    AudioStreamParams params = {};
    capturerInServer_ = std::make_shared<CapturerInServer>(params, config_.streamType);
    CHECK_AND_RETURN_RET_LOG(capturerInServer_ != nullptr, ERR_OPERATION_FAILED, "create CapturerInServer failed");
    return SUCCESS;
}

int32_t IpcStreamInServer::RegisterStreamListener(sptr<IRemoteObject> object)
{
    CHECK_AND_RETURN_RET_LOG(streamListenerHolder_ != nullptr, ERR_OPERATION_FAILED, "RegisterStreamListener failed");
    sptr<IpcStreamListener> listener = iface_cast<IpcStreamListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, ERR_INVALID_PARAM, "RegisterStreamListener obj cast failed");
    streamListenerHolder_->RegisterStreamListener(listener);

    // in plan: get session id, use it as key to find IpcStreamInServer
    // in plan: listener->AddDeathRecipient( server ) // when client died, do release and clear works

    return SUCCESS;
}

int32_t IpcStreamInServer::ResolveBuffer(std::shared_ptr<OHAudioBuffer> &buffer)
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::UpdatePosition()
{
    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::GetAudioSessionID(uint32_t &sessionId)
{
    if (mode_ == AUDIO_MODE_PLAYBACK && rendererInServer_ != nullptr) {
        return rendererInServer_->GetSessionId(sessionId);
    }
    if (mode_ == AUDIO_MODE_RECORD && capturerInServer_!= nullptr) {
        return capturerInServer_->GetSessionId(sessionId);
    }
    AUDIO_ERR_LOG("GetAudioSessionID failed, invalid mode: %{public}d", static_cast<int32_t>(mode_));
    return ERR_OPERATION_FAILED;
}

int32_t IpcStreamInServer::Start()
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::Pause()
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::Stop()
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::Release()
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::Flush()
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::Drain()
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::GetAudioTime(uint64_t &framePos, int64_t &timeStamp)
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::GetLatency(uint64_t &latency)
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::SetRate(int32_t rate)
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::GetRate(int32_t &rate)
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::SetLowPowerVolume(float volume)
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::GetLowPowerVolume(float &volume)
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::SetAudioEffectMode(int32_t effectMode)
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::GetAudioEffectMode(int32_t &effectMode)
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::SetPrivacyType(int32_t privacyType)
{

    // in plan
    return SUCCESS;
}

int32_t IpcStreamInServer::GetPrivacyType(int32_t &privacyType)
{

    // in plan
    return SUCCESS;
}
} // namespace AudioStandard
} // namespace OHOS
