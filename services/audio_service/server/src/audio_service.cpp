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
#undef LOG_TAG
#define LOG_TAG "AudioService"

#include "audio_service.h"

#include <thread>

#include "audio_errors.h"
#include "audio_log.h"
#include "audio_utils.h"
#include "policy_handler.h"
#include "ipc_stream_in_server.h"

namespace OHOS {
namespace AudioStandard {

static uint64_t g_id = 1;

AudioService *AudioService::GetInstance()
{
    static AudioService AudioService;

    return &AudioService;
}

AudioService::AudioService()
{
    AUDIO_INFO_LOG("AudioService()");
}

AudioService::~AudioService()
{
    AUDIO_INFO_LOG("~AudioService()");
}

int32_t AudioService::OnProcessRelease(IAudioProcessStream *process)
{
    std::lock_guard<std::mutex> processListLock(processListMutex_);
    bool isFind = false;
    int32_t ret = ERROR;
    auto paired = linkedPairedList_.begin();
    std::string endpointName;
    bool needRelease = false;
    while (paired != linkedPairedList_.end()) {
        if ((*paired).first == process) {
            ret = UnlinkProcessToEndpoint((*paired).first, (*paired).second);
            if ((*paired).second->GetStatus() == AudioEndpoint::EndpointStatus::UNLINKED) {
                needRelease = true;
                endpointName = (*paired).second->GetEndpointName();
            }
            linkedPairedList_.erase(paired);
            isFind = true;
            break;
        } else {
            paired++;
        }
    }
    if (isFind) {
        AUDIO_INFO_LOG("find and release process result %{public}d", ret);
    } else {
        AUDIO_INFO_LOG("can not find target process, maybe already released.");
    }

    if (needRelease) {
        AUDIO_INFO_LOG("find endpoint unlink, call delay release.");
        std::unique_lock<std::mutex> lock(releaseEndpointMutex_);
        releasingEndpointSet_.insert(endpointName);
        int32_t delayTime = 10000;
        std::thread releaseEndpointThread(&AudioService::DelayCallReleaseEndpoint, this, endpointName, delayTime);
        releaseEndpointThread.detach();
    }

    return SUCCESS;
}

sptr<IpcStreamInServer> AudioService::GetIpcStream(const AudioProcessConfig &config, int32_t &ret)
{
    if (innerCapturerMgr_ == nullptr) {
        innerCapturerMgr_ = PlaybackCapturerManager::GetInstance(); // As mgr is a singleton, lock is needless here.
        innerCapturerMgr_->RegisterCapturerFilterListener(this);
    }

    // in plan: GetDeviceInfoForProcess(config) and stream limit check
    // in plan: call GetProcessDeviceInfo to load inner-cap-sink
    sptr<IpcStreamInServer> ipcStreamInServer = IpcStreamInServer::Create(config, ret);

    // in plan: Put playback into list, check if EnableInnerCap is need.
    if (ipcStreamInServer != nullptr && config.audioMode == AUDIO_MODE_PLAYBACK) {
        uint32_t sessionId = 0;
        std::shared_ptr<RendererInServer> renderer = ipcStreamInServer->GetRenderer();
        if (renderer != nullptr && renderer->GetSessionId(sessionId) == SUCCESS) {
            InsertRenderer(sessionId, renderer); // for all renderers
            CheckInnerCapForRenderer(sessionId, renderer);
        }
    }

    return ipcStreamInServer;
}

void AudioService::InsertRenderer(uint32_t sessionId, std::shared_ptr<RendererInServer> renderer)
{
    std::unique_lock<std::mutex> lock(rendererMapMutex_);
    AUDIO_INFO_LOG("Insert renderer:%{public}u into map", sessionId);
    allRendererMap_[sessionId] = renderer;
}

void AudioService::RemoveRenderer(uint32_t sessionId)
{
    std::unique_lock<std::mutex> lock(rendererMapMutex_);
    AUDIO_INFO_LOG("Renderer:%{public}u will be removed.", sessionId);
    if (!allRendererMap_.count(sessionId)) {
        AUDIO_WARNING_LOG("Renderer in not in map!");
        return;
    }
    allRendererMap_.erase(sessionId);
}

void AudioService::CheckInnerCapForRenderer(uint32_t sessionId, std::shared_ptr<RendererInServer> renderer)
{
    CHECK_AND_RETURN_LOG(renderer != nullptr, "renderer is null.");

    std::unique_lock<std::mutex> lock(rendererMapMutex_);

    // inner-cap not working
    if (workingInnerCapId_ == 0) {
        return;
    }
    // in plan: check if meet with the workingConfig_
    if (ShouldBeInnerCap(renderer->processConfig_)) {
        filteredRendererMap_.push_back(renderer);
        renderer->EnableInnerCap(); // for debug
    }
}

InnerCapFilterPolicy AudioService::GetInnerCapFilterPolicy()
{
    auto usagesSize = workingConfig_.filterOptions.usages.size();
    auto pidsSize = workingConfig_.filterOptions.pids.size();
    if (usagesSize == 0 && pidsSize == 0) {
        AUDIO_ERR_LOG("error, invalid usages and pids");
        return POLICY_INVALID;
    }
    if (usagesSize > 0 && pidsSize == 0) {
        AUDIO_INFO_LOG("usages only");
        return POLICY_USAGES_ONLY;
    }
    return POLICY_USAGES_AND_PIDS;
}

template<typename T>
bool isFilterMatched(const std::vector<T> &params, T param, FilterMode mode)
{
    bool isFound = std::count(params.begin(), params.end(), param) != 0;
    return (mode == FilterMode::INCLUDE && isFound) || (mode == FilterMode::EXCLUDE && !isFound);
}

bool AudioService::ShouldBeInnerCap(const AudioProcessConfig &rendererConfig)
{
    bool canBeCaptured = rendererConfig.privacyType == AudioPrivacyType::PRIVACY_TYPE_PUBLIC;
    if (!canBeCaptured) {
        AUDIO_WARNING_LOG("%{public}d privacy is not public!", rendererConfig.appInfo.appPid);
        return false;
    }
    InnerCapFilterPolicy filterPolicy = GetInnerCapFilterPolicy();
    bool res = false;
    switch (filterPolicy) {
        case POLICY_INVALID:
            return false;
        case POLICY_USAGES_ONLY:
            res = isFilterMatched(workingConfig_.filterOptions.usages,
                rendererConfig.rendererInfo.streamUsage, workingConfig_.filterOptions.usageFilterMode);
            break;
        case POLICY_USAGES_AND_PIDS:
            res = isFilterMatched(workingConfig_.filterOptions.usages, rendererConfig.rendererInfo.streamUsage,
                workingConfig_.filterOptions.usageFilterMode) &&
                isFilterMatched(workingConfig_.filterOptions.pids, rendererConfig.appInfo.appPid,
                workingConfig_.filterOptions.pidFilterMode);
            break;
        default:
            break;
    }

    AUDIO_INFO_LOG("pid:%{public}d usage:%{public}d result:%{public}s", rendererConfig.appInfo.appPid,
        rendererConfig.rendererInfo.streamUsage, res ? "true" : "false");
    return res;
}

void AudioService::FilterAllFastProcess()
{
    std::unique_lock<std::mutex> lock(processListMutex_);
    if (linkedPairedList_.size() == 0) {
        return;
    }
    for (auto paired : linkedPairedList_) {
        AudioProcessConfig temp = paired.first->processConfig_;
        if (temp.audioMode == AUDIO_MODE_PLAYBACK && ShouldBeInnerCap(temp)) {
            paired.first->SetInnerCapState(true);
            paired.second->EnableFastInnerCap();
        } else {
            paired.first->SetInnerCapState(false);
        }
    }

    for (auto pair : endpointList_) {
        if (pair.second->GetDeviceRole() == OUTPUT_DEVICE && !pair.second->ShouldInnerCap()) {
            pair.second->DisableFastInnerCap();
        }
    }
}

int32_t AudioService::OnInitInnerCapList()
{
    AUDIO_INFO_LOG("workingInnerCapId_ is %{public}d", workingInnerCapId_);
    FilterAllFastProcess();
    std::unique_lock<std::mutex> lock(rendererMapMutex_);
    for (auto it = allRendererMap_.begin(); it != allRendererMap_.end(); it++) {
        std::shared_ptr<RendererInServer> renderer = it->second.lock();
        if (renderer == nullptr) {
            AUDIO_WARNING_LOG("Renderer is already released!");
            continue;
        }
        if (ShouldBeInnerCap(renderer->processConfig_)) {
            renderer->EnableInnerCap();
            filteredRendererMap_.push_back(renderer);
        }
    }
    return SUCCESS;
}

int32_t AudioService::OnUpdateInnerCapList()
{
    AUDIO_INFO_LOG("workingInnerCapId_ is %{public}d", workingInnerCapId_);

    std::unique_lock<std::mutex> lock(rendererMapMutex_);
    for (size_t i = 0; i < filteredRendererMap_.size(); i++) {
        std::shared_ptr<RendererInServer> renderer = filteredRendererMap_[i].lock();
        if (renderer == nullptr) {
            AUDIO_WARNING_LOG("Renderer is already released!");
            continue;
        }
        if (!ShouldBeInnerCap(renderer->processConfig_)) {
            renderer->DisableInnerCap();
        }
    }
    filteredRendererMap_.clear();
    lock.unlock();
    // EnableInnerCap will be called twice as it's already in filteredRendererMap_.
    return OnInitInnerCapList();
}

// Only one session is working at the same time.
int32_t AudioService::OnCapturerFilterChange(uint32_t sessionId, const AudioPlaybackCaptureConfig &newConfig)
{
    Trace trace("AudioService::OnCapturerFilterChange");
    // in plan:
    // step 1: if sessionId is not added before, add the sessionId and enbale the filter in allRendererMap_
    // step 2: if sessionId is already in using, this means the config is changed. Check the filtered renderer before,
    // call disable inner-cap for those not meet with the new config, than filter all allRendererMap_.
    if (workingInnerCapId_ == 0) {
        workingInnerCapId_ = sessionId;
        workingConfig_ = newConfig;
        return OnInitInnerCapList();
    }

    if (workingInnerCapId_ == sessionId) {
        workingConfig_ = newConfig;
        return OnUpdateInnerCapList();
    }

    AUDIO_WARNING_LOG("%{public}u is working, comming %{public}u will not work!", workingInnerCapId_, sessionId);
    return ERR_OPERATION_FAILED;
}

int32_t AudioService::OnCapturerFilterRemove(uint32_t sessionId)
{
    if (workingInnerCapId_ != sessionId) {
        AUDIO_WARNING_LOG("%{public}u is working, remove %{public}u will not work!", workingInnerCapId_, sessionId);
        return SUCCESS;
    }
    workingInnerCapId_ = 0;
    workingConfig_ = {};

    std::unique_lock<std::mutex> lockEndpoint(processListMutex_);
    for (auto pair : endpointList_) {
        if (pair.second->GetDeviceRole() == OUTPUT_DEVICE) {
            pair.second->DisableFastInnerCap();
        }
    }
    lockEndpoint.unlock();

    std::lock_guard<std::mutex> lock(rendererMapMutex_);
    for (size_t i = 0; i < filteredRendererMap_.size(); i++) {
        std::shared_ptr<RendererInServer> renderer = filteredRendererMap_[i].lock();
        if (renderer == nullptr) {
            AUDIO_WARNING_LOG("Find renderer is already released!");
            continue;
        }
        renderer->DisableInnerCap();
    }
    AUDIO_INFO_LOG("Filter removed, clear %{public}zu filtered renderer.", filteredRendererMap_.size());

    filteredRendererMap_.clear();

    return SUCCESS;
}

sptr<AudioProcessInServer> AudioService::GetAudioProcess(const AudioProcessConfig &config)
{
    Trace trace("AudioService::GetAudioProcess for " + std::to_string(config.appInfo.appPid));
    AUDIO_INFO_LOG("GetAudioProcess dump %{public}s", ProcessConfig::DumpProcessConfig(config).c_str());
    DeviceInfo deviceInfo = GetDeviceInfoForProcess(config);
    std::shared_ptr<AudioEndpoint> audioEndpoint = GetAudioEndpointForDevice(deviceInfo, config.streamType,
        config.rendererInfo.streamUsage == STREAM_USAGE_VOICE_COMMUNICATION ||
        config.capturerInfo.sourceType == SOURCE_TYPE_VOICE_COMMUNICATION);
    CHECK_AND_RETURN_RET_LOG(audioEndpoint != nullptr, nullptr, "no endpoint found for the process");

    uint32_t totalSizeInframe = 0;
    uint32_t spanSizeInframe = 0;
    audioEndpoint->GetPreferBufferInfo(totalSizeInframe, spanSizeInframe);

    std::lock_guard<std::mutex> lock(processListMutex_);
    sptr<AudioProcessInServer> process = AudioProcessInServer::Create(config, this);
    CHECK_AND_RETURN_RET_LOG(process != nullptr, nullptr, "AudioProcessInServer create failed.");

    std::shared_ptr<OHAudioBuffer> buffer = audioEndpoint->GetEndpointType()
         == AudioEndpoint::TYPE_INDEPENDENT ? audioEndpoint->GetBuffer() : nullptr;
    int32_t ret = process->ConfigProcessBuffer(totalSizeInframe, spanSizeInframe, buffer);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, nullptr, "ConfigProcessBuffer failed");

