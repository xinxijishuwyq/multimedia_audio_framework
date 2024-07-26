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

#include <gtest/gtest.h>
#include "audio_utils.h"

using namespace testing::ext;
namespace OHOS {
namespace AudioStandard {

constexpr int32_t SUCCESS = 0;
class AudioUtilsUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void AudioUtilsUnitTest::SetUpTestCase(void)
{
    // input testsuit setup step，setup invoked before all testcases
}

void AudioUtilsUnitTest::TearDownTestCase(void)
{
    // input testsuit teardown step，teardown invoked after all testcases
}

void AudioUtilsUnitTest::SetUp(void)
{
    // input testcase setup step，setup invoked before each testcases
}

void AudioUtilsUnitTest::TearDown(void)
{
    // input testcase teardown step，teardown invoked after each testcases
}

/**
* @tc.name  : Test ClockTime API
* @tc.type  : FUNC
* @tc.number: ClockTime_001
* @tc.desc  : Test ClockTime interface.
*/
HWTEST(AudioUtilsUnitTest, ClockTime_001, TestSize.Level1)
{
    const int64_t CLOCK_TIME = 0;
    int32_t ret = -1;
    ret = ClockTime::AbsoluteSleep(CLOCK_TIME);
    EXPECT_EQ(SUCCESS - 1, ret);

    int64_t nanoTime = 1000;
    ret = ClockTime::AbsoluteSleep(nanoTime);
    EXPECT_EQ(SUCCESS, ret);

    ret = ClockTime::RelativeSleep(CLOCK_TIME);
    EXPECT_EQ(SUCCESS - 1, ret);

    ret = ClockTime::RelativeSleep(nanoTime);
    EXPECT_EQ(SUCCESS, ret);
}

/**
* @tc.name  : Test Trace API
* @tc.type  : FUNC
* @tc.number: Trace_001
* @tc.desc  : Test Trace interface.
*/
HWTEST(AudioUtilsUnitTest, Trace_001, TestSize.Level1)
{
    std::string value = "Test";
    std::shared_ptr<Trace> trace = std::make_shared<Trace>(value);
    trace->End();
    int64_t count = 1;
    Trace::Count(value, count);
}

/**
* @tc.name  : Test PermissionUtil API
* @tc.type  : FUNC
* @tc.number: PermissionUtil_001
* @tc.desc  : Test PermissionUtil interface.
*/
HWTEST(AudioUtilsUnitTest, PermissionUtil_001, TestSize.Level1)
{
    bool ret1 = PermissionUtil::VerifyIsSystemApp();
    EXPECT_EQ(false, ret1);
    bool ret2 = PermissionUtil::VerifySelfPermission();
    EXPECT_EQ(true, ret2);
    bool ret3 = PermissionUtil::VerifySystemPermission();
    EXPECT_EQ(true, ret3);
}

/**
* @tc.name  : Test AdjustStereoToMonoForPCM API
* @tc.type  : FUNC
* @tc.number: AdjustStereoToMonoForPCM_001
* @tc.desc  : Test AdjustStereoToMonoForPCM interface.
*/
HWTEST(AudioUtilsUnitTest, AdjustStereoToMonoForPCM_001, TestSize.Level1)
{
    uint64_t len = 2;

    const int8_t BitRET = 1;
    int8_t arr1[2] = {1, 2};
    int8_t *data1 = &arr1[0];
    AdjustStereoToMonoForPCM8Bit(data1, len);
    EXPECT_EQ(BitRET, data1[0]);
    EXPECT_EQ(BitRET, data1[1]);

    len = 4;
    const int16_t Bit16RET = 1;
    int16_t arr2[2] = {1, 2};
    int16_t *data2 = &arr2[0];
    AdjustStereoToMonoForPCM16Bit(data2, len);
    EXPECT_EQ(Bit16RET, data2[0]);
    EXPECT_EQ(Bit16RET, data2[1]);

    len = 6;
    const int8_t Bit8RET1 = 2;
    const int8_t Bit8RET2 = 3;
    const int8_t Bit8RET3 = 4;
    int8_t arr3[6] = {1, 2, 3, 4, 5, 6};
    int8_t *data3 = &arr3[0];
    AdjustStereoToMonoForPCM24Bit(data3, len);
    EXPECT_EQ(Bit8RET1, data3[0]);
    EXPECT_EQ(Bit8RET2, data3[1]);
    EXPECT_EQ(Bit8RET3, data3[2]);
    EXPECT_EQ(Bit8RET1, data3[3]);
    EXPECT_EQ(Bit8RET2, data3[4]);
    EXPECT_EQ(Bit8RET3, data3[5]);

    len = 8;
    const int32_t Bit32RET = 1;
    int32_t arr4[2] = {1, 2};
    int32_t *data4 = &arr4[0];
    AdjustStereoToMonoForPCM32Bit(data4, len);
    EXPECT_EQ(Bit32RET, data4[0]);
    EXPECT_EQ(Bit32RET, data4[1]);
}

/**
* @tc.name  : Test AdjustAudioBalanceForPCM API
* @tc.type  : FUNC
* @tc.number: AdjustAudioBalanceForPCM_001
* @tc.desc  : Test AdjustAudioBalanceForPCM interface.
*/
HWTEST(AudioUtilsUnitTest, AdjustAudioBalanceForPCM_001, TestSize.Level1)
{
    float left = 2.0;
    float right = 2.0;
    uint64_t len = 2;

    const int8_t Bit8RET1 = 2;
    const int8_t Bit8RET2 = 4;
    int8_t arr1[2] = {1, 2};
    int8_t *data1 = &arr1[0];
    AdjustAudioBalanceForPCM8Bit(data1, len, left, right);
    EXPECT_EQ(Bit8RET1, data1[0]);
    EXPECT_EQ(Bit8RET2, data1[1]);

    len = 4;
    const int16_t Bit16RET1 = 2;
    const int16_t Bit16RET2 = 4;
    int16_t arr2[2] = {1, 2};
    int16_t *data2 = &arr2[0];
    AdjustAudioBalanceForPCM16Bit(data2, len, left, right);
    EXPECT_EQ(Bit16RET1, data2[0]);
    EXPECT_EQ(Bit16RET2, data2[1]);

    len = 6;
    int8_t arr3[6] = {1, 2, 3, 4, 5, 6};
    int8_t *data3 = &arr3[0];
    AdjustAudioBalanceForPCM24Bit(data3, len, left, right);
    EXPECT_EQ(Bit8RET1, data3[0]);
    EXPECT_EQ(Bit8RET1*2, data3[1]);
    EXPECT_EQ(Bit8RET1*3, data3[2]);
    EXPECT_EQ(Bit8RET1*4, data3[3]);
    EXPECT_EQ(Bit8RET1*5, data3[4]);
    EXPECT_EQ(Bit8RET1*6, data3[5]);

    len = 8;
    const int32_t Bit32RET = 2;
    int32_t arr4[2] = {1, 2};
    int32_t *data4 = &arr4[0];
    AdjustAudioBalanceForPCM32Bit(data4, len, left, right);
    EXPECT_EQ(Bit32RET, data4[0]);
    EXPECT_EQ(Bit32RET*2, data4[1]);
}

/**
* @tc.name  : Test GetSysPara API
* @tc.type  : FUNC
* @tc.number: GetSysPara_001
* @tc.desc  : Test GetSysPara interface.
*/
HWTEST(AudioUtilsUnitTest, GetSysPara_001, TestSize.Level1)
{
    const char *invaildKey = nullptr;
    int32_t value32 = 2;
    bool result = GetSysPara(invaildKey, value32);
    EXPECT_EQ(false, result);
    const char *key = "debug.audio_service.testmodeon";
    bool result1 = GetSysPara(key, value32);
    EXPECT_EQ(true, result1);
    uint32_t valueU32 = 3;
    bool result2 = GetSysPara(key, valueU32);
    EXPECT_EQ(true, result2);
    int64_t value64 = 0;
    bool result3 = GetSysPara(key, value64);
    EXPECT_EQ(true, result3);
    std::string strValue = "100";
    bool result4 = GetSysPara(key, strValue);
    EXPECT_EQ(true, result4);
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetStreamName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetStreamName_001
* @tc.desc  : Test AudioInfoDumpUtils GetStreamName API,Return VOICE_ASSISTANT
*             when streamType is STREAM_VOICE_ASSISTANT
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetStreamName_001, TestSize.Level0)
{
    AudioStreamType streamType = STREAM_VOICE_ASSISTANT;
    const std::string stramName = AudioInfoDumpUtils::GetStreamName(streamType);
    EXPECT_EQ(stramName, "VOICE_ASSISTANT");
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetStreamName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetStreamName_002
* @tc.desc  : Test AudioInfoDumpUtils GetStreamName API,Return VOICE_CALL
*             when streamType is STREAM_VOICE_CALL
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetStreamName_002, TestSize.Level0)
{
    AudioStreamType streamType = STREAM_VOICE_CALL;
    const std::string stramName = AudioInfoDumpUtils::GetStreamName(streamType);
    EXPECT_EQ(stramName, "VOICE_CALL");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetStreamName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetStreamName_003
* @tc.desc  : Test AudioInfoDumpUtils GetStreamName API,Return VOICE_CALL
*             when streamType is STREAM_VOICE_COMMUNICATION
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetStreamName_003, TestSize.Level0)
{
    AudioStreamType streamType = STREAM_VOICE_COMMUNICATION;
    const std::string stramName = AudioInfoDumpUtils::GetStreamName(streamType);
    EXPECT_EQ(stramName, "VOICE_CALL");
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetStreamName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetStreamName_004
* @tc.desc  : Test AudioInfoDumpUtils GetStreamName API,Return SYSTEM
*             when streamType is STREAM_SYSTEM
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetStreamName_004, TestSize.Level0)
{
    AudioStreamType streamType = STREAM_SYSTEM;
    const std::string stramName = AudioInfoDumpUtils::GetStreamName(streamType);
    EXPECT_EQ(stramName, "SYSTEM");
}


/**
* @tc.name  : Test AudioInfoDumpUtils::GetStreamName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetStreamName_005
* @tc.desc  : Test AudioInfoDumpUtils GetStreamName API,Return RING
*             when streamType is STREAM_RING
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetStreamName_005, TestSize.Level0)
{
    AudioStreamType streamType = STREAM_RING;
    const std::string stramName = AudioInfoDumpUtils::GetStreamName(streamType);
    EXPECT_EQ(stramName, "RING");
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetStreamName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetStreamName_006
* @tc.desc  : Test AudioInfoDumpUtils GetStreamName API,Return MUSIC
*             when streamType is STREAM_MUSIC
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetStreamName_006, TestSize.Level0)
{
    AudioStreamType streamType = STREAM_MUSIC;
    const std::string stramName = AudioInfoDumpUtils::GetStreamName(streamType);
    EXPECT_EQ(stramName, "MUSIC");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetStreamName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetStreamName_007
* @tc.desc  : Test AudioInfoDumpUtils GetStreamName API,Return ALARM
*             when streamType is STREAM_ALARM
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetStreamName_007, TestSize.Level0)
{
    AudioStreamType streamType = STREAM_ALARM;
    const std::string stramName = AudioInfoDumpUtils::GetStreamName(streamType);
    EXPECT_EQ(stramName, "ALARM");
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetStreamName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetStreamName_008
* @tc.desc  : Test AudioInfoDumpUtils GetStreamName API,Return NOTIFICATION
*             when streamType is STREAM_NOTIFICATION
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetStreamName_008, TestSize.Level0)
{
    AudioStreamType streamType = STREAM_NOTIFICATION;
    const std::string stramName = AudioInfoDumpUtils::GetStreamName(streamType);
    EXPECT_EQ(stramName, "NOTIFICATION");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetStreamName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetStreamName_009
* @tc.desc  : Test AudioInfoDumpUtils GetStreamName API,Return BLUETOOTH_SCO
*             when streamType is STREAM_BLUETOOTH_SCO
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetStreamName_009, TestSize.Level0)
{
    AudioStreamType streamType = STREAM_BLUETOOTH_SCO;
    const std::string stramName = AudioInfoDumpUtils::GetStreamName(streamType);
    EXPECT_EQ(stramName, "BLUETOOTH_SCO");
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetStreamName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetStreamName_010
* @tc.desc  : Test AudioInfoDumpUtils GetStreamName API,Return DTMF
*             when streamType is STREAM_DTMF
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetStreamName_010, TestSize.Level0)
{
    AudioStreamType streamType = STREAM_DTMF;
    const std::string stramName = AudioInfoDumpUtils::GetStreamName(streamType);
    EXPECT_EQ(stramName, "DTMF");
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetStreamName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetStreamName_011
* @tc.desc  : Test AudioInfoDumpUtils GetStreamName API,Return TTS
*             when streamType is STREAM_TTS
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetStreamName_011, TestSize.Level0)
{
    AudioStreamType streamType = STREAM_TTS;
    const std::string stramName = AudioInfoDumpUtils::GetStreamName(streamType);
    EXPECT_EQ(stramName, "TTS");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetStreamName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetStreamName_013
* @tc.desc  : Test AudioInfoDumpUtils GetStreamName API,Return ULTRASONIC
*             when streamType is STREAM_ULTRASONIC
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetStreamName_013, TestSize.Level0)
{
    AudioStreamType streamType = STREAM_ULTRASONIC;
    const std::string stramName = AudioInfoDumpUtils::GetStreamName(streamType);
    EXPECT_EQ(stramName, "ULTRASONIC");
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetStreamName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetStreamName_014
* @tc.desc  : Test AudioInfoDumpUtils GetStreamName API,Return WAKEUP
*             when streamType is STREAM_WAKEUP
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetStreamName_014, TestSize.Level0)
{
    AudioStreamType streamType = STREAM_WAKEUP;
    const std::string stramName = AudioInfoDumpUtils::GetStreamName(streamType);
    EXPECT_EQ(stramName, "WAKEUP");
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetStreamName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetStreamName_015
* @tc.desc  : Test AudioInfoDumpUtils GetStreamName API,Return UNKNOWN
*             when streamType is others
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetStreamName_015, TestSize.Level0)
{
    AudioStreamType streamType = STREAM_DEFAULT;
    const std::string stramName = AudioInfoDumpUtils::GetStreamName(streamType);
    EXPECT_EQ(stramName, "UNKNOWN");
}


/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceTypeName_001
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceTypeName API,Return EARPIECE
*             when deviceType is DEVICE_TYPE_EARPIECE
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceTypeName_001, TestSize.Level0)
{
    DeviceType deviceType = DEVICE_TYPE_EARPIECE;
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "EARPIECE");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceTypeName_002
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceTypeName API,Return SPEAKER
*             when deviceType is DEVICE_TYPE_SPEAKER
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceTypeName_002, TestSize.Level0)
{
    DeviceType deviceType = DEVICE_TYPE_SPEAKER;
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "SPEAKER");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceTypeName_003
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceTypeName API,Return WIRED_HEADSET
*             when deviceType is DEVICE_TYPE_WIRED_HEADSET
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceTypeName_003, TestSize.Level0)
{
    DeviceType deviceType = DEVICE_TYPE_WIRED_HEADSET;
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "WIRED_HEADSET");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceTypeName_004
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceTypeName API,Return SPEAKER
*             when deviceType is DEVICE_TYPE_WIRED_HEADPHONES
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceTypeName_004, TestSize.Level0)
{
    DeviceType deviceType = DEVICE_TYPE_WIRED_HEADPHONES;
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "WIRED_HEADPHONES");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceTypeName_005
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceTypeName API,Return BLUETOOTH_SCO
*             when deviceType is DEVICE_TYPE_BLUETOOTH_SCO
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceTypeName_005, TestSize.Level0)
{
    DeviceType deviceType = DEVICE_TYPE_BLUETOOTH_SCO;
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "BLUETOOTH_SCO");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceTypeName_006
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceTypeName API,Return BLUETOOTH_A2DP
*             when deviceType is DEVICE_TYPE_BLUETOOTH_A2DP
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceTypeName_006, TestSize.Level0)
{
    DeviceType deviceType = DEVICE_TYPE_BLUETOOTH_A2DP;
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "BLUETOOTH_A2DP");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceTypeName_007
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceTypeName API,Return MIC
*             when deviceType is DEVICE_TYPE_MIC
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceTypeName_007, TestSize.Level0)
{
    DeviceType deviceType = DEVICE_TYPE_MIC;
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "MIC");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceTypeName_008
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceTypeName API,Return WAKEUP
*             when deviceType is DEVICE_TYPE_WAKEUP
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceTypeName_008, TestSize.Level0)
{
    DeviceType deviceType = DEVICE_TYPE_WAKEUP;
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "WAKEUP");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceTypeName_009
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceTypeName API,Return NONE
*             when deviceType is DEVICE_TYPE_NONE
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceTypeName_009, TestSize.Level0)
{
    DeviceType deviceType = DEVICE_TYPE_NONE;
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "NONE");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceTypeName_010
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceTypeName API,Return INVALID
*             when deviceType is DEVICE_TYPE_INVALID
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceTypeName_010, TestSize.Level0)
{
    DeviceType deviceType = DEVICE_TYPE_INVALID;
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "INVALID");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceTypeName_011
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceTypeName API,Return UNKNOWN
*             when deviceType is others
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceTypeName_011, TestSize.Level0)
{
    DeviceType deviceType = DEVICE_TYPE_DEFAULT;
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "UNKNOWN");
}


/**
* @tc.name  : Test AudioInfoDumpUtils::GetConnectTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetConnectTypeName_001
* @tc.desc  : Test AudioInfoDumpUtils GetConnectTypeName API,Return LOCAL
*             when connectType is CONNECT_TYPE_LOCAL
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetConnectTypeName_001, TestSize.Level0)
{
    ConnectType connectType = OHOS::AudioStandard::CONNECT_TYPE_LOCAL;
    const std::string connectTypeName = AudioInfoDumpUtils::GetConnectTypeName(connectType);
    EXPECT_EQ(connectTypeName, "LOCAL");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetConnectTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetConnectTypeName_002
* @tc.desc  : Test AudioInfoDumpUtils GetConnectTypeName API,Return REMOTE
*             when connectType is CONNECT_TYPE_DISTRIBUTED
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetConnectTypeName_002, TestSize.Level0)
{
    ConnectType connectType = AudioStandard::CONNECT_TYPE_DISTRIBUTED;
    const std::string connectTypeName = AudioInfoDumpUtils::GetConnectTypeName(connectType);
    EXPECT_EQ(connectTypeName, "REMOTE");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetConnectTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetConnectTypeName_003
* @tc.desc  : Test AudioInfoDumpUtils GetConnectTypeName API,Return UNKNOWN
*             when connectType is others
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetConnectTypeName_003, TestSize.Level0)
{
    ConnectType connectType =  static_cast<ConnectType>(-1);
    const std::string connectTypeName = AudioInfoDumpUtils::GetConnectTypeName(connectType);
    EXPECT_EQ(connectTypeName, "UNKNOWN");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetSourceName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetSourceName_001
* @tc.desc  : Test AudioInfoDumpUtils GetSourceName API,Return INVALID
*             when sourceType is SOURCE_TYPE_INVALID
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetSourceName_001, TestSize.Level0)
{
    SourceType sourceType = SOURCE_TYPE_INVALID;
    const std::string sourceName = AudioInfoDumpUtils::GetSourceName(sourceType);
    EXPECT_EQ(sourceName, "INVALID");
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetSourceName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetSourceName_002
* @tc.desc  : Test AudioInfoDumpUtils GetSourceName API,Return MIC
*             when sourceType is SOURCE_TYPE_MIC
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetSourceName_002, TestSize.Level0)
{
    SourceType sourceType = SOURCE_TYPE_MIC;
    const std::string sourceName = AudioInfoDumpUtils::GetSourceName(sourceType);
    EXPECT_EQ(sourceName, "MIC");
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetSourceName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetSourceName_003
* @tc.desc  : Test AudioInfoDumpUtils GetSourceName API,Return VOICE_RECOGNITION
*             when sourceType is SOURCE_TYPE_VOICE_RECOGNITION
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetSourceName_003, TestSize.Level0)
{
    SourceType sourceType = SOURCE_TYPE_VOICE_RECOGNITION;
    const std::string sourceName = AudioInfoDumpUtils::GetSourceName(sourceType);
    EXPECT_EQ(sourceName, "VOICE_RECOGNITION");
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetSourceName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetSourceName_004
* @tc.desc  : Test AudioInfoDumpUtils GetSourceName API,Return ULTRASONIC
*             when sourceType is SOURCE_TYPE_ULTRASONIC
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetSourceName_004, TestSize.Level0)
{
    SourceType sourceType = SOURCE_TYPE_ULTRASONIC;
    const std::string sourceName = AudioInfoDumpUtils::GetSourceName(sourceType);
    EXPECT_EQ(sourceName, "ULTRASONIC");
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetSourceName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetSourceName_005
* @tc.desc  : Test AudioInfoDumpUtils GetSourceName API,Return VOICE_COMMUNICATION
*             when sourceType is SOURCE_TYPE_VOICE_COMMUNICATION
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetSourceName_005, TestSize.Level0)
{
    SourceType sourceType = SOURCE_TYPE_VOICE_COMMUNICATION;
    const std::string sourceName = AudioInfoDumpUtils::GetSourceName(sourceType);
    EXPECT_EQ(sourceName, "VOICE_COMMUNICATION");
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetSourceName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetSourceName_006
* @tc.desc  : Test AudioInfoDumpUtils GetSourceName API,Return WAKEUP
*             when sourceType is SOURCE_TYPE_WAKEUP
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetSourceName_006, TestSize.Level0)
{
    SourceType sourceType = SOURCE_TYPE_WAKEUP;
    const std::string sourceName = AudioInfoDumpUtils::GetSourceName(sourceType);
    EXPECT_EQ(sourceName, "WAKEUP");
}
/**
* @tc.name  : Test AudioInfoDumpUtils::GetSourceName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetSourceName_007
* @tc.desc  : Test AudioInfoDumpUtils GetSourceName API,Return UNKNOWN
*             when sourceType is others
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetSourceName_007, TestSize.Level0)
{
    SourceType sourceType = SOURCE_TYPE_MAX;
    const std::string sourceName = AudioInfoDumpUtils::GetSourceName(sourceType);
    EXPECT_EQ(sourceName, "UNKNOWN");
}


/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceVolumeTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceVolumeTypeName_001
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceVolumeTypeName API,Return EARPIECE
*             when deviceType is EARPIECE_VOLUME_TYPE
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceVolumeTypeName_001, TestSize.Level0)
{
    DeviceVolumeType deviceType = EARPIECE_VOLUME_TYPE;
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceVolumeTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "EARPIECE");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceVolumeTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceVolumeTypeName_002
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceVolumeTypeName API,Return SPEAKER
*             when deviceType is SPEAKER_VOLUME_TYPE
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceVolumeTypeName_002, TestSize.Level0)
{
    DeviceVolumeType deviceType = SPEAKER_VOLUME_TYPE;
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceVolumeTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "SPEAKER");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceVolumeTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceVolumeTypeName_003
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceVolumeTypeName API,Return HEADSET
*             when deviceType is HEADSET_VOLUME_TYPE
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceVolumeTypeName_003, TestSize.Level0)
{
    DeviceVolumeType deviceType = HEADSET_VOLUME_TYPE;
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceVolumeTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "HEADSET");
}

/**
* @tc.name  : Test AudioInfoDumpUtils::GetDeviceVolumeTypeName  API
* @tc.type  : FUNC
* @tc.number: AudioInfoDumpUtils_GetDeviceVolumeTypeName_004
* @tc.desc  : Test AudioInfoDumpUtils GetDeviceVolumeTypeName API,Return UNKNOWN
*             when deviceType is others
*/
HWTEST(AudioUtilsUnitTest, AudioInfoDumpUtils_GetDeviceVolumeTypeName_004, TestSize.Level0)
{
    DeviceVolumeType deviceType = static_cast<DeviceVolumeType>(-1);
    const std::string deviceTypeName = AudioInfoDumpUtils::GetDeviceVolumeTypeName(deviceType);
    EXPECT_EQ(deviceTypeName, "UNKNOWN");
}


/**
* @tc.name  : Test GetEncryptStr  API
* @tc.type  : FUNC
* @tc.number: GetEncryptStr_001
* @tc.desc  : Test GetEncryptStr API,Return empty when src is empty
*/
HWTEST(AudioUtilsUnitTest, GetEncryptStr_001, TestSize.Level0)
{
    const std::string src = "";
    std::string dst = GetEncryptStr(src);
    EXPECT_EQ(dst, "");
}
/**
* @tc.name  : Test GetEncryptStr  API
* @tc.type  : FUNC
* @tc.number: GetEncryptStr_002
* @tc.desc  : Test GetEncryptStr APIwhen length of src less than MIN_LEN
*/
HWTEST(AudioUtilsUnitTest, GetEncryptStr_002, TestSize.Level0)
{
    const std::string src = "abcdef";
    std::string dst = GetEncryptStr(src);
    EXPECT_EQ(dst, "*bcdef");
}
} // namespace AudioStandard
} // namespace OHOS