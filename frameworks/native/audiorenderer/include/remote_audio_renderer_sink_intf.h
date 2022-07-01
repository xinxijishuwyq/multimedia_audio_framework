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

#ifndef REMOTE_AUDIO_RENDERER_SINK_INTF_H
#define REMOTE_AUDIO_RENDERER_SINK_INTF_H

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    const char *adapterName;
    uint32_t openMicSpeaker;
    enum AudioFormat format;
    uint32_t sampleFmt;
    uint32_t sampleRate;
    uint32_t channel;
    float volume;
    const char *filePath;
    const char *deviceNetworkId;
    int32_t device_type;
} RemoteAudioSinkAttr;

int32_t FillinRemoteAudioRenderSinkWapper(const char *deviceNetworkId, void **wapper);
int32_t RemoteAudioRendererSinkInit(void *wapper, RemoteAudioSinkAttr *attr);
void RemoteAudioRendererSinkDeInit(void *wapper);
int32_t RemoteAudioRendererSinkStart(void *wapper);
int32_t RemoteAudioRendererSinkStop(void *wapper);
int32_t RemoteAudioRendererSinkPause(void *wapper);
int32_t RemoteAudioRendererSinkResume(void *wapper);
int32_t RemoteAudioRendererRenderFrame(void *wapper, char *data, uint64_t len, uint64_t *writeLen);
int32_t RemoteAudioRendererSinkSetVolume(void *wapper, float left, float right);
int32_t RemoteAudioRendererSinkGetLatency(void *wapper, uint32_t *latency);
int32_t RemoteAudioRendererSinkGetTransactionId(uint64_t *transactionId);
#ifdef __cplusplus
}
#endif
#endif // AUDIO_RENDERER_SINK_INTF_H
