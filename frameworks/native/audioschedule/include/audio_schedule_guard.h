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

#ifndef AUDIO_SCHEDULE_GUARD_H
#define AUDIO_SCHEDULE_GUARD_H

#include <inttypes.h>
#include <string>
#include "audio_schedule.h"

namespace OHOS {
namespace AudioStandard {
class AudioScheduleGuard {
public:
    AudioScheduleGuard(uint32_t pid, uint32_t tid, const std::string &bundleName = "audio_server")
        : pid_(pid), tid_(tid), bundleName_(bundleName)
    {
        ScheduleReportData(pid, tid, bundleName.c_str());
        isReported_ = true;
    }

    AudioScheduleGuard(const AudioScheduleGuard&) = delete;

    AudioScheduleGuard operator=(const AudioScheduleGuard&) = delete;

    AudioScheduleGuard(AudioScheduleGuard&& audioScheduleGuard)
        : pid_(audioScheduleGuard.pid_), tid_(audioScheduleGuard.tid_),
        bundleName_(std::move(audioScheduleGuard.bundleName_)), isReported_(audioScheduleGuard.isReported_)
    {
        audioScheduleGuard.isReported_ = false;
    }

    bool operator==(const AudioScheduleGuard&) const = default;

    AudioScheduleGuard& operator=(AudioScheduleGuard&& audioScheduleGuard)
    {
        if (*this == audioScheduleGuard) {
            audioScheduleGuard.isReported_ = false;
            return *this;
        }

        AudioScheduleGuard temp(std::move(*this));
        this->bundleName_ = std::move(audioScheduleGuard.bundleName_);
        this->isReported_ = audioScheduleGuard.isReported_;
        this->pid_ = audioScheduleGuard.pid_;
        this->tid_ = audioScheduleGuard.tid_;
        audioScheduleGuard.isReported_ = false;

        return *this;
    }

    ~AudioScheduleGuard()
    {
        if (isReported_) {
            UnscheduleReportData(pid_, tid_, bundleName_.c_str());
        }
    }
private:
    uint32_t pid_;
    uint32_t tid_;
    std::string bundleName_;
    bool isReported_ = false;
};
} // namespace AudioStandard
} // namespace OHOS

#endif // AUDIO_SCHEDULE_GUARD_H