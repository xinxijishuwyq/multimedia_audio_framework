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
#ifndef AUDIO_UTILS_H
#define AUDIO_UTILS_H

#include <cstdint>
#include <cinttypes>

namespace OHOS {
namespace AudioStandard {
int64_t GetNowTimeMs();
void AdjustStereoToMono(int16_t *data, uint64_t len);
void AdjustAudioBalance(int16_t *data, uint64_t len, float left, float right);

template <typename T>
bool GetSysPara(const char *key, T &value);
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_UTILS_H
