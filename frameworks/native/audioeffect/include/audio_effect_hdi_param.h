/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef AUDIO_EFFECT_HDI_PARAM_H
#define AUDIO_EFFECT_HDI_PARAM_H

#include <mutex>
#include "v1_0/ieffect_model.h"

namespace OHOS {
namespace AudioStandard {
const uint32_t SEND_HDI_COMMAND_LEN = 20;
const uint32_t GET_HDI_BUFFER_LEN = 10;
class AudioEffectHdiParam {
public:
    AudioEffectHdiParam();
    ~AudioEffectHdiParam();
    void InitHdi();
    int32_t UpdateHdiState(int8_t *effectHdiInput);
private:
    int8_t input[SEND_HDI_COMMAND_LEN];
    int8_t output[GET_HDI_BUFFER_LEN];
    uint32_t replyLen;
    std::string libName;
    std::string effectId;
    IEffectModel *hdiModel_;
    std::vector<IEffectControl *> libHdiControls_;
    void CreateHdiControl();
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif // AUDIO_EFFECT_HDI_PARAM_H