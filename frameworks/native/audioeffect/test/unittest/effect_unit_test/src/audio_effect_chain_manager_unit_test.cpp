/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *111111111111111111111
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "audio_effect_chain_manager_unit_test.h"

#include "audio_effect_chain_manager.h"
#include "audio_errors.h"

#include <chrono>
#include <thread>
#include <fstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace std;
using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace AudioStandard {

void AudioEffectChainManagerUnitTest::SetUpTestCase(void) {}
void AudioEffectChainManagerUnitTest::TearDownTestCase(void) {}
void AudioEffectChainManagerUnitTest::SetUp(void) {}
void AudioEffectChainManagerUnitTest::TearDown(void) {}

/**
* @tc.name   : Test CreateAudioEffectChainDynamic API
* @tc.number : CreateAudioEffectChainDynamic_001
* @tc.desc   : Test InitAudioEffectChainManager interface.Test GetDeviceTypeName interface simultaneously.
*/
HWTEST(AudioEffectChainManagerUnitTest,CreateAudioEffectChainDynamic_001,TestSize.Level1)
{
    string sceneType = "";
   
    int32_t result = AudioEffectChainManager::GetInstance()->CreateAudioEffectChainDynamic(sceneType);
    EXPECT_EQ(ERROR,result);
}

/**
* @tc.name   : Test CreateAudioEffectChainDynamic API
* @tc.number : CreateAudioEffectChainDynamic_002
* @tc.desc   : Test InitAudioEffectChainManager interface.Test GetDeviceTypeName interface simultaneously.
*/
HWTEST(AudioEffectChainManagerUnitTest,CreateAudioEffectChainDynamic_002,TestSize.Level1)
{
    string sceneType = "123";

    int32_t result = AudioEffectChainManager::GetInstance()->CreateAudioEffectChainDynamic(sceneType);
    EXPECT_EQ(ERROR,result);
}

/**
* @tc.name   : Test CreateAudioEffectChainDynamic API
* @tc.number : CreateAudioEffectChainDynamic_003
* @tc.desc   : Test InitAudioEffectChainManager interface.Test GetDeviceTypeName interface simultaneously.
*/
HWTEST(AudioEffectChainManagerUnitTest,CreateAudioEffectChainDynamic_003,TestSize.Level1)
{
    string sceneType = "SCENE_OTHERS";

    int32_t result =  AudioEffectChainManager::GetInstance()->CreateAudioEffectChainDynamic(sceneType);
    EXPECT_EQ(SUCCESS,result);
}
}
}




