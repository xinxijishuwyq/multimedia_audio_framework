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

#ifndef AUDIO_HDIADAPTER_INFO_H
#define AUDIO_HDIADAPTER_INFO_H

// should be same with AudioSampleFormat in audio_info.h
enum HdiAdapterFormat {
    SAMPLE_U8 = 0,
    SAMPLE_S16 = 1,
    SAMPLE_S24 = 2,
    SAMPLE_S32 = 3,
    SAMPLE_F32 = 4,
    INVALID_WIDTH = -1
};

#endif