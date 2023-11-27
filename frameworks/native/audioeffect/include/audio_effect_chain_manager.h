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


#ifndef AUDIO_EFFECT_CHAIN_MANAGER_H
#define AUDIO_EFFECT_CHAIN_MANAGER_H

#include <cstdio>
#include <cstdint>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <set>

#include "v1_0/ieffect_model.h"
#include "audio_effect_chain_adapter.h"
#include "audio_effect.h"
#include "sensor_agent.h"

namespace OHOS {
namespace AudioStandard {

const uint32_t NUM_SET_EFFECT_PARAM = 3;
const uint32_t DEFAULT_FRAMELEN = 1440;
const uint32_t DEFAULT_SAMPLE_RATE = 48000;
const uint32_t DEFAULT_NUM_CHANNEL = STEREO;
const uint64_t DEFAULT_NUM_CHANNELLAYOUT = CH_LAYOUT_STEREO;
const uint32_t FACTOR_TWO = 2;
const uint32_t BASE_TEN = 10;
const std::string DEFAULT_DEVICE_SINK = "Speaker";
const uint32_t SIZE_OF_SPATIALIZATION_STATE = 2;
const uint32_t NONE_SPATIALIZER_ENGINE = 0;
const uint32_t ARM_SPATIALIZER_ENGINE = 1;
const uint32_t DSP_SPATIALIZER_ENGINE = 2;
const uint32_t SEND_HDI_COMMAND_LEN = 20;
const uint32_t GET_HDI_BUFFER_LEN = 10;
const uint32_t HDI_ROOM_MODE_INDEX_TWO = 2;

const std::vector<AudioChannelLayout> HVS_SUPPORTED_CHANNELLAYOUTS {
    CH_LAYOUT_STEREO,
    CH_LAYOUT_5POINT1_BACK,
    CH_LAYOUT_5POINT1POINT2,
    CH_LAYOUT_7POINT1,
    CH_LAYOUT_5POINT1POINT4,
    CH_LAYOUT_7POINT1POINT2,
    CH_LAYOUT_7POINT1POINT4,
    CH_LAYOUT_9POINT1POINT4,
    CH_LAYOUT_9POINT1POINT6
};

class HeadTracker {
public:
    HeadTracker();
    ~HeadTracker();
    int32_t SensorInit();
    int32_t SensorSetConfig(int32_t spatializerEngineState);
    int32_t SensorActive();
    int32_t SensorDeactive();
    HeadPostureData GetHeadPostureData();
    void SetHeadPostureData(HeadPostureData headPostureData);
private:
    static void HeadPostureDataProcCb(SensorEvent *event);
    static HeadPostureData headPostureData_;
    SensorUser sensorUser_;
    int64_t sensorSamplingInterval_ = 30000000; // 30000000 ns = 30 ms
    static std::mutex headTrackerMutex_;
};

class AudioEffectHdi {
public:
    AudioEffectHdi();
    ~AudioEffectHdi();
    void InitHdi();
    int32_t UpdateHdiState(int8_t *effectHdiInput);
private:
    std::string libName;
    std::string effectId;
    int8_t input[SEND_HDI_COMMAND_LEN];
    int8_t output[GET_HDI_BUFFER_LEN];
    uint32_t replyLen;
    IEffectModel *hdiModel_ = nullptr;
    IEffectControl *hdiControl_ = nullptr;
};

class AudioEffectChain {
public:
    AudioEffectChain(std::string scene, std::shared_ptr<HeadTracker> headTracker);
    ~AudioEffectChain();
    std::string GetEffectMode();
    void SetEffectMode(std::string mode);
    void ReleaseEffectChain();
    void AddEffectHandleBegin();
    void AddEffectHandleEnd();
    void AddEffectHandle(AudioEffectHandle effectHandle, AudioEffectLibrary *libHandle);
    void SetEffectChain(std::vector<AudioEffectHandle> &effHandles, std::vector<AudioEffectLibrary *> &libHandles);
    void ApplyEffectChain(float *bufIn, float *bufOut, uint32_t frameLen);
    void SetIOBufferConfig(bool isInput, uint32_t samplingRate, uint32_t channels);
    bool IsEmptyEffectHandles();
    void Dump();
    void UpdateMultichannelIoBufferConfig(const uint32_t &channels, const uint64_t &channelLayout,
        const std::string &deviceName);
    void StoreOldEffectChainInfo(std::string &sceneMode, AudioEffectConfig &ioBufferConfig);
    AudioEffectConfig GetIoBufferConfig();
    void InitEffectChain();
    void SetHeadTrackingDisabled();
private:
    std::mutex reloadMutex;
    std::string sceneType;
    std::string effectMode;
    std::vector<AudioEffectHandle> standByEffectHandles;
    std::vector<AudioEffectLibrary*> libHandles;
    AudioEffectConfig ioBufferConfig;
    AudioBuffer audioBufIn;
    AudioBuffer audioBufOut;
    std::shared_ptr<HeadTracker> headTracker_;
};

class AudioEffectChainManager {
public:
    AudioEffectChainManager();
    ~AudioEffectChainManager();
    static AudioEffectChainManager *GetInstance();
    void InitAudioEffectChainManager(std::vector<EffectChain> &effectChains,
        std::unordered_map<std::string, std::string> &map,
        std::vector<std::unique_ptr<AudioEffectLibEntry>> &effectLibraryList);
    bool CheckAndAddSessionID(std::string sessionID);
    int32_t CreateAudioEffectChainDynamic(std::string sceneType);
    int32_t SetAudioEffectChainDynamic(std::string sceneType, std::string effectMode);
    bool CheckAndRemoveSessionID(std::string sessionID);
    int32_t ReleaseAudioEffectChainDynamic(std::string sceneType);
    bool ExistAudioEffectChain(std::string sceneType, std::string effectMode, std::string spatializationEnabled);
    int32_t ApplyAudioEffectChain(std::string sceneType, BufferAttr *bufferAttr);
    int32_t SetOutputDeviceSink(int32_t device, std::string &sinkName);
    std::string GetDeviceTypeName();
    int32_t GetFrameLen();
    int32_t SetFrameLen(int32_t frameLen);
    void Dump();
    int32_t UpdateMultichannelConfig(const std::string &sceneTypeString, const uint32_t &channels,
        const uint64_t &channelLayout);
    int32_t InitAudioEffectChainDynamic(std::string sceneType);
    int32_t UpdateSpatializationState(std::vector<bool> spatializationState);
    int32_t SetHdiParam(std::string sceneType, std::string effectMode, bool enabled);
private:
    void UpdateSensorState();
    std::map<std::string, AudioEffectLibEntry*> EffectToLibraryEntryMap_;
    std::map<std::string, std::string> EffectToLibraryNameMap_;
    std::map<std::string, std::vector<std::string>> EffectChainToEffectsMap_;
    std::map<std::string, std::string> SceneTypeAndModeToEffectChainNameMap_;
    std::map<std::string, AudioEffectChain*> SceneTypeToEffectChainMap_;
    std::map<std::string, int32_t> SceneTypeToEffectChainCountMap_;
    std::set<std::string> SessionIDSet_;
    uint32_t frameLen_ = DEFAULT_FRAMELEN;
    DeviceType deviceType_ = DEVICE_TYPE_SPEAKER;
    std::string deviceSink_ = DEFAULT_DEVICE_SINK;
    bool isInitialized_ = false;
    std::mutex dynamicMutex_;
    bool spatializatonEnabled_ = true;
    bool headTrackingEnabled_ = false;
    bool offloadEnabled_ = false;
    std::shared_ptr<HeadTracker> headTracker_;
    std::shared_ptr<AudioEffectHdi> audioEffectHdi_;
    int8_t effectHdiInput[SEND_HDI_COMMAND_LEN];
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif // AUDIO_EFFECT_CHAIN_MANAGER_H
