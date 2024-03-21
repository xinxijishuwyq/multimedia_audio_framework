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
#undef LOG_TAG
#define LOG_TAG "AudioUtils"

#include <cinttypes>
#include <ctime>
#include <sstream>
#include <ostream>
#include "audio_utils.h"
#include "audio_utils_c.h"
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
    CHECK_AND_RETURN_RET_LOG(ret >= 0, result,
        "GetCurNanoTime fail, result:%{public}d", ret);
    result = (time.tv_sec * AUDIO_NS_PER_SECOND) + time.tv_nsec;
    return result;
}

int32_t ClockTime::AbsoluteSleep(int64_t nanoTime)
{
    int32_t ret = -1; // -1 for bad result.
    CHECK_AND_RETURN_RET_LOG(nanoTime > 0, ret,
        "AbsoluteSleep invalid sleep time :%{public}" PRId64 " ns", nanoTime);
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
    CHECK_AND_RETURN_RET_LOG(nanoTime > 0, ret,
        "AbsoluteSleep invalid sleep time :%{public}" PRId64 " ns", nanoTime);
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

AudioXCollie::AudioXCollie(const std::string &tag, uint32_t timeoutSeconds,
    std::function<void(void *)> func, void *arg, uint32_t flag)
{
    AUDIO_DEBUG_LOG("Start AudioXCollie, tag: %{public}s, timeoutSeconds: %{public}u, flag: %{public}u",
        tag.c_str(), timeoutSeconds, flag);
    id_ = HiviewDFX::XCollie::GetInstance().SetTimer(tag, timeoutSeconds, func, arg, flag);
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
    bool tmp = Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(fullTokenId);
    CHECK_AND_RETURN_RET(!tmp, true);

    AUDIO_ERR_LOG("Check system app permission reject");
    return false;
}

bool PermissionUtil::VerifySelfPermission()
{
    Security::AccessToken::FullTokenID selfToken = IPCSkeleton::GetSelfTokenID();

    auto tokenTypeFlag = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(static_cast<uint32_t>(selfToken));

    CHECK_AND_RETURN_RET(tokenTypeFlag != Security::AccessToken::TOKEN_NATIVE, true);

    CHECK_AND_RETURN_RET(tokenTypeFlag != Security::AccessToken::TOKEN_SHELL, true);

    bool tmp = Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(selfToken);
    CHECK_AND_RETURN_RET(!tmp, true);

    AUDIO_ERR_LOG("Check self app permission reject");
    return false;
}

bool PermissionUtil::VerifySystemPermission()
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenTypeFlag = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);

    CHECK_AND_RETURN_RET(tokenTypeFlag != Security::AccessToken::TOKEN_NATIVE, true);

    CHECK_AND_RETURN_RET(tokenTypeFlag != Security::AccessToken::TOKEN_SHELL, true);

    bool tmp = VerifyIsSystemApp();
    CHECK_AND_RETURN_RET(!tmp, true);

    AUDIO_ERR_LOG("Check system permission reject");
    return false;
}

void AdjustStereoToMonoForPCM8Bit(int8_t *data, uint64_t len)
{
    // the number 2: stereo audio has 2 channels
    uint64_t count = len / 2;

    while (count > 0) {
        // the number 2 is the count of stereo audio channels
        data[0] = data[0] / 2 + data[1] / 2;
        data[1] = data[0];
        data += 2;
        count--;
    }
}

void AdjustStereoToMonoForPCM16Bit(int16_t *data, uint64_t len)
{
    uint64_t count = len / 2 / 2;
    // first number 2: stereo audio has 2 channels
    // second number 2: the bit depth of PCM16Bit is 16 bits (2 bytes)

    while (count > 0) {
        // the number 2 is the count of stereo audio channels
        data[0] = data[0] / 2 + data[1] / 2;
        data[1] = data[0];
        data += 2;
        count--;
    }
}

