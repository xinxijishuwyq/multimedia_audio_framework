/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "audio_interrupt_test.h"
#include "media_log.h"
#include "pcm2wav.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
namespace AudioTestConstants {
    constexpr int32_t SUCCESS = 0;
    constexpr float TRACK_VOLUME = 0.2f;
}

void AudioInterruptCallbackTest::OnInterrupt(const InterruptAction &interruptAction)
{
    MEDIA_DEBUG_LOG("AudioInterruptCallbackTest:  OnInterrupt");
    MEDIA_DEBUG_LOG("AudioInterruptCallbackTest: actionType: %{public}d", interruptAction.actionType);
    MEDIA_DEBUG_LOG("AudioInterruptCallbackTest: interruptType: %{public}d", interruptAction.interruptType);
    MEDIA_DEBUG_LOG("AudioInterruptCallbackTest: interruptHint: %{public}d", interruptAction.interruptHint);

    switch (interruptAction.actionType) {
        case TYPE_ACTIVATED:
            MEDIA_DEBUG_LOG("AudioInterruptCallbackTest: StartRender");
            AudioInterruptTest::GetInstance().renderThread_ = std::thread(AudioInterruptTest::StartRender);
            break;
        case TYPE_DEACTIVATED:
            MEDIA_DEBUG_LOG("AudioInterruptCallbackTest: StopRender");
            if (AudioInterruptTest::GetInstance().audioRenderer_->Stop()) {
                MEDIA_ERR_LOG("AudioInterruptCallbackTest: Stop Success");
            } else {
                MEDIA_DEBUG_LOG("AudioInterruptCallbackTest: Stop Failed");
            }
            break;
        case TYPE_INTERRUPTED:
            if (interruptAction.interruptHint == INTERRUPT_HINT_PAUSE) {
                MEDIA_DEBUG_LOG("AudioInterruptCallbackTest: PauseRender");
                AudioInterruptTest::GetInstance().isRenderPaused_ = true;
                if (AudioInterruptTest::GetInstance().audioRenderer_->Pause()) {
                    MEDIA_DEBUG_LOG("AudioInterruptCallbackTest: Pause Success");
                } else {
                    MEDIA_ERR_LOG("AudioInterruptCallbackTest: Pause Failed");
                }
            } else if (interruptAction.interruptHint == INTERRUPT_HINT_RESUME) {
                MEDIA_DEBUG_LOG("AudioInterruptCallbackTest: ResumeRender");
                AudioInterruptTest::GetInstance().audioRenderer_->SetVolume(1.0);
                if (AudioInterruptTest::GetInstance().audioRenderer_->Start()) {
                    MEDIA_DEBUG_LOG("AudioInterruptCallbackTest: Resume Success");
                    AudioInterruptTest::GetInstance().isRenderPaused_ = false;
                } else {
                    MEDIA_DEBUG_LOG("AudioInterruptCallbackTest: Resume Failed");
                }
            }
            break;
        default:
            MEDIA_ERR_LOG("AudioInterruptCallbackTest: Stop failed");
    }
}

AudioInterruptTest& AudioInterruptTest::GetInstance()
{
    static AudioInterruptTest interruptTest;
    return interruptTest;
}

bool AudioInterruptTest::GetBufferLen(size_t &bufferLen) const
{
    if (audioRenderer_->GetBufferSize(bufferLen)) {
        return false;
    }
    MEDIA_DEBUG_LOG("minimum buffer length: %{public}zu", bufferLen);

    return true;
}

