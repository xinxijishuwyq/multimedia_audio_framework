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

#ifndef FAST_AUDIO_RENDERER_SINK_H
#define FAST_AUDIO_RENDERER_SINK_H

#include "audio_info.h"
#include "i_audio_renderer_sink.h"

namespace OHOS {
namespace AudioStandard {
class FastAudioRendererSink : public IAudioRendererSink {
public:
    static FastAudioRendererSink *GetInstance(void);

    virtual int32_t GetMmapBufferInfo(int &fd, uint32_t &totalSizeInframe, uint32_t &spanSizeInframe,
        uint32_t &byteSizePerFrame) = 0;
    virtual int32_t GetMmapHandlePosition(uint64_t &frames, int64_t &timeSec, int64_t &timeNanoSec) = 0;

    FastAudioRendererSink() = default;
    ~FastAudioRendererSink() = default;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif // FAST_AUDIO_RENDERER_SINK_H