void AdjustStereoToMonoForPCM24Bit(int8_t *data, uint64_t len)
{
    uint64_t count = len / 2 / 3;
    // first number 2: stereo audio has 2 channels
    // second number 3: the bit depth of PCM24Bit is 24 bits (3 bytes)

    // int8_t is used for reading data of PCM24BIT here
    // 24 / 8 = 3, so we need repeat the calculation three times in each loop
    while (count > 0) {
        // the number 2 is the count of stereo audio channels, 2 * 3 = 6
        data[0] = data[0] / 2 + data[3] / 2;
        data[3] = data[0];
        data[1] = data[1] / 2 + data[4] / 2;
        data[4] = data[1];
        data[2] = data[2] / 2 + data[5] / 2;
        data[5] = data[2];
        data += 6;
        count--;
    }
}

void AdjustStereoToMonoForPCM32Bit(int32_t *data, uint64_t len)
{
    uint64_t count = len / 2 / 4;
    // first number 2: stereo audio has 2 channels
    // second number 4: the bit depth of PCM32Bit is 32 bits (4 bytes)

    while (count > 0) {
        // the number 2 is the count of stereo audio channels
        data[0] = data[0] / 2 + data[1] / 2;
        data[1] = data[0];
        data += 2;
        count--;
    }
}

void AdjustAudioBalanceForPCM8Bit(int8_t *data, uint64_t len, float left, float right)
{
    uint64_t count = len / 2;
    // the number 2: stereo audio has 2 channels

    while (count > 0) {
        // the number 2 is the count of stereo audio channels
        data[0] *= left;
        data[1] *= right;
        data += 2;
        count--;
    }
}

void AdjustAudioBalanceForPCM16Bit(int16_t *data, uint64_t len, float left, float right)
{
    uint64_t count = len / 2 / 2;
    // first number 2: stereo audio has 2 channels
    // second number 2: the bit depth of PCM16Bit is 16 bits (2 bytes)

    while (count > 0) {
        // the number 2 is the count of stereo audio channels
        data[0] *= left;
        data[1] *= right;
        data += 2;
        count--;
    }
}

void AdjustAudioBalanceForPCM24Bit(int8_t *data, uint64_t len, float left, float right)
{
    uint64_t count = len / 2 / 3;
    // first number 2: stereo audio has 2 channels
    // second number 3: the bit depth of PCM24Bit is 24 bits (3 bytes)

    // int8_t is used for reading data of PCM24BIT here
    // 24 / 8 = 3, so we need repeat the calculation three times in each loop
    while (count > 0) {
        // the number 2 is the count of stereo audio channels, 2 * 3 = 6
        data[0] *= left;
        data[1] *= left;
        data[2] *= left;
        data[3] *= right;
        data[4] *= right;
        data[5] *= right;
        data += 6;
        count--;
    }
}

void AdjustAudioBalanceForPCM32Bit(int32_t *data, uint64_t len, float left, float right)
{
    uint64_t count = len / 2 / 4;
    // first number 2: stereo audio has 2 channels
    // second number 4: the bit depth of PCM32Bit is 32 bits (4 bytes)

    while (count > 0) {
        // the number 2 is the count of stereo audio channels
        data[0] *= left;
        data[1] *= right;
        data += 2;
        count--;
    }
}

uint32_t Read24Bit(const uint8_t *p)
{
    return ((uint32_t) p[BIT_DEPTH_TWO] << BIT_16) | ((uint32_t) p[1] << BIT_8) | ((uint32_t) p[0]);
}

void Write24Bit(uint8_t *p, uint32_t u)
{
    p[BIT_DEPTH_TWO] = (uint8_t) (u >> BIT_16);
    p[1] = static_cast<uint8_t>(u >> BIT_8);
    p[0] = static_cast<uint8_t>(u);
}

void ConvertFrom24BitToFloat(unsigned n, const uint8_t *a, float *b)
{
    for (; n > 0; n--) {
        int32_t s = Read24Bit(a) << BIT_8;
        *b = s * (1.0f / (1U << (BIT_32 - 1)));
        a += OFFSET_BIT_24;
        b++;
    }
}

void ConvertFrom32BitToFloat(unsigned n, const int32_t *a, float *b)
{
    for (; n > 0; n--) {
        *(b++) = *(a++) * (1.0f / (1U << (BIT_32 - 1)));
    }
}

