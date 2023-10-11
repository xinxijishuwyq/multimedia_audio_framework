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

#include <cinttypes>
#include <ctime>
#include <sstream>
#include <ostream>
#include "audio_utils.h"
#include "audio_errors.h"
#include "audio_log.h"
#ifdef FEATURE_HITRACE_METER
#include "hitrace_meter.h"
#endif
#include "parameter.h"
#include "tokenid_kit.h"
#include "ipc_skeleton.h"
#include "access_token.h"
#include "accesstoken_kit.h"
#include "xcollie/xcollie.h"
#include "xcollie/xcollie_define.h"

using OHOS::Security::AccessToken::AccessTokenKit;

namespace OHOS {
namespace AudioStandard {
int64_t ClockTime::GetCurNano()
{
    int64_t result = -1; // -1 for bad result.
    struct timespec time;
    clockid_t clockId = CLOCK_MONOTONIC;
    int ret = clock_gettime(clockId, &time);
    if (ret < 0) {
        AUDIO_WARNING_LOG("GetCurNanoTime fail, result:%{public}d", ret);
        return result;
    }
    result = (time.tv_sec * AUDIO_NS_PER_SECOND) + time.tv_nsec;
    return result;
}

int32_t ClockTime::AbsoluteSleep(int64_t nanoTime)
{
    int32_t ret = -1; // -1 for bad result.
    if (nanoTime <= 0) {
        AUDIO_WARNING_LOG("AbsoluteSleep invalid sleep time :%{public}" PRId64 " ns", nanoTime);
        return ret;
    }
    struct timespec time;
    time.tv_sec = nanoTime / AUDIO_NS_PER_SECOND;
    time.tv_nsec = nanoTime - (time.tv_sec * AUDIO_NS_PER_SECOND); // Avoids % operation.

    clockid_t clockId = CLOCK_MONOTONIC;
    ret = clock_nanosleep(clockId, TIMER_ABSTIME, &time, nullptr);
    if (ret != 0) {
        AUDIO_WARNING_LOG("AbsoluteSleep may failed, ret is :%{public}d", ret);
    }

    return ret;
}

int32_t ClockTime::RelativeSleep(int64_t nanoTime)
{
    int32_t ret = -1; // -1 for bad result.
    if (nanoTime <= 0) {
        AUDIO_WARNING_LOG("AbsoluteSleep invalid sleep time :%{public}" PRId64 " ns", nanoTime);
        return ret;
    }
    struct timespec time;
    time.tv_sec = nanoTime / AUDIO_NS_PER_SECOND;
    time.tv_nsec = nanoTime - (time.tv_sec * AUDIO_NS_PER_SECOND); // Avoids % operation.

    clockid_t clockId = CLOCK_MONOTONIC;
    const int relativeFlag = 0; // flag of relative sleep.
    ret = clock_nanosleep(clockId, relativeFlag, &time, nullptr);
    if (ret != 0) {
        AUDIO_WARNING_LOG("RelativeSleep may failed, ret is :%{public}d", ret);
    }

    return ret;
}

void Trace::Count(const std::string &value, int64_t count, bool isEnable)
{
#ifdef FEATURE_HITRACE_METER
    CountTraceDebug(isEnable, HITRACE_TAG_ZAUDIO, value, count);
#endif
}

Trace::Trace(const std::string &value, bool isShowLog, bool isEnable)
{
    value_ = value;
    isShowLog_ = isShowLog;
    isEnable_ = isEnable;
    isFinished_ = false;
#ifdef FEATURE_HITRACE_METER
    if (isShowLog) {
        isShowLog_ = true;
        AUDIO_INFO_LOG("%{public}s start.", value_.c_str());
    }
    StartTraceDebug(isEnable_, HITRACE_TAG_ZAUDIO, value);
#endif
}

void Trace::End()
{
#ifdef FEATURE_HITRACE_METER
    if (!isFinished_) {
        FinishTraceDebug(isEnable_, HITRACE_TAG_ZAUDIO);
        isFinished_ = true;
        if (isShowLog_) {
            AUDIO_INFO_LOG("%{public}s end.", value_.c_str());
        }
    }
#endif
}

Trace::~Trace()
{
    End();
}

AudioXCollie::AudioXCollie(const std::string &tag, uint32_t timeoutSeconds)
{
    AUDIO_DEBUG_LOG("Start AudioXCollie, tag: %{public}s, timeoutSeconds: %{public}u",
        tag.c_str(), timeoutSeconds);
    id_ = HiviewDFX::XCollie::GetInstance().SetTimer(tag, timeoutSeconds,
        nullptr, nullptr, HiviewDFX::XCOLLIE_FLAG_LOG);
    tag_ = tag;
    isCanceled_ = false;
}

AudioXCollie::~AudioXCollie()
{
    CancelXCollieTimer();
}

void AudioXCollie::CancelXCollieTimer()
{
    if (!isCanceled_) {
        HiviewDFX::XCollie::GetInstance().CancelTimer(id_);
        isCanceled_ = true;
        AUDIO_DEBUG_LOG("CancelXCollieTimer: cancel timer %{public}s", tag_.c_str());
    }
}

bool PermissionUtil::VerifyIsSystemApp()
{
    uint64_t fullTokenId = IPCSkeleton::GetCallingFullTokenID();
    if (Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(fullTokenId)) {
        return true;
    }

    AUDIO_ERR_LOG("Check system app permission reject");
    return false;
}

bool PermissionUtil::VerifySelfPermission()
{
    Security::AccessToken::FullTokenID selfToken = IPCSkeleton::GetSelfTokenID();

    auto tokenTypeFlag = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(static_cast<uint32_t>(selfToken));
    if (tokenTypeFlag == Security::AccessToken::TOKEN_NATIVE) {
        return true;
    }

    if (tokenTypeFlag == Security::AccessToken::TOKEN_SHELL) {
        return true;
    }

    if (Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(selfToken)) {
        return true;
    }

    AUDIO_ERR_LOG("Check self app permission reject");
    return false;
}

bool PermissionUtil::VerifySystemPermission()
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenTypeFlag = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (tokenTypeFlag == Security::AccessToken::TOKEN_NATIVE) {
        return true;
    }

