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

#include "audio_errors.h"
#include "audio_focus_parser.h"
#include "audio_manager_base.h"
#include "iservice_registry.h"
#include "audio_log.h"
#include "hisysevent.h"
#include "system_ability_definition.h"

#include "audio_policy_service.h"


namespace OHOS {
namespace AudioStandard {
using namespace std;
const uint32_t PCM_8_BIT = 8;
const uint32_t PCM_16_BIT = 16;
const uint32_t PCM_24_BIT = 24;
const uint32_t PCM_32_BIT = 32;
const uint32_t BT_BUFFER_ADJUSTMENT_FACTOR = 50;
static sptr<IStandardAudioService> g_sProxy = nullptr;

AudioPolicyService::~AudioPolicyService()
{
    AUDIO_ERR_LOG("~AudioPolicyService()");
    Deinit();
}

bool AudioPolicyService::Init(void)
{
    AUDIO_DEBUG_LOG("AudioPolicyService init");
    serviceFlag_.reset();
    mAudioPolicyManager.Init();
    if (!mConfigParser.LoadConfiguration()) {
        AUDIO_ERR_LOG("Audio Config Load Configuration failed");
        return false;
    }
    if (!mConfigParser.Parse()) {
        AUDIO_ERR_LOG("Audio Config Parse failed");
        return false;
    }

    std::unique_ptr<AudioFocusParser> audioFocusParser = make_unique<AudioFocusParser>();
    CHECK_AND_RETURN_RET_LOG(audioFocusParser != nullptr, false, "Failed to create AudioFocusParser");
    std::string AUDIO_FOCUS_CONFIG_FILE = "vendor/etc/audio/audio_interrupt_policy_config.xml";

    if (audioFocusParser->LoadConfig(focusTable_[0][0])) {
        AUDIO_ERR_LOG("Audio Interrupt Load Configuration failed");
        return false;
    }

    if (mDeviceStatusListener->RegisterDeviceStatusListener()) {
        AUDIO_ERR_LOG("[Policy Service] Register for device status events failed");
        return false;
    }

    return true;
}

void AudioPolicyService::InitKVStore()
{
    mAudioPolicyManager.InitKVStore();
}

bool AudioPolicyService::ConnectServiceAdapter()
{
    if (!mAudioPolicyManager.ConnectServiceAdapter()) {
        AUDIO_ERR_LOG("AudioPolicyService::ConnectServiceAdapter  Error in connecting to audio service adapter");
        return false;
    }

    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        AUDIO_ERR_LOG("[Policy Service] Get samgr failed");
        return false;
    }

    sptr<IRemoteObject> object = samgr->GetSystemAbility(AUDIO_DISTRIBUTED_SERVICE_ID);
    if (object == nullptr) {
        AUDIO_DEBUG_LOG("[Policy Service] audio service remote object is NULL.");
        return false;
    }

    g_sProxy = iface_cast<IStandardAudioService>(object);
    if (g_sProxy == nullptr) {
        AUDIO_DEBUG_LOG("[Policy Service] init g_sProxy is NULL.");
        return false;
    }

    if (serviceFlag_.count() != MIN_SERVICE_COUNT) {
        OnServiceConnected(AudioServiceIndex::AUDIO_SERVICE_INDEX);
    }