    ret = LinkProcessToEndpoint(process, audioEndpoint);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, nullptr, "LinkProcessToEndpoint failed");

    linkedPairedList_.push_back(std::make_pair(process, audioEndpoint));
    CheckInnerCapForProcess(process, audioEndpoint);
    return process;
}

void AudioService::ResetAudioEndpoint()
{
    std::lock_guard<std::mutex> lock(processListMutex_);
    AudioProcessConfig config;
    sptr<AudioProcessInServer> processInServer;
    auto paired = linkedPairedList_.begin();
    while (paired != linkedPairedList_.end()) {
        if ((*paired).second->GetEndpointType() == AudioEndpoint::TYPE_MMAP) {
            linkedPairedList_.erase(paired);
            config = (*paired).first->processConfig_;
            int32_t ret = UnlinkProcessToEndpoint((*paired).first, (*paired).second);
            CHECK_AND_RETURN_LOG(ret == SUCCESS, "Unlink process to old endpoint failed");
            std::string endpointName = (*paired).second->GetEndpointName();
            if (endpointList_.find(endpointName) != endpointList_.end()) {
                (*paired).second->Release();
                endpointList_.erase(endpointName);
            }

            DeviceInfo deviceInfo = GetDeviceInfoForProcess(config);
            std::shared_ptr<AudioEndpoint> audioEndpoint = GetAudioEndpointForDevice(deviceInfo, config.streamType,
                config.rendererInfo.streamUsage == STREAM_USAGE_VOICE_COMMUNICATION ||
                config.capturerInfo.sourceType == SOURCE_TYPE_VOICE_COMMUNICATION);
            CHECK_AND_RETURN_LOG(audioEndpoint != nullptr, "Get new endpoint failed");

            ret = LinkProcessToEndpoint((*paired).first, audioEndpoint);
            CHECK_AND_RETURN_LOG(ret == SUCCESS, "LinkProcessToEndpoint failed");
            linkedPairedList_.push_back(std::make_pair((*paired).first, audioEndpoint));
            CheckInnerCapForProcess((*paired).first, audioEndpoint);
        }
        paired++;
    }
}

