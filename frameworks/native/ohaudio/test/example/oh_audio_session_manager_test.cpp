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

#include "native_audio_session_manager.h"
#include "native_audio_common.h"
#include "audio_session_manager.h"
#include <ostream>
#include <sstream>
#include <iostream>
#include <string>
#include <map>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>
#include "audio_errors.h"
#include "audio_log.h"
#include "audio_utils.h"

namespace OHOS {
namespace AudioStandard {

class SessionNdkTest {
public:
    SessionNdkTest() = default;
    ~SessionNdkTest() = default;
    void Init();
    void RegisterCallback(OH_AudioSession_DeactivatedCallback callback);
    void UnRegisterCallback(OH_AudioSession_DeactivatedCallback callback);
    void ActivateAudioSession(OH_AudioSession_Strategy strategy);
    void DeActivateAudioSession();
    void IsAudioSessionActivated();
    int32_t MyCallbackFunction(OH_AudioSession_DeactivatedEvent event);
};

OH_AudioSessionManager *audioSessionManager;

OH_AudioSession_DeactivatedEvent event;

std::shared_ptr<SessionNdkTest> g_sessionNdkTest = nullptr;

OH_AudioSession_Strategy strategy;

const int CASE_EXIT = 0;

const int CASE_REGISTER = 1;

const int CASE_UN_REGISTER = 2;

const int CASE_ACTIVE = 3;

const int CASE_DEACTIVE = 4;

const int CASE_IS_ACTIVE = 5;

void SessionNdkTest::Init()
{
    OH_AudioManager_GetAudioSessionManager(&audioSessionManager);
}

void SessionNdkTest::RegisterCallback(OH_AudioSession_DeactivatedCallback callback)
{
    std::cout << "Start Register callback" << std::endl;
    OH_AudioCommon_Result result =
        OH_AudioSessionManager_RegisterSessionDeactivatedCallback(audioSessionManager, callback);
    if (result == AUDIOCOMMON_RESULT_SUCCESS) {
        std::cout << "Register callback SUCCESS" << std::endl;
    } else {
        std::cout << "Register callback FAILED" << std::endl;
    }
}

void SessionNdkTest::UnRegisterCallback(OH_AudioSession_DeactivatedCallback callback)
{
    std::cout << "Start UnRegister callback" << std::endl;
    OH_AudioCommon_Result result =
        OH_AudioSessionManager_UnregisterSessionDeactivatedCallback(audioSessionManager, callback);
    if (result == AUDIOCOMMON_RESULT_SUCCESS) {
        std::cout << "UnRegister callback SUCCESS" << std::endl;
    } else {
        std::cout << "UnRegister callback FAILED" << std::endl;
    }
}

void SessionNdkTest::ActivateAudioSession(OH_AudioSession_Strategy strategy)
{
    std::cout << "Start Activate AudioSession" << std::endl;
    OH_AudioCommon_Result result =
        OH_AudioSessionManager_ActivateAudioSession(audioSessionManager, strategy);
    if (result == AUDIOCOMMON_RESULT_SUCCESS) {
        std::cout << "Activate AudioSession SUCCESS" << std::endl;
    } else {
        std::cout << "Activate AudioSession FAILED" << std::endl;
    }
}

void SessionNdkTest::DeActivateAudioSession()
{
    std::cout << "Start DeActivate AudioSession" << std::endl;
    OH_AudioCommon_Result result = OH_AudioSessionManager_DeactivateAudioSession(audioSessionManager);
    if (result == AUDIOCOMMON_RESULT_SUCCESS) {
        std::cout << "DeActivate AudioSession SUCCESS" << std::endl;
    } else {
        std::cout << "DeActivate AudioSession FAILED" << std::endl;
    }
}

void SessionNdkTest::IsAudioSessionActivated()
{
    std::cout << "Start Is Activate AudioSession" << std::endl;
    bool result = OH_AudioSessionManager_IsAudioSessionActivated(audioSessionManager);
    if (result) {
        std::cout << "Is Activate AudioSession SUCCESS" << std::endl;
    } else {
        std::cout << "Is Activate AudioSession FAILED" << std::endl;
    }
}

int32_t MyCallbackFunction(OH_AudioSession_DeactivatedEvent event)
{
    std::cout << "Callback For Event" << std::endl;
    return 0;
}

} // namespace AudioStandard
} // namespace OHOS


using namespace OHOS::AudioStandard;
int main()
{
    // Init Get AudioSessionManager
    g_sessionNdkTest = std::make_shared<SessionNdkTest>();
    
    g_sessionNdkTest->Init();

    std::cout << "Init Completed, Start Test" << std::endl;

    bool runFlag = true;
    int operate;

    OH_AudioSession_DeactivatedCallback callback = MyCallbackFunction;

    while (runFlag) {
        std::cout << "Please Input Operate" << std::endl;
        std::cout << "1 -----> Register Callback" << std::endl;
        std::cout << "2 -----> UnRegister Callback" << std::endl;
        std::cout << "3 -----> Activate AudioSession" << std::endl;
        std::cout << "4 -----> DeActivate AudioSession" << std::endl;
        std::cout << "5 -----> Is Activate Judging" << std::endl;
        std::cout << "0 -----> Exit" << std::endl;

        std::cin >> operate;

        if (operate == CASE_EXIT) {
            runFlag = false;
        }

        switch (operate) {
            case CASE_REGISTER:
                std::cout << "Start Build Event" << std::endl;
                event.reason = DEACTIVATED_LOWER_PRIORITY;
                std::cout << "Build Event Completed; Use Event Start Build Callback" << std::endl;
                callback(event);
                std::cout << "Build Call Completed" << std::endl;
                g_sessionNdkTest->RegisterCallback(callback);
                break;
            case CASE_UN_REGISTER:
                event.reason = DEACTIVATED_TIMEOUT;
                callback(event);
                g_sessionNdkTest->UnRegisterCallback(callback);
                break;
            case CASE_ACTIVE:
                strategy.concurrencyMode = CONCURRENCY_MIX_WITH_OTHERS;
                g_sessionNdkTest->ActivateAudioSession(strategy);
                break;
            case CASE_DEACTIVE:
                g_sessionNdkTest->DeActivateAudioSession();
                break;
            case CASE_IS_ACTIVE:
                g_sessionNdkTest->IsAudioSessionActivated();
                break;
            default:
                std::cout << "Input Valid, RE Input";
                break;
        }
    }
    std::cout << "End Test" << std::endl;
}