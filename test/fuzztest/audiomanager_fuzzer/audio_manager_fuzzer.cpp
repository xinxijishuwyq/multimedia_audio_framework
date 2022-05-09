/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <iostream>

#include "audio_errors.h"
#include "audio_info.h"
#include "audio_system_manager.h"
#include "audio_manager_fuzzer.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
void AudioManagerSetVolumeFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size <= 0)) {
        std::cout << "Invalid data" << std::endl;
        return;
    }

    AudioSystemManager::AudioVolumeType type = *reinterpret_cast<const AudioSystemManager::AudioVolumeType *>(data);
    int32_t volume = *reinterpret_cast<const int32_t *>(data);
    AudioSystemManager::GetInstance()->SetVolume(type, volume);
}

void AudioManagerSetDeviceActiveFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size <= 0)) {
        std::cout << "Invalid data" << std::endl;
        return;
    }

    AudioSystemManager::GetInstance()->SetDeviceActive(*reinterpret_cast<const ActiveDeviceType *>(data), true);
}

void AudioManagerSetRingerModeFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size <= 0)) {
        std::cout << "Invalid data" << std::endl;
        return;
    }

    AudioSystemManager::GetInstance()->SetRingerMode(*reinterpret_cast<const AudioRingerMode *>(data));
}
} // namespace AudioStandard
} // namesapce OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioStandard::AudioManagerSetVolumeFuzzTest(data, size);
    OHOS::AudioStandard::AudioManagerSetDeviceActiveFuzzTest(data, size);
    OHOS::AudioStandard::AudioManagerSetRingerModeFuzzTest(data, size);
    return 0;
}
