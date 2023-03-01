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

#ifndef ST_AUDIO_STREAM_MANAGER_H
#define ST_AUDIO_STREAM_MANAGER_H

#include <iostream>

#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {
class AudioRendererStateChangeCallback {
public:
    virtual ~AudioRendererStateChangeCallback() = default;
    /**
     * Called when the renderer state changes
     *
     * @param rendererChangeInfo Contains the renderer state information.
     */
    virtual void OnRendererStateChange(
        const std::vector<std::unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos) = 0;
};

class AudioCapturerStateChangeCallback {
public:
    virtual ~AudioCapturerStateChangeCallback() = default;
    /**
     * Called when the capturer state changes
     *
     * @param capturerChangeInfo Contains the renderer state information.
     */
    virtual void OnCapturerStateChange(
        const std::vector<std::unique_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos) = 0;
};

class AudioClientTracker {
public:
    virtual ~AudioClientTracker() = default;
    /**
     * Paused Stream was controlled by system application
     *
     * @param streamSetStateEventInternal Contains the set even information.
     */
    virtual void PausedStreamImpl(const StreamSetStateEventInternal &streamSetStateEventInternal) = 0;
     /**
     * Resumed Stream was controlled by system application
     *
     * @param streamSetStateEventInternal Contains the set even information.
     */
    virtual void ResumeStreamImpl(const StreamSetStateEventInternal &streamSetStateEventInternal) = 0;
    virtual void SetLowPowerVolumeImpl(float volume) = 0;
    virtual void GetLowPowerVolumeImpl(float &volume) = 0;
    virtual void GetSingleStreamVolumeImpl(float &volume) = 0;
};

class AudioStreamManager {
public:
    AudioStreamManager() = default;
    virtual ~AudioStreamManager() = default;

    static AudioStreamManager *GetInstance();
    int32_t RegisterAudioRendererEventListener(const int32_t clientUID,
                                              const std::shared_ptr<AudioRendererStateChangeCallback> &callback);
    int32_t UnregisterAudioRendererEventListener(const int32_t clientUID);
    int32_t RegisterAudioCapturerEventListener(const int32_t clientUID,
                                              const std::shared_ptr<AudioCapturerStateChangeCallback> &callback);
    int32_t UnregisterAudioCapturerEventListener(const int32_t clientUID);
    int32_t GetCurrentRendererChangeInfos(
        std::vector<std::unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos);
    int32_t GetCurrentCapturerChangeInfos(
        std::vector<std::unique_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos);
    bool IsAudioRendererLowLatencySupported(const AudioStreamInfo &audioStreamInfo);
};
} // namespace AudioStandard
} // namespace OHOS
#endif // ST_AUDIO_STREAM_MANAGER_H
