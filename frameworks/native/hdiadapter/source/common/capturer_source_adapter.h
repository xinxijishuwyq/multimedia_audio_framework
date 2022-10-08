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

#include <stdint.h>
#include "audio_types.h"

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
    const char *deviceNetworkId;
    int32_t device_type;
} SourceAttr;

struct CapturerSourceAdapter {
    int32_t deviceClass;
    void *wapper;
    int32_t (*CapturerSourceInit)(void *wapper, const SourceAttr *attr);
    void (*CapturerSourceDeInit)(void *wapper);
    int32_t (*CapturerSourceStart)(void *wapper);
    int32_t (*CapturerSourceSetMute)(void *wapper, bool isMute);
    bool (*CapturerSourceIsMuteRequired)(void *wapper);
    int32_t (*CapturerSourceStop)(void *wapper);
    int32_t (*CapturerSourceFrame)(void *wapper, char *frame, uint64_t requestBytes, uint64_t *replyBytes);
    int32_t (*CapturerSourceSetVolume)(void *wapper, float left, float right);
    int32_t (*CapturerSourceGetVolume)(void *wapper, float *left, float *right);
};

int32_t LoadSourceAdapter(const char *device, const char *deviceNetworkId,
    struct CapturerSourceAdapter **sourceAdapter);
int32_t UnLoadSourceAdapter(struct CapturerSourceAdapter *sourceAdapter);
const char *GetDeviceClass(void);
#ifdef __cplusplus
}
#endif
#endif // CAPTURER_SOURCE_ADAPTER_H
