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
#include <set>

#ifdef RESSCHE_ENABLE
#include "res_type.h"
#include "res_sched_client.h"
#endif

#include "audio_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RESSCHE_ENABLE
const uint32_t AUDIO_QOS_LEVEL = 7;
static std::mutex g_rssMutex;
static std::set<uint32_t> g_tidToReport = {};
constexpr uint32_t g_type = OHOS::ResourceSchedule::ResType::RES_TYPE_THREAD_QOS_CHANGE;
constexpr int64_t g_value = 0;

void ConfigPayload(uint32_t pid, uint32_t tid, const char *bundleName, int32_t qosLevel,
    std::unordered_map<std::string, std::string> &mapPayload)
{
    std::string strBundleName = bundleName;
    std::string strPid = std::to_string(pid);
    std::string strTid = std::to_string(tid);
    std::string strQos = std::to_string(qosLevel);
    mapPayload["pid"] = strPid;
    mapPayload[strTid] = strQos;
    mapPayload["bundleName"] = strBundleName;
}

void ScheduleReportData(uint32_t pid, uint32_t tid, const char *bundleName)
{
    AUDIO_INFO_LOG("Report tid %{public}u", tid);
    std::unordered_map<std::string, std::string> mapPayload;
    ConfigPayload(pid, tid, bundleName, AUDIO_QOS_LEVEL, mapPayload);
    OHOS::ResourceSchedule::ResSchedClient::GetInstance().ReportData(g_type, g_value, mapPayload);
}

void UnscheduleThreadInServer(uint32_t tid)
{
    std::lock_guard<std::mutex> lock(g_rssMutex);
    if (g_tidToReport.find(tid) != g_tidToReport.end()) {
        AUDIO_INFO_LOG("Remove tid in server %{public}u", tid);
        g_tidToReport.erase(tid);
    }
}

void ScheduleThreadInServer(uint32_t pid, uint32_t tid)
{
    std::lock_guard<std::mutex> lock(g_rssMutex);
    if (g_tidToReport.find(tid) == g_tidToReport.end()) {
        AUDIO_INFO_LOG("Add tid in server %{public}u", tid);
        g_tidToReport.insert(tid);
    }
    ScheduleReportData(pid, tid, "audio_server");
}

void OnAddResSchedService(uint32_t audioServerPid)
{
    std::lock_guard<std::mutex> lock(g_rssMutex);
    for (auto tid : g_tidToReport) {
        AUDIO_INFO_LOG("On add rss, report %{public}u", tid);
        ScheduleReportData(audioServerPid, tid, "audio_server");
    }
}
#else
void ScheduleReportData(uint32_t /* pid */, uint32_t /* tid */, const char* /* bundleName*/) {};
void ScheduleThreadInServer(uint32_t pid, uint32_t tid) {};
void UnscheduleThreadInServer(uint32_t tid) {};
void OnAddResSchedService(uint32_t audioServerPid) {};
#endif

#ifdef __cplusplus
}
#endif
