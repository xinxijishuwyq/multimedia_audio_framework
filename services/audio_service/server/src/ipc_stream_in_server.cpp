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
StreamListenerHolder::StreamListenerHolder(sptr<IpcStreamListener> listener) : streamListener_(listener)
{
    AUDIO_INFO_LOG("StreamListenerHolder()");
}

StreamListenerHolder::~StreamListenerHolder()
{
    AUDIO_INFO_LOG("~StreamListenerHolder()");
}

IpcStreamInServer::IpcStreamInServer(const AudioProcessConfig &config) : config_(config)
{
    AUDIO_INFO_LOG("IpcStreamInServer()"); // in plan: add uid.
}

IpcStreamInServer::~IpcStreamInServer()
{
    AUDIO_INFO_LOG("~IpcStreamInServer()"); // in plan: add uid.
}

int32_t IpcStreamInServer::RegisterStreamListener(sptr<IRemoteObject> object)
{

    // in plan
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
