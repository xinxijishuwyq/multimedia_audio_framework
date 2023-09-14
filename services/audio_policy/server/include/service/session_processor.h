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

#ifndef SESSION_PROCESSOR_H
#define SESSION_PROCESSOR_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include "audio_policy_server.h"
#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {
struct SessionEvent {
    enum class Type {
        ADD,
        REMOVE
    };

    SessionEvent() = delete;

    Type type;
    uint32_t sessionID;
};

class SessionProcessor {
public:
    DISALLOW_COPY_AND_MOVE(SessionProcessor);

    SessionProcessor(std::function<void(const uint32_t)> processorSessionRemoved)
        : processorSessionRemoved_(processorSessionRemoved)
    {
        Start();
    }

    ~SessionProcessor()
    {
        Stop();
    }

    void Post(const SessionEvent event)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        sessionEvents_.emplace(event);
        cv_.notify_one();
    }

    void Start()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (exitLoop_ == false) {
            AUDIO_INFO_LOG("exitLoop_ == false return");
            return;
        } else if (started_ == true) {
            AUDIO_INFO_LOG("started_ == false return");
            return;
        }
        exitLoop_ = false;
        started_ = true;
        loopThread_ = std::thread{&SessionProcessor::Loop, this};
        AUDIO_INFO_LOG("Start end");
    }

    void Stop()
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            exitLoop_ = true;
        }
        cv_.notify_one();
        if (loopThread_.joinable()) {
            loopThread_.join();
        }
        AUDIO_INFO_LOG("Stop end");
    }

private:
    void ProcessSessionEvent(SessionEvent event)
    {
        switch (event.type) {
            case SessionEvent::Type::REMOVE :
                processorSessionRemoved_(event.sessionID);
                break;
            default:
                break;
        }
    }

    void Loop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!exitLoop_) {
            while (sessionEvents_.size() > 0) {
                auto frontEvent = sessionEvents_.front();
                sessionEvents_.pop();
                ProcessSessionEvent(frontEvent);
            }
            cv_.wait(lock, [this] {
                bool res = (sessionEvents_.size() > 0 || exitLoop_);
                return res;
            });
        }
        started_ = false;
        AUDIO_INFO_LOG("Loop exit");
        return;
    }

    bool exitLoop_ = true;
    bool started_ = false;
    std::thread loopThread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<SessionEvent> sessionEvents_;
    std::function<void(const uint32_t)> processorSessionRemoved_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // SESSION_PROCESSOR_H