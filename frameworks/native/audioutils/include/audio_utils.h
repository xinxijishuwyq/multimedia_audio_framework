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
#ifndef AUDIO_UTILS_H
#define AUDIO_UTILS_H

#include <cstdint>
#include <string>
#include <map>
#include <mutex>

#define AUDIO_MS_PER_SECOND 1000
#define AUDIO_US_PER_SECOND 1000000
#define AUDIO_NS_PER_SECOND ((int64_t)1000000000)
namespace OHOS {
namespace AudioStandard {
class Trace {
public:
    static void Count(const std::string &value, int64_t count, bool isEnable = true);
    Trace(const std::string &value, bool isShowLog = false, bool isEnable = true);
    void End();
    ~Trace();
private:
    std::string value_;
    bool isShowLog_;
    bool isEnable_;
    bool isFinished_;
};

class AudioXCollie {
public:
    AudioXCollie(const std::string &tag, uint32_t timeoutSeconds);
    ~AudioXCollie();
    void CancelXCollieTimer();
private:
    int32_t id_;
    std::string tag_;
    bool isCanceled_;
};

class ClockTime {
public:
    static int64_t GetCurNano();
    static int32_t AbsoluteSleep(int64_t nanoTime);
    static int32_t RelativeSleep(int64_t nanoTime);
};

class PermissionUtil {
public:
    static bool VerifyIsSystemApp();
    static bool VerifySelfPermission();
    static bool VerifySystemPermission();
};

void AdjustStereoToMonoForPCM8Bit(int8_t *data, uint64_t len);
void AdjustStereoToMonoForPCM16Bit(int16_t *data, uint64_t len);
void AdjustStereoToMonoForPCM24Bit(int8_t *data, uint64_t len);
void AdjustStereoToMonoForPCM32Bit(int32_t *data, uint64_t len);
void AdjustAudioBalanceForPCM8Bit(int8_t *data, uint64_t len, float left, float right);
void AdjustAudioBalanceForPCM16Bit(int16_t *data, uint64_t len, float left, float right);
void AdjustAudioBalanceForPCM24Bit(int8_t *data, uint64_t len, float left, float right);
void AdjustAudioBalanceForPCM32Bit(int32_t *data, uint64_t len, float left, float right);

template <typename T>
bool GetSysPara(const char *key, T &value);

enum AudioDumpFileType {
    AUDIO_APP = 0,
    AUDIO_SERVICE = 1,
    AUDIO_PULSE = 2,
};

const std::string DUMP_SERVER_PARA = "sys.audio.dump.writehdi.enable";
const std::string DUMP_CLIENT_PARA = "sys.audio.dump.writeclient.enable";
const std::string DUMP_PULSE_DIR = "/data/data/.pulse_dir/";
const std::string DUMP_SERVICE_DIR = "/data/local/tmp/";
const std::string DUMP_APP_DIR = "/data/storage/el2/base/temp/";
const std::string DUMP_AUDIO_RENDERER_FILENAME = "dump_client_audio.pcm";
const std::string DUMP_BLUETOOTH_RENDER_SINK_FILENAME = "dump_bluetooth_audiosink.pcm";
const std::string DUMP_RENDER_SINK_FILENAME = "dump_audiosink.pcm";
const std::string DUMP_CAPTURER_SOURCE_FILENAME = "dump_capture_audiosource.pcm";
const std::string DUMP_TONEPLAYER_FILENAME = "dump_toneplayer_audio.pcm";
const std::string DUMP_PROCESS_IN_CLIENT_FILENAME = "dump_process_client_audio.pcm";
const std::string DUMP_REMOTE_RENDER_SINK_FILENAME = "dump_remote_audiosink.pcm";
const std::string DUMP_REMOTE_CAPTURE_SOURCE_FILENAME = "dump_remote_capture_audiosource.pcm";
const std::string DUMP_ENDPOINT_DCP_FILENAME = "dump_endpoint_dcp_audio.pcm";
const std::string DUMP_ENDPOINT_HDI_FILENAME = "dump_endpoint_hdi_audio.pcm";
constexpr uint32_t PARAM_VALUE_LENTH = 128;

class DumpFileUtil {
public:
    static void WriteDumpFile(FILE *dumpFile, void *buffer, size_t bufferSize);
    static void CloseDumpFile(FILE **dumpFile);
    static std::map<std::string, std::string> g_lastPara;
    static void OpenDumpFile(std::string para, std::string fileName, FILE **file);
private:
    static FILE *OpenDumpFileInner(std::string para, std::string fileName, AudioDumpFileType fileType);
    static void ChangeDumpFileState(std::string para, FILE **dumpFile, std::string fileName);
};

template<typename T>
class ObjectRefMap {
public:
    static std::mutex allObjLock;
    static std::map<T*, uint32_t> refMap;
    static void Insert(T *obj);
    static void Erase(T *obj);
    static T *IncreaseRef(T *obj);
    static void DecreaseRef(T *obj);

    ObjectRefMap(T *obj);
    ~ObjectRefMap();
    T *GetPtr();

private:
    T *obj_ = nullptr;
};

template <typename T>
std::mutex ObjectRefMap<T>::allObjLock;

template <typename T>
std::map<T *, uint32_t> ObjectRefMap<T>::refMap;

template <typename T>
void ObjectRefMap<T>::Insert(T *obj)
{
    std::lock_guard<std::mutex> lock(allObjLock);
    refMap[obj] = 1;
}

template <typename T>
void ObjectRefMap<T>::Erase(T *obj)
{
    std::lock_guard<std::mutex> lock(allObjLock);
    auto it = refMap.find(obj);
    if (it != refMap.end()) {
        refMap.erase(it);
    }
}

template <typename T>
T *ObjectRefMap<T>::IncreaseRef(T *obj)
{
    std::lock_guard<std::mutex> lock(allObjLock);
    if (refMap.count(obj)) {
        refMap[obj]++;
        return obj;
    } else {
        return nullptr;
    }
}

template <typename T>
void ObjectRefMap<T>::DecreaseRef(T *obj)
{
    std::lock_guard<std::mutex> lock(allObjLock);
    if (refMap.count(obj) && --refMap[obj] == 0) {
        delete obj;
        refMap.erase(obj);
    }
}

template <typename T>
ObjectRefMap<T>::ObjectRefMap(T *obj)
{
    if (obj != nullptr) {
        obj_ = ObjectRefMap::IncreaseRef(obj);
    }
}

template <typename T>
ObjectRefMap<T>::~ObjectRefMap()
{
    if (obj_ != nullptr) {
        ObjectRefMap::DecreaseRef(obj_);
    }
}

template <typename T>
T *ObjectRefMap<T>::GetPtr()
{
    return obj_;
}
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_UTILS_H
