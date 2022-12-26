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

#ifndef AUDIO_SERVICE_H
#define AUDIO_SERVICE_H

#include <set>
#include <map>

#include "audio_process_in_server.h"
#include "audio_endpoint.h"

namespace OHOS {
namespace AudioStandard {
class AudioService {
public:
    static AudioService *GetInstance();
    ~AudioService();
    sptr<AudioProcessInServer> GetAudioProcess(const AudioProcessConfig &config);

    DeviceInfo GetDeviceInfoForProcess(const AudioProcessConfig &config);
    std::shared_ptr<AudioEndpoint> GetAudioEndpointForDevice(DeviceInfo deviceInfo);

    int32_t LinkProcessToEndpoint(sptr<AudioProcessInServer> process, std::shared_ptr<AudioEndpoint> endpoint);
private:
    AudioService();
    void Dump();
    std::set<sptr<AudioProcessInServer>> processList_;
    std::map<int32_t, std::shared_ptr<AudioEndpoint>> endpointList_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_SERVICE_H
