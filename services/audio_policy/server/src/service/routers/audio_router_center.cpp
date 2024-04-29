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
#undef LOG_TAG
#define LOG_TAG "AudioRouterCenter"

#include "audio_router_center.h"
#include "audio_policy_service.h"
#include "audio_log.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {

const string MEDIA_RENDER_ROUTERS = "MediaRenderRouters";
const string CALL_RENDER_ROUTERS = "CallRenderRouters";
const string RECORD_CAPTURE_ROUTERS = "RecordCaptureRouters";
const string CALL_CAPTURE_ROUTERS = "CallCaptureRouters";
const string RING_RENDER_ROUTERS = "RingRenderRouters";
const string TONE_RENDER_ROUTERS = "ToneRenderRouters";

unique_ptr<AudioDeviceDescriptor> AudioRouterCenter::FetchMediaRenderDevice(
    StreamUsage streamUsage, int32_t clientUID, RouterType &routerType)
{
    for (auto &router : mediaRenderRouters_) {
        unique_ptr<AudioDeviceDescriptor> desc = router->GetMediaRenderDevice(streamUsage, clientUID);
        if (desc->deviceType_ != DEVICE_TYPE_NONE) {
            routerType = router->GetRouterType();
            return desc;
        }
    }
    return make_unique<AudioDeviceDescriptor>();
}

unique_ptr<AudioDeviceDescriptor> AudioRouterCenter::FetchCallRenderDevice(
    StreamUsage streamUsage, int32_t clientUID, RouterType &routerType)
{
    for (auto &router : callRenderRouters_) {
        unique_ptr<AudioDeviceDescriptor> desc = router->GetCallRenderDevice(streamUsage, clientUID);
        if (desc->deviceType_ != DEVICE_TYPE_NONE) {
            routerType = router->GetRouterType();
            return desc;
        }
    }
    return make_unique<AudioDeviceDescriptor>();
}

bool AudioRouterCenter::HasScoDevice()
{
    vector<unique_ptr<AudioDeviceDescriptor>> descs =
        AudioDeviceManager::GetAudioDeviceManager().GetCommRenderPrivacyDevices();
    for (auto &desc : descs) {
        if (desc->deviceType_ == DEVICE_TYPE_BLUETOOTH_SCO) {
            return true;
        }
    }

    vector<unique_ptr<AudioDeviceDescriptor>> publicDescs =
        AudioDeviceManager::GetAudioDeviceManager().GetCommRenderPublicDevices();
    for (auto &desc : publicDescs) {
        if (desc->deviceType_ == DEVICE_TYPE_BLUETOOTH_SCO && desc->deviceCategory_ == BT_CAR) {
            return true;
        }
    }
    return false;
}

unique_ptr<AudioDeviceDescriptor> AudioRouterCenter::FetchOutputDevice(StreamUsage streamUsage, int32_t clientUID)
{
    AUDIO_INFO_LOG("streamUsage %{public}d clientUID %{public}d start fetch device", streamUsage, clientUID);
    unique_ptr<AudioDeviceDescriptor> desc = make_unique<AudioDeviceDescriptor>();
    RouterType routerType = ROUTER_TYPE_NONE;
    if (renderConfigMap_[streamUsage] == MEDIA_RENDER_ROUTERS ||
        renderConfigMap_[streamUsage] == RING_RENDER_ROUTERS ||
        renderConfigMap_[streamUsage] == TONE_RENDER_ROUTERS) {
        AudioScene audioScene = AudioPolicyService::GetAudioPolicyService().GetAudioScene();
        if (audioScene == AUDIO_SCENE_PHONE_CALL || audioScene == AUDIO_SCENE_PHONE_CHAT ||
            (audioScene == AUDIO_SCENE_RINGING && HasScoDevice())) {
            auto isPresent = [] (const unique_ptr<RouterBase> &router) {
                return router->name_ == "package_filter_router";
            };
            auto itr = find_if(mediaRenderRouters_.begin(), mediaRenderRouters_.end(), isPresent);
            if (itr != mediaRenderRouters_.end()) {
                desc = (*itr)->GetMediaRenderDevice(streamUsage, clientUID);
                routerType = (*itr)->GetRouterType();
            }
            if (desc->deviceType_ == DEVICE_TYPE_NONE) {
                streamUsage = audioScene == AUDIO_SCENE_PHONE_CALL ? STREAM_USAGE_VOICE_MODEM_COMMUNICATION
                                                               : STREAM_USAGE_VOICE_COMMUNICATION;
                desc = FetchCallRenderDevice(streamUsage, clientUID, routerType);
            }
        } else {
            desc = FetchMediaRenderDevice(streamUsage, clientUID, routerType);
        }
    } else if (renderConfigMap_[streamUsage] == CALL_RENDER_ROUTERS) {
        desc = FetchCallRenderDevice(streamUsage, clientUID, routerType);
    } else {
        AUDIO_INFO_LOG("streamUsage %{public}d didn't config router strategy, skipped", streamUsage);
        return desc;
    }
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    if (audioDeviceRefinerCb_ != nullptr) {
        audioDeviceRefinerCb_->OnAudioOutputDeviceRefined(descs, routerType, streamUsage, clientUID, PRIMARY);
    }
    AUDIO_INFO_LOG("streamUsage %{public}d clientUID %{public}d fetch device %{public}d", streamUsage, clientUID,
        descs[0]->deviceType_);
    return move(descs[0]);
}

