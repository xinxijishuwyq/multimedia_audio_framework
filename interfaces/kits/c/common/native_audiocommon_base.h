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

/**
 * @addtogroup OHAudio
 * @{
 *
 * @brief Provide the definition of the C interface for the audio module.
 *
 * @syscap SystemCapability.Multimedia.Audio.Core
 *
 * @since 12
 * @version 1.0
 */

/**
 * @file native_audiocommon_base.h
 *
 * @brief Declare the audio common base data structure.
 *
 * @syscap SystemCapability.Multimedia.Audio.Core
 * @since 12
 * @version 1.0
 */

#ifndef NATIVE_AUDIOCOMMON_BASE_H
#define NATIVE_AUDIOCOMMON_BASE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Define the result of the function execution.
 *
 * @since 12
 */
typedef enum {
    /**
     * The call was successful.
     *
     * @since 12
     */
    AUDIOCOMMON_RESULT_SUCCESS = 0,

    /**
     * This means that the input parameter is invalid.
     *
     * @since 12
     */
    AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM = 6800101,

    /**
     * This means there is no memory left.
     *
     * @since 12
     */
    AUDIOCOMMON_RESULT_ERROR_NO_MEMORY = 6800102,

    /**
     * Execution status exception.
     *
     * @since 12
     */
    AUDIOCOMMON_RESULT_ERROR_ILLEGAL_STATE = 6800103,

    /**
     * This means the operation is unsupported.
     *
     * @since 12
     */
    AUDIOCOMMON_RESULT_UNSUPPORTED = 6800104,

    /**
     * This means the operation is timeout.
     *
     * @since 12
     */
    AUDIOCOMMON_RESULT_TIMEOUT = 6800105,

    /**
     * This means reached stream limit.
     *
     * @since 12
     */
    AUDIOCOMMON_RESULT_STREAM_LIMIT = 6800201,

    /**
     * An system error has occurred.
     *
     * @since 12
     */
    AUDIOCOMMON_RESULT_ERROR_SYSTEM = 6800301,
} OH_AudioCommon_Result;

#ifdef __cplusplus
}

#endif

#endif // NATIVE_AUDIOCOMMON_BASE_H
