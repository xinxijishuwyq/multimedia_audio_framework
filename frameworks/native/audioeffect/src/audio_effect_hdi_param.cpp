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

#include "audio_effect.h"
#include "audio_effect_hdi_param.h"
#include "audio_errors.h"
#include "audio_log.h"
#include "securec.h"

namespace OHOS {
namespace AudioStandard {
AudioEffectHdiParam::AudioEffectHdiParam()
{
    AUDIO_DEBUG_LOG("constructor.");
    libHdiControls_.clear();
    memset_s(static_cast<void *>(input_), sizeof(input_), 0, sizeof(input_));
    memset_s(static_cast<void *>(output_), sizeof(output_), 0, sizeof(output_));
    replyLen_ = GET_HDI_BUFFER_LEN;
    hdiModel_ = nullptr;
}

AudioEffectHdiParam::~AudioEffectHdiParam()
{
    AUDIO_DEBUG_LOG("destructor!");
}

void AudioEffectHdiParam::CreateHdiControl()
{
    // todo read from vendor/...
    libName = strdup("libspatialization_processing_dsp");
    effectId = strdup("aaaabbbb-8888-9999-6666-aabbccdd9966gg");
    EffectInfo info = {
        .libName = &libName[0],
        .effectId = &effectId[0],
        .ioDirection = 1,
    };
    ControllerId controllerId;
    IEffectControl *hdiControl = nullptr;
    int32_t ret = hdiModel_->CreateEffectController(hdiModel_, &info, &hdiControl, &controllerId);
    if ((ret != 0) || (hdiControl == nullptr)) {
        AUDIO_WARNING_LOG("hdi init failed");
    } else {
        libHdiControls_.emplace_back(hdiControl);
    }
    
    return;
}

void AudioEffectHdiParam::InitHdi()
{
    hdiModel_ = IEffectModelGet(false);
    if (hdiModel_ == nullptr) {
        AUDIO_ERR_LOG("IEffectModelGet failed");
        return;
    }

    CreateHdiControl();
}

int32_t AudioEffectHdiParam::UpdateHdiState(int8_t *effectHdiInput)
{
    int32_t ret;
    for (IEffectControl *hdiControl : libHdiControls_) {
        if (hdiControl == nullptr) {
            AUDIO_WARNING_LOG("hdiControl is nullptr.");
            continue;
        }
        ret = memcpy_s(static_cast<void *>(input), sizeof(input), static_cast<void *>(effectHdiInput), sizeof(input));
        if (ret != 0) {
            AUDIO_WARNING_LOG("hdi memcpy failed");
            continue;
        }
        uint32_t replyLen = GET_HDI_BUFFER_LEN;
        ret = hdiControl->SendCommand(hdiControl, HDI_SET_PATAM, input, SEND_HDI_COMMAND_LEN,
            output, &replyLen);
        if (ret != 0) {
            AUDIO_WARNING_LOG("hdi send command failed");
            continue;
        }
    }
    return ret;
}
}  // namespace AudioStandard
}  // namespace OHOS