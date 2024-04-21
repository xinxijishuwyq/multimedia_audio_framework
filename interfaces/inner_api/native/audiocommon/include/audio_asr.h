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
#ifndef AUDIO_ASR_H
#define AUDIO_ASR_H
/**
 * ASR noise suppression mode.
 * @enum { number }
 * @syscap SystemCapability.Multimedia.Audio.Capturer
 * @systemapi
 * @since 12
 */
enum class AsrNoiseSuppressionMode {
    /**
     * Bypass noise suppression.
     * @syscap SystemCapability.Multimedia.Audio.Capturer
     * @systemapi
     * @since 12
     */
    BYPASS = 0,
    /**
     * Standard noise suppression.
     * @syscap SystemCapability.Multimedia.Audio.Capturer
     * @systemapi
     * @since 12
     */
    STANDARD = 1,
    /**
     * Near field noise suppression.
     * @syscap SystemCapability.Multimedia.Audio.Capturer
     * @systemapi
     * @since 12
     */
    NEAR_FIELD = 2,
    /**
     * Far field noise suppression.
     * @syscap SystemCapability.Multimedia.Audio.Capturer
     * @systemapi
     * @since 12
     */
    FAR_FIELD = 3,
};

/**
 * ASR AEC mode.
 * @enum { number }
 * @syscap SystemCapability.Multimedia.Audio.Capturer
 * @systemapi
 * @since 12
 */
enum class AsrAecMode {
    /**
     * Bypass AEC.
     * @syscap SystemCapability.Multimedia.Audio.Capturer
     * @systemapi
     * @since 12
     */
    BYPASS = 0,
    /**
     * Using standard AEC.
     * @syscap SystemCapability.Multimedia.Audio.Capturer
     * @systemapi
     * @since 12
     */
    STANDARD = 1,
};
#endif