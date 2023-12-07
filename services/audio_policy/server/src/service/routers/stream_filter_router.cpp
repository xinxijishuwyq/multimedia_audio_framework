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

#include "stream_filter_router.h"
#include "audio_log.h"
#include "audio_policy_service.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {

unique_ptr<AudioDeviceDescriptor> StreamFilterRouter::GetMediaRenderDevice(StreamUsage streamUsage,
    int32_t clientUID)
{
    DistributedRoutingInfo routingInfo = AudioPolicyService::GetAudioPolicyService().GetDistributedRoutingRoleInfo();
    if (routingInfo.descriptor != nullptr) {
        sptr<AudioDeviceDescriptor> deviceDescriptor = routingInfo.descriptor;
        CastType type = routingInfo.type;
        bool hasDescriptor = false;
        AUDIO_ERR_LOG("StreamfilterRouter GetMediaRenderDevice streamUsage %{public}d clientUid %{public}d",
            streamUsage, clientUID);

        switch (type) {
            case CAST_TYPE_NULL: {
                break;
            }
            case CAST_TYPE_ALL: {
                vector<unique_ptr<AudioDeviceDescriptor>> descriptors =
                    AudioDeviceManager::GetAudioDeviceManager().GetRemoteRenderDevices();
                hasDescriptor = AudioPolicyService::GetAudioPolicyService().
                    IsIncomingDeviceInRemoteDevice(descriptors, deviceDescriptor);
                break;
            }
            case CAST_TYPE_PROJECTION: {
                if (streamUsage == STREAM_USAGE_MUSIC) {
                    vector<unique_ptr<AudioDeviceDescriptor>> descriptors =
                        AudioDeviceManager::GetAudioDeviceManager().GetRemoteRenderDevices();
                    hasDescriptor = AudioPolicyService::GetAudioPolicyService().
                        IsIncomingDeviceInRemoteDevice(descriptors, deviceDescriptor);
                }
                break;
            }
            case CAST_TYPE_COOPERATION: {
                break;
            }
            default: {
                AUDIO_ERR_LOG("GetMediaRenderDevice unhandled castc type: %{public}d", type);
                break;
            }
        }
        if (hasDescriptor) {
            unique_ptr<AudioDeviceDescriptor> incomingDevice = make_unique<AudioDeviceDescriptor>(deviceDescriptor);
            return incomingDevice;
        }
    }
    return make_unique<AudioDeviceDescriptor>();
}

unique_ptr<AudioDeviceDescriptor> StreamFilterRouter::GetCallRenderDevice(StreamUsage streamUsage,
    int32_t clientUID)
{
    DistributedRoutingInfo routingInfo = AudioPolicyService::GetAudioPolicyService().GetDistributedRoutingRoleInfo();
    if (routingInfo.descriptor != nullptr) {
        sptr<AudioDeviceDescriptor> deviceDescriptor = routingInfo.descriptor;
        CastType type = routingInfo.type;
        bool hasDescriptor = false;
        AUDIO_ERR_LOG("StreamfilterRouter GetMediaRenderDevice streamUsage %{public}d clientUid %{public}d",
            streamUsage, clientUID);

        switch (type) {
            case CAST_TYPE_NULL: {
                break;
            }
            case CAST_TYPE_ALL: {
                vector<unique_ptr<AudioDeviceDescriptor>> descriptors =
                    AudioDeviceManager::GetAudioDeviceManager().GetRemoteRenderDevices();
                hasDescriptor = AudioPolicyService::GetAudioPolicyService().
                    IsIncomingDeviceInRemoteDevice(descriptors, deviceDescriptor);
                break;
            }
            case CAST_TYPE_PROJECTION: {
                break;
            }
            case CAST_TYPE_COOPERATION: {
                break;
            }
            default: {
                AUDIO_ERR_LOG("GetMediaRenderDevice unhandled castc type: %{public}d", type);
                break;
            }
        }
        if (hasDescriptor) {
            unique_ptr<AudioDeviceDescriptor> incomingDevice = make_unique<AudioDeviceDescriptor>(deviceDescriptor);
            return incomingDevice;
        }
    }
    return make_unique<AudioDeviceDescriptor>();
}

