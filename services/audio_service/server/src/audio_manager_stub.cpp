/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "AudioManagerStub"

#include "audio_manager_base.h"
#include "audio_system_manager.h"
#include "audio_log.h"
#include "i_audio_process.h"
#include "audio_effect_server.h"
#include "audio_asr.h"
#include "audio_utils.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
namespace {
constexpr int32_t AUDIO_EXTRA_PARAMETERS_COUNT_UPPER_LIMIT = 40;
const char *g_audioServerCodeStrs[] = {
    "GET_AUDIO_PARAMETER",
    "SET_AUDIO_PARAMETER",
    "GET_EXTRA_AUDIO_PARAMETERS",
    "SET_EXTRA_AUDIO_PARAMETERS",
    "SET_MICROPHONE_MUTE",
    "SET_AUDIO_SCENE",
    "UPDATE_ROUTE_REQ",
    "UPDATE_ROUTES_REQ",
    "UPDATE_DUAL_TONE_REQ",
    "GET_TRANSACTION_ID",
    "SET_PARAMETER_CALLBACK",
    "GET_REMOTE_AUDIO_PARAMETER",
    "SET_REMOTE_AUDIO_PARAMETER",
    "NOTIFY_DEVICE_INFO",
    "CHECK_REMOTE_DEVICE_STATE",
    "SET_VOICE_VOLUME",
    "SET_AUDIO_MONO_STATE",
    "SET_AUDIO_BALANCE_VALUE",
    "CREATE_AUDIOPROCESS",
    "LOAD_AUDIO_EFFECT_LIBRARIES",
    "REQUEST_THREAD_PRIORITY",
    "CREATE_AUDIO_EFFECT_CHAIN_MANAGER",
    "SET_OUTPUT_DEVICE_SINK",
    "CREATE_PLAYBACK_CAPTURER_MANAGER",
    "SET_SUPPORT_STREAM_USAGE",
    "REGISET_POLICY_PROVIDER",
    "SET_WAKEUP_CLOSE_CALLBACK",
    "SET_CAPTURE_SILENT_STATE",
    "UPDATE_SPATIALIZATION_STATE",
    "OFFLOAD_SET_VOLUME",
    "OFFLOAD_DRAIN",
    "OFFLOAD_GET_PRESENTATION_POSITION",
    "OFFLOAD_SET_BUFFER_SIZE",
    "NOTIFY_STREAM_VOLUME_CHANGED",
    "GET_CAPTURE_PRESENTATION_POSITION",
    "GET_RENDER_PRESENTATION_POSITION",
    "SET_SPATIALIZATION_SCENE_TYPE",
    "GET_MAX_AMPLITUDE",
    "RESET_AUDIO_ENDPOINT",
    "RESET_ROUTE_FOR_DISCONNECT",
    "GET_EFFECT_LATENCY",
    "UPDATE_LATENCY_TIMESTAMP",
    "SET_ASR_AEC_MODE",
    "GET_ASR_AEC_MODE",
    "SET_ASR_NOISE_SUPPRESSION_MODE",
    "GET_ASR_NOISE_SUPPRESSION_MODE",
    "SET_ASR_WHISPER_DETECTION_MODE",
    "GET_ASR_WHISPER_DETECTION_MODE",
    "SET_ASR_VOICE_CONTROL_MODE",
    "SET_ASR_VOICE_MUTE_MODE",
    "IS_WHISPERING",
    "GET_EFFECT_OFFLOAD_ENABLED",
    "SUSPEND_RENDERSINK",
    "RESTORE_RENDERSINK",
    "LOAD_HDI_EFFECT_MODEL",
};
constexpr size_t codeNums = sizeof(g_audioServerCodeStrs) / sizeof(const char *);
static_assert(codeNums == (static_cast<size_t> (AudioServerInterfaceCode::AUDIO_SERVER_CODE_MAX) + 1),
    "keep same with AudioServerInterfaceCode");
}
static void LoadEffectLibrariesReadData(vector<Library>& libList, vector<Effect>& effectList, MessageParcel &data,
    int32_t countLib, int32_t countEff)
{
    int32_t i;
    for (i = 0; i < countLib; i++) {
        string libName = data.ReadString();
        string libPath = data.ReadString();
        libList.push_back({libName, libPath});
    }
    for (i = 0; i < countEff; i++) {
        string effectName = data.ReadString();
        string libName = data.ReadString();
        effectList.push_back({effectName, libName});
    }
}

