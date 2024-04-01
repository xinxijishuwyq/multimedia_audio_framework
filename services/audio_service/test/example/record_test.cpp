/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#include <cstdio>
#include <iostream>

#include "audio_stream.h"
#include "audio_log.h"

using namespace OHOS::AudioStandard;

namespace {
constexpr uint8_t DEFAULT_FORMAT = SAMPLE_S16LE;
constexpr uint8_t DEFAULT_CHANNELS = 2;
constexpr int32_t INIT_CLIENT_ERROR = -1;
} // namespace

class RecordTest : public AudioCapturerCallbacks {
public:
    void OnSourceDeviceUpdatedCb() const
    {
        AUDIO_INFO_LOG("My custom callback OnSourceDeviceUpdatedCb");
    }
    // Need to check required state changes to update applications
    virtual void OnStreamStateChangeCb() const
    {
        AUDIO_INFO_LOG("My custom callback OnStreamStateChangeCb");
    }

    virtual void OnStreamBufferUnderFlowCb() const {}
    virtual void OnStreamBufferOverFlowCb() const {}
    virtual void OnErrorCb(AudioServiceErrorCodes error) const {}
    virtual void OnEventCb(AudioServiceEventTypes error) const {}
};

static bool InitClient(std::unique_ptr<AudioServiceClient> &client, uint32_t samplingRate)
{
    AudioStreamParams audioParams;
    audioParams.format = DEFAULT_FORMAT;
    audioParams.samplingRate = samplingRate;
    audioParams.channels = DEFAULT_CHANNELS;

    AUDIO_INFO_LOG("Initializing of AudioServiceClient");
    if (client->Initialize(AUDIO_SERVICE_CLIENT_RECORD) < 0)
        return false;

    RecordTest customCb;
    client->RegisterAudioCapturerCallbacks(customCb);

    AUDIO_INFO_LOG("Creating Stream");
    if (client->CreateStream(audioParams, STREAM_MUSIC) < 0) {
        client->ReleaseStream();
        return false;
    }

    AUDIO_INFO_LOG("Starting Stream");
    if (client->StartStream() < 0)
        return false;

    return true;
}

int main(int argc, char* argv[])
{
    std::unique_ptr<AudioServiceClient> client = std::make_unique<AudioStream>(STREAM_MUSIC, AUDIO_MODE_RECORD,
        getuid());
    CHECK_AND_RETURN_RET_LOG(client != nullptr, INIT_CLIENT_ERROR, "AudioStream create error");

    int32_t rateArgIndex = 2;
    if (!InitClient(client, atoi(argv[rateArgIndex]))) {
        AUDIO_ERR_LOG("Initialize client failed");
        return -1;
    }

    size_t bufferLen;
    if (client->GetMinimumBufferSize(bufferLen) < 0) {
        return -1;
    }

    AUDIO_DEBUG_LOG("minimum buffer length: %{public}zu", bufferLen);

    uint8_t* buffer = nullptr;
    buffer = (uint8_t *) malloc(bufferLen);
    if (buffer == nullptr) {
        AUDIO_ERR_LOG("Failed to allocate buffer");
        return -1;
    }

    FILE *pFile = fopen(argv[1], "wb");

    size_t size = 1;
    size_t numBuffersToCapture = 1024;
    uint64_t timestamp;
    StreamBuffer stream;
    stream.buffer = buffer;
    stream.bufferLen = bufferLen;
    int32_t bytesRead = 0;

    while (numBuffersToCapture) {
        bytesRead = client->ReadStream(stream, false);
        if (bytesRead < 0) {
            break;
        }

        if (bytesRead > 0) {
            fwrite(stream.buffer, size, bytesRead, pFile);
            if (client->GetCurrentTimeStamp(timestamp) >= 0)
                AUDIO_DEBUG_LOG("current timestamp: %{public}" PRIu64, timestamp);
            numBuffersToCapture--;
        }
    }

    fflush(pFile);
    client->FlushStream();
    client->StopStream();
    client->ReleaseStream();
    free(buffer);
    fclose(pFile);
    AUDIO_INFO_LOG("Exit from test app");
    return 0;
}