void AudioService::CheckInnerCapForProcess(sptr<AudioProcessInServer> process, std::shared_ptr<AudioEndpoint> endpoint)
{
    Trace trace("AudioService::CheckInnerCapForProcess:" + std::to_string(process->processConfig_.appInfo.appPid));
    // inner-cap not working
    if (workingInnerCapId_ == 0) {
        return;
    }

    if (ShouldBeInnerCap(process->processConfig_)) {
        process->SetInnerCapState(true);
        endpoint->EnableFastInnerCap();
    } else {
        process->SetInnerCapState(false);
    }
}

int32_t AudioService::NotifyStreamVolumeChanged(AudioStreamType streamType, float volume)
{
    int32_t ret = SUCCESS;
    for (auto item : endpointList_) {
        std::string endpointName = item.second->GetEndpointName();
        if (endpointName == item.first) {
            ret = ret != SUCCESS ? ret : item.second->SetVolume(streamType, volume);
        }
    }
    return ret;
}

int32_t AudioService::LinkProcessToEndpoint(sptr<AudioProcessInServer> process,
    std::shared_ptr<AudioEndpoint> endpoint)
{
    int32_t ret = endpoint->LinkProcessStream(process);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "LinkProcessStream failed");

    ret = process->AddProcessStatusListener(endpoint);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "AddProcessStatusListener failed");

    std::unique_lock<std::mutex> lock(releaseEndpointMutex_);
    if (releasingEndpointSet_.count(endpoint->GetEndpointName())) {
        AUDIO_INFO_LOG("LinkProcessToEndpoint find endpoint is releasing, call break.");
        releasingEndpointSet_.erase(endpoint->GetEndpointName());
        releaseEndpointCV_.notify_all();
    }
    return SUCCESS;
}

