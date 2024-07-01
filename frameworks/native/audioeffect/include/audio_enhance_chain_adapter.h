/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef AUDIO_ENHANCE_CHAIN_ADAPTER_H
#define AUDIO_ENHANCE_CHAIN_ADAPTER_H

#include <stdio.h>
#include <stdint.h>
#include <pulse/pulseaudio.h>
#include <pulse/sample.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t EnhanceChainManagerCreateCb(const char *sceneType, const char *enhanceMode, const char *upDevice,
    const char *downDevice);
int32_t EnhanceChainManagerReleaseCb(const char *sceneType, const char *upDevice, const char *downDevice);
bool EnhanceChainManagerExist(const char *sceneKey);
pa_sample_spec EnhanceChainManagerGetAlgoConfig(const char *sceneType, const char *upDevice, const char *downDevice);
int32_t EnhanceChainManagerInitEnhanceBuffer();

#ifdef __cplusplus
}
#endif
#endif // AUDIO_ENHANCE_CHAIN_ADAPTER_H