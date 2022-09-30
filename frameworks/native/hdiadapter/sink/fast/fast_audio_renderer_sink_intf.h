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

#ifndef FAST_AUDIO_RENDERER_SINK_INTF_H
#define FAST_AUDIO_RENDERER_SINK_INTF_H

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    enum AudioFormat format;
    uint32_t sampleFmt;
    uint32_t sampleRate;
    uint32_t channel;
    float volume;
} AudioSinkAttr;

int32_t FillinFastAudioRenderSinkWapper(const char *deviceNetworkId, void **wapper);
int32_t FastAudioRendererSinkInit(void *wapper, AudioSinkAttr *attr);
void FastAudioRendererSinkDeInit(void *wapper);
int32_t FastAudioRendererSinkStart(void *wapper);
int32_t FastAudioRendererSinkStop(void *wapper);
int32_t FastAudioRendererSinkPause(void *wapper);
int32_t FastAudioRendererSinkResume(void *wapper);
int32_t FastAudioRendererRenderFrame(void *wapper, char *data, uint64_t len, uint64_t *writeLen);
int32_t FastAudioRendererSinkSetVolume(void *wapper, float left, float right);
int32_t FastAudioRendererSinkGetLatency(void *wapper, uint32_t *latency);
int32_t FastAudioRendererSinkGetTransactionId(uint64_t *transactionId);
#ifdef __cplusplus
}
#endif
#endif // FAST_AUDIO_RENDERER_SINK_INTF_H