int32_t AudioService::UnlinkProcessToEndpoint(sptr<AudioProcessInServer> process,
    std::shared_ptr<AudioEndpoint> endpoint)
{
    int32_t ret = endpoint->UnlinkProcessStream(process);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "UnlinkProcessStream failed");

    ret = process->RemoveProcessStatusListener(endpoint);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "RemoveProcessStatusListener failed");

    return SUCCESS;
}

void AudioService::DelayCallReleaseEndpoint(std::string endpointName, int32_t delayInMs)
{
    AUDIO_INFO_LOG("Delay release endpoint [%{public}s] start.", endpointName.c_str());
    CHECK_AND_RETURN_LOG(endpointList_.count(endpointName),
        "Find no such endpoint: %{public}s", endpointName.c_str());
    std::unique_lock<std::mutex> lock(releaseEndpointMutex_);
    releaseEndpointCV_.wait_for(lock, std::chrono::milliseconds(delayInMs), [this, endpointName] {
        if (releasingEndpointSet_.count(endpointName)) {
            AUDIO_DEBUG_LOG("Wake up but keep release endpoint %{public}s in delay", endpointName.c_str());
            return false;
        }
        AUDIO_DEBUG_LOG("Delay release endpoint break when reuse: %{public}s", endpointName.c_str());
        return true;
    });

    if (!releasingEndpointSet_.count(endpointName)) {
        AUDIO_DEBUG_LOG("Timeout or not need to release: %{public}s", endpointName.c_str());
        return;
    }
    releasingEndpointSet_.erase(endpointName);

    std::shared_ptr<AudioEndpoint> temp = nullptr;
    CHECK_AND_RETURN_LOG(endpointList_.find(endpointName) != endpointList_.end() &&
        endpointList_[endpointName] != nullptr, "Endpoint %{public}s not available, stop call release",
        endpointName.c_str());
    temp = endpointList_[endpointName];
    if (temp->GetStatus() == AudioEndpoint::EndpointStatus::UNLINKED) {
        AUDIO_INFO_LOG("%{public}s not in use anymore, call release!", endpointName.c_str());
        temp->Release();
        temp = nullptr;
        endpointList_.erase(endpointName);
        return;
    }
    AUDIO_WARNING_LOG("%{public}s is not unlinked, stop call release", endpointName.c_str());
    return;
}

