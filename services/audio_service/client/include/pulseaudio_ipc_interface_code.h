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
        IS_MICROPHONE_MUTE = 6,
        SET_AUDIO_SCENE = 7,
        UPDATE_ROUTE_REQ = 8,
        RETRIEVE_COOKIE = 9,
        GET_TRANSACTION_ID = 10,
        SET_PARAMETER_CALLBACK = 11,
        GET_REMOTE_AUDIO_PARAMETER = 12,
        SET_REMOTE_AUDIO_PARAMETER = 13,
        NOTIFY_DEVICE_INFO = 14,
        CHECK_REMOTE_DEVICE_STATE = 15,
        SET_VOICE_VOLUME = 16,
        SET_AUDIO_MONO_STATE = 17,
        SET_AUDIO_BALANCE_VALUE = 18,
        CREATE_AUDIOPROCESS = 19,
        LOAD_AUDIO_EFFECT_LIBRARIES = 20,
        REQUEST_THREAD_PRIORITY = 21,
        CREATE_AUDIO_EFFECT_CHAIN_MANAGER = 22,
        SET_OUTPUT_DEVICE_SINK = 23,
    };
} // namespace AudioStandard
} // namespace OHOS

#endif // I_AUDIO_SERVER_INTERFACE_H
