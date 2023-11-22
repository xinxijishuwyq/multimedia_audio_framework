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
#ifndef AUDIO_CONVERTER_PARSER_H
#define AUDIO_CONVERTER_PARSER_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <cstdio>
#include "audio_log.h"
#include "audio_effect.h"

namespace OHOS {
namespace AudioStandard {

struct ConverterConfig {
    float version;
    Library library;
    AudioChannelLayout outChannelLayout;
};

class AudioConverterParser {
public:
    explicit AudioConverterParser();
    ~AudioConverterParser();
    int32_t LoadConfig(ConverterConfig &result);
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_CONVERTER_PARSER_H