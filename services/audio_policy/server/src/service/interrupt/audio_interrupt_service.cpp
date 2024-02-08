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

#include "audio_interrupt_service.h"

#include "audio_log.h"
#include "audio_errors.h"

namespace OHOS {
namespace AudioStandard {

AudioInterruptService::AudioInterruptService()
{
}

AudioInterruptService::~AudioInterruptService()
{
    AUDIO_ERR_LOG("should not happen");
}

int32_t AudioInterruptService::SetAudioManagerInterruptCallback(const sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_PARAM,
        "object is nullptr");

    sptr<IStandardAudioPolicyManagerListener> listener = iface_cast<IStandardAudioPolicyManagerListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, ERR_INVALID_PARAM,
        "obj cast failed");

    std::shared_ptr<AudioInterruptCallback> callback = std::make_shared<AudioPolicyManagerListenerCallback>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_PARAM,
        "create cb failed");

    int32_t callerPid = IPCSkeleton::GetCallingPid();

    if (handler_ != nullptr) {
        handler_->AddExternInterruptCbsMap(callerPid, callback);
    }

    AUDIO_DEBUG_LOG("for client id %{public}d done", callerPid);

    return SUCCESS;
}

int32_t AudioInterruptService::UnsetAudioManagerInterruptCallback()
{
    int32_t callerPid = IPCSkeleton::GetCallingPid();
    if (handler_ != nullptr) {
        return handler_->RemoveExternInterruptCbsMap(callerPid);
    }

    return SUCCESS;
}

int32_t AudioInterruptService::RequestAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt)
{
    AUDIO_INFO_LOG("in");
    std::lock_guard<std::mutex> lock(mutex_);

    if (clientOnFocus_ == clientId) {
        AUDIO_INFO_LOG("client already has focus");
        NotifyFocusGranted(clientId, audioInterrupt);
        return SUCCESS;
    }

    if (focussedAudioInterruptInfo_ != nullptr) {
        AUDIO_INFO_LOG("Existing stream: %{public}d, incoming stream: %{public}d",
            (focussedAudioInterruptInfo_->audioFocusType).streamType, audioInterrupt.audioFocusType.streamType);
        NotifyFocusAbandoned(clientOnFocus_, *focussedAudioInterruptInfo_);
        AbandonAudioFocus(clientOnFocus_, *focussedAudioInterruptInfo_);
    }

    NotifyFocusGranted(clientId, audioInterrupt);

    return SUCCESS;
}

int32_t AudioInterruptService::AbandonAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt)
{
    AUDIO_INFO_LOG("in");
    std::lock_guard<std::mutex> lock(mutex_);

    if (clientId == clientOnFocus_) {
        AUDIO_INFO_LOG("remove app focus");
        focussedAudioInterruptInfo_.reset();
        focussedAudioInterruptInfo_ = nullptr;
        clientOnFocus_ = 0;
    }

    return SUCCESS;
}

} // namespace AudioStandard
} // namespace OHOS
