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

unique_ptr<AudioDeviceDescriptor> AudioRouterCenter::FetchMediaRenderDevice(StreamUsage streamUsage, int32_t clientUID)
{
    for (auto &router : mediaRenderRouters_) {
        unique_ptr<AudioDeviceDescriptor> desc = router->GetMediaRenderDevice(streamUsage, clientUID);
        if (desc->deviceType_ != DEVICE_TYPE_NONE) {
            AUDIO_INFO_LOG("MediaRender streamUsage %{public}d clientUID %{public}d fetch device %{public}d",
                streamUsage, clientUID, desc->deviceType_);
            return desc;
        }
    }
    return make_unique<AudioDeviceDescriptor>();
}

unique_ptr<AudioDeviceDescriptor> AudioRouterCenter::FetchCallRenderDevice(StreamUsage streamUsage, int32_t clientUID)
{
    for (auto &router : callRenderRouters_) {
        unique_ptr<AudioDeviceDescriptor> desc = router->GetCallRenderDevice(streamUsage, clientUID);
        if (desc->deviceType_ != DEVICE_TYPE_NONE) {
            AUDIO_INFO_LOG("CallRender streamUsage %{public}d clientUID %{public}d fetch device %{public}d",
                streamUsage, clientUID, desc->deviceType_);
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
            unique_ptr<AudioDeviceDescriptor> desc = make_unique<AudioDeviceDescriptor>();
            if (itr != mediaRenderRouters_.end()) {
                desc = (*itr)->GetMediaRenderDevice(streamUsage, clientUID);
            }
            if (desc->deviceType_ != DEVICE_TYPE_NONE) {
                AUDIO_INFO_LOG("CallRender streamUsage %{public}d clientUID %{public}d fetch device %{public}d",
                    streamUsage, clientUID, desc->deviceType_);
                return desc;
            }
            streamUsage = audioScene == AUDIO_SCENE_PHONE_CALL ? STREAM_USAGE_VOICE_MODEM_COMMUNICATION
                                                               : STREAM_USAGE_VOICE_COMMUNICATION;
            desc = FetchCallRenderDevice(streamUsage, clientUID);
            if (desc->deviceType_ != DEVICE_TYPE_NONE) {
                return desc;
            }
        } else {
            unique_ptr<AudioDeviceDescriptor> desc = FetchMediaRenderDevice(streamUsage, clientUID);
            if (desc->deviceType_ != DEVICE_TYPE_NONE) {
                return desc;
            }
        }
    } else if (renderConfigMap_[streamUsage] == CALL_RENDER_ROUTERS) {
        unique_ptr<AudioDeviceDescriptor> desc = FetchCallRenderDevice(streamUsage, clientUID);
        if (desc->deviceType_ != DEVICE_TYPE_NONE) {
            return desc;
        }
    }
    AUDIO_DEBUG_LOG("streamUsage %{public}d clientUID %{public}d fetch no device", streamUsage, clientUID);
    return make_unique<AudioDeviceDescriptor>();
}

unique_ptr<AudioDeviceDescriptor> AudioRouterCenter::FetchInputDevice(SourceType sourceType, int32_t clientUID)
{
    AUDIO_INFO_LOG("sourceType %{public}d clientUID %{public}d start fetch input device", sourceType, clientUID);
    if (capturerConfigMap_[sourceType] == "RecordCaptureRouters") {
        for (auto &router : recordCaptureRouters_) {
            unique_ptr<AudioDeviceDescriptor> desc = router->GetRecordCaptureDevice(sourceType, clientUID);
            if (desc->deviceType_ != DEVICE_TYPE_NONE) {
                AUDIO_INFO_LOG("RecordCapture sourceType %{public}d clientUID %{public}d fetch input device %{public}d",
                    sourceType, clientUID, desc->deviceType_);
                return desc;
            }
        }
    } else if (capturerConfigMap_[sourceType] == "CallCaptureRouters") {
        for (auto &router : callCaptureRouters_) {
            unique_ptr<AudioDeviceDescriptor> desc = router->GetCallCaptureDevice(sourceType, clientUID);
            if (desc->deviceType_ != DEVICE_TYPE_NONE) {
                AUDIO_INFO_LOG("CallCapture sourceType %{public}d clientUID %{public}d fetch input device %{public}d",
                    sourceType, clientUID, desc->deviceType_);
                return desc;
            }
        }
    }
    AUDIO_DEBUG_LOG("sourceType %{public}d clientUID %{public}d fetch no device", sourceType, clientUID);
    return make_unique<AudioDeviceDescriptor>();
}

} // namespace AudioStandard
} // namespace OHOS