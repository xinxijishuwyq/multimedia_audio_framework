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
#include "policy_handler.h"

using namespace testing::ext;
namespace OHOS {
namespace AudioStandard {
class PolicyHandlerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PolicyHandlerTest::SetUpTestCase(void)
{
    // input testsuit setup step，setup invoked before all testcases
}

void PolicyHandlerTest::TearDownTestCase(void)
{
    // input testsuit teardown step，teardown invoked after all testcases
}

void PolicyHandlerTest::SetUp(void)
{
    // input testcase setup step，setup invoked before each testcases
}

void PolicyHandlerTest::TearDown(void)
{
    // input testcase teardown step，teardown invoked after each testcases
}

/**
 * @tc.name  : Test IsAbsVolumeSupported
 * @tc.number: IsAbsVolumeSupported_001
 * @tc.desc  : Test function IsAbsVolumeSupported, always returns false when bluetooth is not connected.
 */
HWTEST(AudioPolicyUnitTest, IsAbsVolumeSupported_001, TestSize.Level1)
{
    bool ret = PolicyHandler::GetInstance().IsAbsVolumeSupported();
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name  : Test IsAbsVolumeSupported
 * @tc.number: IsAbsVolumeSupported_002
 * @tc.desc  : Test function IsAbsVolumeSupported, always returns false when bluetooth is not connected, try 100 times.
 */
HWTEST(AudioPolicyUnitTest, IsAbsVolumeSupported_002, TestSize.Level1)
{
    for (int32_t i = 0; i < 100; i++) { // Call IsAbsVolumeSupported 100 times
        bool ret = PolicyHandler::GetInstance().IsAbsVolumeSupported();
        EXPECT_EQ(false, ret);
    }
}
} // namespace AudioStandard
} // namespace OHOS
