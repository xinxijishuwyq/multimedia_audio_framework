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

#ifndef I_AUDIO_CAPTURER_SINK_INTF_H
#define I_AUDIO_CAPTURER_SINK_INTF_H

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
} IAudioSourceAttr;

int32_t FillinSourceWapper(const char *deviceClass, const char *deviceNetworkId, void **wapper);
int32_t IAudioCapturerSourceInit(void *wapper, IAudioSourceAttr *attr);
void IAudioCapturerSourceDeInit(void *wapper);
int32_t IAudioCapturerSourceStart(void *wapper);
int32_t IAudioCapturerSourceStop(void *wapper);
int32_t IAudioCapturerSourceFrame(void *wapper, char *frame, uint64_t requestBytes, uint64_t *replyBytes);
int32_t IAudioCapturerSourceSetVolume(void *wapper, float left, float right);
bool IAudioCapturerSourceIsMuteRequired(void *wapper);
int32_t IAudioCapturerSourceSetMute(void *wapper, bool isMute);
int32_t IAudioCapturerSourceGetVolume(void *wapper, float *left, float *right);
#ifdef __cplusplus
}
#endif

#endif  // AUDIO_CAPTURER_SINK_INTF_H
