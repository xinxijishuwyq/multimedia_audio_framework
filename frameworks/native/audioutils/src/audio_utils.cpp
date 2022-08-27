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
int64_t GetNowTimeMs()
{
    std::chrono::milliseconds nowMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    return nowMs.count();
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
