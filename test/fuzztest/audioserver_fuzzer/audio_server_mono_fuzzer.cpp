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

#include <iostream>
#include <cstddef>
#include <cstdint>

#include "audio_manager_base.h"
#include "audio_server.h"
#include "message_parcel.h"

using namespace std;

namespace OHOS {
    const int32_t SYSTEM_ABILITY_ID = 3001;
    const bool RUN_ON_CREATE = false;
    namespace AudioStandard {
        bool Convert2Bool(const uint8_t *ptr)
        {
            // 根据ptr的大小随机生成bool值
            return (*ptr < 128)? true: false;
        }
        void AudioServerMonoFuzzTest(const uint8_t *rawData, size_t size)
        {
            if (rawData == nullptr) {
                std::cout << "Invalid data" << std::endl;
                return;
            }
            bool monoState = Convert2Bool(rawData);

            MessageParcel data;
            data.WriteBool(monoState);
            MessageParcel reply;
            MessageOption option;
            std::shared_ptr<AudioServer> AudioServerPtr =
                std::make_shared<AudioServer>(SYSTEM_ABILITY_ID, RUN_ON_CREATE);
            AudioServerPtr->OnRemoteRequest(AudioManagerStub::SET_AUDIO_MONO_STATE, data, reply, option);
        }
    } // namespace AudioStandard
} // namesapce OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioStandard::AudioServerMonoFuzzTest(data, size);
    return 0;
}
