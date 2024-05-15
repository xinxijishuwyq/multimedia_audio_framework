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
#include <gtest/gtest.h>
#include "audio_renderer_sink.h"
#include "audio_errors.h"

using namespace testing::ext;
namespace OHOS {
namespace AudioStandard {
const std::string DIRECT_SINK_NAME = "direct";
const std::string VOIP_SINK_NAME = "voip";
const char *SINK_ADAPTER_NAME = "primary";
class AudioDirectSinkUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    AudioRendererSink *sink;
};

void AudioDirectSinkUnitTest::SetUpTestCase(void)
{
    // input testsuit setup step，setup invoked before all testcases
}

void AudioDirectSinkUnitTest::TearDownTestCase(void)
{
    // input testsuit teardown step，teardown invoked after all testcases
}

void AudioDirectSinkUnitTest::SetUp(void)
{
    sink = AudioRendererSink::GetInstance(DIRECT_SINK_NAME);
}

void AudioDirectSinkUnitTest::TearDown(void)
{
    if (sink && sink->isInited()) {
        sink->DeInit();
    }
}

/**
 * @tc.name  : Test Create Direct Sink
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkCreate_001
 * @tc.desc  : Test create direct sink success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkCreate_001, TestSize.Level1)
{
    EXPECT_NE(nullptr, sink);
}

/**
 * @tc.name  : Test Init Direct Sink
 * @tc.type  : FUNC
 * @tc.number: InitAudioSinkCreate_001
 * @tc.desc  : Test init direct sink success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkInit_001, TestSize.Level1)
{
    EXPECT_NE(nullptr, sink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 48000;
    attr.channel = 2; // 2 two channel
    attr.format = HdiAdapterFormat::SAMPLE_S32;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_WIRED_HEADSET;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    float volume = 1.0f;
    ret = sink->SetVolume(volume, volume);
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Direct Sink State
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkState_001
 * @tc.desc  : Test direct sink state
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkState_001, TestSize.Level1)
{
    EXPECT_NE(nullptr, sink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 48000;
    attr.channel = 2; // 2 two channel
    attr.format = HdiAdapterFormat::SAMPLE_S32;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_WIRED_HEADSET;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Stop();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Pause(); // todo
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Stop();
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Create Voip Sink
 * @tc.type  : FUNC
 * @tc.number: VoipAudioSinkCreate_001
 * @tc.desc  : Test create voip sink success
 */
HWTEST_F(AudioDirectSinkUnitTest, VoipAudioSinkCreate_001, TestSize.Level1)
{
    AudioRendererSink *voipSink = AudioRendererSink::GetInstance(VOIP_SINK_NAME);
    EXPECT_NE(nullptr, voipSink);
}
} // namespace AudioStandard
} // namespace OHOS