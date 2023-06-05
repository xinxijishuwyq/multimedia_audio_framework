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

#include "audio_schedule.h"

#include <unistd.h>
#include <sys/types.h>
#include <cstring>
#include <unordered_map>

#ifdef RESSCHE_ENABLE
#include "res_type.h"
#include "res_sched_client.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RESSCHE_ENABLE
const uint32_t AUDIO_QOS_LEVEL = 7;

void ScheduleReportData(uint32_t pid, uint32_t tid, const char* bundleName)
{
    std::string strBundleName = bundleName;
    std::string strPid = std::to_string(pid);
    std::string strTid = std::to_string(tid);
    std::string strQos = std::to_string(AUDIO_QOS_LEVEL);
    std::unordered_map<std::string, std::string> mapPayload;
    mapPayload["pid"] = strPid;
    mapPayload[strTid] = strQos;
    mapPayload["bundleName"] = strBundleName;
    uint32_t type = OHOS::ResourceSchedule::ResType::RES_TYPE_THREAD_QOS_CHANGE;
    int64_t value = 0;
    OHOS::ResourceSchedule::ResSchedClient::GetInstance().ReportData(type, value, mapPayload);
}
#else
void ScheduleReportData(uint32_t /* pid */, uint32_t /* tid */, const char* /* bundleName*/) {};
#endif

#ifdef __cplusplus
}
#endif