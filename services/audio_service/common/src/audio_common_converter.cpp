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
#include <cmath>
namespace OHOS {
namespace AudioStandard {
constexpr float AUDIO_SAMPLE_32BIT_VALUE = 2147483647.f;
constexpr int32_t BYTES_ALIGNMENT_SIZE = 8;

void AudioCommonConverter::ConvertBufferTo32Bit(const uint8_t *buffer, int32_t format, int32_t *dst, size_t count,
                                                float volume)
{
    switch (format) {
    case 0:
        dst += count;
        buffer += count;
        for (; count > 0; --count) {
            *--dst = ((int32_t)(*--buffer) - 0x80) << 24;
        }
        break;
    case 1: {
        const int16_t *src = (const int16_t *)buffer;
        dst += count;
        src += count;
        for (; count > 0; --count) {
            *--dst = (int32_t)((*--src << 16) * volume);
        }
    } break;
    case 2:
        dst += count;
        buffer += count * 3;
        for (; count > 0; --count) {
            buffer -= 3;
            *--dst = ((buffer[0] << 8) | (buffer[1] << 16) | (buffer[2] << 24)) * volume;
        }
        break;
    case 3: {
        const int32_t *src = (const int32_t *)buffer;
        dst += count;
        src += count;
        for (; count > 0; --count) {
            *--dst = (*--src * volume);
        }
        break;
    }
    case 4: {
        const float *src = (const float *)buffer;
        for (; count > 0; --count) {
            *dst++ = *src++ * volume * AUDIO_SAMPLE_32BIT_VALUE;
        }
    } break;
    default:
        break;
    }
}

void AudioCommonConverter::ConvertBufferTo16Bit(const uint8_t *buffer, int32_t format, int16_t *dst, size_t count,
                                                float volume)
{
    switch (format) {
    case 0:
        dst += count;
        buffer += count;
        for (; count > 0; --count) {
            *--dst = ((int16_t)(*--buffer) - 0x80) << 8;
        }
        break;
    case 1: {
        const int16_t *src = (const int16_t *)buffer;
        dst += count;
        src += count;
        for (; count > 0; --count) {
            *--dst = (*--src * volume);
        }
    }
    case 2:
        dst += count;
        buffer += count * 3;
        for (; count > 0; --count) {
            *dst++ = ((buffer[1]) | (buffer[2] << 8)) * volume;
            buffer += 3;
        }
        break;
    case 3: {
        const int32_t *src = (const int32_t *)buffer;
        dst += count;
        src += count;
        for (; count > 0; --count) {
            *--dst = ((*--src >> 16) * volume);
        }
        break;
    }
    case 4: {
        const float *src = (const float *)buffer;
        static const float scale = 1 << 15;
        for (; count > 0; --count) {
            *dst++ = *src++ * scale * volume;
        }
    } break;
    default:
        break;
    }
}

void AudioCommonConverter::ConvertBufferToFloat(const uint8_t *buffer, uint32_t samplePerFrame,
                                                std::vector<float> &floatBuffer, float volume)
{
    uint32_t convertValue = samplePerFrame * BYTES_ALIGNMENT_SIZE - 1;
    for (uint32_t i = 0; i < floatBuffer.size(); i++) {
        int32_t sampleValue = 0;
        if (samplePerFrame == 3) {
            sampleValue = (buffer[i * samplePerFrame + 2] & 0xff) << 24 |
                          (buffer[i * samplePerFrame + 1] & 0xff) << 16 |
                          (buffer[i * samplePerFrame] & 0xff) << BYTES_ALIGNMENT_SIZE;
            floatBuffer[i] = sampleValue * volume * (1.0f / AUDIO_SAMPLE_32BIT_VALUE);
            continue;
        }
        for (uint32_t j = 0; j < samplePerFrame; j++) {
            sampleValue |= (buffer[i * samplePerFrame + j] & 0xff) << (j * BYTES_ALIGNMENT_SIZE);
        }
        floatBuffer[i] = sampleValue * volume * (1.0f / (1U << convertValue));
    }
}

void AudioCommonConverter::ConvertFloatToAudioBuffer(const std::vector<float> &floatBuffer, uint8_t *buffer,
                                                     uint32_t samplePerFrame)
{
    uint32_t convertValue = samplePerFrame * BYTES_ALIGNMENT_SIZE - 1;
    for (uint32_t i = 0; i < floatBuffer.size(); i++) {
        int32_t sampleValue = static_cast<int32_t>(floatBuffer[i] * std::pow(2, convertValue));
        for (uint32_t j = 0; j < samplePerFrame; j++) {
            uint8_t tempValue = (sampleValue >> (BYTES_ALIGNMENT_SIZE * j)) & 0xff;
            buffer[samplePerFrame * i + j] = tempValue;
        }
    }
}
} // namespace AudioStandard
} // namespace OHOS