    if (tokenTypeFlag == Security::AccessToken::TOKEN_SHELL) {
        return true;
    }

    if (VerifyIsSystemApp()) {
        return true;
    }

    AUDIO_ERR_LOG("Check system permission reject");
    return false;
}

void AdjustStereoToMonoForPCM8Bit(int8_t *data, uint64_t len)
{
    for (unsigned i = len >> 1; i > 0; i--) {
        // the number 2 is the count of stereo audio channels
        data[0] = data[0] / 2 + data[1] / 2;
        data[1] = data[0];
        data += 2;
    }
}

void AdjustStereoToMonoForPCM16Bit(int16_t *data, uint64_t len)
{
    for (unsigned i = len >> 1; i > 0; i--) {
        // the number 2 is the count of stereo audio channels
        data[0] = data[0] / 2 + data[1] / 2;
        data[1] = data[0];
        data += 2;
    }
}

void AdjustStereoToMonoForPCM24Bit(int8_t *data, uint64_t len)
{
    // int8_t is used for reading data of PCM24BIT here
    // 24 / 8 = 3, so we need repeat the calculation three times in each loop
    for (unsigned i = len >> 1; i > 0; i--) {
        // the number 2 is the count of stereo audio channels, 2 * 3 = 6
        data[0] = data[0] / 2 + data[3] / 2;
        data[3] = data[0];
        data[1] = data[1] / 2 + data[4] / 2;
        data[4] = data[1];
        data[2] = data[2] / 2 + data[5] / 2;
        data[5] = data[2];
        data += 6;
    }
}

void AdjustStereoToMonoForPCM32Bit(int32_t *data, uint64_t len)
{
    for (unsigned i = len >> 1; i > 0; i--) {
        // the number 2 is the count of stereo audio channels
        data[0] = data[0] / 2 + data[1] / 2;
        data[1] = data[0];
        data += 2;
    }
}

void AdjustAudioBalanceForPCM8Bit(int8_t *data, uint64_t len, float left, float right)
{
    for (unsigned i = len >> 1; i > 0; i--) {
        // the number 2 is the count of stereo audio channels
        data[0] *= left;
        data[1] *= right;
        data += 2;
    }
}

void AdjustAudioBalanceForPCM16Bit(int16_t *data, uint64_t len, float left, float right)
{
    for (unsigned i = len >> 1; i > 0; i--) {
        // the number 2 is the count of stereo audio channels
        data[0] *= left;
        data[1] *= right;
        data += 2;
    }
}

void AdjustAudioBalanceForPCM24Bit(int8_t *data, uint64_t len, float left, float right)
{
    // int8_t is used for reading data of PCM24BIT here
    // 24 / 8 = 3, so we need repeat the calculation three times in each loop
    for (unsigned i = len >> 1; i > 0; i--) {
        // the number 2 is the count of stereo audio channels, 2 * 3 = 6
        data[0] *= left;
        data[1] *= left;
        data[2] *= left;
        data[3] *= right;
        data[4] *= right;
        data[5] *= right;
        data += 6;
    }
}

void AdjustAudioBalanceForPCM32Bit(int32_t *data, uint64_t len, float left, float right)
{
    for (unsigned i = len >> 1; i > 0; i--) {
        // the number 2 is the count of stereo audio channels
        data[0] *= left;
        data[1] *= right;
        data += 2;
    }
}

template <typename T>
bool GetSysPara(const char *key, T &value)
{
    if (key == nullptr) {
        AUDIO_ERR_LOG("GetSysPara: key is nullptr");
        return false;
    }
    char paraValue[20] = {0}; // 20 for system parameter
    auto res = GetParameter(key, "-1", paraValue, sizeof(paraValue));
    if (res <= 0) {
        AUDIO_WARNING_LOG("GetSysPara fail, key:%{public}s res:%{public}d", key, res);
        return false;
    }
    AUDIO_INFO_LOG("GetSysPara: key:%{public}s value:%{public}s", key, paraValue);
    std::stringstream valueStr;
    valueStr << paraValue;
    valueStr >> value;
    return true;
}

template bool GetSysPara(const char *key, int32_t &value);
template bool GetSysPara(const char *key, uint32_t &value);
template bool GetSysPara(const char *key, int64_t &value);
template bool GetSysPara(const char *key, std::string &value);

std::map<std::string, std::string> DumpFileUtil::g_lastPara = {};

FILE *DumpFileUtil::OpenDumpFileInner(std::string para, std::string fileName, AudioDumpFileType fileType)
{
    std::string filePath;
    switch (fileType) {
        case AUDIO_APP:
            filePath = DUMP_APP_DIR + fileName;
            break;
        case AUDIO_SERVICE:
            filePath = DUMP_SERVICE_DIR + fileName;
            break;
        case AUDIO_PULSE:
            filePath = DUMP_PULSE_DIR + fileName;
            break;
        default:
            AUDIO_ERR_LOG("Invalid AudioDumpFileType");
            break;
    }
    std::string dumpPara;
    FILE *dumpFile = nullptr;
    bool res = GetSysPara(para.c_str(), dumpPara);
    if (!res || dumpPara.empty()) {
        AUDIO_INFO_LOG("%{public}s is not set, dump audio is not required", para.c_str());
        g_lastPara[para] = dumpPara;
        return dumpFile;
    }
    AUDIO_INFO_LOG("%{public}s = %{public}s", para.c_str(), dumpPara.c_str());
    if (dumpPara == "w") {
        dumpFile = fopen(filePath.c_str(), "wb+");
        if (dumpFile == nullptr) {
            AUDIO_ERR_LOG("Error opening pcm test file!");
            return dumpFile;
        }
    } else if (dumpPara == "a") {
        dumpFile = fopen(filePath.c_str(), "ab+");
        if (dumpFile == nullptr) {
            AUDIO_ERR_LOG("Error opening pcm test file!");
            return dumpFile;
        }
    }
    g_lastPara[para] = dumpPara;
    return dumpFile;
}

void DumpFileUtil::WriteDumpFile(FILE *dumpFile, void *buffer, size_t bufferSize)
{
    if (dumpFile == nullptr) {
        return;
    }
    if (buffer == nullptr) {
        AUDIO_ERR_LOG("Invalid write param");
        return;
    }
    size_t writeResult = fwrite(buffer, 1, bufferSize, dumpFile);
    if (writeResult != bufferSize) {
        AUDIO_ERR_LOG("Failed to write the file.");
        return;
    }
}

void DumpFileUtil::CloseDumpFile(FILE **dumpFile)
{
    if (*dumpFile) {
        fclose(*dumpFile);
        *dumpFile = nullptr;
    }
}

void DumpFileUtil::ChangeDumpFileState(std::string para, FILE **dumpFile, std::string filePath)
{
    if (*dumpFile == nullptr) {
        AUDIO_ERR_LOG("Invalid file para");
        return;
    }
    if (g_lastPara[para] != "w" && g_lastPara[para] != "a") {
        AUDIO_ERR_LOG("Invalid input para");
        return;
    }
    std::string dumpPara;
    bool res = GetSysPara(para.c_str(), dumpPara);
    if (!res || dumpPara.empty()) {
        AUDIO_INFO_LOG("get %{public}s fail", para.c_str());
    }
    if (g_lastPara[para] == "w" && dumpPara == "w") {
        return;
    }
    CloseDumpFile(dumpFile);
    OpenDumpFile(para, filePath, dumpFile);
}

void DumpFileUtil::OpenDumpFile(std::string para, std::string fileName, FILE **file)
{
    if (*file != nullptr) {
        DumpFileUtil::ChangeDumpFileState(para, file, fileName);
        return;
    }
    if (para == DUMP_SERVER_PARA) {
        if (fileName == DUMP_BLUETOOTH_RENDER_SINK_FILENAME || fileName == DUMP_RENDER_SINK_FILENAME ||
            fileName == DUMP_CAPTURER_SOURCE_FILENAME) {
            *file = DumpFileUtil::OpenDumpFileInner(para, fileName, AUDIO_PULSE);
            return;
        }
        *file = DumpFileUtil::OpenDumpFileInner(para, fileName, AUDIO_SERVICE);
    } else {
        *file = DumpFileUtil::OpenDumpFileInner(para, fileName, AUDIO_APP);
        if (*file == nullptr) {
            *file = DumpFileUtil::OpenDumpFileInner(para, fileName, AUDIO_SERVICE);
        }
    }
}

} // namespace AudioStandard
} // namespace OHOS