unique_ptr<AudioDeviceDescriptor> AudioRouterCenter::FetchInputDevice(SourceType sourceType, int32_t clientUID)
{
    AUDIO_INFO_LOG("sourceType %{public}d clientUID %{public}d start fetch input device", sourceType, clientUID);
    unique_ptr<AudioDeviceDescriptor> desc = make_unique<AudioDeviceDescriptor>();
    RouterType routerType = ROUTER_TYPE_NONE;
    if (capturerConfigMap_[sourceType] == "RecordCaptureRouters") {
        for (auto &router : recordCaptureRouters_) {
            desc = router->GetRecordCaptureDevice(sourceType, clientUID);
            if (desc->deviceType_ != DEVICE_TYPE_NONE) {
                routerType = router->GetRouterType();
                break;
            }
        }
    } else if (capturerConfigMap_[sourceType] == "CallCaptureRouters") {
        for (auto &router : callCaptureRouters_) {
            desc = router->GetCallCaptureDevice(sourceType, clientUID);
            if (desc->deviceType_ != DEVICE_TYPE_NONE) {
                routerType = router->GetRouterType();
                break;
            }
        }
    } else if (capturerConfigMap_[sourceType] == "VoiceMessages") {
        for (auto &router : voiceMessageRouters_) {
            desc = router->GetRecordCaptureDevice(sourceType, clientUID);
            if (desc->deviceType_ != DEVICE_TYPE_NONE) {
                routerType = router->GetRouterType();
                break;
            }
        }
    } else {
        AUDIO_INFO_LOG("sourceType %{public}d didn't config router strategy, skipped", sourceType);
        return desc;
    }
    vector<unique_ptr<AudioDeviceDescriptor>> descs;
    descs.push_back(make_unique<AudioDeviceDescriptor>(*desc));
    if (audioDeviceRefinerCb_ != nullptr) {
        audioDeviceRefinerCb_->OnAudioInputDeviceRefined(descs, routerType, sourceType, clientUID, PRIMARY);
    }
    AUDIO_INFO_LOG("sourceType %{public}d clientUID %{public}d fetch device %{public}d", sourceType, clientUID,
        descs[0]->deviceType_);
    return move(descs[0]);
}

int32_t AudioRouterCenter::SetAudioDeviceRefinerCallback(const sptr<IRemoteObject> &object)
{
    sptr<IStandardAudioRoutingManagerListener> listener = iface_cast<IStandardAudioRoutingManagerListener>(object);
    if (listener != nullptr) {
        audioDeviceRefinerCb_ = listener;
        return SUCCESS;
    } else {
        return ERROR;
    }
}

int32_t AudioRouterCenter::UnsetAudioDeviceRefinerCallback()
{
    audioDeviceRefinerCb_ = nullptr;
    return SUCCESS;
}

} // namespace AudioStandard
} // namespace OHOS