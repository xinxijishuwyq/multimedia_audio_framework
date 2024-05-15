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
#include "none_mix_engine.h"
#include "pro_renderer_stream_impl.h"
#include "audio_errors.h"

using namespace testing::ext;
namespace OHOS {
namespace AudioStandard {
class NoneMixEngineUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    AudioProcessConfig InitProcessConfig();

protected:
    std::unique_ptr<AudioPlaybackEngine> playbackEngine_;
};

void NoneMixEngineUnitTest::SetUpTestCase(void)
{
    // input testsuit setup step，setup invoked before all testcases
}

void NoneMixEngineUnitTest::TearDownTestCase(void)
{
    // input testsuit teardown step，teardown invoked after all testcases
}

void NoneMixEngineUnitTest::SetUp(void)
{
    DeviceInfo deviceInfo;
    deviceInfo.deviceType = DEVICE_TYPE_USB_HEADSET;
    playbackEngine_ = std::make_unique<NoneMixEngine>(deviceInfo, false);
}

void NoneMixEngineUnitTest::TearDown(void)
{
    if (playbackEngine_) {
        playbackEngine_->Stop();
        playbackEngine_ = nullptr;
    }
}

AudioProcessConfig NoneMixEngineUnitTest::InitProcessConfig()
{
    AudioProcessConfig config;
    config.appInfo.appUid = 10;
    config.appInfo.appPid = 10;
    config.streamInfo.format = SAMPLE_S32LE;
    config.streamInfo.samplingRate = SAMPLE_RATE_48000;
    config.streamInfo.channels = STEREO;
    config.streamInfo.channelLayout = AudioChannelLayout::CH_LAYOUT_STEREO;
    config.audioMode = AudioMode::AUDIO_MODE_PLAYBACK;
    config.streamType = AudioStreamType::STREAM_MUSIC;
    config.deviceType = DEVICE_TYPE_USB_HEADSET;
    return config;
}

/**
 * @tc.name  : Test Create Direct Sink
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkCreate_001
 * @tc.desc  : Test create direct sink success
 */
HWTEST_F(NoneMixEngineUnitTest, DirectAudioPlayBackEngine_001, TestSize.Level1)
{
    AudioProcessConfig config = InitProcessConfig();
    std::shared_ptr<ProRendererStreamImpl> rendererStream = std::make_shared<ProRendererStreamImpl>(config, true);
    int32_t ret = rendererStream->InitParams();
    EXPECT_EQ(SUCCESS, ret);
    rendererStream->SetStreamIndex(10);
    ret = playbackEngine_->AddRendererStream(rendererStream);
    EXPECT_EQ(SUCCESS, ret);
    ret = playbackEngine_->Start();
    EXPECT_EQ(SUCCESS, ret);
    playbackEngine_->RemoveRenderer(rendererStream);
    ret = playbackEngine_->Stop();
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Create Direct Sink
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkCreate_001
 * @tc.desc  : Test create direct sink success
 */
HWTEST_F(NoneMixEngineUnitTest, DirectAudioPlayBackEngine_001, TestSize.Level1)
{
    AudioProcessConfig config = InitProcessConfig();
    std::shared_ptr<ProRendererStreamImpl> rendererStream = std::make_shared<ProRendererStreamImpl>(config, true);
    int32_t ret = rendererStream->InitParams();
    EXPECT_EQ(SUCCESS, ret);
    rendererStream->SetStreamIndex(10);
    ret = rendererStream->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = rendererStream->Pause();
    EXPECT_EQ(SUCCESS, ret);
    ret = rendererStream->Stop();
    EXPECT_EQ(SUCCESS, ret);
    ret = rendererStream->Release();
    EXPECT_EQ(SUCCESS, ret);
}
} // namespace AudioStandard
} // namespace OHOS