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

#include "audio_group_manager_unit_test.h"

#include "audio_errors.h"
#include "audio_info.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {
namespace {
    constexpr int32_t MAX_VOL = 15;
    constexpr int32_t MIN_VOL = 0;
    std::string networkId = "LocalDevice";
}

void AudioGroupManagerUnitTest::SetUpTestCase(void) {}
void AudioGroupManagerUnitTest::TearDownTestCase(void) {}
void AudioGroupManagerUnitTest::SetUp(void) {}
void AudioGroupManagerUnitTest::TearDown(void) {}

/**
* @tc.name  : Test AudioVolume API
* @tc.number: AudioVolume_001
* @tc.desc  : Test AudioVolume manager interface multiple requests
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, AudioVolume_001, TestSize.Level1)
{
    int32_t volume = 0;
    bool mute = true;
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_ALL, volume);
        EXPECT_EQ(SUCCESS, ret);

        ret = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ALL);
        EXPECT_EQ(volume, ret);

        ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_ALL, mute);
        EXPECT_EQ(SUCCESS, ret);

        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_ALL, isMute);
        EXPECT_EQ(true, isMute);
    }
}

/**
* @tc.name  : Test AudioVolume API
* @tc.number: AudioVolume_002
* @tc.desc  : Test AudioVolume manager interface multiple requests
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, AudioVolume_002, TestSize.Level1)
{
    int32_t volume = 2;
    bool mute = true;
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_ALARM, volume);
        EXPECT_EQ(SUCCESS, ret);

        ret = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ALARM);
        EXPECT_EQ(volume, ret);

        ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_ALARM, mute);
        EXPECT_EQ(SUCCESS, ret);

        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_ALARM, isMute);
        EXPECT_EQ(true, isMute);
    }
}

/**
* @tc.name  : Test AudioVolume API
* @tc.number: AudioVolume_003
* @tc.desc  : Test AudioVolume manager interface multiple requests
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, AudioVolume_003, TestSize.Level1)
{
    int32_t volume = 4;
    bool mute = true;
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_ACCESSIBILITY, volume);
        EXPECT_EQ(SUCCESS, ret);

        ret = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ACCESSIBILITY);
        EXPECT_EQ(volume, ret);

        ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_ACCESSIBILITY, mute);
        EXPECT_EQ(SUCCESS, ret);

        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_ACCESSIBILITY, isMute);
        EXPECT_EQ(true, isMute);
    }
}

/**
* @tc.name  : Test AudioVolume API
* @tc.number: AudioVolume_004
* @tc.desc  : Test AudioVolume manager interface multiple requests
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, AudioVolume_004, TestSize.Level1)
{
    int32_t volume = 5;
    bool mute = true;
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_ULTRASONIC, volume);
        EXPECT_EQ(SUCCESS, ret);

        ret = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ULTRASONIC);
        EXPECT_EQ(volume, ret);

        ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_ULTRASONIC, mute);
        EXPECT_EQ(SUCCESS, ret);

        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_ULTRASONIC, isMute);
        EXPECT_EQ(true, isMute);
    }
}

/**
* @tc.name  : Test SetVolume API
* @tc.number: SetVolumeTest_001
* @tc.desc  : Test setting volume of ringtone stream with max volume
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetVolumeTest_001, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_RING, MAX_VOL);
        EXPECT_EQ(SUCCESS, ret);

        int32_t volume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_RING);
        EXPECT_EQ(MAX_VOL, volume);
    }
}

/**
* @tc.name  : Test SetVolume API
* @tc.number: SetVolumeTest_002
* @tc.desc  : Test setting volume of ringtone stream with min volume
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetVolumeTest_002, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_RING, MIN_VOL);
        EXPECT_EQ(SUCCESS, ret);

        int32_t volume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_RING);
        EXPECT_EQ(MIN_VOL, volume);
    }
}

/**
* @tc.name  : Test SetVolume API
* @tc.number: SetVolumeTest_003
* @tc.desc  : Test setting volume of media stream with max volume
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetVolumeTest_003, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_MUSIC, MAX_VOL);
        EXPECT_EQ(SUCCESS, ret);

        int32_t mediaVol = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_MUSIC);
        EXPECT_EQ(MAX_VOL, mediaVol);

        int32_t ringVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_RING);
        EXPECT_EQ(MIN_VOL, ringVolume);
    }
}

/**
* @tc.name  : Test SetVolume API
* @tc.number: SetVolumeTest_004
* @tc.desc  : Test setting volume of alarm stream with error volume
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetVolumeTest_004, TestSize.Level0)
{
    int32_t ErrorVolume = 17;
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        int32_t FirstVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ALARM);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_ALARM, ErrorVolume);
        EXPECT_NE(SUCCESS, ret);

        int32_t SecondVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ALARM);
        EXPECT_EQ(FirstVolume, SecondVolume);
    }
}

/**
* @tc.name  : Test SetVolume API
* @tc.number: SetVolumeTest_005
* @tc.desc  : Test setting volume of accessibility stream with error volume
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetVolumeTest_005, TestSize.Level0)
{
    int32_t ErrorVolume = 18;
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        int32_t FirstVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ACCESSIBILITY);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_ACCESSIBILITY, ErrorVolume);
        EXPECT_NE(SUCCESS, ret);

        int32_t SecondVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ACCESSIBILITY);
        EXPECT_EQ(FirstVolume, SecondVolume);
    }
}

/**
* @tc.name  : Test SetVolume API
* @tc.number: SetVolumeTest_006
* @tc.desc  : Test setting volume of ultrasonic stream with error volume
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetVolumeTest_006, TestSize.Level0)
{
    int32_t ErrorVolume = -5;
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        int32_t FirstVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ULTRASONIC);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_ULTRASONIC, ErrorVolume);
        EXPECT_NE(SUCCESS, ret);

        int32_t SecondVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ULTRASONIC);
        EXPECT_EQ(FirstVolume, SecondVolume);
    }
}

/**
* @tc.name  : Test GetMaxVolume API
* @tc.number: GetMaxVolumeTest_001
* @tc.desc  : Test GetMaxVolume of media stream
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, GetMaxVolumeTest_001, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
    int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        int32_t mediaVol = audioGroupMngr_->GetMaxVolume(AudioVolumeType::STREAM_MUSIC);
        EXPECT_EQ(MAX_VOL, mediaVol);

        int32_t ringVolume = audioGroupMngr_->GetMaxVolume(AudioVolumeType::STREAM_RING);
        EXPECT_EQ(MAX_VOL, ringVolume);
    }
}

/**
* @tc.name  : Test GetMaxVolume API
* @tc.number: GetMinVolumeTest_001
* @tc.desc  : Test GetMaxVolume of media stream
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, GetMinVolumeTest_001, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        int32_t mediaVol = audioGroupMngr_->GetMinVolume(AudioVolumeType::STREAM_MUSIC);
        EXPECT_EQ(MIN_VOL, mediaVol);

        int32_t ringVolume = audioGroupMngr_->GetMinVolume(AudioVolumeType::STREAM_RING);
        EXPECT_EQ(MIN_VOL, ringVolume);
    }
}

/**
* @tc.name  : Test SetMute API
* @tc.number: SetMute_001
* @tc.desc  : Test mute functionality of ringtone stream
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetMute_001, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_RING, true);
        EXPECT_EQ(SUCCESS, ret);

        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_RING, isMute);
        EXPECT_EQ(true, isMute);
    }
}

/**
* @tc.name  : Test SetMute IsStreamMute API
* @tc.number: SetMute_002
* @tc.desc  : Test unmute functionality of ringtone stream
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetMute_002, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_RING, false);
        EXPECT_EQ(SUCCESS, ret);

        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_RING, isMute);
        EXPECT_EQ(false, isMute);
    }
}

/**
* @tc.name  : Test SetMute IsStreamMute API
* @tc.number: SetMute_003
* @tc.desc  : Test mute functionality of media stream
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetMute_003, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_MUSIC, true);
        EXPECT_EQ(SUCCESS, ret);

        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_MUSIC, isMute);
        EXPECT_EQ(true, isMute);
    }
}

/**
* @tc.name  : Test SetMute IsStreamMute API
* @tc.number: SetMute_004
* @tc.desc  : Test unmute functionality of media stream
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetMute_004, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_MUSIC, false);
        EXPECT_EQ(SUCCESS, ret);

        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_RING, isMute);
        EXPECT_EQ(false, isMute);
    }
}
} // namespace AudioStandard
} // namespace OHOS
