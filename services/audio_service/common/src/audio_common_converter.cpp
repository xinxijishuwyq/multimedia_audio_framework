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
#include "audio_common_converter.h"
namespace OHOS {
namespace AudioStandard {

void AudioCommonConverter::ConvertBufferToFloat(const uint8_t *buffer, uint32_t samplePerFrame,
                                                std::vector<float> &floatBuffer)
{
    uint32_t convertValue = samplePerFrame * 8 - 1;
    for (uint32_t i = 0; i < floatBuffer.size(); i++) {
        int32_t sampleValue = 0;
        for (uint32_t j = 0; j < samplePerFrame; j++) {
            sampleValue |= (buffer[i * samplePerFrame + j] & 0xff) << (j * 8);
        }
        floatBuffer[i] = sampleValue * (1.0f / (1U << convertValue));
    }
}

void AudioCommonConverter::ConvertFloatToAudioBuffe(const std::vector<float> &floatBuffer, uint8_t *buffer,
                                                    uint32_t samplePerFrame)
{
    uint32_t convertValue = samplePerFrame * 8 - 1;
    for (uint32_t i = 0; i < floatBuffer.size(); i++) {
        int32_t sampleValue = static_cast<int32_t>(floatBuffer[i] * std::pow(2, convertValue));
        for (uint32_t j = 0; j < samplePerFrame; j++) {
            uint8_t tempValue = (sampleValue >> (8 * j)) & 0xff;
            buffer[samplePerFrame * i + j] = tempValue;
        }
    }
}
} // namespace AudioStandard
} // namespace OHOS