    return true;
}

void AudioPolicyService::Deinit(void)
{
    AUDIO_ERR_LOG("Policy service died. closing active ports");
    std::for_each(mIOHandles.begin(), mIOHandles.end(), [&](std::pair<std::string, AudioIOHandle> handle) {
        mAudioPolicyManager.CloseAudioPort(handle.second);
    });

    mIOHandles.clear();
    mDeviceStatusListener->UnRegisterDeviceStatusListener();
    return;
}

int32_t AudioPolicyService::SetAudioSessionCallback(AudioSessionCallback *callback)
{
    return mAudioPolicyManager.SetAudioSessionCallback(callback);
}

int32_t AudioPolicyService::SetStreamVolume(AudioStreamType streamType, float volume) const
{
    return mAudioPolicyManager.SetStreamVolume(streamType, volume);
}

float AudioPolicyService::GetStreamVolume(AudioStreamType streamType) const
{
    return mAudioPolicyManager.GetStreamVolume(streamType);
}

int32_t AudioPolicyService::SetStreamMute(AudioStreamType streamType, bool mute) const
{
    return mAudioPolicyManager.SetStreamMute(streamType, mute);
}

bool AudioPolicyService::GetStreamMute(AudioStreamType streamType) const
{
    return mAudioPolicyManager.GetStreamMute(streamType);
}

bool AudioPolicyService::IsStreamActive(AudioStreamType streamType) const
{
    return mAudioPolicyManager.IsStreamActive(streamType);
}

std::string AudioPolicyService::GetPortName(InternalDeviceType deviceType)
{
    std::string portName = PORT_NONE;
    switch (deviceType) {
        case InternalDeviceType::DEVICE_TYPE_BLUETOOTH_A2DP:
            portName = BLUETOOTH_SPEAKER;
            break;
        case InternalDeviceType::DEVICE_TYPE_SPEAKER:
        case InternalDeviceType::DEVICE_TYPE_WIRED_HEADSET:
        case InternalDeviceType::DEVICE_TYPE_USB_HEADSET:
        case InternalDeviceType::DEVICE_TYPE_BLUETOOTH_SCO:
            portName = PRIMARY_SPEAKER;
            break;
        case InternalDeviceType::DEVICE_TYPE_MIC:
            portName = PRIMARY_MIC;
            break;
        case InternalDeviceType::DEVICE_TYPE_FILE_SINK:
            portName = FILE_SINK;
            break;
        case InternalDeviceType::DEVICE_TYPE_FILE_SOURCE:
            portName = FILE_SOURCE;
            break;
        default:
            portName = PORT_NONE;
            break;
    }

    AUDIO_INFO_LOG("port name is %{public}s", portName.c_str());
    return portName;
}

// private method
AudioModuleInfo AudioPolicyService::ConstructRemoteAudioModuleInfo(std::string networkId, DeviceRole deviceRole,
    DeviceType deviceType)
{
    AudioModuleInfo audioModuleInfo = {};

    if (deviceRole == DeviceRole::OUTPUT_DEVICE) {
        audioModuleInfo.lib = "libmodule-hdi-sink.z.so";
        audioModuleInfo.format = "s16le"; // 16bit little endian
    } else if (deviceRole == DeviceRole::INPUT_DEVICE) {
        audioModuleInfo.lib = "libmodule-hdi-source.z.so";
        audioModuleInfo.format = "s16le"; // we assume it is bigger endian
    } else {
        AUDIO_ERR_LOG("Invalid flag provided %{public}d", static_cast<int32_t>(deviceType));
    }

    audioModuleInfo.name = networkId; // used as "sink_name" in hdi_sink.c, hope we could use name to find target sink.
    audioModuleInfo.networkId = networkId;

    std::stringstream typeValue;
    typeValue << static_cast<int32_t>(deviceType);
    audioModuleInfo.deviceType = typeValue.str();

    audioModuleInfo.adapterName = "remote";
    audioModuleInfo.className = "remote"; // used in renderer_sink_adapter.c
    audioModuleInfo.fixedLatency = "1";
    audioModuleInfo.fileName = "remote_test";

    audioModuleInfo.channels = "2";
    audioModuleInfo.rate = "48000";
    audioModuleInfo.bufferSize = "4096";

    return audioModuleInfo;
}

std::vector<sptr<AudioDeviceDescriptor>> AudioPolicyService::GetDevices(DeviceFlag deviceFlag)
{
    AUDIO_INFO_LOG("Entered AudioPolicyService::%{public}s", __func__);
    std::vector<sptr<AudioDeviceDescriptor>> deviceList = {};

    if (deviceFlag < DeviceFlag::OUTPUT_DEVICES_FLAG || deviceFlag > DeviceFlag::ALL_L_D_DEVICES_FLAG) {
        AUDIO_ERR_LOG("Invalid flag provided %{public}d", deviceFlag);
        return deviceList;
    }

    if (deviceFlag == DeviceFlag::ALL_L_D_DEVICES_FLAG) {
        return mConnectedDevices;
    }

    for (const auto& device : mConnectedDevices) {
        if (device == nullptr) {
            continue;
        }
        bool filterAllLocal = deviceFlag == DeviceFlag::ALL_DEVICES_FLAG && device->networkId_ == LOCAL_NETWORK_ID;
        bool filterLocalOutput = deviceFlag == DeviceFlag::OUTPUT_DEVICES_FLAG
            && device->networkId_ == LOCAL_NETWORK_ID
            && device->deviceRole_ == DeviceRole::OUTPUT_DEVICE;
        bool filterLocalInput = deviceFlag == DeviceFlag::INPUT_DEVICES_FLAG
            && device->networkId_ == LOCAL_NETWORK_ID
            && device->deviceRole_ == DeviceRole::INPUT_DEVICE;

        bool filterAllRemote = deviceFlag == DeviceFlag::ALL_DISTRIBUTED_DEVICES_FLAG
            && device->networkId_ != LOCAL_NETWORK_ID;
        bool filterRemoteOutput = deviceFlag == DeviceFlag::DISTRIBUTED_OUTPUT_DEVICES_FLAG
            && device->networkId_ != LOCAL_NETWORK_ID
            && device->deviceRole_ == DeviceRole::OUTPUT_DEVICE;
        bool filterRemoteInput = deviceFlag == DeviceFlag::DISTRIBUTED_INPUT_DEVICES_FLAG
            && device->networkId_ != LOCAL_NETWORK_ID
        && device->deviceRole_ == DeviceRole::INPUT_DEVICE;

        if (filterAllLocal || filterLocalOutput || filterLocalInput || filterAllRemote || filterRemoteOutput
            || filterRemoteInput) {
            sptr<AudioDeviceDescriptor> devDesc = new(std::nothrow) AudioDeviceDescriptor(*device);
            deviceList.push_back(devDesc);
        }
    }

    AUDIO_INFO_LOG("GetDevices list size = [%{public}zu]", deviceList.size());
    return deviceList;
}

DeviceType AudioPolicyService::FetchHighPriorityDevice()
{
    AUDIO_DEBUG_LOG("Entered AudioPolicyService::%{public}s", __func__);
    DeviceType priorityDevice = DEVICE_TYPE_SPEAKER;

    for (const auto &device : priorityList) {
        auto isPresent = [&device, this] (const sptr<AudioDeviceDescriptor> &desc) {
            CHECK_AND_RETURN_RET_LOG(desc != nullptr, false, "Invalid device descriptor");
            if (((mAudioScene != AUDIO_SCENE_DEFAULT) && (desc->deviceType_ == DEVICE_TYPE_BLUETOOTH_A2DP)) ||
                ((mAudioScene == AUDIO_SCENE_DEFAULT) && (desc->deviceType_ == DEVICE_TYPE_BLUETOOTH_SCO))) {
                return false;
            } else {
                return desc->deviceType_ == device;
            }
        };

        auto itr = std::find_if(mConnectedDevices.begin(), mConnectedDevices.end(), isPresent);
        if (itr != mConnectedDevices.end()) {
            priorityDevice = (*itr)->deviceType_;
            break;
        }
    }

    return priorityDevice;
}

void UpdateActiveDeviceRoute(InternalDeviceType deviceType)
{
    AUDIO_DEBUG_LOG("UpdateActiveDeviceRoute Device type[%{public}d]", deviceType);

    auto ret = SUCCESS;

    switch (deviceType) {
        case DEVICE_TYPE_BLUETOOTH_SCO:
        case DEVICE_TYPE_USB_HEADSET:
        case DEVICE_TYPE_WIRED_HEADSET: {
            ret = g_sProxy->UpdateActiveDeviceRoute(deviceType, DeviceFlag::ALL_DEVICES_FLAG);
            CHECK_AND_RETURN_LOG(ret == SUCCESS, "Failed to update the route for %{public}d", deviceType);
            break;
        }
        case DEVICE_TYPE_SPEAKER: {
            ret = g_sProxy->UpdateActiveDeviceRoute(deviceType, DeviceFlag::OUTPUT_DEVICES_FLAG);
            CHECK_AND_RETURN_LOG(ret == SUCCESS, "Failed to update the route for %{public}d", deviceType);
            break;
        }
        case DEVICE_TYPE_MIC: {
            ret = g_sProxy->UpdateActiveDeviceRoute(deviceType, DeviceFlag::INPUT_DEVICES_FLAG);
            CHECK_AND_RETURN_LOG(ret == SUCCESS, "Failed to update the route for %{public}d", deviceType);
            break;
        }
        default:
            break;
    }
}

static string ConvertToHDIAudioFormat(AudioSampleFormat sampleFormat)
{
    switch (sampleFormat) {
        case SAMPLE_U8:
            return "u8";
        case SAMPLE_S16LE:
            return "s16le";
        case SAMPLE_S24LE:
            return "s24le";
        case SAMPLE_S32LE:
            return "s32le";
        default:
            return "";
    }
}

static uint32_t GetSampleFormatValue(AudioSampleFormat sampleFormat)
{
    switch (sampleFormat) {
        case SAMPLE_U8:
            return PCM_8_BIT;
        case SAMPLE_S16LE:
            return PCM_16_BIT;
        case SAMPLE_S24LE:
            return PCM_24_BIT;
        case SAMPLE_S32LE:
            return PCM_32_BIT;
        default:
            return PCM_16_BIT;
    }
}

int32_t AudioPolicyService::ActivateNewDevice(DeviceType deviceType, bool isSceneActivation = false)
{
    int32_t result = SUCCESS;

    if (mCurrentActiveDevice_ == deviceType) {
        return result;
    }

    if (deviceType == DEVICE_TYPE_BLUETOOTH_A2DP) {
        auto primaryModulesPos = deviceClassInfo_.find(ClassType::TYPE_A2DP);
        if (primaryModulesPos != deviceClassInfo_.end()) {
            auto moduleInfoList = primaryModulesPos->second;
            for (auto &moduleInfo : moduleInfoList) {
                if (mIOHandles.find(moduleInfo.name) == mIOHandles.end()) {
                    AUDIO_INFO_LOG("Load a2dp module [%{public}s]", moduleInfo.name.c_str());
                    AudioStreamInfo audioStreamInfo = {};
                    GetActiveDeviceStreamInfo(deviceType, audioStreamInfo);
                    uint32_t bufferSize
                        = (audioStreamInfo.samplingRate * GetSampleFormatValue(audioStreamInfo.format)
                            * audioStreamInfo.channels) / (PCM_8_BIT * BT_BUFFER_ADJUSTMENT_FACTOR);
                    AUDIO_INFO_LOG("a2dp rate: %{public}d, format: %{public}d, channel: %{public}d",
                        audioStreamInfo.samplingRate, audioStreamInfo.format, audioStreamInfo.channels);
                    moduleInfo.channels = to_string(audioStreamInfo.channels);
                    moduleInfo.rate = to_string(audioStreamInfo.samplingRate);
                    moduleInfo.format = ConvertToHDIAudioFormat(audioStreamInfo.format);
                    moduleInfo.bufferSize = to_string(bufferSize);

                    AudioIOHandle ioHandle = mAudioPolicyManager.OpenAudioPort(moduleInfo);
                    CHECK_AND_RETURN_RET_LOG(ioHandle != OPEN_PORT_FAILURE, ERR_OPERATION_FAILED,
                        "OpenAudioPort failed %{public}d", ioHandle);
                    mIOHandles[moduleInfo.name] = ioHandle;
                }

                std::string activePort = GetPortName(mCurrentActiveDevice_);
                AUDIO_INFO_LOG("port %{public}s, active device %{public}d", activePort.c_str(), mCurrentActiveDevice_);
                mAudioPolicyManager.SuspendAudioDevice(activePort, true);
            }
        }
    } else if (mCurrentActiveDevice_ == DEVICE_TYPE_BLUETOOTH_A2DP) {
        std::string activePort = GetPortName(mCurrentActiveDevice_);
        mAudioPolicyManager.SuspendAudioDevice(activePort, true);
    }

    AudioIOHandle ioHandle = GetAudioIOHandle(deviceType);
    std::string portName = GetPortName(deviceType);
    CHECK_AND_RETURN_RET_LOG(portName != PORT_NONE, result, "Invalid port name %{public}s", portName.c_str());

    result = mAudioPolicyManager.SetDeviceActive(ioHandle, deviceType, portName, true);
    CHECK_AND_RETURN_RET_LOG(portName != PORT_NONE, result, "SetDeviceActive failed %{public}d", result);
    mAudioPolicyManager.SuspendAudioDevice(portName, false);

    if (isUpdateRouteSupported_ && !isSceneActivation) {
        UpdateActiveDeviceRoute(deviceType);
    }

    UpdateInputDeviceInfo(deviceType);

    return SUCCESS;
}

int32_t AudioPolicyService::ActivateNewDevice(std::string networkId, DeviceType deviceType, bool isRemote)
{
    if (isRemote) {
        AudioModuleInfo moduleInfo = ConstructRemoteAudioModuleInfo(networkId, GetDeviceRole(deviceType), deviceType);
        AudioIOHandle ioHandle = mAudioPolicyManager.OpenAudioPort(moduleInfo);
        CHECK_AND_RETURN_RET_LOG(ioHandle != ERR_OPERATION_FAILED && ioHandle != ERR_INVALID_HANDLE,
            ERR_OPERATION_FAILED, "OpenAudioPort failed %{public}d", ioHandle);
        mIOHandles[networkId] = ioHandle;
    }
    return SUCCESS;
}

int32_t AudioPolicyService::SetDeviceActive(InternalDeviceType deviceType, bool active)
{
    AUDIO_DEBUG_LOG("[Policy Service] Device type[%{public}d] flag[%{public}d]", deviceType, active);
    CHECK_AND_RETURN_RET_LOG(deviceType != DEVICE_TYPE_NONE, ERR_DEVICE_NOT_SUPPORTED, "Invalid device");

    int32_t result = SUCCESS;

    if (!active) {
        CHECK_AND_RETURN_RET_LOG(deviceType == mCurrentActiveDevice_, SUCCESS, "This device is not active");
        deviceType = FetchHighPriorityDevice();
    }

    if (deviceType == mCurrentActiveDevice_) {
        AUDIO_INFO_LOG("Device already activated %{public}d. No need to activate again", mCurrentActiveDevice_);
        return SUCCESS;
    }

    // Activate new device if its already connected
    auto isPresent = [&deviceType] (const sptr<AudioDeviceDescriptor> &desc) {
        CHECK_AND_RETURN_RET_LOG(desc != nullptr, false, "SetDeviceActive::Invalid device descriptor");
        return ((deviceType == desc->deviceType_) || (deviceType == DEVICE_TYPE_FILE_SINK));
    };

    auto itr = std::find_if(mConnectedDevices.begin(), mConnectedDevices.end(), isPresent);
    if (itr == mConnectedDevices.end()) {
        AUDIO_ERR_LOG("Requested device not available %{public}d ", deviceType);
        return ERR_OPERATION_FAILED;
    }

    switch (deviceType) {
        case DEVICE_TYPE_SPEAKER:
            result = ActivateNewDevice(DEVICE_TYPE_SPEAKER);
            CHECK_AND_RETURN_RET_LOG(result == SUCCESS, result, "Speaker activation err %{public}d", result);
            result = ActivateNewDevice(DEVICE_TYPE_MIC);
            CHECK_AND_RETURN_RET_LOG(result == SUCCESS, result, "Microphone activation err %{public}d", result);
            break;
        case DEVICE_TYPE_FILE_SINK:
            result = ActivateNewDevice(DEVICE_TYPE_FILE_SINK);
            CHECK_AND_RETURN_RET_LOG(result == SUCCESS, result, "FILE_SINK activation err %{public}d", result);
            result = ActivateNewDevice(DEVICE_TYPE_FILE_SOURCE);
            CHECK_AND_RETURN_RET_LOG(result == SUCCESS, result, "FILE_SOURCE activation err %{public}d", result);
            break;
        default:
            result = ActivateNewDevice(deviceType);
            CHECK_AND_RETURN_RET_LOG(result == SUCCESS, result, "Activation failed for %{public}d", deviceType);
            break;
    }

    mCurrentActiveDevice_ = deviceType;
    return result;
}

bool AudioPolicyService::IsDeviceActive(InternalDeviceType deviceType) const
{
    AUDIO_INFO_LOG("Entered AudioPolicyService::%{public}s", __func__);
    return mCurrentActiveDevice_ == deviceType;
}

DeviceType AudioPolicyService::GetActiveOutputDevice() const
{
    return mCurrentActiveDevice_;
}

DeviceType AudioPolicyService::GetActiveInputDevice() const
{
    return mActiveInputDevice_;
}

int32_t AudioPolicyService::SetRingerMode(AudioRingerMode ringMode)
{
    return mAudioPolicyManager.SetRingerMode(ringMode);
}

AudioRingerMode AudioPolicyService::GetRingerMode() const
{
    return mAudioPolicyManager.GetRingerMode();
}

int32_t AudioPolicyService::SetAudioScene(AudioScene audioScene)
{
    CHECK_AND_RETURN_RET_LOG(g_sProxy != nullptr, ERR_OPERATION_FAILED, "Service proxy unavailable");
    mAudioScene = audioScene;

    auto priorityDev = FetchHighPriorityDevice();
    AUDIO_INFO_LOG("Priority device for setAudioScene: %{public}d", priorityDev);

    int32_t result = ActivateNewDevice(priorityDev, true);
    CHECK_AND_RETURN_RET_LOG(result == SUCCESS, ERR_OPERATION_FAILED, "Device activation failed [%{public}d]", result);

    mCurrentActiveDevice_ = priorityDev;

    result = g_sProxy->SetAudioScene(audioScene, priorityDev);
    CHECK_AND_RETURN_RET_LOG(result == SUCCESS, ERR_OPERATION_FAILED, "SetAudioScene failed [%{public}d]", result);

    return SUCCESS;
}

AudioScene AudioPolicyService::GetAudioScene() const
{
    AUDIO_INFO_LOG("GetAudioScene return value: %{public}d", mAudioScene);
    return mAudioScene;
}

bool AudioPolicyService::IsAudioInterruptEnabled() const
{
    return interruptEnabled_;
}

void AudioPolicyService::OnAudioInterruptEnable(bool enable)
{
    interruptEnabled_ = enable;
}

void AudioPolicyService::OnUpdateRouteSupport(bool isSupported)
{
    isUpdateRouteSupported_ = isSupported;
}

bool AudioPolicyService::GetActiveDeviceStreamInfo(DeviceType deviceType, AudioStreamInfo &streamInfo)
{
    if (deviceType == DEVICE_TYPE_BLUETOOTH_A2DP) {
        auto streamInfoPos = connectedA2dpDeviceMap_.find(activeBTDevice_);
        if (streamInfoPos != connectedA2dpDeviceMap_.end()) {
            streamInfo.samplingRate = streamInfoPos->second.samplingRate;
            streamInfo.format = streamInfoPos->second.format;
            streamInfo.channels = streamInfoPos->second.channels;
            return true;
        }
    }

    return false;
}

bool AudioPolicyService::IsConfigurationUpdated(DeviceType deviceType, const AudioStreamInfo &streamInfo)
{
    if (deviceType == DEVICE_TYPE_BLUETOOTH_A2DP) {
        AudioStreamInfo audioStreamInfo = {};
        if (GetActiveDeviceStreamInfo(deviceType, audioStreamInfo)) {
            AUDIO_DEBUG_LOG("Device configurations current rate: %{public}d, format: %{public}d, channel: %{public}d",
                audioStreamInfo.samplingRate, audioStreamInfo.format, audioStreamInfo.channels);
            AUDIO_DEBUG_LOG("Device configurations updated rate: %{public}d, format: %{public}d, channel: %{public}d",
                streamInfo.samplingRate, streamInfo.format, streamInfo.channels);
            if ((audioStreamInfo.samplingRate != streamInfo.samplingRate)
                || (audioStreamInfo.channels != streamInfo.channels)
                || (audioStreamInfo.format != streamInfo.format)) {
                return true;
            }
        }
    }

    return false;
}

void AudioPolicyService::UpdateConnectedDevices(const AudioDeviceDescriptor &deviceDescriptor,
    std::vector<sptr<AudioDeviceDescriptor>> &desc, bool isConnected)
{
    sptr<AudioDeviceDescriptor> audioDescriptor = nullptr;

    if (std::find(ioDeviceList.begin(), ioDeviceList.end(), deviceDescriptor.deviceType_) != ioDeviceList.end()) {
        AUDIO_INFO_LOG("Filling io device list for %{public}d", deviceDescriptor.deviceType_);
        audioDescriptor = new(std::nothrow) AudioDeviceDescriptor(deviceDescriptor);
        audioDescriptor->deviceRole_ = INPUT_DEVICE;
        if ((deviceDescriptor.deviceType_ == DEVICE_TYPE_WIRED_HEADSET)
            || (deviceDescriptor.deviceType_ == DEVICE_TYPE_USB_HEADSET)) {
            auto isBuiltInMicPresent = [](const sptr<AudioDeviceDescriptor> &devDesc) {
                CHECK_AND_RETURN_RET_LOG(devDesc != nullptr, false, "Invalid device descriptor");
                return (DEVICE_TYPE_MIC == devDesc->deviceType_);
            };

            auto itr = std::find_if(mConnectedDevices.begin(), mConnectedDevices.end(), isBuiltInMicPresent);
            if (itr != mConnectedDevices.end()) {
                audioDescriptor->SetDeviceCapability((*itr)->audioStreamInfo_, 0);
            }
        }

        desc.push_back(audioDescriptor);
        if (isConnected) {
            mConnectedDevices.insert(mConnectedDevices.begin(), audioDescriptor);
        }

        audioDescriptor = new(std::nothrow) AudioDeviceDescriptor(deviceDescriptor);
        audioDescriptor->deviceRole_ = OUTPUT_DEVICE;
        if ((deviceDescriptor.deviceType_ == DEVICE_TYPE_WIRED_HEADSET)
            || (deviceDescriptor.deviceType_ == DEVICE_TYPE_USB_HEADSET)) {
            auto isSpeakerPresent = [](const sptr<AudioDeviceDescriptor> &devDesc) {
                CHECK_AND_RETURN_RET_LOG(devDesc != nullptr, false, "Invalid device descriptor");
                return (DEVICE_TYPE_SPEAKER == devDesc->deviceType_);
            };

            auto itr = std::find_if(mConnectedDevices.begin(), mConnectedDevices.end(), isSpeakerPresent);
            if (itr != mConnectedDevices.end()) {
                audioDescriptor->SetDeviceCapability((*itr)->audioStreamInfo_, 0);
            }
        }
        desc.push_back(audioDescriptor);
        if (isConnected) {
            mConnectedDevices.insert(mConnectedDevices.begin(), audioDescriptor);
        }
    } else {
        AUDIO_INFO_LOG("Filling non-io device list for %{public}d", deviceDescriptor.deviceType_);
        audioDescriptor = new(std::nothrow) AudioDeviceDescriptor(deviceDescriptor);
        audioDescriptor->deviceRole_ = GetDeviceRole(deviceDescriptor.deviceType_);
        desc.push_back(audioDescriptor);
        if (isConnected) {
            mConnectedDevices.insert(mConnectedDevices.begin(), audioDescriptor);
        }
    }
}

void AudioPolicyService::OnDeviceStatusUpdated(DeviceType devType, bool isConnected, const std::string& macAddress,
    const std::string& deviceName, const AudioStreamInfo& streamInfo)
{
    AUDIO_INFO_LOG("Device connection state updated | TYPE[%{public}d] STATUS[%{public}d]", devType, isConnected);
    int32_t result = ERROR;
    AudioDeviceDescriptor deviceDesc(devType, GetDeviceRole(devType));
    deviceDesc.SetDeviceInfo(deviceName, macAddress);
    deviceDesc.SetDeviceCapability(streamInfo, 0);

    UpdateGroupInfo(VOLUME_TYPE, GROUP_NAME_DEFAULT, deviceDesc.volumeGroupId_, LOCAL_NETWORK_ID, isConnected,
        NO_REMOTE_ID);
    UpdateGroupInfo(INTERRUPT_TYPE, GROUP_NAME_DEFAULT, deviceDesc.interruptGroupId_, LOCAL_NETWORK_ID, isConnected,
        NO_REMOTE_ID);
    deviceDesc.networkId_ = LOCAL_NETWORK_ID;

    // fill device change action for callback
    std::vector<sptr<AudioDeviceDescriptor>> deviceChangeDescriptor = {};
    auto isPresent = [&devType] (const sptr<AudioDeviceDescriptor> &descriptor) {
        return descriptor->deviceType_ == devType;
    };

    // If device already in list, remove it else do not modify the list
    mConnectedDevices.erase(std::remove_if(mConnectedDevices.begin(), mConnectedDevices.end(), isPresent),
                            mConnectedDevices.end());

    if (devType == DEVICE_TYPE_BLUETOOTH_A2DP) {
        if (isConnected) {
            connectedA2dpDeviceMap_.insert(make_pair(macAddress, streamInfo));
            activeBTDevice_ = macAddress;
        } else {
            connectedA2dpDeviceMap_.erase(macAddress);
            activeBTDevice_ = "";
        }
    } else if ((devType == DEVICE_TYPE_BLUETOOTH_SCO) && isConnected && (GetAudioScene() == AUDIO_SCENE_DEFAULT)) {
        // For SCO device, add to connected device and donot activate now
        AUDIO_INFO_LOG("BT SCO device detected in non-call mode [%{public}d]", GetAudioScene());
        UpdateConnectedDevices(deviceDesc, deviceChangeDescriptor, isConnected);
        TriggerDeviceChangedCallback(deviceChangeDescriptor, isConnected);
        return;
    }

    // new device found. If connected, add into active device list
    if (isConnected) {
        result = ActivateNewDevice(devType);
        CHECK_AND_RETURN_LOG(result == SUCCESS, "Failed to activate new device %{public}d", devType);
        mCurrentActiveDevice_ = devType;
        UpdateConnectedDevices(deviceDesc, deviceChangeDescriptor, isConnected);
    } else {
        UpdateConnectedDevices(deviceDesc, deviceChangeDescriptor, isConnected);

        auto priorityDev = FetchHighPriorityDevice();
        AUDIO_INFO_LOG("Priority device is [%{public}d]", priorityDev);

        if (priorityDev == DEVICE_TYPE_SPEAKER) {
            result = ActivateNewDevice(DEVICE_TYPE_SPEAKER);
            CHECK_AND_RETURN_LOG(result == SUCCESS, "Failed to activate new device [%{public}d]", result);

            result = ActivateNewDevice(DEVICE_TYPE_MIC);
            CHECK_AND_RETURN_LOG(result == SUCCESS, "Failed to activate new device [%{public}d]", result);
        } else {
            result = ActivateNewDevice(priorityDev);
            CHECK_AND_RETURN_LOG(result == SUCCESS, "Failed to activate new device [%{public}d]", result);
        }

        if (devType == DEVICE_TYPE_BLUETOOTH_A2DP) {
            if (mIOHandles.find(BLUETOOTH_SPEAKER) != mIOHandles.end()) {
                mAudioPolicyManager.CloseAudioPort(mIOHandles[BLUETOOTH_SPEAKER]);
                mIOHandles.erase(BLUETOOTH_SPEAKER);
            }
        }

        mCurrentActiveDevice_ = priorityDev;
    }

    TriggerDeviceChangedCallback(deviceChangeDescriptor, isConnected);
}

void AudioPolicyService::OnDeviceConfigurationChanged(DeviceType deviceType, const std::string &macAddress,
    const std::string &deviceName, const AudioStreamInfo &streamInfo)
{
    AUDIO_DEBUG_LOG("OnDeviceConfigurationChanged in");
    if ((deviceType == DEVICE_TYPE_BLUETOOTH_A2DP) && !macAddress.compare(activeBTDevice_)
        && IsDeviceActive(deviceType)) {
        if (!IsConfigurationUpdated(deviceType, streamInfo)) {
            AUDIO_DEBUG_LOG("Audio configuration same");
            return;
        }

        uint32_t bufferSize
            = (streamInfo.samplingRate * GetSampleFormatValue(streamInfo.format)
                * streamInfo.channels) / (PCM_8_BIT * BT_BUFFER_ADJUSTMENT_FACTOR);
        AUDIO_DEBUG_LOG("Updated buffer size: %{public}d", bufferSize);
        connectedA2dpDeviceMap_[macAddress] = streamInfo;

        auto a2dpModulesPos = deviceClassInfo_.find(ClassType::TYPE_A2DP);
        if (a2dpModulesPos != deviceClassInfo_.end()) {
            auto moduleInfoList = a2dpModulesPos->second;
            for (auto &moduleInfo : moduleInfoList) {
                if (mIOHandles.find(moduleInfo.name) != mIOHandles.end()) {
                    moduleInfo.channels = to_string(streamInfo.channels);
                    moduleInfo.rate = to_string(streamInfo.samplingRate);
                    moduleInfo.format = ConvertToHDIAudioFormat(streamInfo.format);
                    moduleInfo.bufferSize = to_string(bufferSize);

                    // First unload the existing bt sink
                    AUDIO_DEBUG_LOG("UnLoad existing a2dp module");
                    std::string currentActivePort = GetPortName(mCurrentActiveDevice_);
                    AudioIOHandle activateDeviceIOHandle = mIOHandles[BLUETOOTH_SPEAKER];
                    mAudioPolicyManager.SuspendAudioDevice(currentActivePort, true);
                    mAudioPolicyManager.CloseAudioPort(activateDeviceIOHandle);

                    // Load bt sink module again with new configuration
                    AUDIO_DEBUG_LOG("Reload a2dp module [%{public}s]", moduleInfo.name.c_str());
                    AudioIOHandle ioHandle = mAudioPolicyManager.OpenAudioPort(moduleInfo);
                    CHECK_AND_RETURN_LOG(ioHandle != OPEN_PORT_FAILURE, "OpenAudioPort failed %{public}d", ioHandle);
                    mIOHandles[moduleInfo.name] = ioHandle;
                    std::string portName = GetPortName(deviceType);
                    mAudioPolicyManager.SetDeviceActive(ioHandle, deviceType, portName, true);
                    mAudioPolicyManager.SuspendAudioDevice(portName, false);

                    auto isPresent = [&deviceType] (const sptr<AudioDeviceDescriptor> &descriptor) {
                        return descriptor->deviceType_ == deviceType;
                    };

                    sptr<AudioDeviceDescriptor> audioDescriptor
                        = new(std::nothrow) AudioDeviceDescriptor(deviceType, OUTPUT_DEVICE);
                    audioDescriptor->SetDeviceInfo(deviceName, macAddress);
                    audioDescriptor->SetDeviceCapability(streamInfo, 0);
                    std::replace_if(mConnectedDevices.begin(), mConnectedDevices.end(), isPresent, audioDescriptor);
                    break;
                }
            }
        }
    }
}

void AudioPolicyService::OnDeviceStatusUpdated(DStatusInfo statusInfo)
{
    DeviceType devType = GetDeviceTypeFromPin(statusInfo.hdiPin);
    AUDIO_INFO_LOG("Device connection state updated | TYPE[%{public}d] STATUS[%{public}d] NETWORKID[%{public}s]",
        devType, statusInfo.isConnected, statusInfo.networkId);
    const std::string networkId = statusInfo.networkId;
    AudioDeviceDescriptor deviceDesc(devType, GetDeviceRole(devType));
    deviceDesc.SetDeviceInfo(statusInfo.deviceName, statusInfo.macAddress);
    deviceDesc.SetDeviceCapability(statusInfo.streamInfo, 0);
    deviceDesc.networkId_ = networkId;
    UpdateGroupInfo(VOLUME_TYPE, GROUP_NAME_DEFAULT, deviceDesc.volumeGroupId_, networkId, statusInfo.isConnected,
        statusInfo.mappingVolumeId);
    UpdateGroupInfo(INTERRUPT_TYPE, GROUP_NAME_DEFAULT, deviceDesc.interruptGroupId_, networkId,
        statusInfo.isConnected, statusInfo.mappingInterruptId);

    // fill device change action for callback
    std::vector<sptr<AudioDeviceDescriptor>> deviceChangeDescriptor = {};

    for (auto devDes : mConnectedDevices) {
        if (statusInfo.isConnected && devDes->deviceType_ == devType && devDes->networkId_== networkId) {
            AUDIO_INFO_LOG("Device [%{public}s] Type [%{public}d] has connected already!", networkId.c_str(), devType);
            return;
        }
    }

    auto isPresent = [&devType, &networkId](const sptr<AudioDeviceDescriptor>& descriptor) {
        return descriptor->deviceType_ == devType && descriptor->networkId_ == networkId;
    };
    // If device already in list, remove it else do not modify the list
    mConnectedDevices.erase(std::remove_if(mConnectedDevices.begin(), mConnectedDevices.end(), isPresent),
        mConnectedDevices.end());

    // new device found. If connected, add into active device list
    if (statusInfo.isConnected) {
        AUDIO_INFO_LOG("=== DEVICE CONNECTED === TYPE[%{public}d], ConnectType[%{public}d]", devType,
            statusInfo.connectType);
        ActivateNewDevice(statusInfo.networkId, devType,
            statusInfo.connectType==ConnectType::CONNECT_TYPE_DISTRIBUTED);

        UpdateConnectedDevices(deviceDesc, deviceChangeDescriptor, statusInfo.isConnected);
    } else {
        AUDIO_INFO_LOG("=== DEVICE DISCONNECTED === TYPE[%{public}d], ConnectType[%{public}d]", devType,
            statusInfo.connectType);
        if (mIOHandles.find(networkId) != mIOHandles.end()) {
            mAudioPolicyManager.CloseAudioPort(mIOHandles[networkId]);
            mIOHandles.erase(networkId);
        }

        UpdateConnectedDevices(deviceDesc, deviceChangeDescriptor, statusInfo.isConnected);
    }

    TriggerDeviceChangedCallback(deviceChangeDescriptor, statusInfo.isConnected);
    AUDIO_INFO_LOG("device list size = [%{public}zu]", mConnectedDevices.size());
}

void AudioPolicyService::OnServiceConnected(AudioServiceIndex serviceIndex)
{
    AUDIO_INFO_LOG("[module_load]::OnServiceConnected for [%{public}d]", serviceIndex);
    CHECK_AND_RETURN_LOG(serviceIndex >= HDI_SERVICE_INDEX && serviceIndex <= AUDIO_SERVICE_INDEX, "invalid index");

    // If audio service or hdi service is not ready, donot load default modules
    serviceFlag_.set(serviceIndex, true);
    if (serviceFlag_.count() != MIN_SERVICE_COUNT) {
        AUDIO_INFO_LOG("[module_load]::hdi service or audio service not up. Cannot load default module now");
        return;
    }

    int32_t result = ERROR;
    AUDIO_INFO_LOG("[module_load]::HDI and AUDIO SERVICE is READY. Loading default modules");
    for (const auto &device : deviceClassInfo_) {
        if (device.first == ClassType::TYPE_PRIMARY || device.first == ClassType::TYPE_FILE_IO) {
            auto moduleInfoList = device.second;
            for (auto &moduleInfo : moduleInfoList) {
                AUDIO_INFO_LOG("[module_load]::Load module[%{public}s]", moduleInfo.name.c_str());
                moduleInfo.sinkLatency = sinkLatencyInMsec_ != 0 ? to_string(sinkLatencyInMsec_) : "";
                AudioIOHandle ioHandle = mAudioPolicyManager.OpenAudioPort(moduleInfo);
                if (ioHandle == OPEN_PORT_FAILURE) {
                    AUDIO_INFO_LOG("[module_load]::Open port failed");
                    continue;
                }
                mIOHandles[moduleInfo.name] = ioHandle;
                auto devType = GetDeviceType(moduleInfo.name);
                if (devType == DEVICE_TYPE_SPEAKER || devType == DEVICE_TYPE_MIC) {
                    result = mAudioPolicyManager.SetDeviceActive(ioHandle, devType, moduleInfo.name, true);
                    if (result != SUCCESS) {
                        AUDIO_ERR_LOG("[module_load]::Device failed %{public}d", devType);
                        continue;
                    }
                    AddAudioDevice(moduleInfo, devType);
                }
            }
        }
    }

    if (result == SUCCESS) {
        AUDIO_INFO_LOG("[module_load]::Setting speaker as active device on bootup");
        mCurrentActiveDevice_ = DEVICE_TYPE_SPEAKER;
        mActiveInputDevice_ = DEVICE_TYPE_MIC;
    }
}

void AudioPolicyService::AddAudioDevice(AudioModuleInfo& moduleInfo, InternalDeviceType devType)
{
    // add new device into active device list
    std::string volumeGroupName = GetGroupName(moduleInfo.name, VOLUME_TYPE);
    std::string interruptGroupName = GetGroupName(moduleInfo.name, INTERRUPT_TYPE);
    int32_t volumeGroupId = GROUP_ID_NONE;
    int32_t interruptGroupId = GROUP_ID_NONE;
    UpdateGroupInfo(GroupType::VOLUME_TYPE, volumeGroupName, volumeGroupId, LOCAL_NETWORK_ID, true,
        NO_REMOTE_ID);
    UpdateGroupInfo(GroupType::INTERRUPT_TYPE, interruptGroupName, interruptGroupId, LOCAL_NETWORK_ID,
        true, NO_REMOTE_ID);

    sptr<AudioDeviceDescriptor> audioDescriptor = new(std::nothrow) AudioDeviceDescriptor(devType,
        GetDeviceRole(moduleInfo.role), volumeGroupId, interruptGroupId, LOCAL_NETWORK_ID);
    if (!moduleInfo.rate.empty() && !moduleInfo.channels.empty()) {
        AudioStreamInfo streamInfo = {};
        streamInfo.samplingRate = static_cast<AudioSamplingRate>(stoi(moduleInfo.rate));
        streamInfo.channels = static_cast<AudioChannel>(stoi(moduleInfo.channels));
        audioDescriptor->SetDeviceCapability(streamInfo, 0);
    }

    mConnectedDevices.insert(mConnectedDevices.begin(), audioDescriptor);
}

// Parser callbacks
void AudioPolicyService::OnXmlParsingCompleted(const std::unordered_map<ClassType, std::list<AudioModuleInfo>> &xmlData)
{
    AUDIO_INFO_LOG("AudioPolicyService::%{public}s, device class num [%{public}zu]", __func__, xmlData.size());
    if (xmlData.empty()) {
        AUDIO_ERR_LOG("failed to parse xml file. Received data is empty");
        return;
    }

    deviceClassInfo_ = xmlData;
}

void AudioPolicyService::OnVolumeGroupParsed(std::unordered_map<std::string, std::string>& volumeGroupData)
{
    AUDIO_INFO_LOG("AudioPolicyService::%{public}s, group data num [%{public}zu]", __func__, volumeGroupData.size());
    if (volumeGroupData.empty()) {
        AUDIO_ERR_LOG("failed to parse xml file. Received data is empty");
        return;
    }

    volumeGroupData_ = volumeGroupData;
}

void AudioPolicyService::OnInterruptGroupParsed(std::unordered_map<std::string, std::string>& interruptGroupData)
{
    AUDIO_INFO_LOG("AudioPolicyService::%{public}s, group data num [%{public}zu]", __func__, interruptGroupData.size());
    if (interruptGroupData.empty()) {
        AUDIO_ERR_LOG("failed to parse xml file. Received data is empty");
        return;
    }

    interruptGroupData_ = interruptGroupData;
}

int32_t AudioPolicyService::SetDeviceChangeCallback(const int32_t clientId, const DeviceFlag flag,
    const sptr<IRemoteObject> &object)
{
    AUDIO_INFO_LOG("Entered AudioPolicyService::%{public}s", __func__);

    sptr<IStandardAudioPolicyManagerListener> callback = iface_cast<IStandardAudioPolicyManagerListener>(object);
    if (callback != nullptr) {
        deviceChangeCallbackMap_[clientId] = std::make_pair(flag, callback);
    }

    return SUCCESS;
}

int32_t AudioPolicyService::UnsetDeviceChangeCallback(const int32_t clientId)
{
    AUDIO_INFO_LOG("Entered AudioPolicyService::%{public}s", __func__);

    if (deviceChangeCallbackMap_.erase(clientId)) {
        AUDIO_DEBUG_LOG("AudioPolicyServer:UnsetDeviceChangeCallback for clientID %{public}d done", clientId);
    } else {
        AUDIO_DEBUG_LOG("AudioPolicyServer:UnsetDeviceChangeCallback clientID %{public}d not present/unset already",
                        clientId);
    }

    return SUCCESS;
}

int32_t AudioPolicyService::RegisterAudioRendererEventListener(int32_t clientUID, const sptr<IRemoteObject> &object,
    bool hasBTPermission)
{
    return mStreamCollector.RegisterAudioRendererEventListener(clientUID, object, hasBTPermission);
}

int32_t AudioPolicyService::UnregisterAudioRendererEventListener(int32_t clientUID)
{
    return mStreamCollector.UnregisterAudioRendererEventListener(clientUID);
}

int32_t AudioPolicyService::RegisterAudioCapturerEventListener(int32_t clientUID, const sptr<IRemoteObject> &object,
    bool hasBTPermission)
{
    return mStreamCollector.RegisterAudioCapturerEventListener(clientUID, object, hasBTPermission);
}

int32_t AudioPolicyService::UnregisterAudioCapturerEventListener(int32_t clientUID)
{
    return mStreamCollector.UnregisterAudioCapturerEventListener(clientUID);
}

static void UpdateDeviceInfo(DeviceInfo &deviceInfo, const sptr<AudioDeviceDescriptor> &desc, bool hasBTPermission)
{
    deviceInfo.deviceType = desc->deviceType_;
    deviceInfo.deviceRole = desc->deviceRole_;
    deviceInfo.deviceId = desc->deviceId_;
    deviceInfo.channelMasks = desc->channelMasks_;

    if (hasBTPermission) {
        deviceInfo.deviceName = desc->deviceName_;
        deviceInfo.macAddress = desc->macAddress_;
    } else {
        deviceInfo.deviceName = "";
        deviceInfo.macAddress = "";
    }

    deviceInfo.audioStreamInfo.samplingRate = desc->audioStreamInfo_.samplingRate;
    deviceInfo.audioStreamInfo.encoding = desc->audioStreamInfo_.encoding;
    deviceInfo.audioStreamInfo.format = desc->audioStreamInfo_.format;
    deviceInfo.audioStreamInfo.channels = desc->audioStreamInfo_.channels;
}

void AudioPolicyService::UpdateStreamChangeDeviceInfo(AudioMode &mode, AudioStreamChangeInfo &streamChangeInfo)
{
    if (mode == AUDIO_MODE_PLAYBACK) {
        std::vector<sptr<AudioDeviceDescriptor>> outputDevices = GetDevices(OUTPUT_DEVICES_FLAG);
        DeviceType activeDeviceType = mCurrentActiveDevice_;
        DeviceRole activeDeviceRole = OUTPUT_DEVICE;
        for (sptr<AudioDeviceDescriptor> desc : outputDevices) {
            if ((desc->deviceType_ == activeDeviceType) && (desc->deviceRole_ == activeDeviceRole)) {
                UpdateDeviceInfo(streamChangeInfo.audioRendererChangeInfo.outputDeviceInfo, desc, true);
                break;
            }
        }
    } else {
        std::vector<sptr<AudioDeviceDescriptor>> inputDevices = GetDevices(INPUT_DEVICES_FLAG);
        DeviceType activeDeviceType = mActiveInputDevice_;
        DeviceRole activeDeviceRole = INPUT_DEVICE;
        for (sptr<AudioDeviceDescriptor> desc : inputDevices) {
            if ((desc->deviceType_ == activeDeviceType) && (desc->deviceRole_ == activeDeviceRole)) {
                UpdateDeviceInfo(streamChangeInfo.audioCapturerChangeInfo.inputDeviceInfo, desc, true);
                break;
            }
        }
    }
}

int32_t AudioPolicyService::RegisterTracker(AudioMode &mode, AudioStreamChangeInfo &streamChangeInfo,
    const sptr<IRemoteObject> &object)
{
    UpdateStreamChangeDeviceInfo(mode, streamChangeInfo);
    return mStreamCollector.RegisterTracker(mode, streamChangeInfo, object);
}

int32_t AudioPolicyService::UpdateTracker(AudioMode &mode, AudioStreamChangeInfo &streamChangeInfo)
{
    AUDIO_INFO_LOG("Entered AudioPolicyService::%{public}s", __func__);
    UpdateStreamChangeDeviceInfo(mode, streamChangeInfo);
    return mStreamCollector.UpdateTracker(mode, streamChangeInfo);
}

int32_t AudioPolicyService::GetCurrentRendererChangeInfos(
    vector<unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos, bool hasBTPermission)
{
    int32_t status = mStreamCollector.GetCurrentRendererChangeInfos(audioRendererChangeInfos);
    if (status != SUCCESS) {
        AUDIO_ERR_LOG("AudioPolicyServer:: Get renderer change info failed");
        return status;
    }

    std::vector<sptr<AudioDeviceDescriptor>> outputDevices = GetDevices(OUTPUT_DEVICES_FLAG);
    DeviceType activeDeviceType = mCurrentActiveDevice_;
    DeviceRole activeDeviceRole = OUTPUT_DEVICE;
    for (sptr<AudioDeviceDescriptor> desc : outputDevices) {
        if ((desc->deviceType_ == activeDeviceType) && (desc->deviceRole_ == activeDeviceRole)) {
            size_t rendererInfosSize = audioRendererChangeInfos.size();
            for (size_t i = 0; i < rendererInfosSize; i++) {
                UpdateDeviceInfo(audioRendererChangeInfos[i]->outputDeviceInfo, desc, hasBTPermission);
            }
        }

        break;
    }

    return status;
}

int32_t AudioPolicyService::GetCurrentCapturerChangeInfos(
    vector<unique_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos, bool hasBTPermission)
{
    int status = mStreamCollector.GetCurrentCapturerChangeInfos(audioCapturerChangeInfos);
    if (status != SUCCESS) {
        AUDIO_ERR_LOG("AudioPolicyServer:: Get capturer change info failed");
        return status;
    }

    std::vector<sptr<AudioDeviceDescriptor>> inputDevices = GetDevices(INPUT_DEVICES_FLAG);
    DeviceType activeDeviceType = mActiveInputDevice_;
    DeviceRole activeDeviceRole = INPUT_DEVICE;
    for (sptr<AudioDeviceDescriptor> desc : inputDevices) {
        if ((desc->deviceType_ == activeDeviceType) && (desc->deviceRole_ == activeDeviceRole)) {
            size_t capturerInfosSize = audioCapturerChangeInfos.size();
            for (size_t i = 0; i < capturerInfosSize; i++) {
                UpdateDeviceInfo(audioCapturerChangeInfos[i]->inputDeviceInfo, desc, hasBTPermission);
            }
        }

        break;
    }

    return status;
}

void AudioPolicyService::RegisteredTrackerClientDied(pid_t pid)
{
    mStreamCollector.RegisteredTrackerClientDied(static_cast<int32_t>(pid));
}

void AudioPolicyService::RegisteredStreamListenerClientDied(pid_t pid)
{
    mStreamCollector.RegisteredStreamListenerClientDied(static_cast<int32_t>(pid));
}

int32_t AudioPolicyService::ReconfigureAudioChannel(const uint32_t &channelCount, DeviceType deviceType)
{
    if (mCurrentActiveDevice_ != DEVICE_TYPE_FILE_SINK) {
        AUDIO_INFO_LOG("FILE_SINK_DEVICE is not active. Cannot reconfigure now");
        return ERROR;
    }

    std::string module = FILE_SINK;

    if (deviceType == DeviceType::DEVICE_TYPE_FILE_SINK) {
        CHECK_AND_RETURN_RET_LOG(channelCount <= CHANNEL_8 && channelCount >= MONO, ERROR, "Invalid sink channel");
        module = FILE_SINK;
    } else if (deviceType == DeviceType::DEVICE_TYPE_FILE_SOURCE) {
        CHECK_AND_RETURN_RET_LOG(channelCount <= CHANNEL_6 && channelCount >= MONO, ERROR, "Invalid src channel");
        module = FILE_SOURCE;
    } else {
        AUDIO_ERR_LOG("Invalid DeviceType");
        return ERROR;
    }

    if (mIOHandles.find(module) != mIOHandles.end()) {
        mAudioPolicyManager.CloseAudioPort(mIOHandles[module]);
        mIOHandles.erase(module);
    }

    auto fileClass = deviceClassInfo_.find(ClassType::TYPE_FILE_IO);
    if (fileClass != deviceClassInfo_.end()) {
        auto moduleInfoList = fileClass->second;
        for (auto &moduleInfo : moduleInfoList) {
            if (module == moduleInfo.name) {
                moduleInfo.channels = to_string(channelCount);
                AudioIOHandle ioHandle = mAudioPolicyManager.OpenAudioPort(moduleInfo);
                mIOHandles[moduleInfo.name] = ioHandle;
                mAudioPolicyManager.SetDeviceActive(ioHandle, deviceType, module, true);
            }
        }
    }

    return SUCCESS;
}

// private methods
AudioIOHandle AudioPolicyService::GetAudioIOHandle(InternalDeviceType deviceType)
{
    AudioIOHandle ioHandle;
    switch (deviceType) {
        case InternalDeviceType::DEVICE_TYPE_WIRED_HEADSET:
        case InternalDeviceType::DEVICE_TYPE_USB_HEADSET:
        case InternalDeviceType::DEVICE_TYPE_SPEAKER:
        case InternalDeviceType::DEVICE_TYPE_BLUETOOTH_SCO:
            ioHandle = mIOHandles[PRIMARY_SPEAKER];
            break;
        case InternalDeviceType::DEVICE_TYPE_BLUETOOTH_A2DP:
            ioHandle = mIOHandles[BLUETOOTH_SPEAKER];
            break;
        case InternalDeviceType::DEVICE_TYPE_MIC:
            ioHandle = mIOHandles[PRIMARY_MIC];
            break;
        case InternalDeviceType::DEVICE_TYPE_FILE_SINK:
            ioHandle = mIOHandles[FILE_SINK];
            break;
        case InternalDeviceType::DEVICE_TYPE_FILE_SOURCE:
            ioHandle = mIOHandles[FILE_SOURCE];
            break;
        default:
            ioHandle = mIOHandles[PRIMARY_MIC];
            break;
    }
    return ioHandle;
}

InternalDeviceType AudioPolicyService::GetDeviceType(const std::string &deviceName)
{
    InternalDeviceType devType = InternalDeviceType::DEVICE_TYPE_NONE;
    if (deviceName == "Speaker") {
        devType = InternalDeviceType::DEVICE_TYPE_SPEAKER;
    } else if (deviceName == "Built_in_mic") {
        devType = InternalDeviceType::DEVICE_TYPE_MIC;
    } else if (deviceName == "fifo_output" || deviceName == "fifo_input") {
        devType = DEVICE_TYPE_BLUETOOTH_SCO;
    } else if (deviceName == "file_sink") {
        devType = DEVICE_TYPE_FILE_SINK;
    } else if (deviceName == "file_source") {
        devType = DEVICE_TYPE_FILE_SOURCE;
    }

    return devType;
}

std::string AudioPolicyService::GetGroupName(const std::string& deviceName, const GroupType type)
{
    std::string groupName = GROUP_NAME_NONE;
    if (type == VOLUME_TYPE) {
        auto iter = volumeGroupData_.find(deviceName);
        if (iter != volumeGroupData_.end()) {
            groupName = iter->second;
        }
    } else {
        auto iter = interruptGroupData_.find(deviceName);
        if (iter != interruptGroupData_.end()) {
            groupName = iter->second;
        }
    }
    return groupName;
}

void AudioPolicyService::WriteDeviceChangedSysEvents(const vector<sptr<AudioDeviceDescriptor>> &desc, bool isConnected)
{
    for (auto deviceDescriptor : desc) {
        if (deviceDescriptor != nullptr) {
            if ((deviceDescriptor->deviceType_ == DEVICE_TYPE_WIRED_HEADSET)
                || (deviceDescriptor->deviceType_ == DEVICE_TYPE_USB_HEADSET)) {
                HiviewDFX::HiSysEvent::Write("AUDIO", "AUDIO_HEADSET_CHANGE",
                    HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
                    "ISCONNECT", isConnected ? 1 : 0,
                    "HASMIC", 1,
                    "DEVICETYPE", deviceDescriptor->deviceType_);
            }

            if (!isConnected) {
                continue;
            }

            if (deviceDescriptor->deviceRole_ == OUTPUT_DEVICE) {
                vector<SinkInput> sinkInputs = mAudioPolicyManager.GetAllSinkInputs();
                for (SinkInput sinkInput : sinkInputs) {
                    HiviewDFX::HiSysEvent::Write("AUDIO", "AUDIO_DEVICE_CHANGE",
                        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
                        "ISOUTPUT", 1,
                        "STREAMID", sinkInput.streamId,
                        "STREAMTYPE", sinkInput.streamType,
                        "DEVICETYPE", deviceDescriptor->deviceType_);
                }
            } else if (deviceDescriptor->deviceRole_ == INPUT_DEVICE) {
                vector<SourceOutput> sourceOutputs = mAudioPolicyManager.GetAllSourceOutputs();
                for (SourceOutput sourceOutput : sourceOutputs) {
                    HiviewDFX::HiSysEvent::Write("AUDIO", "AUDIO_DEVICE_CHANGE",
                        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
                        "ISOUTPUT", 0,
                        "STREAMID", sourceOutput.streamId,
                        "STREAMTYPE", sourceOutput.streamType,
                        "DEVICETYPE", deviceDescriptor->deviceType_);
                }
            }
        }
    }
}

void AudioPolicyService::UpdateTrackerDeviceChange(const vector<sptr<AudioDeviceDescriptor>> &desc)
{
    for (sptr<AudioDeviceDescriptor> deviceDesc : desc) {
        if (deviceDesc->deviceRole_ == OUTPUT_DEVICE) {
            DeviceType activeDevice = mCurrentActiveDevice_;
            auto isPresent = [&activeDevice] (const sptr<AudioDeviceDescriptor> &desc) {
                CHECK_AND_RETURN_RET_LOG(desc != nullptr, false, "Invalid device descriptor");
                return ((activeDevice == desc->deviceType_) && (OUTPUT_DEVICE == desc->deviceRole_));
            };

            auto itr = std::find_if(mConnectedDevices.begin(), mConnectedDevices.end(), isPresent);
            if (itr != mConnectedDevices.end()) {
                DeviceInfo outputDevice = {};
                UpdateDeviceInfo(outputDevice, *itr, true);
                mStreamCollector.UpdateTracker(AUDIO_MODE_PLAYBACK, outputDevice);
            }
        }

        if (deviceDesc->deviceRole_ == INPUT_DEVICE) {
            DeviceType activeDevice = mActiveInputDevice_;
            auto isPresent = [&activeDevice] (const sptr<AudioDeviceDescriptor> &desc) {
                CHECK_AND_RETURN_RET_LOG(desc != nullptr, false, "Invalid device descriptor");
                return ((activeDevice == desc->deviceType_) && (INPUT_DEVICE == desc->deviceRole_));
            };

            auto itr = std::find_if(mConnectedDevices.begin(), mConnectedDevices.end(), isPresent);
            if (itr != mConnectedDevices.end()) {
                DeviceInfo inputDevice = {};
                UpdateDeviceInfo(inputDevice, *itr, true);
                mStreamCollector.UpdateTracker(AUDIO_MODE_RECORD, inputDevice);
            }
        }
    }
}

void AudioPolicyService::UpdateGroupInfo(GroupType type, std::string groupName, int32_t& groupId, std::string networkId,
    bool connected, int32_t mappingId)
{
    ConnectType connectType = CONNECT_TYPE_LOCAL;
    if (networkId != LOCAL_NETWORK_ID) {
        connectType = CONNECT_TYPE_DISTRIBUTED;
    }
    if (type == GroupType::VOLUME_TYPE) {
        auto isPresent = [&groupName, &networkId] (const sptr<VolumeGroupInfo> &volumeInfo) {
            return ((groupName == volumeInfo->groupName_) || (networkId == volumeInfo->networkId_));
        };

        auto iter = std::find_if(mVolumeGroups.begin(), mVolumeGroups.end(), isPresent);
        if (iter != mVolumeGroups.end()) {
            groupId = (*iter)->volumeGroupId_;
            // if status is disconnected, remove the group that has none audio device
            std::vector<sptr<AudioDeviceDescriptor>> devsInGroup = GetDevicesForGroup(type, groupId);
            if (!connected && devsInGroup.size() == 0) {
                mVolumeGroups.erase(iter);
            }
            return;
        }
        if (groupName != GROUP_NAME_NONE) {
            groupId = AudioGroupHandle::GetInstance().GetNextId(type);
            sptr<VolumeGroupInfo> volumeGroupInfo = new(std::nothrow) VolumeGroupInfo(groupId,
                mappingId, groupName, networkId, connectType);
            mVolumeGroups.push_back(volumeGroupInfo);
        }
    } else {
        auto isPresent = [&groupName, &networkId] (const sptr<InterruptGroupInfo> &info) {
            return ((groupName == info->groupName_) || (networkId == info->networkId_));
        };

        auto iter = std::find_if(mInterruptGroups.begin(), mInterruptGroups.end(), isPresent);
        if (iter != mInterruptGroups.end()) {
            groupId = (*iter)->interruptGroupId_;
            // if status is disconnected, remove the group that has none audio device
            std::vector<sptr<AudioDeviceDescriptor>> devsInGroup = GetDevicesForGroup(type, groupId);
            if (!connected && devsInGroup.size() == 0) {
                mInterruptGroups.erase(iter);
            }
            return;
        }
        if (groupName != GROUP_NAME_NONE) {
            groupId = AudioGroupHandle::GetInstance().GetNextId(type);
            sptr<InterruptGroupInfo> interruptGroupInfo = new(std::nothrow) InterruptGroupInfo(groupId, mappingId,
                groupName, networkId, connectType);
            mInterruptGroups.push_back(interruptGroupInfo);
        }
    }
}

std::vector<sptr<OHOS::AudioStandard::AudioDeviceDescriptor>> AudioPolicyService::GetDevicesForGroup(GroupType type,
    int32_t groupId)
{
    std::vector<sptr<OHOS::AudioStandard::AudioDeviceDescriptor>> devices = {};
    for (auto devDes : mConnectedDevices) {
        bool inVolumeGroup = type == VOLUME_TYPE && devDes->volumeGroupId_ == groupId;
        bool inInterruptGroup = type == INTERRUPT_TYPE && devDes->interruptGroupId_ == groupId;

        if (inVolumeGroup || inInterruptGroup) {
            sptr<AudioDeviceDescriptor> device = new AudioDeviceDescriptor(*devDes);
            devices.push_back(device);
        }
    }
    return devices;
}

void AudioPolicyService::TriggerDeviceChangedCallback(const vector<sptr<AudioDeviceDescriptor>> &desc, bool isConnected)
{
    DeviceChangeAction deviceChangeAction;
    deviceChangeAction.type = isConnected ? DeviceChangeType::CONNECT : DeviceChangeType::DISCONNECT;

    WriteDeviceChangedSysEvents(desc, isConnected);

    for (auto it = deviceChangeCallbackMap_.begin(); it != deviceChangeCallbackMap_.end(); ++it) {
        deviceChangeAction.deviceDescriptors = DeviceFilterByFlag(it->second.first, desc);
        if (it->second.second && deviceChangeAction.deviceDescriptors.size() > 0) {
            it->second.second->OnDeviceChange(deviceChangeAction);
        }
    }

    UpdateTrackerDeviceChange(desc);
}

std::vector<sptr<AudioDeviceDescriptor>> AudioPolicyService::DeviceFilterByFlag(DeviceFlag flag,
    const std::vector<sptr<AudioDeviceDescriptor>>& desc)
{
    std::vector<sptr<AudioDeviceDescriptor>> descRet;
    switch (flag) {
        case DeviceFlag::ALL_DEVICES_FLAG:
            for (sptr<AudioDeviceDescriptor> var : desc) {
            if (var->networkId_ == LOCAL_NETWORK_ID) {
                    descRet.insert(descRet.end(), var);
                }
            }
            break;
        case DeviceFlag::ALL_DISTRIBUTED_DEVICES_FLAG:
            for (sptr<AudioDeviceDescriptor> var : desc) {
                if (var->networkId_ != LOCAL_NETWORK_ID) {
                    descRet.insert(descRet.end(), var);
                }
            }
            break;
        case DeviceFlag::ALL_L_D_DEVICES_FLAG:
            descRet = desc;
            break;
        default:
            AUDIO_INFO_LOG("AudioPolicyService::%{public}s:deviceFlag type are not supported", __func__);
            break;
    }
    return descRet;
}

DeviceRole AudioPolicyService::GetDeviceRole(DeviceType deviceType) const
{
    switch (deviceType) {
        case DeviceType::DEVICE_TYPE_SPEAKER:
        case DeviceType::DEVICE_TYPE_BLUETOOTH_SCO:
        case DeviceType::DEVICE_TYPE_BLUETOOTH_A2DP:
        case DeviceType::DEVICE_TYPE_WIRED_HEADSET:
        case DeviceType::DEVICE_TYPE_USB_HEADSET:
            return DeviceRole::OUTPUT_DEVICE;
        case DeviceType::DEVICE_TYPE_MIC:
            return DeviceRole::INPUT_DEVICE;
        default:
            return DeviceRole::DEVICE_ROLE_NONE;
    }
}

DeviceRole AudioPolicyService::GetDeviceRole(const std::string &role)
{
    if (role == ROLE_SINK) {
        return DeviceRole::OUTPUT_DEVICE;
    } else if (role == ROLE_SOURCE) {
        return DeviceRole::INPUT_DEVICE;
    } else {
        return DeviceRole::DEVICE_ROLE_NONE;
    }
}

DeviceRole AudioPolicyService::GetDeviceRole(AudioPin pin) const
{
    switch (pin) {
        case OHOS::AudioStandard::AUDIO_PIN_NONE:
            return DeviceRole::DEVICE_ROLE_NONE;
        case OHOS::AudioStandard::AUDIO_PIN_OUT_SPEAKER:
        case OHOS::AudioStandard::AUDIO_PIN_OUT_HEADSET:
        case OHOS::AudioStandard::AUDIO_PIN_OUT_LINEOUT:
        case OHOS::AudioStandard::AUDIO_PIN_OUT_HDMI:
        case OHOS::AudioStandard::AUDIO_PIN_OUT_USB:
        case OHOS::AudioStandard::AUDIO_PIN_OUT_USB_EXT:
        case OHOS::AudioStandard::AUDIO_PIN_OUT_DAUDIO_DEFAULT:
            return DeviceRole::OUTPUT_DEVICE;
        case OHOS::AudioStandard::AUDIO_PIN_IN_MIC:
        case OHOS::AudioStandard::AUDIO_PIN_IN_HS_MIC:
        case OHOS::AudioStandard::AUDIO_PIN_IN_LINEIN:
        case OHOS::AudioStandard::AUDIO_PIN_IN_USB_EXT:
        case OHOS::AudioStandard::AUDIO_PIN_IN_DAUDIO_DEFAULT:
            return DeviceRole::INPUT_DEVICE;
        default:
            return DeviceRole::DEVICE_ROLE_NONE;
    }
}

void AudioPolicyService::OnAudioLatencyParsed(uint64_t latency)
{
    audioLatencyInMsec_ = latency;
}

int32_t AudioPolicyService::GetAudioLatencyFromXml() const
{
    return audioLatencyInMsec_;
}

void AudioPolicyService::OnSinkLatencyParsed(uint32_t latency)
{
    sinkLatencyInMsec_ = latency;
}

uint32_t AudioPolicyService::GetSinkLatencyFromXml() const
{
    return sinkLatencyInMsec_;
}

void AudioPolicyService::UpdateInputDeviceInfo(DeviceType deviceType)
{
    AUDIO_DEBUG_LOG("Current input device is %{public}d", mActiveInputDevice_);

    switch (deviceType) {
        case DEVICE_TYPE_SPEAKER:
        case DEVICE_TYPE_BLUETOOTH_A2DP:
            mActiveInputDevice_ = DEVICE_TYPE_MIC;
            break;
        case DEVICE_TYPE_FILE_SINK:
            mActiveInputDevice_ = DEVICE_TYPE_FILE_SOURCE;
            break;
        case DEVICE_TYPE_WIRED_HEADSET:
        case DEVICE_TYPE_USB_HEADSET:
        case DEVICE_TYPE_BLUETOOTH_SCO:
            mActiveInputDevice_ = deviceType;
            break;
        default:
            break;
    }

    AUDIO_DEBUG_LOG("Input device updated to %{public}d", mActiveInputDevice_);
}

int32_t AudioPolicyService::PausedOrResumeStream(int32_t clientUid,
    StreamSetStateEventInternal &streamSetStateEventInternal)
{
    return mStreamCollector.PausedOrResumeStream(clientUid, streamSetStateEventInternal);
}

DeviceType AudioPolicyService::GetDeviceTypeFromPin(AudioPin hdiPin)
{
    switch (hdiPin) {
        case OHOS::AudioStandard::AUDIO_PIN_NONE:
            break;
        case OHOS::AudioStandard::AUDIO_PIN_OUT_SPEAKER:
        case OHOS::AudioStandard::AUDIO_PIN_OUT_DAUDIO_DEFAULT:
            return DeviceType::DEVICE_TYPE_SPEAKER;
        case OHOS::AudioStandard::AUDIO_PIN_OUT_HEADSET:
            break;
        case OHOS::AudioStandard::AUDIO_PIN_OUT_LINEOUT:
            break;
        case OHOS::AudioStandard::AUDIO_PIN_OUT_HDMI:
            break;
        case OHOS::AudioStandard::AUDIO_PIN_OUT_USB:
            break;
        case OHOS::AudioStandard::AUDIO_PIN_OUT_USB_EXT:
            break;
        case OHOS::AudioStandard::AUDIO_PIN_IN_MIC:
        case OHOS::AudioStandard::AUDIO_PIN_IN_DAUDIO_DEFAULT:
            return DeviceType::DEVICE_TYPE_MIC;
        case OHOS::AudioStandard::AUDIO_PIN_IN_HS_MIC:
            break;
        case OHOS::AudioStandard::AUDIO_PIN_IN_LINEIN:
            break;
        case OHOS::AudioStandard::AUDIO_PIN_IN_USB_EXT:
            break;
        default:
            break;
    }
    return DeviceType::DEVICE_TYPE_DEFAULT;
}

std::unordered_map<int32_t, sptr<VolumeGroupInfo>> AudioPolicyService::GetVolumeGroupInfos()
{
    std::unordered_map<int32_t, sptr<VolumeGroupInfo>> volumeGroupInfos = {};

    for (auto& v : mVolumeGroups) {
        sptr<VolumeGroupInfo> info = new(std::nothrow) VolumeGroupInfo(v->volumeGroupId_, v->mappingId_, v->groupName_,
            v->networkId_, v->connectType_);
        volumeGroupInfos.insert(std::pair(v->volumeGroupId_, info));
    }
    return volumeGroupInfos;
}
} // namespace AudioStandard
} // namespace OHOS
