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

#ifndef AUDIO_EFFECT_CHAIN_ADAPTER_H
#define AUDIO_EFFECT_CHAIN_ADAPTER_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BufferAttr {
    float *bufIn;
    float *bufOut;
    int samplingRate;
    int numChanIn;
    int numChanOut;
    int frameLen;
    float *tempBufIn;
    float *tempBufOut;
} BufferAttr;

int32_t EffectChainManagerProcess(char *sceneType, BufferAttr *bufferAttr);
int32_t EffectChainManagerGetFrameLen(void);
bool EffectChainManagerExist(const char *sceneType, const char *effectMode, const char *spatializationEnabled);
int32_t EffectChainManagerCreateCb(const char *sceneType, const char *sessionID);
int32_t EffectChainManagerReleaseCb(const char *sceneType, const char *sessionID);
int32_t EffectChainManagerMultichannelUpdate(const char *sceneType, const uint32_t channels,
    const char *channelLayout);
bool IsChannelLayoutHVSSupported(const uint64_t channelLayout);
bool NeedPARemap(const char *sinkSceneType, const char *sinkSceneMode, uint8_t sinkChannels,
    const char *sinkChannelLayout, const char *sinkSpatializationEnabled);
int32_t EffectChainManagerInitCb(const char *sceneType);
int32_t EffectChainManagerSetHdiParam(const char *sceneType, const char *effectMode, bool enabled);
bool EffectChainManagerCheckBluetooth();

#ifdef __cplusplus
}
#endif
#endif // AUDIO_EFFECT_CHAIN_ADAPTER_H
