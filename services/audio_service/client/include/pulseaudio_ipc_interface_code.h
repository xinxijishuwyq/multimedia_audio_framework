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

#ifndef I_AUDIO_SERVER_INTERFACE_H
#define I_AUDIO_SERVER_INTERFACE_H

#include "audio_info.h"

/* SAID: 3001 */
namespace OHOS {
namespace AudioStandard {
    enum class AudioServerInterfaceCode {
        GET_MAX_VOLUME = 0,
        GET_MIN_VOLUME = 1,
        GET_DEVICES = 2,
        GET_AUDIO_PARAMETER = 3,
        SET_AUDIO_PARAMETER = 4,
        SET_MICROPHONE_MUTE = 5,
        SET_AUDIO_SCENE = 6,
        UPDATE_ROUTE_REQ = 7,
        RETRIEVE_COOKIE = 8,
        GET_TRANSACTION_ID = 9,
        SET_PARAMETER_CALLBACK = 10,
        GET_REMOTE_AUDIO_PARAMETER = 11,
        SET_REMOTE_AUDIO_PARAMETER = 12,
        NOTIFY_DEVICE_INFO = 13,
        CHECK_REMOTE_DEVICE_STATE = 14,
        SET_VOICE_VOLUME = 15,
        SET_AUDIO_MONO_STATE = 16,
        SET_AUDIO_BALANCE_VALUE = 17,
        CREATE_AUDIOPROCESS = 18,
        LOAD_AUDIO_EFFECT_LIBRARIES = 19,
        REQUEST_THREAD_PRIORITY = 20,
        CREATE_AUDIO_EFFECT_CHAIN_MANAGER = 21,
        SET_OUTPUT_DEVICE_SINK = 22,
        CREATE_PLAYBACK_CAPTURER_MANAGER = 23,
        SET_SUPPORT_STREAM_USAGE = 24,
        REGISET_POLICY_PROVIDER = 25,
        SET_WAKEUP_CLOSE_CALLBACK = 26,
        SET_CAPTURE_SILENT_STATE = 27,
        UPDATE_SPATIALIZATION_STATE = 28,
        AUDIO_SERVER_CODE_MAX = UPDATE_SPATIALIZATION_STATE,
    };
} // namespace AudioStandard
} // namespace OHOS

#endif // I_AUDIO_SERVER_INTERFACE_H
