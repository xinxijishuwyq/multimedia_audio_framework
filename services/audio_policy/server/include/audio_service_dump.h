/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef AUDIO_SERVICE_DUMP_H
#define AUDIO_SERVICE_DUMP_H

#include <vector>
#include <list>
#include <queue>
#include <map>
#include <pwd.h>
#include "securec.h"
#include "audio_adapter_info.h"
#include "nocopyable.h"

#include <pulse/pulseaudio.h>

#include "audio_timer.h"
#include "audio_errors.h"
#include "audio_info.h"
#include "audio_effect_manager.h"
#include "audio_volume_config.h"
#include "microphone_descriptor.h"

namespace OHOS {
namespace AudioStandard {

static const int32_t AUDIO_DUMP_SUCCESS = 0;
static const int32_t AUDIO_DUMP_INIT_ERR = -1;

typedef struct {
    DeviceType deviceType;
    DeviceRole deviceRole;
    ConnectType conneceType;
} DevicesInfo;

typedef struct {
    std::string name;
    pa_sample_spec sampleSpec;
} SinkSourceInfo;

typedef struct {
    uint32_t userId;
    uint32_t corked;                   // status
    std::string sessionId;
    std::string sessionStartTime;
    std::string applicationName;
    std::string processId;
    pa_sample_spec sampleSpec;
}InputOutputInfo;

typedef struct {
    std::vector<SinkSourceInfo> sinkDevices;
    std::vector<SinkSourceInfo> sourceDevices;
    std::vector<InputOutputInfo> sinkInputs;
    std::vector<InputOutputInfo> sourceOutputs;
} StreamData;

typedef struct {
    ConnectType type;
    std::string groupName;
    std::int32_t groupId;
} GroupInfo;

typedef struct {
    int32_t zoneId; // Zone ID value should 0 on local device.
    std::set<int32_t> pids; // When Zone ID is 0, there does not need to be a value.
    std::set<uint32_t> interruptCbSessionIdsMap;
    std::set<int32_t> audioPolicyClientProxyCBClientPidMap;
    std::list<std::pair<AudioInterrupt, AudioFocuState>> audioFocusInfoList;
} AudioInterruptZoneDump;

typedef struct {
    std::vector<DevicesInfo> inputDevices;
    std::vector<DevicesInfo> outputDevices;
    DeviceType priorityOutputDevice;
    DeviceType priorityInputDevice;
    std::map<AudioStreamType, int32_t> streamVolumes;
    AudioRingerMode ringerMode;
    AudioScene callStatus;
    AudioInterrupt audioFocusInfo;
    std::list<std::pair<AudioInterrupt, AudioFocuState>> audioFocusInfoList;
    std::vector<GroupInfo> groupInfos;
    OriginalEffectConfig oriEffectConfig;
    std::vector<Effect> availableEffects;
    StreamVolumeInfoMap streamVolumeInfos;
    std::vector<sptr<MicrophoneDescriptor>> availableMicrophones;
    std::unordered_map<int32_t, std::shared_ptr<AudioInterruptZoneDump>> audioInterruptZonesMapDump;
    std::unordered_map<AdaptersType, AudioAdapterInfo> adapterInfoMap;
    std::unordered_map<std::string, std::string> volumeGroupData;
    std::unordered_map<std::string, std::string> interruptGroupData;
    std::unordered_map<ClassType, std::list<AudioModuleInfo>> deviceClassInfo;
    GlobalConfigs globalConfigs;
} PolicyData;

typedef struct {
    StreamData streamData;
    PolicyData policyData;
} AudioData;

class AudioServiceDump : public AudioTimer {
public:
    DISALLOW_COPY_AND_MOVE(AudioServiceDump);

    AudioServiceDump();
    ~AudioServiceDump();
    int32_t Initialize();
    void AudioDataDump(PolicyData &policyData, std::string &dumpString,
        std::queue<std::u16string>& argQue);
    static bool IsStreamSupported(AudioStreamType streamType);
    virtual void OnTimeOut();

private:
    pa_threaded_mainloop *mainLoop;
    pa_mainloop_api *api;
    pa_context *context;
    std::mutex ctrlMutex_;

    bool isMainLoopStarted_;
    bool isContextConnected_;
    AudioData audioData_ = {};

    int32_t ConnectStreamToPA();
    void ResetPAAudioDump();

    void PlaybackStreamDump(std::string &dumpString);
    void RecordStreamDump(std::string &dumpString);
    void HDFModulesDump(std::string &dumpString);
    void CallStatusDump(std::string &dumpString);
    void RingerModeDump(std::string &dumpString);
    void StreamVolumesDump(std::string &dumpString);
    void DevicesInfoDump(std::string &dumpString);
    void AudioFocusInfoDump(std::string &dumpString);
    void AudioInterruptZoneDump(std::string &dumpString);
    void AudioPolicyParserDump(std::string &dumpString);
    void DumpXmlParsedDataMap(std::string &dumpString);
    void GroupInfoDump(std::string& dumpString);
    void EffectManagerInfoDump(std::string& dumpString);
    void DataDump(std::string &dumpString);
    void ArgDataDump(std::string &dumpString, std::queue<std::u16string>& argQue);
    void StreamVolumeInfosDump(std::string& dumpString);
    void DeviceVolumeInfosDump(std::string& dumpString, DeviceVolumeInfoMap &deviceVolumeInfos);
    void HelpInfoDump(std::string& dumpString);
    void MicrophoneDescriptorsDump(std::string& dumpString);
    static const std::string GetStreamName(AudioStreamType streamType);
    static const std::string GetSourceName(SourceType sourceType);
    static const std::string GetStreamUsgaeName(StreamUsage streamUsage);
    static const std::string GetContentTypeName(ContentType contentType);
    static const std::string GetDeviceTypeName(DeviceType deviceType);
    static const std::string GetConnectTypeName(ConnectType connectType);
    static const std::string GetDeviceVolumeTypeName(DeviceVolumeType deviceType);
    static bool IsEndWith(const std::string &mainStr, const std::string &toMatch);
    static bool IsValidModule (const std::string moduleName);

    // Callbacks
    static void PAContextStateCb(pa_context *context, void *userdata);
    static void PASinkInfoCallback(pa_context *c, const pa_sink_info *i, int eol, void *userdata);
    static void PASinkInputInfoCallback(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata);
    static void PASourceInfoCallback(pa_context *c, const pa_source_info *i, int eol, void *userdata);
    static void PASourceOutputInfoCallback(pa_context *c, const pa_source_output_info *i, int eol, void *userdata);

    using DumpFunc = void(AudioServiceDump::*)(std::string &dumpString);
    std::map<std::u16string, DumpFunc> dumpFuncMap;
    void InitDumpFuncMap();
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_SERVICE_DUMP_H