unique_ptr<AudioDeviceDescriptor> StreamFilterRouter::GetCallCaptureDevice(SourceType sourceType,
    int32_t clientUID)
{
    DistributedRoutingInfo routingInfo = AudioPolicyService::GetAudioPolicyService().GetDistributedRoutingRoleInfo();
    if (routingInfo.descriptor != nullptr) {
        sptr<AudioDeviceDescriptor> deviceDescriptor = routingInfo.descriptor;
        CastType type = routingInfo.type;
        bool hasDescriptor = false;

        switch (type) {
            case CAST_TYPE_NULL: {
                break;
            }
            case CAST_TYPE_ALL: {
                vector<unique_ptr<AudioDeviceDescriptor>> descriptors =
                    AudioDeviceManager::GetAudioDeviceManager().GetRemoteCaptureDevices();
                hasDescriptor = AudioPolicyService::GetAudioPolicyService().
                    IsIncomingDeviceInRemoteDevice(descriptors, deviceDescriptor);
                break;
            }
            case CAST_TYPE_PROJECTION: {
                break;
            }
            case CAST_TYPE_COOPERATION: {
                break;
            }
            default: {
                AUDIO_ERR_LOG("GetMediaRenderDevice unhandled castc type: %{public}d", type);
                break;
            }
        }
        if (hasDescriptor) {
            unique_ptr<AudioDeviceDescriptor> incomingDevice = make_unique<AudioDeviceDescriptor>(deviceDescriptor);
            return incomingDevice;
        }
    }
    return make_unique<AudioDeviceDescriptor>();
}

unique_ptr<AudioDeviceDescriptor> StreamFilterRouter::GetRingRenderDevice(StreamUsage streamUsage,
    int32_t clientUID)
{
    return make_unique<AudioDeviceDescriptor>();
}

unique_ptr<AudioDeviceDescriptor> StreamFilterRouter::GetRecordCaptureDevice(SourceType sourceType,
    int32_t clientUID)
{
    DistributedRoutingInfo routingInfo = AudioPolicyService::GetAudioPolicyService().GetDistributedRoutingRoleInfo();
    if (routingInfo.descriptor != nullptr) {
        sptr<AudioDeviceDescriptor> deviceDescriptor = routingInfo.descriptor;
        CastType type = routingInfo.type;
        bool hasDescriptor = false;

        switch (type) {
            case CAST_TYPE_NULL: {
                break;
            }
            case CAST_TYPE_ALL: {
                vector<unique_ptr<AudioDeviceDescriptor>> descriptors =
                    AudioDeviceManager::GetAudioDeviceManager().GetRemoteCaptureDevices();
                hasDescriptor = AudioPolicyService::GetAudioPolicyService().
                    IsIncomingDeviceInRemoteDevice(descriptors, deviceDescriptor);
                break;
            }
            case CAST_TYPE_PROJECTION: {
                if (sourceType == SOURCE_TYPE_MIC) {
                    vector<unique_ptr<AudioDeviceDescriptor>> descriptors =
                        AudioDeviceManager::GetAudioDeviceManager().GetRemoteCaptureDevices();
                    hasDescriptor = AudioPolicyService::GetAudioPolicyService().
                        IsIncomingDeviceInRemoteDevice(descriptors, deviceDescriptor);
                }
                break;
            }
            case CAST_TYPE_COOPERATION: {
                break;
            }
            default: {
                AUDIO_ERR_LOG("GetMediaRenderDevice unhandled castc type: %{public}d", type);
                break;
            }
        }
        if (hasDescriptor) {
            unique_ptr<AudioDeviceDescriptor> incomingDevice = make_unique<AudioDeviceDescriptor>(deviceDescriptor);
            return incomingDevice;
        }
    }
    return make_unique<AudioDeviceDescriptor>();
}

unique_ptr<AudioDeviceDescriptor> StreamFilterRouter::GetToneRenderDevice(StreamUsage streamUsage,
    int32_t clientUID)
{
    return make_unique<AudioDeviceDescriptor>();
}

} // namespace AudioStandard
} // namespace OHOS