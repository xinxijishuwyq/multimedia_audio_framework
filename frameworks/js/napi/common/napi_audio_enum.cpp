/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "hilog/log.h"
#include "napi_audio_enum.h"

using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace OHOS {
namespace AudioStandard {

napi_ref NapiAudioEnum::sConstructor_ = nullptr;
unique_ptr<AudioParameters> NapiAudioEnum::sAudioParameters_ = nullptr;

NapiAudioEnum::NapiAudioEnum()
    : env_(nullptr) {
}

NapiAudioEnum::~NapiAudioEnum()
{
    audioParameters_ = nullptr;
}
}  // namespace AudioStandard
}  // namespace OHOS
