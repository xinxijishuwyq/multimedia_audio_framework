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

#ifndef ST_AUDIO_ROUTER_CENTER_H
#define ST_AUDIO_ROUTER_CENTER_H

#include "router_base.h"
#include "user_select_router.h"
#include "privacy_priority_router.h"
#include "public_priority_router.h"
#include "package_filter_router.h"
#include "stream_filter_router.h"
#include "cockpit_phone_router.h"
#include "pair_device_router.h"
#include "default_router.h"
#include "audio_stream_collector.h"

namespace OHOS {
namespace AudioStandard {
class AudioRouterCenter {
public:
    static AudioRouterCenter& GetAudioRouterCenter()
    {
        static AudioRouterCenter audioRouterCenter;
        return audioRouterCenter;
    }

    std::unique_ptr<AudioDeviceDescriptor> FetchOutputDevice(StreamUsage streamUsage, int32_t clientUID);
    std::unique_ptr<AudioDeviceDescriptor> FetchInputDevice(SourceType sourceType, int32_t clientUID);
private:
    AudioRouterCenter()
    {
        // media render router
        mediaRenderRouters_.push_back(std::make_unique<UserSelectRouter>());
        mediaRenderRouters_.push_back(std::make_unique<PrivacyPriorityRouter>());
        mediaRenderRouters_.push_back(std::make_unique<PublicPriorityRouter>());
        mediaRenderRouters_.push_back(std::make_unique<StreamFilterRouter>());
        mediaRenderRouters_.push_back(std::make_unique<DefaultRouter>());

        // record capture router
        recordCaptureRouters_.push_back(std::make_unique<UserSelectRouter>());
        recordCaptureRouters_.push_back(std::make_unique<PrivacyPriorityRouter>());
        recordCaptureRouters_.push_back(std::make_unique<PublicPriorityRouter>());
        recordCaptureRouters_.push_back(std::make_unique<StreamFilterRouter>());
        recordCaptureRouters_.push_back(std::make_unique<DefaultRouter>());

        // call render router
        callRenderRouters_.push_back(std::make_unique<UserSelectRouter>());
        callRenderRouters_.push_back(std::make_unique<PrivacyPriorityRouter>());
        callRenderRouters_.push_back(std::make_unique<CockpitPhoneRouter>());
        callRenderRouters_.push_back(std::make_unique<DefaultRouter>());

        // call capture router
        callCaptureRouters_.push_back(std::make_unique<UserSelectRouter>());
        callCaptureRouters_.push_back(std::make_unique<PairDeviceRouter>());
        callCaptureRouters_.push_back(std::make_unique<PrivacyPriorityRouter>());
        callCaptureRouters_.push_back(std::make_unique<CockpitPhoneRouter>());
        callCaptureRouters_.push_back(std::make_unique<DefaultRouter>());
    };

    ~AudioRouterCenter() {};

    unique_ptr<AudioDeviceDescriptor> FetchMediaRenderDevice(StreamUsage streamUsage, int32_t clientUID);
    unique_ptr<AudioDeviceDescriptor> FetchCallRenderDevice(StreamUsage streamUsage, int32_t clientUID);

    std::vector<std::unique_ptr<RouterBase>> mediaRenderRouters_;
    std::vector<std::unique_ptr<RouterBase>> callRenderRouters_;
    std::vector<std::unique_ptr<RouterBase>> callCaptureRouters_;
    std::vector<std::unique_ptr<RouterBase>> ringRenderRouters_;
    std::vector<std::unique_ptr<RouterBase>> toneRenderRouters_;
    std::vector<std::unique_ptr<RouterBase>> recordCaptureRouters_;

    static unordered_map<StreamUsage, string> renderConfigMap_;
    static unordered_map<SourceType, string> capturerConfigMap_;
};

} // namespace AudioStandard
} // namespace OHOS

#endif // ST_AUDIO_ROUTER_CENTER_H