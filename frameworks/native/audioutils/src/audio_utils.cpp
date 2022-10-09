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

#include "audio_utils.h"

#include <chrono>
#include <sstream>

#include "parameter.h"
#include "audio_log.h"
namespace OHOS {
namespace AudioStandard {
namespace {
const uint32_t STEREO_CHANNEL_COUNT = 2;
} // namespace

int64_t GetNowTimeMs()
{
    std::chrono::milliseconds nowMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    return nowMs.count();
}

void AdjustStereoToMonoForPCM8Bit(int8_t *data, uint64_t len)
{
    for (unsigned i = len >> 1; i > 0; i--) {
        data[0] = data[0] / STEREO_CHANNEL_COUNT + data[1] / STEREO_CHANNEL_COUNT;
        data[1] = data[0];
        data += STEREO_CHANNEL_COUNT;
    }
}

void AdjustStereoToMonoForPCM16Bit(int16_t *data, uint64_t len)
{
    for (unsigned i = len >> 1; i > 0; i--) {
        data[0] = data[0] / STEREO_CHANNEL_COUNT + data[1] / STEREO_CHANNEL_COUNT;
        data[1] = data[0];
        data += STEREO_CHANNEL_COUNT;
    }
}

void AdjustStereoToMonoForPCM24Bit(int8_t *data, uint64_t len)
{
    // int8_t is used for reading data of PCM24BIT here.
    // 24 / 8 = 3, so we need repeat the calculation three times.
    for (unsigned i = len >> 1; i > 0; i--) {
        data[0] = data[0] / STEREO_CHANNEL_COUNT + data[3] / STEREO_CHANNEL_COUNT;
        data[3] = data[0];
        data[1] = data[1] / STEREO_CHANNEL_COUNT + data[4] / STEREO_CHANNEL_COUNT;
        data[4] = data[1];
        data[2] = data[2] / STEREO_CHANNEL_COUNT + data[5] / STEREO_CHANNEL_COUNT;
        data[5] = data[2];
        data += (STEREO_CHANNEL_COUNT * 3);
    }
}

void AdjustStereoToMonoForPCM32Bit(int32_t *data, uint64_t len)
{
    for (unsigned i = len >> 1; i > 0; i--) {
        data[0] = data[0] / STEREO_CHANNEL_COUNT + data[1] / STEREO_CHANNEL_COUNT;
        data[1] = data[0];
        data += STEREO_CHANNEL_COUNT;
    }
}

void AdjustAudioBalanceForPCM8Bit(int8_t *data, uint64_t len, float left, float right)
{
    for (unsigned i = len >> 1; i > 0; i--) {
        data[0] *= left;
        data[1] *= right;
        data += STEREO_CHANNEL_COUNT;
    }
}

void AdjustAudioBalanceForPCM16Bit(int16_t *data, uint64_t len, float left, float right)
{
    for (unsigned i = len >> 1; i > 0; i--) {
        data[0] *= left;
        data[1] *= right;
        data += STEREO_CHANNEL_COUNT;
    }
}

void AdjustAudioBalanceForPCM24Bit(int8_t *data, uint64_t len, float left, float right)
{
    for (unsigned i = len >> 1; i > 0; i--) {
        // int8_t is used for reading data of PCM24BIT here.
        // 24 / 8 = 3, so we need repeat the calculation three times.
        data[0] *= left;
        data[1] *= left;
        data[2] *= left;
        data[3] *= right;
        data[4] *= right;
        data[5] *= right;
        data += (STEREO_CHANNEL_COUNT * 3);
    }
}

void AdjustAudioBalanceForPCM32Bit(int32_t *data, uint64_t len, float left, float right)
{
    for (unsigned i = len >> 1; i > 0; i--) {
        data[0] *= left;
        data[1] *= right;
        data += STEREO_CHANNEL_COUNT;
    }
}

template <typename T>
bool GetSysPara(const char *key, T &value)
{
    if (key == nullptr) {
        AUDIO_ERR_LOG("GetSysPara: key is nullptr");
        return false;
    }
    char paraValue[20] = {0}; // 20 for system parameter
    auto res = GetParameter(key, "-1", paraValue, sizeof(paraValue));
    if (res <= 0) {
        AUDIO_WARNING_LOG("GetSysPara fail, key:%{public}s res:%{public}d", key, res);
        return false;
    }
    AUDIO_INFO_LOG("GetSysPara: key:%{public}s value:%{public}s", key, paraValue);
    std::stringstream valueStr;
    valueStr << paraValue;
    valueStr >> value;
    return true;
}

template bool GetSysPara(const char *key, int32_t &value);
template bool GetSysPara(const char *key, uint32_t &value);
template bool GetSysPara(const char *key, int64_t &value);
template bool GetSysPara(const char *key, std::string &value);
} // namespace AudioStandard
} // namespace OHOS
