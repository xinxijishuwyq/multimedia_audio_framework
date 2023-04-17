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
#include <cassert>
#include <cstdint>
#include <stddef.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "audio_info.h"

#ifndef ST_AUDIO_EFFECT_SERVER_H
#define ST_AUDIO_EFFECT_SERVER_H

#define AUDIO_EFFECT_LIBRARY_INFO_SYM AELI
#define AUDIO_EFFECT_LIBRARY_INFO_SYM_AS_STR "AELI"
#define EFFECT_STRING_LEN_MAX 64

namespace OHOS {
namespace AudioStandard {

typedef struct EffectInterfaceS **EffectHandleT;

typedef struct AudioBufferS {
    size_t frameCount;        // number of frames in buffer
    union {
        void *raw;        // raw pointer to start of buffer
        float *f32;        // pointer to float 32 bit data at start of buffer
        int32_t *s32;        // pointer to signed 32 bit data at start of buffer
        int16_t *s16;        // pointer to signed 16 bit data at start of buffer
        uint8_t *u8;         // pointer to unsigned 8 bit data at start of buffer
    };
} AudioBufferT;

// for initial version
typedef struct EffectDescriptorS {
    std::string type;
    std::string id;
    uint32_t apiVersion;
    uint32_t flags;
    uint16_t cpuLoad;
    uint16_t memoryUsage;
    char name[EFFECT_STRING_LEN_MAX];
    char implementor[EFFECT_STRING_LEN_MAX];
} EffectDescriptorT;

struct EffectInterfaceS {
    int32_t (*Process)(EffectHandleT self, AudioBufferT *inBuffer, AudioBufferT *outBuffer);

    int32_t (*Command)(EffectHandleT self, uint32_t cmdCode, uint32_t cmdSize, void *pCmdData, uint32_t *replySize,
        void *pReplyData);

    int32_t (*GetDescriptor)(EffectHandleT self, EffectDescriptorT *pDescriptor);

    int32_t (*ProcessReverse)(EffectHandleT self, AudioBufferT *inBuffer, AudioBufferT *outBuffer);
};

// for initial version
typedef struct AudioEffectLibraryS {
    uint32_t tag;
    uint32_t version;
    const char *name;
    const char *implementor;

    int32_t (*CreateEffect)(const std::string *id, int32_t sessionId, int32_t ioId, EffectHandleT *pHandle);

    int32_t (*ReleaseEffect)(EffectHandleT handle);

    int32_t (*GetDescriptor)(const std::string *id, EffectDescriptorT *pDescriptor);
} AudioEffectLibraryT;

typedef struct ListNodeS {
    void *object;
    struct ListNodeS *next;
} ListNodeT;

typedef struct LibEntryS {
    AudioEffectLibraryT *desc;
    std::string name;
    std::string path;
    void *handle;
    std::vector<std::unique_ptr<EffectDescriptorT>> effects;
} LibEntryT;

class AudioEffectServer {
public:
    explicit AudioEffectServer();
    ~AudioEffectServer();

    bool LoadAudioEffects(const std::vector<Library> libraries, const std::vector<Effect> effects,
                          std::vector<Effect>& successEffectList);

    std::vector<std::unique_ptr<LibEntryT>>& GetAvailableEffects();
                                
private:
    std::vector<std::unique_ptr<LibEntryT>> effectLibraryList;
    std::vector<std::unique_ptr<LibEntryT>> effectLibraryFailedList;
    std::vector<std::unique_ptr<EffectDescriptorT>> effectSkippedEffects;
};

} // namespce AudioStandard
} // namespace OHOS
#endif