static void LoadEffectLibrariesWriteReply(const vector<Effect>& successEffectList, MessageParcel &reply)
{
    reply.WriteInt32(successEffectList.size());
    for (Effect effect: successEffectList) {
        reply.WriteString(effect.name);
        reply.WriteString(effect.libraryName);
    }
}

int AudioManagerStub::HandleGetAudioParameter(MessageParcel &data, MessageParcel &reply)
{
    const std::string key = data.ReadString();
    const std::string value = GetAudioParameter(key);
    reply.WriteString(value);
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetAudioParameter(MessageParcel &data, MessageParcel &reply)
{
    const std::string key = data.ReadString();
    const std::string value = data.ReadString();
    SetAudioParameter(key, value);
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetAsrAecMode(MessageParcel &data, MessageParcel &reply)
{
    AsrAecMode asrAecMode = (static_cast<AsrAecMode>(data.ReadInt32()));
    int32_t result = SetAsrAecMode(asrAecMode);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleGetAsrAecMode(MessageParcel &data, MessageParcel &reply)
{
    AsrAecMode asrAecMode = (static_cast<AsrAecMode>(data.ReadInt32()));
    int32_t ret = GetAsrAecMode(asrAecMode);
    CHECK_AND_RETURN_RET_LOG(ret == 0, AUDIO_ERR, "Get AsrAec Mode audio parameters failed");
    reply.WriteInt32(int32_t(asrAecMode));
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetAsrNoiseSuppressionMode(MessageParcel &data, MessageParcel &reply)
{
    AsrNoiseSuppressionMode asrNoiseSuppressionMode = (static_cast<AsrNoiseSuppressionMode>(data.ReadInt32()));
    int32_t result = SetAsrNoiseSuppressionMode(asrNoiseSuppressionMode);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleGetAsrNoiseSuppressionMode(MessageParcel &data, MessageParcel &reply)
{
    AsrNoiseSuppressionMode asrNoiseSuppressionMode = (static_cast<AsrNoiseSuppressionMode>(data.ReadInt32()));
    int32_t ret = GetAsrNoiseSuppressionMode(asrNoiseSuppressionMode);
    CHECK_AND_RETURN_RET_LOG(ret == 0, AUDIO_ERR, "Get AsrNoiseSuppression Mode audio parameters failed");
    reply.WriteInt32(int32_t(asrNoiseSuppressionMode));
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetAsrWhisperDetectionMode(MessageParcel &data, MessageParcel &reply)
{
    AsrWhisperDetectionMode asrWhisperDetectionMode = (static_cast<AsrWhisperDetectionMode>(data.ReadInt32()));
    int32_t result = SetAsrWhisperDetectionMode(asrWhisperDetectionMode);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleGetAsrWhisperDetectionMode(MessageParcel &data, MessageParcel &reply)
{
    AsrWhisperDetectionMode asrWhisperDetectionMode = (static_cast<AsrWhisperDetectionMode>(data.ReadInt32()));
    int32_t ret = GetAsrWhisperDetectionMode(asrWhisperDetectionMode);
    CHECK_AND_RETURN_RET_LOG(ret == 0, AUDIO_ERR, "Get AsrWhisperDetection Mode audio parameters failed");
    reply.WriteInt32(int32_t(asrWhisperDetectionMode));
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetAsrVoiceControlMode(MessageParcel &data, MessageParcel &reply)
{
    AsrVoiceControlMode asrVoiceControlMode = (static_cast<AsrVoiceControlMode>(data.ReadInt32()));
    bool on = data.ReadBool();
    int32_t result = SetAsrVoiceControlMode(asrVoiceControlMode, on);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetAsrVoiceMuteMode(MessageParcel &data, MessageParcel &reply)
{
    AsrVoiceMuteMode asrVoiceMuteMode = (static_cast<AsrVoiceMuteMode>(data.ReadInt32()));
    bool on = data.ReadBool();
    int32_t result = SetAsrVoiceMuteMode(asrVoiceMuteMode, on);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleIsWhispering(MessageParcel &data, MessageParcel &reply)
{
    const std::string key = data.ReadString();
    const std::string value = data.ReadString();
    int32_t result = IsWhispering();
    return result;
}

int AudioManagerStub::HandleGetEffectOffloadEnabled(MessageParcel &data, MessageParcel &reply)
{
    bool result = GetEffectOffloadEnabled();
    return result;
}

int AudioManagerStub::HandleGetExtraAudioParameters(MessageParcel &data, MessageParcel &reply)
{
    const std::string mainKey = data.ReadString();
    int32_t num = data.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(num >= 0 && num <= AUDIO_EXTRA_PARAMETERS_COUNT_UPPER_LIMIT,
        AUDIO_ERR, "Get extra audio parameters failed");
    std::vector<std::string> subKeys = {};
    for (int32_t i = 0; i < num; i++) {
        std::string subKey = data.ReadString();
        subKeys.push_back(subKey);
    }

    std::vector<std::pair<std::string, std::string>> values;
    int32_t result = GetExtraParameters(mainKey, subKeys, values);
    reply.WriteInt32(static_cast<int32_t>(values.size()));
    for (auto it = values.begin(); it != values.end(); it++) {
        reply.WriteString(static_cast<std::string>(it->first));
        reply.WriteString(static_cast<std::string>(it->second));
    }
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetExtraAudioParameters(MessageParcel &data, MessageParcel &reply)
{
    const std::string mainKey = data.ReadString();
    std::vector<std::pair<std::string, std::string>> audioParametersSubKVPairs;
    int32_t mapSize = data.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(mapSize >= 0 && mapSize <= AUDIO_EXTRA_PARAMETERS_COUNT_UPPER_LIMIT,
        AUDIO_ERR, "Set extra audio parameters failed");
    for (int32_t i = 0; i < mapSize; i++) {
        std::string subKey = data.ReadString();
        std::string value = data.ReadString();
        audioParametersSubKVPairs.push_back(std::make_pair(subKey, value));
    }
    int32_t result = SetExtraParameters(mainKey, audioParametersSubKVPairs);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetMicrophoneMute(MessageParcel &data, MessageParcel &reply)
{
    bool isMute = data.ReadBool();
    int32_t result = SetMicrophoneMute(isMute);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetAudioScene(MessageParcel &data, MessageParcel &reply)
{
    AudioScene audioScene = (static_cast<AudioScene>(data.ReadInt32()));
    std::vector<DeviceType> activeOutputDevices;
    int32_t vecSize = data.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(vecSize > 0 && static_cast<size_t>(vecSize) <= AUDIO_CONCURRENT_ACTIVE_DEVICES_LIMIT,
        AUDIO_ERR, "HandleSetAudioScene failed");
    for (int32_t i = 0; i < vecSize; i++) {
        DeviceType deviceType = (static_cast<DeviceType>(data.ReadInt32()));
        activeOutputDevices.push_back(deviceType);
    }
    DeviceType activeInputDevice = (static_cast<DeviceType>(data.ReadInt32()));
    int32_t result = SetAudioScene(audioScene, activeOutputDevices, activeInputDevice);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleUpdateActiveDeviceRoute(MessageParcel &data, MessageParcel &reply)
{
    DeviceType type = static_cast<DeviceType>(data.ReadInt32());
    DeviceFlag flag = static_cast<DeviceFlag>(data.ReadInt32());
    int32_t ret = UpdateActiveDeviceRoute(type, flag);
    reply.WriteInt32(ret);
    return AUDIO_OK;
}

int AudioManagerStub::HandleUpdateActiveDevicesRoute(MessageParcel &data, MessageParcel &reply)
{
    std::vector<std::pair<DeviceType, DeviceFlag>> activeDevices;
    int32_t vecSize = data.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(vecSize > 0 && static_cast<size_t>(vecSize) <= AUDIO_CONCURRENT_ACTIVE_DEVICES_LIMIT,
        AUDIO_ERR, "HandleUpdateActiveDevicesRoute failed");
    for (int32_t i = 0; i < vecSize; i++) {
        DeviceType deviceType = (static_cast<DeviceType>(data.ReadInt32()));
        DeviceFlag deviceFlag = (static_cast<DeviceFlag>(data.ReadInt32()));
        activeDevices.push_back(std::make_pair(deviceType, deviceFlag));
    }
    int32_t ret = UpdateActiveDevicesRoute(activeDevices);
    reply.WriteInt32(ret);
    return AUDIO_OK;
}

int AudioManagerStub::HandleDualToneState(MessageParcel &data, MessageParcel &reply)
{
    bool enable = data.ReadBool();
    int32_t sessionId = data.ReadInt32();

    int32_t ret = UpdateDualToneState(enable, sessionId);
    reply.WriteInt32(ret);
    return AUDIO_OK;
}

int AudioManagerStub::HandleGetTransactionId(MessageParcel &data, MessageParcel &reply)
{
    DeviceType deviceType = (static_cast<DeviceType>(data.ReadInt32()));
    DeviceRole deviceRole = (static_cast<DeviceRole>(data.ReadInt32()));
    uint64_t transactionId = GetTransactionId(deviceType, deviceRole);
    reply.WriteUint64(transactionId);
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetParameterCallback(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AUDIO_ERR, "SET_PARAMETER_CALLBACK obj is null");
    int32_t result = SetParameterCallback(object);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleGetRemoteAudioParameter(MessageParcel &data, MessageParcel &reply)
{
    const std::string networkId = data.ReadString();
    AudioParamKey key = static_cast<AudioParamKey>(data.ReadInt32());
    const std::string condition = data.ReadString();
    const std::string value = GetAudioParameter(networkId, key, condition);
    reply.WriteString(value);
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetRemoteAudioParameter(MessageParcel &data, MessageParcel &reply)
{
    const std::string networkId = data.ReadString();
    AudioParamKey key = static_cast<AudioParamKey>(data.ReadInt32());
    const std::string condtion = data.ReadString();
    const std::string value = data.ReadString();
    SetAudioParameter(networkId, key, condtion, value);
    return AUDIO_OK;
}

int AudioManagerStub::HandleNotifyDeviceInfo(MessageParcel &data, MessageParcel &reply)
{
    const std::string networkId = data.ReadString();
    const bool connected = data.ReadBool();
    NotifyDeviceInfo(networkId, connected);
    return AUDIO_OK;
}

int AudioManagerStub::HandleCheckRemoteDeviceState(MessageParcel &data, MessageParcel &reply)
{
    std::string networkId = data.ReadString();
    DeviceRole deviceRole = static_cast<DeviceRole>(data.ReadInt32());
    bool isStartDevice = data.ReadBool();
    int32_t result = CheckRemoteDeviceState(networkId, deviceRole, isStartDevice);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetVoiceVolume(MessageParcel &data, MessageParcel &reply)
{
    const float volume = data.ReadFloat();
    int32_t result = SetVoiceVolume(volume);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetAudioMonoState(MessageParcel &data, MessageParcel &reply)
{
    bool audioMonoState = data.ReadBool();
    SetAudioMonoState(audioMonoState);
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetAudioBalanceValue(MessageParcel &data, MessageParcel &reply)
{
    float audioBalanceValue = data.ReadFloat();
    SetAudioBalanceValue(audioBalanceValue);
    return AUDIO_OK;
}

int AudioManagerStub::HandleCreateAudioProcess(MessageParcel &data, MessageParcel &reply)
{
    AudioProcessConfig config;
    ProcessConfig::ReadConfigFromParcel(config, data);
    sptr<IRemoteObject> process = CreateAudioProcess(config);
    CHECK_AND_RETURN_RET_LOG(process != nullptr, AUDIO_ERR,
        "CREATE_AUDIOPROCESS AudioManagerStub CreateAudioProcess failed");
    reply.WriteRemoteObject(process);
    return AUDIO_OK;
}

int AudioManagerStub::HandleLoadAudioEffectLibraries(MessageParcel &data, MessageParcel &reply)
{
    vector<Library> libList = {};
    vector<Effect> effectList = {};
    int32_t countLib = data.ReadInt32();
    int32_t countEff = data.ReadInt32();
    CHECK_AND_RETURN_RET_LOG((countLib >= 0) && (countLib <= AUDIO_EFFECT_COUNT_UPPER_LIMIT) &&
        (countEff >= 0) && (countEff <= AUDIO_EFFECT_COUNT_UPPER_LIMIT), AUDIO_ERR,
        "LOAD_AUDIO_EFFECT_LIBRARIES read data failed");
    LoadEffectLibrariesReadData(libList, effectList, data, countLib, countEff);
    if (countLib > 0) {
        // load lib and reply success list
        vector<Effect> successEffectList = {};
        bool loadSuccess = LoadAudioEffectLibraries(libList, effectList, successEffectList);
        CHECK_AND_RETURN_RET_LOG(loadSuccess, AUDIO_ERR, "Load audio effect libraries failed, please check log");
        LoadEffectLibrariesWriteReply(successEffectList, reply);
    }
    return AUDIO_OK;
}

int AudioManagerStub::HandleRequestThreadPriority(MessageParcel &data, MessageParcel &reply)
{
    uint32_t tid = data.ReadUint32();
    string bundleName = data.ReadString();
    RequestThreadPriority(tid, bundleName);
    return AUDIO_OK;
}

int AudioManagerStub::HandleCreateAudioEffectChainManager(MessageParcel &data, MessageParcel &reply)
{
    int32_t i;
    vector<EffectChain> effectChains = {};
    vector<int32_t> countEffect = {};
    int32_t countChains = data.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(countChains >= 0 && countChains <= AUDIO_EFFECT_CHAIN_COUNT_UPPER_LIMIT,
        AUDIO_ERR, "Create audio effect chain manager failed, please check log");
    for (i = 0; i < countChains; i++) {
        int32_t count = data.ReadInt32();
        CHECK_AND_RETURN_RET_LOG(count >= 0 && count <= AUDIO_EFFECT_COUNT_PER_CHAIN_UPPER_LIMIT,
            AUDIO_ERR, "Create audio effect chain manager failed, please check log");
        countEffect.emplace_back(count);
    }

    for (int32_t count: countEffect) {
        EffectChain effectChain;
        effectChain.name = data.ReadString();
        for (i = 0; i < count; i++) {
            effectChain.apply.emplace_back(data.ReadString());
        }
        effectChains.emplace_back(effectChain);
    }

    unordered_map<string, string> sceneTypeToEffectChainNameMap;
    int32_t mapSize = data.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(mapSize >= 0 && mapSize <= AUDIO_EFFECT_CHAIN_CONFIG_UPPER_LIMIT,
        AUDIO_ERR, "Create audio effect chain manager failed, please check log");
    for (i = 0; i < mapSize; i++) {
        string key = data.ReadString();
        string value = data.ReadString();
        sceneTypeToEffectChainNameMap[key] = value;
    }

    unordered_map<string, string> sceneTypeToEnhanceChainNameMap;
    mapSize = data.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(mapSize >= 0 && mapSize <= AUDIO_EFFECT_CHAIN_CONFIG_UPPER_LIMIT,
        AUDIO_ERR, "Create audio enhance chain manager failed, please check log");
    for (i = 0; i < mapSize; i++) {
        string key = data.ReadString();
        string value = data.ReadString();
        sceneTypeToEnhanceChainNameMap[key] = value;
    }

    bool createSuccess = CreateEffectChainManager(effectChains, sceneTypeToEffectChainNameMap,
        sceneTypeToEnhanceChainNameMap);
    CHECK_AND_RETURN_RET_LOG(createSuccess, AUDIO_ERR,
        "Create audio effect chain manager failed, please check log");
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetOutputDeviceSink(MessageParcel &data, MessageParcel &reply)
{
    int32_t deviceType = data.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(deviceType >= DEVICE_TYPE_NONE && deviceType <= DEVICE_TYPE_MAX, AUDIO_ERR,
        "Set output device sink failed, please check log");
    std::string sinkName = data.ReadString();
    SetOutputDeviceSink(deviceType, sinkName);
    return AUDIO_OK;
}

int AudioManagerStub::HandleCreatePlaybackCapturerManager(MessageParcel &data, MessageParcel &reply)
{
    bool ret = CreatePlaybackCapturerManager();
    reply.WriteBool(ret);
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetSupportStreamUsage(MessageParcel &data, MessageParcel &reply)
{
    vector<int32_t> usage;
    size_t cnt = static_cast<size_t>(data.ReadInt32());
    CHECK_AND_RETURN_RET_LOG(cnt <= AUDIO_SUPPORTED_STREAM_USAGES.size(), AUDIO_ERR,
        "Set support stream usage failed, please check");
    for (size_t i = 0; i < cnt; i++) {
        int32_t tmp_usage = data.ReadInt32();
        if (find(AUDIO_SUPPORTED_STREAM_USAGES.begin(), AUDIO_SUPPORTED_STREAM_USAGES.end(), tmp_usage) ==
            AUDIO_SUPPORTED_STREAM_USAGES.end()) {
            continue;
        }
        usage.emplace_back(tmp_usage);
    }
    int32_t ret = SetSupportStreamUsage(usage);
    reply.WriteInt32(ret);
    return AUDIO_OK;
}

int AudioManagerStub::HandleRegiestPolicyProvider(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AUDIO_ERR, "obj is null");
    int32_t result = RegiestPolicyProvider(object);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetWakeupSourceCallback(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AUDIO_ERR,
        "SET_WAKEUP_CLOSE_CALLBACK obj is null");
    int32_t result = SetWakeupSourceCallback(object);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleSetCaptureSilentState(MessageParcel &data, MessageParcel &reply)
{
    bool state = false;
    int32_t flag = data.ReadInt32();
    if (flag == 1) {
        state = true;
    }
    int32_t ret = SetCaptureSilentState(state);
    reply.WriteInt32(ret);
    return AUDIO_OK;
}

int AudioManagerStub::HandleUpdateSpatializationState(MessageParcel &data, MessageParcel &reply)
{
    AudioSpatializationState spatializationState;
    spatializationState.spatializationEnabled = data.ReadBool();
    spatializationState.headTrackingEnabled = data.ReadBool();
    int32_t ret = UpdateSpatializationState(spatializationState);
    reply.WriteInt32(ret);
    return AUDIO_OK;
}

int AudioManagerStub::HandleGetCapturePresentationPosition(MessageParcel &data, MessageParcel &reply)
{
    const std::string deviceClass = data.ReadString();
    uint64_t frames;
    int64_t timeSec;
    int64_t timeNanoSec;
    int32_t result = GetCapturePresentationPosition(deviceClass, frames, timeSec, timeNanoSec);
    reply.WriteInt32(result);
    reply.WriteUint64(frames);
    reply.WriteInt64(timeSec);
    reply.WriteInt64(timeNanoSec);

    return AUDIO_OK;
}

int AudioManagerStub::HandleGetRenderPresentationPosition(MessageParcel &data, MessageParcel &reply)
{
    const std::string deviceClass = data.ReadString();
    uint64_t frames;
    int64_t timeSec;
    int64_t timeNanoSec;
    int32_t result = GetRenderPresentationPosition(deviceClass, frames, timeSec, timeNanoSec);
    reply.WriteInt32(result);
    reply.WriteUint64(frames);
    reply.WriteInt64(timeSec);
    reply.WriteInt64(timeNanoSec);

    return AUDIO_OK;
}

int AudioManagerStub::HandleOffloadSetVolume(MessageParcel &data, MessageParcel &reply)
{
    const float volume = data.ReadFloat();
    int32_t result = OffloadSetVolume(volume);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleOffloadDrain(MessageParcel &data, MessageParcel &reply)
{
    int32_t result = OffloadDrain();
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleOffloadGetPresentationPosition(MessageParcel &data, MessageParcel &reply)
{
    uint64_t frames;
    int64_t timeSec;
    int64_t timeNanoSec;
    int32_t result = OffloadGetPresentationPosition(frames, timeSec, timeNanoSec);
    reply.WriteInt32(result);
    reply.WriteUint64(frames);
    reply.WriteInt64(timeSec);
    reply.WriteInt64(timeNanoSec);

    return AUDIO_OK;
}

int AudioManagerStub::HandleOffloadSetBufferSize(MessageParcel &data, MessageParcel &reply)
{
    uint32_t sizeMs = data.ReadUint32();
    int32_t result = OffloadSetBufferSize(sizeMs);
    reply.WriteInt32(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleNotifyStreamVolumeChanged(MessageParcel &data, MessageParcel &reply)
{
    AudioStreamType type = static_cast<AudioStreamType>(data.ReadInt32());
    float volume = data.ReadFloat();
    int32_t ret = NotifyStreamVolumeChanged(type, volume);
    reply.WriteInt32(ret);

    return AUDIO_OK;
}

int AudioManagerStub::HandleSetSpatializationSceneType(MessageParcel &data, MessageParcel &reply)
{
    AudioSpatializationSceneType spatializationSceneType = static_cast<AudioSpatializationSceneType>(data.ReadInt32());
    int32_t ret = SetSpatializationSceneType(spatializationSceneType);
    reply.WriteInt32(ret);
    return AUDIO_OK;
}

int AudioManagerStub::HandleResetRouteForDisconnect(MessageParcel &data, MessageParcel &reply)
{
    DeviceType deviceType = static_cast<DeviceType>(data.ReadInt32());
    int32_t ret = ResetRouteForDisconnect(deviceType);
    reply.WriteInt32(ret);
    return AUDIO_OK;
}

int AudioManagerStub::HandleGetEffectLatency(MessageParcel &data, MessageParcel &reply)
{
    string sessionId = data.ReadString();
    uint32_t ret = GetEffectLatency(sessionId);
    reply.WriteUint32(ret);
    return AUDIO_OK;
}

int AudioManagerStub::HandleGetMaxAmplitude(MessageParcel &data, MessageParcel &reply)
{
    bool isOutputDevice = data.ReadBool();
    int32_t deviceType = data.ReadInt32();
    float result = GetMaxAmplitude(isOutputDevice, deviceType);
    reply.WriteFloat(result);
    return AUDIO_OK;
}

int AudioManagerStub::HandleResetAudioEndpoint(MessageParcel &data, MessageParcel &reply)
{
    ResetAudioEndpoint();
    return AUDIO_OK;
}

int AudioManagerStub::HandleSuspendRenderSink(MessageParcel &data, MessageParcel &reply)
{
    std::string sinkName = data.ReadString();
    int32_t ret = SuspendRenderSink(sinkName);
    reply.WriteInt32(ret);
    return AUDIO_OK;
}

int AudioManagerStub::HandleRestoreRenderSink(MessageParcel &data, MessageParcel &reply)
{
    std::string sinkName = data.ReadString();
    int32_t ret = SuspendRenderSink(sinkName);
    reply.WriteInt32(ret);
    return AUDIO_OK;
}

int AudioManagerStub::HandleUpdateLatencyTimestamp(MessageParcel &data, MessageParcel &reply)
{
    std::string timestamp = data.ReadString();
    bool isRenderer = data.ReadBool();
    UpdateLatencyTimestamp(timestamp, isRenderer);
    return AUDIO_OK;
}

int AudioManagerStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    CHECK_AND_RETURN_RET_LOG(data.ReadInterfaceToken() == GetDescriptor(),
        -1, "ReadInterfaceToken failed");
    Trace trace(code >= codeNums ? "invalid audio server code!" : g_audioServerCodeStrs[code]);
    CHECK_AND_RETURN_RET(code > static_cast<uint32_t>(AudioServerInterfaceCode::AUDIO_SERVER_CODE_MAX),
        (this->*handlers[code])(data, reply));
    AUDIO_ERR_LOG("default case, need check AudioManagerStub");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int AudioManagerStub::HandleLoadHdiEffectModel(MessageParcel &data, MessageParcel &reply)
{
    LoadHdiEffectModel();
    return AUDIO_OK;
}
} // namespace AudioStandard
} // namespace OHOS
