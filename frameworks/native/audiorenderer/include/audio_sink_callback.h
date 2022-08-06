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
#ifndef ST_AUDIO_SINK_CALLBACK_H
#define ST_AUDIO_SINK_CALLBACK_H

#include "audio_info.h"
namespace OHOS {
namespace AudioStandard {
class AudioSinkCallback {
public:
    virtual void OnAudioParameterChange(std::string netWorkId, const AudioParamKey key, const std::string& condition,
        const std::string value) = 0;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // ST_AUDIO_SINK_CALLBACK_H
