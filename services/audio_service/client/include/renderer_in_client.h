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
#ifndef RENDERER_IN_SERVER_H
#define RENDERER_IN_SERVER_H

#include "i_audio_stream.h"

namespace OHOS {
namespace AudioStandard {
const std::unordered_map<std::string, StreamUsage> STREAM_USAGE_MAP = {
    {"STREAM_USAGE_UNKNOWN", STREAM_USAGE_UNKNOWN},
    {"STREAM_USAGE_MEDIA", STREAM_USAGE_MEDIA},
    {"STREAM_USAGE_MUSIC", STREAM_USAGE_MUSIC},
    {"STREAM_USAGE_VOICE_COMMUNICATION", STREAM_USAGE_VOICE_COMMUNICATION},
    {"STREAM_USAGE_VOICE_ASSISTANT", STREAM_USAGE_VOICE_ASSISTANT},
    {"STREAM_USAGE_ALARM", STREAM_USAGE_ALARM},
    {"STREAM_USAGE_VOICE_MESSAGE", STREAM_USAGE_VOICE_MESSAGE},
    {"STREAM_USAGE_NOTIFICATION_RINGTONE", STREAM_USAGE_NOTIFICATION_RINGTONE},
    {"STREAM_USAGE_RINGTONE", STREAM_USAGE_RINGTONE},
    {"STREAM_USAGE_NOTIFICATION", STREAM_USAGE_NOTIFICATION},
    {"STREAM_USAGE_ACCESSIBILITY", STREAM_USAGE_ACCESSIBILITY},
    {"STREAM_USAGE_SYSTEM", STREAM_USAGE_SYSTEM},
    {"STREAM_USAGE_MOVIE", STREAM_USAGE_MOVIE},
    {"STREAM_USAGE_GAME", STREAM_USAGE_GAME},
    {"STREAM_USAGE_AUDIOBOOK", STREAM_USAGE_AUDIOBOOK},
    {"STREAM_USAGE_NAVIGATION", STREAM_USAGE_NAVIGATION},
    {"STREAM_USAGE_DTMF", STREAM_USAGE_DTMF},
    {"STREAM_USAGE_ENFORCED_TONE", STREAM_USAGE_ENFORCED_TONE},
    {"STREAM_USAGE_ULTRASONIC", STREAM_USAGE_ULTRASONIC},
    {"STREAM_USAGE_RANGING", STREAM_USAGE_RANGING},
    {"STREAM_USAGE_VOICE_MODEM_COMMUNICATION", STREAM_USAGE_VOICE_MODEM_COMMUNICATION},
};
class RendererInClient : public IAudioStream {
public:
    virtual ~RendererInClient() = default;
    static std::shared_ptr<RendererInClient> GetInstance(AudioStreamType eStreamType, int32_t appUid);
};
} // namespace AudioStandard
} // namespace OHOS
#endif // RENDERER_IN_SERVER_H