DeviceInfo AudioService::GetDeviceInfoForProcess(const AudioProcessConfig &config)
{
    // send the config to AudioPolicyServera and get the device info.
    DeviceInfo deviceInfo;
    bool ret = PolicyHandler::GetInstance().GetProcessDeviceInfo(config, deviceInfo);
    if (ret) {
        AUDIO_INFO_LOG("Get DeviceInfo from policy server success, deviceType: %{public}d, "
            "supportLowLatency: %{public}d", deviceInfo.deviceType, deviceInfo.isLowLatencyDevice);
        return deviceInfo;
    } else {
        AUDIO_WARNING_LOG("GetProcessDeviceInfo from audio policy server failed!");
    }

    if (config.audioMode == AUDIO_MODE_RECORD) {
        deviceInfo.deviceId = 1;
        deviceInfo.networkId = LOCAL_NETWORK_ID;
        deviceInfo.deviceRole = INPUT_DEVICE;
        deviceInfo.deviceType = DEVICE_TYPE_MIC;
    } else {
        deviceInfo.deviceId = 6; // 6 for test
        deviceInfo.networkId = LOCAL_NETWORK_ID;
        deviceInfo.deviceRole = OUTPUT_DEVICE;
        deviceInfo.deviceType = DEVICE_TYPE_SPEAKER;
    }
    AudioStreamInfo targetStreamInfo = {SAMPLE_RATE_48000, ENCODING_PCM, SAMPLE_S16LE, STEREO}; // note: read from xml
    deviceInfo.audioStreamInfo = targetStreamInfo;
    deviceInfo.deviceName = "mmap_device";
    return deviceInfo;
}

