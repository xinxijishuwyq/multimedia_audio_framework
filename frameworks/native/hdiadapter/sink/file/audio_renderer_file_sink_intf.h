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

#ifndef AUDIO_RENDERER_FILE_SINK_INTF_H
#define AUDIO_RENDERER_FILE_SINK_INTF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
int32_t FillinAudioRenderFileSinkWapper(const char *deviceNetworkId, void **wapper);
int32_t AudioRendererFileSinkInit(void *wapper, const char *filePath);
void AudioRendererFileSinkDeInit(void *wapper);
int32_t AudioRendererFileSinkStart(void *wapper);
int32_t AudioRendererFileSinkStop(void *wapper);
int32_t AudioRendererFileSinkPause(void *wapper);
int32_t AudioRendererFileSinkResume(void *wapper);
int32_t AudioRendererFileSinkRenderFrame(void *wapper, char *data, uint64_t len, uint64_t *writeLen);
int32_t AudioRendererFileSinkSetVolume(void *wapper, float left, float right);
int32_t AudioRendererFileSinkGetLatency(void *wapper, uint32_t *latency);
int32_t AudioRendererFileSinkGetTransactionId(uint64_t *transactionId);
#ifdef __cplusplus
}
#endif
#endif // AUDIO_RENDERER_FILE_SINK_INTF_H