void AudioInterruptTest::WriteBuffer()
{
    size_t bufferLen = 0;
    if (!GetBufferLen(bufferLen)) {
        isRenderingCompleted_ = true;
        return;
    }

    int32_t n = 2;
    auto buffer = std::make_unique<uint8_t[]>(n * bufferLen);
    if (buffer == nullptr) {
        MEDIA_ERR_LOG("AudioInterruptTest: Failed to allocate buffer");
        isRenderingCompleted_ = true;
        return;
    }

    size_t bytesToWrite = 0;
    size_t bytesWritten = 0;
    size_t minBytes = 4;
    while (true) {
        while (!feof(wavFile_) && !isRenderPaused_) {
            bytesToWrite = fread(buffer.get(), 1, bufferLen, wavFile_);
            bytesWritten = 0;
            MEDIA_INFO_LOG("AudioInterruptTest: Bytes to write: %{public}zu", bytesToWrite);

            while ((bytesWritten < bytesToWrite) && ((bytesToWrite - bytesWritten) > minBytes)) {
                bytesWritten += audioRenderer_->Write(buffer.get() + bytesWritten,
                                                      bytesToWrite - bytesWritten);
                MEDIA_INFO_LOG("AudioInterruptTest: Bytes written: %{public}zu", bytesWritten);
                if (bytesWritten < 0) {
                    break;
                }
            }
        }

        if (feof(wavFile_)) {
            MEDIA_INFO_LOG("AudioInterruptTest: EOF set isRenderingCompleted_ true ");
            isRenderingCompleted_ = true;
            break;
        }
    }
}

void AudioInterruptTest::StartRender()
{
    MEDIA_INFO_LOG("AudioInterruptTest: Starting renderer");
    if (!AudioInterruptTest::GetInstance().audioRenderer_->Start()) {
        MEDIA_ERR_LOG("AudioInterruptTest: Start failed");
        if (!AudioInterruptTest::GetInstance().audioRenderer_->Release()) {
            MEDIA_ERR_LOG("AudioInterruptTest: Release failed");
        }
        AudioInterruptTest::GetInstance().isRenderingCompleted_ = true;
        return;
    }
    MEDIA_INFO_LOG("AudioInterruptTest: Playback started");
    AudioInterruptTest::GetInstance().WriteBuffer();
}

bool AudioInterruptTest::InitRender() const
{
    wav_hdr wavHeader;
    size_t headerSize = sizeof(wav_hdr);
    size_t bytesRead = fread(&wavHeader, 1, headerSize, wavFile_);
    if (bytesRead != headerSize) {
        MEDIA_ERR_LOG("AudioInterruptTest: File header reading error");
        return false;
    }

    AudioRendererParams rendererParams;
    rendererParams.sampleFormat = static_cast<AudioSampleFormat>(wavHeader.bitsPerSample);
    rendererParams.sampleRate = static_cast<AudioSamplingRate>(wavHeader.SamplesPerSec);
    rendererParams.channelCount = static_cast<AudioChannel>(wavHeader.NumOfChan);
    rendererParams.encodingType = static_cast<AudioEncodingType>(ENCODING_PCM);

    if (audioRenderer_->SetParams(rendererParams) !=  AudioTestConstants::SUCCESS) {
        MEDIA_ERR_LOG("AudioInterruptTest: Set audio renderer parameters failed");
        if (!audioRenderer_->Release()) {
            MEDIA_ERR_LOG("AudioInterruptTest: Release failed");
        }
        return false;
    }
    MEDIA_INFO_LOG("AudioInterruptTest: Playback renderer created");

    if (audioRenderer_->SetVolume(AudioTestConstants::TRACK_VOLUME) == AudioTestConstants::SUCCESS) {
        MEDIA_INFO_LOG("AudioInterruptTest: volume set to: %{public}f", audioRenderer_->GetVolume());
    }

    return true;
}