std::shared_ptr<AudioEndpoint> AudioService::GetAudioEndpointForDevice(DeviceInfo &deviceInfo,
    AudioStreamType streamType, bool isVoipStream)
{
    if (deviceInfo.deviceRole == INPUT_DEVICE || deviceInfo.networkId != LOCAL_NETWORK_ID ||
        deviceInfo.deviceRole == OUTPUT_DEVICE) {
        // Create shared stream.
        int32_t endpointFlag = AUDIO_FLAG_MMAP;
        if (isVoipStream) {
            endpointFlag = AUDIO_FLAG_VOIP_FAST;
        }
        std::string deviceKey = deviceInfo.networkId + std::to_string(deviceInfo.deviceId) + "_" +
            std::to_string(endpointFlag);
        if (endpointList_.find(deviceKey) != endpointList_.end()) {
            AUDIO_INFO_LOG("AudioService find endpoint already exist for deviceKey:%{public}s", deviceKey.c_str());
            return endpointList_[deviceKey];
        } else {
            std::shared_ptr<AudioEndpoint> endpoint = AudioEndpoint::CreateEndpoint(isVoipStream ?
                AudioEndpoint::TYPE_VOIP_MMAP : AudioEndpoint::TYPE_MMAP, endpointFlag, streamType, deviceInfo);
            CHECK_AND_RETURN_RET_LOG(endpoint != nullptr, nullptr, "Create mmap AudioEndpoint failed.");
            endpointList_[deviceKey] = endpoint;
            return endpoint;
        }
    } else {
        // Create Independent stream.
        std::string deviceKey = deviceInfo.networkId + std::to_string(deviceInfo.deviceId) + "_" + std::to_string(g_id);
        std::shared_ptr<AudioEndpoint> endpoint = AudioEndpoint::CreateEndpoint(AudioEndpoint::TYPE_INDEPENDENT,
            g_id, streamType, deviceInfo);
        CHECK_AND_RETURN_RET_LOG(endpoint != nullptr, nullptr, "Create independent AudioEndpoint failed.");
        g_id++;
        endpointList_[deviceKey] = endpoint;
        return endpoint;
    }
}

void AudioService::Dump(std::stringstream &dumpStringStream)
{
    AUDIO_INFO_LOG("AudioService dump begin");
    if (workingInnerCapId_ != 0) {
        dumpStringStream << "InnerCap filter:" << ProcessConfig::DumpInnerCapConfig(workingConfig_) << std::endl;
    }
    // dump process
    for (auto paired : linkedPairedList_) {
        paired.first->Dump(dumpStringStream);
    }
    // dump endpoint
    for (auto item : endpointList_) {
        dumpStringStream << std::endl << "Endpoint device id:" << item.first << std::endl;
        item.second->Dump(dumpStringStream);
    }
    PolicyHandler::GetInstance().Dump(dumpStringStream);
}

float AudioService::GetMaxAmplitude(bool isOutputDevice)
{
    std::lock_guard<std::mutex> lock(processListMutex_);
    
    if (linkedPairedList_.size() == 0) {
        return 0;
    }

    float fastAudioMaxAmplitude = 0;
    for (auto paired : linkedPairedList_) {
        if (isOutputDevice && (paired.second->GetDeviceRole() == OUTPUT_DEVICE)) {
            float curFastAudioMaxAmplitude = paired.second->GetMaxAmplitude();
            if (curFastAudioMaxAmplitude > fastAudioMaxAmplitude) {
                fastAudioMaxAmplitude = curFastAudioMaxAmplitude;
            }
        }
        if (!isOutputDevice && (paired.second->GetDeviceRole() == INPUT_DEVICE)) {
            float curFastAudioMaxAmplitude = paired.second->GetMaxAmplitude();
            if (curFastAudioMaxAmplitude > fastAudioMaxAmplitude) {
                fastAudioMaxAmplitude = curFastAudioMaxAmplitude;
            }
        }
    }
    return fastAudioMaxAmplitude;
}
} // namespace AudioStandard
} // namespace OHOS
