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
} BufferAttr;

int32_t EffectChainManagerCreate(char *sceneType, BufferAttr *bufferAttr);
int32_t EffectChainManagerProcess(char *sceneType, BufferAttr *bufferAttr);
int32_t EffectChainManagerGetFrameLen(void);
bool EffectChainManagerExist(const char *sceneType, const char *effectMode);

#ifdef __cplusplus
}
#endif
#endif // AUDIO_EFFECT_CHAIN_ADAPTER_H