float CapMax(float v)
{
    float value = v;
    if (v > 1.0f) {
        value = 1.0f - FLOAT_EPS;
    } else if (v < -1.0f) {
        value = -1.0f + FLOAT_EPS;
    }
    return value;
}

void ConvertFromFloatTo24Bit(unsigned n, const float *a, uint8_t *b)
{
    for (; n > 0; n--) {
        float tmp = *a++;
        float v = CapMax(tmp) * (1U << (BIT_32 - 1));
        Write24Bit(b, (static_cast<int32_t>(v)) >> BIT_8);
        b += OFFSET_BIT_24;
    }
}

void ConvertFromFloatTo32Bit(unsigned n, const float *a, int32_t *b)
{
    for (; n > 0; n--) {
        float tmp = *a++;
        float v = CapMax(tmp) * (1U << (BIT_32 - 1));
        *(b++) = static_cast<int32_t>(v);
    }
}

template <typename T>
bool GetSysPara(const char *key, T &value)
{
    CHECK_AND_RETURN_RET_LOG(key != nullptr, false, "key is nullptr");
    char paraValue[20] = {0}; // 20 for system parameter
    auto res = GetParameter(key, "-1", paraValue, sizeof(paraValue));

    CHECK_AND_RETURN_RET_LOG(res > 0, false, "GetSysPara fail, key:%{public}s res:%{public}d", key, res);
    AUDIO_DEBUG_LOG("key:%{public}s value:%{public}s", key, paraValue);
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
    AUDIO_DEBUG_LOG("%{public}s = %{public}s", para.c_str(), dumpPara.c_str());
    if (dumpPara == "w") {
        dumpFile = fopen(filePath.c_str(), "wb+");
        CHECK_AND_RETURN_RET_LOG(dumpFile != nullptr, dumpFile,
            "Error opening pcm test file!");
    } else if (dumpPara == "a") {
        dumpFile = fopen(filePath.c_str(), "ab+");
        CHECK_AND_RETURN_RET_LOG(dumpFile != nullptr, dumpFile,
            "Error opening pcm test file!");
    }
    g_lastPara[para] = dumpPara;
    return dumpFile;
}

void DumpFileUtil::WriteDumpFile(FILE *dumpFile, void *buffer, size_t bufferSize)
{
    if (dumpFile == nullptr) {
        return;
    }
    CHECK_AND_RETURN_LOG(buffer != nullptr, "Invalid write param");
    size_t writeResult = fwrite(buffer, 1, bufferSize, dumpFile);
    CHECK_AND_RETURN_LOG(writeResult == bufferSize, "Failed to write the file.");
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
    CHECK_AND_RETURN_LOG(*dumpFile != nullptr, "Invalid file para");
    CHECK_AND_RETURN_LOG(g_lastPara[para] == "w" || g_lastPara[para] == "a", "Invalid input para");
    std::string dumpPara;
    bool res = GetSysPara(para.c_str(), dumpPara);
    if (!res || dumpPara.empty()) {
        AUDIO_WARNING_LOG("get %{public}s fail", para.c_str());
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
            fileName == DUMP_CAPTURER_SOURCE_FILENAME || fileName == DUMP_OFFLOAD_RENDER_SINK_FILENAME) {
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

#ifdef __cplusplus
extern "C" {
#endif

struct CTrace {
    explicit CTrace(const char *traceName) : trace(OHOS::AudioStandard::Trace(traceName)) {};
    OHOS::AudioStandard::Trace trace;
};

CTrace *GetAndStart(const char *traceName)
{
    std::unique_ptr<CTrace> cTrace = std::make_unique<CTrace>(traceName);

    return cTrace.release();
}

void EndCTrace(CTrace *cTrace)
{
    if (cTrace != nullptr) {
        cTrace->trace.End();
    }
}

void CTraceCount(const char *traceName, int64_t count)
{
    OHOS::AudioStandard::Trace::Count(traceName, count);
}

void CallEndAndClear(CTrace **cTrace)
{
    if (cTrace != nullptr && *cTrace != nullptr) {
        EndCTrace(*cTrace);
        delete *cTrace;
        *cTrace = nullptr;
    }
}

#ifdef __cplusplus
}
#endif
