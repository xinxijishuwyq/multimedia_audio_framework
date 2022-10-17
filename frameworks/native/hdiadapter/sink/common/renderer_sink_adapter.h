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

#ifndef RENDERER_SINK_ADAPTER_H
#define RENDERER_SINK_ADAPTER_H

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
    const char *filePath;
    const char *deviceNetworkId;
    int32_t device_type;
} SinkAttr;

struct RendererSinkAdapter {
    int32_t deviceClass;
    void* wapper;
    int32_t (*RendererSinkInit)(void *wapper, const SinkAttr *attr);
    void (*RendererSinkDeInit)(void *wapper);
    int32_t (*RendererSinkStart)(void *wapper);
    int32_t (*RendererSinkPause)(void *wapper);
    int32_t (*RendererSinkResume)(void *wapper);
    int32_t (*RendererSinkStop)(void *wapper);
    int32_t (*RendererRenderFrame)(void *wapper, char *data, uint64_t len, uint64_t *writeLen);
    int32_t (*RendererSinkSetVolume)(void *wapper, float left, float right);
    int32_t (*RendererSinkGetLatency)(void *wapper, uint32_t *latency);
    int32_t (*RendererSinkGetTransactionId)(uint64_t *transactionId);
};

int32_t LoadSinkAdapter(const char *device, const char *deviceNetworkId, struct RendererSinkAdapter **sinkAdapter);
int32_t UnLoadSinkAdapter(struct RendererSinkAdapter *sinkAdapter);
const char *GetDeviceClass(int32_t deviceClass);
#ifdef __cplusplus
}
#endif
#endif // RENDERER_SINK_ADAPTER_H
