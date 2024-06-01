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

#ifndef AUDIO_RUNNING_LOCK_MANAGER_H
#define AUDIO_RUNNING_LOCK_MANAGER_H

#include <unordered_set>
#include <mutex>
#include <unordered_set>
#include <vector>
#include "audio_errors.h"

namespace OHOS {
namespace AudioStandard {
template<typename T>
class AudioRunningLockManager {
public:
    explicit AudioRunningLockManager(std::shared_ptr<T> runningLock) : runningLock_(runningLock)
    {
    }

    auto Lock(const int32_t TimeoutMs)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            lastAppsUid_ = {};
        }

        auto ret = runningLock_->Lock(TimeoutMs);
        isLocked_ = true;
        return ret;
    }

    auto UnLock()
    {
        isLocked_ = false;
        return runningLock_->UnLock();
    }

    template<typename U>
    int32_t UpdateAppsUid(const U &itBegin, const U &itEnd)
    {
        std::unordered_set<int32_t> appsUidSet(itBegin, itEnd);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            currentAppsUid_ = std::move(appsUidSet);
            return SUCCESS;
        }
    }

    int32_t UpdateAppsUidToPowerMgr()
    {
        if (!isLocked_) {
            return SUCCESS;
        }
        std::vector<int32_t> appsUid;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (currentAppsUid_ == lastAppsUid_) {
                return SUCCESS;
            }
            lastAppsUid_ = currentAppsUid_;
            appsUid.insert(appsUid.end(), currentAppsUid_.begin(), currentAppsUid_.end());
        }
        return runningLock_->UpdateWorkSource(appsUid);
    }

private:
    std::shared_ptr<T> runningLock_ = nullptr;
    std::mutex mutex_;
    std::unordered_set<int32_t> currentAppsUid_;
    std::unordered_set<int32_t> lastAppsUid_;
    std::atomic<bool> isLocked_ = false;
};

}  // namespace AudioStandard
}  // namespace OHOS

#endif // AUDIO_RUNNING_LOCK_MANAGER_H