int32_t AudioInterruptTest::TestPlayback(const AudioStreamType &streamType)
{
    MEDIA_INFO_LOG("AudioInterruptTest: TestPlayback start ");

    audioSystemMgr_ = AudioSystemManager::GetInstance();
    std::shared_ptr<AudioManagerCallback> audioManagerCB = std::make_shared<AudioInterruptCallbackTest>();
    int32_t retMngr = audioSystemMgr_->SetAudioManagerCallback(
        static_cast<AudioSystemManager::AudioVolumeType>(streamType), audioManagerCB);
    MEDIA_DEBUG_LOG("SetAudioManagerCallback result : %{public}d", retMngr);
    if (retMngr) {
        MEDIA_ERR_LOG("SetAudioManagerCallback failed : %{public}d", retMngr);
        fclose(wavFile_);
        audioSystemMgr_->UnsetAudioManagerCallback(static_cast<AudioSystemManager::AudioVolumeType>(streamType));
        return -1;
    }

    audioRenderer_ = AudioRenderer::Create(streamType);
    if (!InitRender()) {
        fclose(wavFile_);
        audioSystemMgr_->UnsetAudioManagerCallback(static_cast<AudioSystemManager::AudioVolumeType>(streamType));
        return -1;
    }

    AudioInterrupt audioInterrupt {STREAM_USAGE_MEDIA, CONTENT_TYPE_MUSIC, STREAM_MUSIC, 1000};
    if (streamType == STREAM_VOICE_ASSISTANT) {
        audioInterrupt = {STREAM_USAGE_VOICE_ASSISTANT, CONTENT_TYPE_SPEECH, STREAM_VOICE_ASSISTANT, 1001};
    }

    retMngr = audioSystemMgr_->ActivateAudioInterrupt(audioInterrupt);
    MEDIA_DEBUG_LOG("AudioInterruptTest: ActivateAudioInterrupt returned : %{public}d", retMngr);
    if (retMngr) {
        MEDIA_ERR_LOG("AudioInterruptTest: ActivateAudioInterrupt request rejected : %{public}d", retMngr);
        fclose(wavFile_);
        audioSystemMgr_->UnsetAudioManagerCallback(static_cast<AudioSystemManager::AudioVolumeType>(streamType));
        return -1;
    }

    renderThread_.join();

    retMngr = audioSystemMgr_->DeactivateAudioInterrupt(audioInterrupt);
    MEDIA_DEBUG_LOG("AudioInterruptTest: DeactivateAudioInterrupt returned : %{public}d", retMngr);
    if (retMngr) {
        MEDIA_ERR_LOG("AudioInterruptTest: DeactivateAudioInterrupt request rejected : %{public}d", retMngr);
        fclose(wavFile_);
        audioSystemMgr_->UnsetAudioManagerCallback(static_cast<AudioSystemManager::AudioVolumeType>(streamType));
        return -1;
    }

    if (!audioRenderer_->Release()) {
        MEDIA_ERR_LOG("AudioInterruptTest: Release failed");
    }

    fclose(wavFile_);

    retMngr = audioSystemMgr_->UnsetAudioManagerCallback(
        static_cast<AudioSystemManager::AudioVolumeType>(streamType));
    MEDIA_DEBUG_LOG("SetAudioManagerCallback result : %{public}d", retMngr);

    MEDIA_INFO_LOG("AudioInterruptTest: TestPlayback end");

    return 0;
}
} // AudioStandard
} // OHOS

using namespace OHOS;
using namespace OHOS::AudioStandard;

int main(int argc, char *argv[])
{
    MEDIA_INFO_LOG("AudioInterruptTest: Render test in");
    constexpr int32_t minNumOfArgs = 2;
    constexpr int32_t argIndexTwo = 2;

    if ((argv == nullptr) || (argc < minNumOfArgs)) {
        MEDIA_ERR_LOG("AudioInterruptTest: argv is null");
        return 0;
    }

    MEDIA_INFO_LOG("AudioInterruptTest: argc=%d", argc);
    MEDIA_INFO_LOG("AudioInterruptTest: argv[1]=%{public}s", argv[1]);

    int numBase = 10;
    char *path = argv[1];
    if (access(path, F_OK)) {
        MEDIA_ERR_LOG("AudioInterruptTest: Invalid input filepath");
        return -1;
    }
    MEDIA_INFO_LOG("AudioInterruptTest: path = %{public}s", path);
    AudioInterruptTest::GetInstance().wavFile_ = fopen(path, "rb");
    if (AudioInterruptTest::GetInstance().wavFile_ == nullptr) {
        MEDIA_INFO_LOG("AudioInterruptTest: Unable to open wave file");
        return -1;
    }

    AudioStreamType streamType = AudioStreamType::STREAM_MUSIC;
    if (argc > minNumOfArgs)
        streamType = static_cast<AudioStreamType>(strtol(argv[argIndexTwo], NULL, numBase));

    int32_t ret = AudioInterruptTest::GetInstance().TestPlayback(streamType);

    return ret;
}
