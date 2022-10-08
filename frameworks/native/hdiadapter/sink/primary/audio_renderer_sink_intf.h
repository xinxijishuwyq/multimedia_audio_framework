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

#ifndef AUDIO_RENDERER_SINK_INTF_H
#define AUDIO_RENDERER_SINK_INTF_H

#include <audio_types.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    enum AudioFormat format;
    uint32_t sampleFmt;
    uint32_t sampleRate;
    uint32_t channel;
    float volume;
    const char *filePath;
} AudioSinkAttr;

int32_t FillinAudioRenderSinkWapper(const char *deviceNetworkId, void **wapper);
int32_t AudioRendererSinkInit(void *wapper, AudioSinkAttr *attr);
void AudioRendererSinkDeInit(void *wapper);
int32_t AudioRendererSinkStart(void *wapper);
int32_t AudioRendererSinkStop(void *wapper);
int32_t AudioRendererSinkPause(void *wapper);
int32_t AudioRendererSinkResume(void *wapper);
int32_t AudioRendererRenderFrame(void *wapper, char *data, uint64_t len, uint64_t *writeLen);
int32_t AudioRendererSinkSetVolume(void *wapper, float left, float right);
int32_t AudioRendererSinkGetLatency(void *wapper, uint32_t *latency);
int32_t AudioRendererSinkGetTransactionId(uint64_t *transactionId);
#ifdef __cplusplus
}
#endif
#endif // AUDIO_RENDERER_SINK_INTF_H
