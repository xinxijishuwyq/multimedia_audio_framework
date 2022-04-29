/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef CAPTURER_SOURCE_ADAPTER_H
#define CAPTURER_SOURCE_ADAPTER_H

#include <stdio.h>

#include "audio_capturer_source_intf.h"
#include "audio_capturer_file_source_intf.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    const char *adapterName;
    uint32_t open_mic_speaker;
    enum AudioFormat format;
    uint32_t sampleFmt;
    uint32_t sampleRate;
    uint32_t channel;
    float volume;
    uint32_t bufferSize;
    bool isBigEndian;
    const char *filePath;
} SourceAttr;

struct CapturerSourceAdapter {
    int32_t (*CapturerSourceInit)(const SourceAttr *attr);
    void (*CapturerSourceDeInit)(void);
    int32_t (*CapturerSourceStart)(void);
    int32_t (*CapturerSourceSetMute)(bool isMute);
    bool (*CapturerSourceIsMuteRequired)(void);
    int32_t (*CapturerSourceStop)(void);
    int32_t (*CapturerSourceFrame)(char *frame, uint64_t requestBytes, uint64_t *replyBytes);
    int32_t (*CapturerSourceSetVolume)(float left, float right);
    int32_t (*CapturerSourceGetVolume)(float *left, float *right);
};

int32_t LoadSourceAdapter(const char *device, struct CapturerSourceAdapter **sourceAdapter);
int32_t UnLoadSourceAdapter(struct CapturerSourceAdapter *sourceAdapter);
const char *GetDeviceClass(void);
#ifdef __cplusplus
}
#endif
#endif // CAPTURER_SOURCE_ADAPTER_H
