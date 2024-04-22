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
#undef LOG_TAG
#define LOG_TAG "AudioPolicyServer"

#include "audio_policy_server.h"

#include <csignal>
#include <memory>
#include <unordered_set>
#include <vector>
#include <condition_variable>

#ifdef FEATURE_MULTIMODALINPUT_INPUT
#include "input_manager.h"
#include "key_event.h"
#include "key_option.h"
#endif
#include "power_mgr_client.h"

#include "privacy_kit.h"
#include "accesstoken_kit.h"
#include "permission_state_change_info.h"
#include "token_setproc.h"
#include "tokenid_kit.h"

#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "audio_log.h"
#include "audio_errors.h"
#include "audio_utils.h"
#include "audio_policy_manager_listener_proxy.h"
#include "audio_routing_manager_listener_proxy.h"
#include "i_standard_audio_policy_manager_listener.h"
#include "microphone_descriptor.h"
#include "parameter.h"
#include "parameters.h"

using OHOS::Security::AccessToken::PrivacyKit;
using OHOS::Security::AccessToken::TokenIdKit;
using namespace std;

namespace OHOS {
namespace AudioStandard {

constexpr int32_t PARAMS_VOLUME_NUM = 5;
constexpr int32_t PARAMS_INTERRUPT_NUM = 4;
constexpr int32_t PARAMS_RENDER_STATE_NUM = 2;
constexpr int32_t EVENT_DES_SIZE = 60;
constexpr int32_t ADAPTER_STATE_CONTENT_DES_SIZE = 60;
constexpr int32_t API_VERSION_REMAINDER = 1000;
constexpr uid_t UID_ROOT = 0;
constexpr uid_t UID_MSDP_SA = 6699;
constexpr uid_t UID_INTELLIGENT_VOICE_SA = 1042;
constexpr uid_t UID_CAST_ENGINE_SA = 5526;
constexpr uid_t UID_CAAS_SA = 5527;
constexpr uid_t UID_DISTRIBUTED_AUDIO_SA = 3055;
constexpr uid_t UID_MEDIA_SA = 1013;
constexpr uid_t UID_VM_MANAGER = 5010;
constexpr uid_t UID_AUDIO = 1041;
constexpr uid_t UID_FOUNDATION_SA = 5523;
constexpr uid_t UID_BLUETOOTH_SA = 1002;
constexpr uid_t UID_DISTRIBUTED_CALL_SA = 3069;
constexpr int64_t OFFLOAD_NO_SESSION_ID = -1;

REGISTER_SYSTEM_ABILITY_BY_ID(AudioPolicyServer, AUDIO_POLICY_SERVICE_ID, true)

const std::list<uid_t> AudioPolicyServer::RECORD_ALLOW_BACKGROUND_LIST = {
    UID_ROOT,
    UID_MSDP_SA,
    UID_INTELLIGENT_VOICE_SA,
    UID_CAAS_SA,
    UID_DISTRIBUTED_AUDIO_SA,
    UID_AUDIO,
    UID_FOUNDATION_SA,
    UID_DISTRIBUTED_CALL_SA
};

const std::list<uid_t> AudioPolicyServer::RECORD_PASS_APPINFO_LIST = {
    UID_MEDIA_SA,
    UID_CAST_ENGINE_SA
};

const std::set<uid_t> RECORD_CHECK_FORWARD_LIST = {
    UID_VM_MANAGER
};

AudioPolicyServer::AudioPolicyServer(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate),
      audioPolicyService_(AudioPolicyService::GetAudioPolicyService()),
      audioSpatializationService_(AudioSpatializationService::GetAudioSpatializationService()),
      audioRouterCenter_(AudioRouterCenter::GetAudioRouterCenter())
{
    volumeStep_ = system::GetIntParameter("const.multimedia.audio.volumestep", 1);
    AUDIO_INFO_LOG("Get volumeStep parameter success %{public}d", volumeStep_);

    powerStateCallbackRegister_ = false;
    volumeApplyToAll_ = system::GetBoolParameter("const.audio.volume_apply_to_all", false);
}

void AudioPolicyServer::OnDump()
{
    return;
}

void AudioPolicyServer::OnStart()
{
    AUDIO_INFO_LOG("OnStart");

    interruptService_ = std::make_shared<AudioInterruptService>();
    interruptService_->Init(this);

    audioPolicyServerHandler_ = DelayedSingleton<AudioPolicyServerHandler>::GetInstance();
    audioPolicyServerHandler_->Init(interruptService_);

    interruptService_->SetCallbackHandler(audioPolicyServerHandler_);

    if (audioPolicyService_.SetAudioSessionCallback(this)) {
        AUDIO_ERR_LOG("SetAudioSessionCallback failed");
    }
    audioPolicyService_.Init();

    AddSystemAbilityListener(AUDIO_DISTRIBUTED_SERVICE_ID);
    AddSystemAbilityListener(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
#ifdef FEATURE_MULTIMODALINPUT_INPUT
    AddSystemAbilityListener(MULTIMODAL_INPUT_SERVICE_ID);
#endif
    AddSystemAbilityListener(BLUETOOTH_HOST_SYS_ABILITY_ID);
    AddSystemAbilityListener(ACCESSIBILITY_MANAGER_SERVICE_ID);
    AddSystemAbilityListener(POWER_MANAGER_SERVICE_ID);

    bool res = Publish(this);
    if (!res) {
        AUDIO_INFO_LOG("publish sa err");
    }

    Security::AccessToken::PermStateChangeScope scopeInfo;
    scopeInfo.permList = {"ohos.permission.MICROPHONE"};
    auto callbackPtr = std::make_shared<PerStateChangeCbCustomizeCallback>(scopeInfo, this);
    callbackPtr->ready_ = false;
    int32_t iRes = Security::AccessToken::AccessTokenKit::RegisterPermStateChangeCallback(callbackPtr);
    if (iRes < 0) {
        AUDIO_ERR_LOG("fail to call RegisterPermStateChangeCallback.");
    }

#ifdef FEATURE_MULTIMODALINPUT_INPUT
    SubscribeVolumeKeyEvents();
#endif
}

void AudioPolicyServer::OnStop()
{
    audioPolicyService_.Deinit();
    UnRegisterPowerStateListener();
    return;
}

void AudioPolicyServer::OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    AUDIO_DEBUG_LOG("OnAddSystemAbility systemAbilityId:%{public}d", systemAbilityId);
    int64_t stamp = ClockTime::GetCurNano();
    switch (systemAbilityId) {
#ifdef FEATURE_MULTIMODALINPUT_INPUT
        case MULTIMODAL_INPUT_SERVICE_ID:
            AUDIO_INFO_LOG("OnAddSystemAbility input service start");
            SubscribeVolumeKeyEvents();
            break;
#endif
        case DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID:
            AUDIO_INFO_LOG("OnAddSystemAbility kv data service start");
            InitKVStore();
            RegisterDataObserver();
            break;
        case AUDIO_DISTRIBUTED_SERVICE_ID:
            AUDIO_INFO_LOG("OnAddSystemAbility audio service start");
            if (!isFirstAudioServiceStart_) {
                ConnectServiceAdapter();
                sessionProcessor_.Start();
                RegisterParamCallback();
                LoadEffectLibrary();
                InitMicrophoneMute();
                isFirstAudioServiceStart_ = true;
            } else {
                AUDIO_WARNING_LOG("OnAddSystemAbility audio service is not first start");
            }
            break;
        case BLUETOOTH_HOST_SYS_ABILITY_ID:
            AUDIO_INFO_LOG("OnAddSystemAbility bluetooth service start");
            RegisterBluetoothListener();
            break;
        case ACCESSIBILITY_MANAGER_SERVICE_ID:
            AUDIO_INFO_LOG("OnAddSystemAbility accessibility service start");
            SubscribeAccessibilityConfigObserver();
            InitKVStore();
            RegisterDataObserver();
            break;
        case POWER_MANAGER_SERVICE_ID:
            AUDIO_INFO_LOG("OnAddSystemAbility power manager service start");
            SubscribePowerStateChangeEvents();
            RegisterPowerStateListener();
            break;
        default:
            AUDIO_WARNING_LOG("OnAddSystemAbility unhandled sysabilityId:%{public}d", systemAbilityId);
            break;
    }
    AUDIO_INFO_LOG("done systemAbilityId: %{public}d cost [%{public}" PRId64 "]", systemAbilityId,
        ClockTime::GetCurNano() - stamp);
}

void AudioPolicyServer::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    AUDIO_DEBUG_LOG("AudioPolicyServer::OnRemoveSystemAbility systemAbilityId:%{public}d removed", systemAbilityId);
}

#ifdef FEATURE_MULTIMODALINPUT_INPUT
bool AudioPolicyServer::MaxOrMinVolumeOption(const int32_t &volLevel, const int32_t keyType,
    const AudioStreamType &streamInFocus)
{
    bool volLevelCheck = (keyType == OHOS::MMI::KeyEvent::KEYCODE_VOLUME_UP) ?
        volLevel >= GetMaxVolumeLevel(streamInFocus) : volLevel <= GetMinVolumeLevel(streamInFocus);
    if (volLevelCheck) {
        VolumeEvent volumeEvent;
        volumeEvent.volumeType = (streamInFocus == STREAM_ALL) ? STREAM_MUSIC : streamInFocus;
        volumeEvent.volume = volLevel;
        volumeEvent.updateUi = true;
        volumeEvent.volumeGroupId = 0;
        volumeEvent.networkId = LOCAL_NETWORK_ID;
        CHECK_AND_RETURN_RET_LOG(audioPolicyServerHandler_ != nullptr, false, "audioPolicyServerHandler_ is nullptr");
        audioPolicyServerHandler_->SendVolumeKeyEventCallback(volumeEvent);
        return true;
    }

    return false;
}
#endif

#ifdef FEATURE_MULTIMODALINPUT_INPUT
int32_t AudioPolicyServer::RegisterVolumeKeyEvents(const int32_t keyType)
{
    if ((keyType != OHOS::MMI::KeyEvent::KEYCODE_VOLUME_UP) && (keyType != OHOS::MMI::KeyEvent::KEYCODE_VOLUME_DOWN)) {
        AUDIO_ERR_LOG("VolumeKeyEvents: invalid key type : %{public}d", keyType);
        return ERR_INVALID_PARAM;
    }
    AUDIO_INFO_LOG("RegisterVolumeKeyEvents: volume key: %{public}s.",
        (keyType == OHOS::MMI::KeyEvent::KEYCODE_VOLUME_UP) ? "up" : "down");

    MMI::InputManager *im = MMI::InputManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(im != nullptr, ERR_INVALID_PARAM, "Failed to obtain INPUT manager");

    std::set<int32_t> preKeys;
    std::shared_ptr<OHOS::MMI::KeyOption> keyOption = std::make_shared<OHOS::MMI::KeyOption>();
    CHECK_AND_RETURN_RET_LOG(keyOption != nullptr, ERR_INVALID_PARAM, "Invalid key option");
    keyOption->SetPreKeys(preKeys);
    keyOption->SetFinalKey(keyType);
    keyOption->SetFinalKeyDown(true);
    keyOption->SetFinalKeyDownDuration(VOLUME_KEY_DURATION);
    int32_t keySubId = im->SubscribeKeyEvent(keyOption, [=](std::shared_ptr<MMI::KeyEvent> keyEventCallBack) {
        AUDIO_INFO_LOG("Receive volume key event: %{public}s.",
            (keyType == OHOS::MMI::KeyEvent::KEYCODE_VOLUME_UP) ? "up" : "down");
        std::lock_guard<std::mutex> lock(keyEventMutex_);
        AudioStreamType streamInFocus = AudioStreamType::STREAM_MUSIC; // use STREAM_MUSIC as default stream type
        if (volumeApplyToAll_) {
            streamInFocus = AudioStreamType::STREAM_ALL;
        } else {
            streamInFocus = GetVolumeTypeFromStreamType(GetStreamInFocus());
        }
        if (keyType == OHOS::MMI::KeyEvent::KEYCODE_VOLUME_UP && GetStreamMuteInternal(streamInFocus)) {
            AUDIO_INFO_LOG("VolumeKeyEvents: volumeKey: Up. volumeType %{public}d is mute. Unmute.", streamInFocus);
            SetStreamMuteInternal(streamInFocus, false, true);
            return;
        }
        int32_t volumeLevelInInt = GetSystemVolumeLevelInternal(streamInFocus, false);
        if (MaxOrMinVolumeOption(volumeLevelInInt, keyType, streamInFocus)) {
            return;
        }

        volumeLevelInInt = (keyType == OHOS::MMI::KeyEvent::KEYCODE_VOLUME_UP) ?
            ++volumeLevelInInt : --volumeLevelInInt;
        SetSystemVolumeLevelInternal(streamInFocus, volumeLevelInInt, true);
    });
    if (keySubId < 0) {
        AUDIO_ERR_LOG("SubscribeKeyEvent: subscribing for volume key: %{public}s option failed",
            (keyType == OHOS::MMI::KeyEvent::KEYCODE_VOLUME_UP) ? "up" : "down");
    }
    return keySubId;
}
#endif

#ifdef FEATURE_MULTIMODALINPUT_INPUT
int32_t AudioPolicyServer::RegisterVolumeKeyMuteEvents()
{
    AUDIO_INFO_LOG("RegisterVolumeKeyMuteEvents: volume key: mute");
    MMI::InputManager *im = MMI::InputManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(im != nullptr, ERR_INVALID_PARAM, "Failed to obtain INPUT manager");

    std::shared_ptr<OHOS::MMI::KeyOption> keyOptionMute = std::make_shared<OHOS::MMI::KeyOption>();
    CHECK_AND_RETURN_RET_LOG(keyOptionMute != nullptr, ERR_INVALID_PARAM, "keyOptionMute: Invalid key option");
    std::set<int32_t> preKeys;
    keyOptionMute->SetPreKeys(preKeys);
    keyOptionMute->SetFinalKey(OHOS::MMI::KeyEvent::KEYCODE_VOLUME_MUTE);
    keyOptionMute->SetFinalKeyDown(true);
    keyOptionMute->SetFinalKeyDownDuration(VOLUME_MUTE_KEY_DURATION);
    int32_t muteKeySubId = im->SubscribeKeyEvent(keyOptionMute,
        [this](std::shared_ptr<MMI::KeyEvent> keyEventCallBack) {
            AUDIO_INFO_LOG("Receive volume key event: mute");
            std::lock_guard<std::mutex> lock(keyEventMutex_);
            bool isMuted = GetStreamMute(AudioStreamType::STREAM_ALL);
            SetStreamMuteInternal(AudioStreamType::STREAM_ALL, !isMuted, true);
        });
    if (muteKeySubId < 0) {
        AUDIO_ERR_LOG("SubscribeKeyEvent: subscribing for mute failed ");
    }
    return muteKeySubId;
}
#endif

#ifdef FEATURE_MULTIMODALINPUT_INPUT
void AudioPolicyServer::SubscribeVolumeKeyEvents()
{
    if (hasSubscribedVolumeKeyEvents_.load()) {
        AUDIO_INFO_LOG("SubscribeVolumeKeyEvents: volume key events has been sunscirbed!");
        return;
    }

    AUDIO_INFO_LOG("SubscribeVolumeKeyEvents: first time.");
    int32_t resultOfVolumeUp = RegisterVolumeKeyEvents(OHOS::MMI::KeyEvent::KEYCODE_VOLUME_UP);
    int32_t resultOfVolumeDown = RegisterVolumeKeyEvents(OHOS::MMI::KeyEvent::KEYCODE_VOLUME_DOWN);
    int32_t resultOfMute = RegisterVolumeKeyMuteEvents();
    if (resultOfVolumeUp >= 0 && resultOfVolumeDown >= 0 && resultOfMute >= 0) {
        hasSubscribedVolumeKeyEvents_.store(true);
    } else {
        AUDIO_ERR_LOG("SubscribeVolumeKeyEvents: failed to subscribe key events.");
        hasSubscribedVolumeKeyEvents_.store(false);
    }
}
#endif

AudioVolumeType AudioPolicyServer::GetVolumeTypeFromStreamType(AudioStreamType streamType)
{
    switch (streamType) {
        case STREAM_VOICE_CALL:
        case STREAM_VOICE_MESSAGE:
        case STREAM_VOICE_COMMUNICATION:
            return STREAM_VOICE_CALL;
        case STREAM_RING:
        case STREAM_SYSTEM:
        case STREAM_NOTIFICATION:
        case STREAM_SYSTEM_ENFORCED:
        case STREAM_DTMF:
            return STREAM_RING;
        case STREAM_MUSIC:
        case STREAM_MEDIA:
        case STREAM_MOVIE:
        case STREAM_GAME:
        case STREAM_SPEECH:
        case STREAM_NAVIGATION:
            return STREAM_MUSIC;
        case STREAM_VOICE_ASSISTANT:
            return STREAM_VOICE_ASSISTANT;
        case STREAM_ALARM:
            return STREAM_ALARM;
        case STREAM_ACCESSIBILITY:
            return STREAM_ACCESSIBILITY;
        case STREAM_ULTRASONIC:
            return STREAM_ULTRASONIC;
        case STREAM_ALL:
            return STREAM_ALL;
        default:
            return STREAM_MUSIC;
    }
}

bool AudioPolicyServer::IsVolumeTypeValid(AudioStreamType streamType)
{
    bool result = false;
    switch (streamType) {
        case STREAM_MUSIC:
        case STREAM_RING:
        case STREAM_NOTIFICATION:
        case STREAM_VOICE_CALL:
        case STREAM_VOICE_COMMUNICATION:
        case STREAM_VOICE_ASSISTANT:
        case STREAM_ALARM:
        case STREAM_ACCESSIBILITY:
        case STREAM_ULTRASONIC:
        case STREAM_ALL:
            result = true;
            break;
        default:
            result = false;
            AUDIO_ERR_LOG("IsVolumeTypeValid: streamType[%{public}d] is not supported", streamType);
            break;
    }
    return result;
}

bool AudioPolicyServer::IsVolumeLevelValid(AudioStreamType streamType, int32_t volumeLevel)
{
    bool result = true;
    if (volumeLevel < audioPolicyService_.GetMinVolumeLevel(streamType) ||
        volumeLevel > audioPolicyService_.GetMaxVolumeLevel(streamType)) {
        AUDIO_ERR_LOG("IsVolumeLevelValid: volumeLevel[%{public}d] is out of valid range for streamType[%{public}d]",
            volumeLevel, streamType);
        result = false;
    }
    return result;
}

void AudioPolicyServer::SubscribePowerStateChangeEvents()
{
    sptr<PowerMgr::IPowerStateCallback> powerStateCallback_;

    if (powerStateCallback_ == nullptr) {
        powerStateCallback_ = new (std::nothrow) AudioPolicyServerPowerStateCallback(this);
    }

    if (powerStateCallback_ == nullptr) {
        AUDIO_ERR_LOG("subscribe create power state callback Create Error");
        return;
    }

    bool RegisterSuccess = PowerMgr::PowerMgrClient::GetInstance().RegisterPowerStateCallback(powerStateCallback_);
    if (!RegisterSuccess) {
        AUDIO_ERR_LOG("register power state callback failed");
    } else {
        AUDIO_INFO_LOG("register power state callback success");
        powerStateCallbackRegister_ = true;
    }
}

void AudioPolicyServer::CheckSubscribePowerStateChange()
{
    if (powerStateCallbackRegister_) {
        return;
    }

    SubscribePowerStateChangeEvents();

    if (powerStateCallbackRegister_) {
        AUDIO_DEBUG_LOG("PowerState CallBack Register Success");
    } else {
        AUDIO_ERR_LOG("PowerState CallBack Register Failed");
    }
}

void AudioPolicyServer::OffloadStreamCheck(int64_t activateSessionId, AudioStreamType activateStreamType,
    int64_t deactivateSessionId)
{
    CheckSubscribePowerStateChange();
    if (deactivateSessionId != OFFLOAD_NO_SESSION_ID) {
        audioPolicyService_.OffloadStreamReleaseCheck(deactivateSessionId);
    }
    if (activateSessionId != OFFLOAD_NO_SESSION_ID) {
        if (activateStreamType == AudioStreamType::STREAM_MUSIC ||
            activateStreamType == AudioStreamType::STREAM_SPEECH) {
            audioPolicyService_.OffloadStreamSetCheck(activateSessionId);
        } else {
            AUDIO_DEBUG_LOG("session:%{public}d not get offload stream, type is %{public}d",
                (int32_t)activateSessionId, (int32_t)activateStreamType);
        }
    }
}

AudioPolicyServer::AudioPolicyServerPowerStateCallback::AudioPolicyServerPowerStateCallback(
    AudioPolicyServer* policyServer) : PowerMgr::PowerStateCallbackStub(), policyServer_(policyServer)
{}

void AudioPolicyServer::AudioPolicyServerPowerStateCallback::OnPowerStateChanged(PowerMgr::PowerState state)
{
    policyServer_->audioPolicyService_.HandlePowerStateChanged(state);
}

void AudioPolicyServer::InitKVStore()
{
    audioPolicyService_.InitKVStore();
}

void AudioPolicyServer::ConnectServiceAdapter()
{
    if (!audioPolicyService_.ConnectServiceAdapter()) {
        AUDIO_ERR_LOG("ConnectServiceAdapter Error in connecting to audio service adapter");
        return;
    }
}

void AudioPolicyServer::LoadEffectLibrary()
{
    audioPolicyService_.LoadEffectLibrary();
}

int32_t AudioPolicyServer::GetMaxVolumeLevel(AudioVolumeType volumeType)
{
    return audioPolicyService_.GetMaxVolumeLevel(volumeType);
}

int32_t AudioPolicyServer::GetMinVolumeLevel(AudioVolumeType volumeType)
{
    return audioPolicyService_.GetMinVolumeLevel(volumeType);
}

int32_t AudioPolicyServer::SetSystemVolumeLevel(AudioStreamType streamType, int32_t volumeLevel, API_VERSION api_v,
    int32_t volumeFlag)
{
    if (api_v == API_9 && !PermissionUtil::VerifySystemPermission()) {
        AUDIO_ERR_LOG("SetSystemVolumeLevel: No system permission");
        return ERR_PERMISSION_DENIED;
    }

    if (!IsVolumeTypeValid(streamType)) {
        return ERR_NOT_SUPPORTED;
    }
    if (!IsVolumeLevelValid(streamType, volumeLevel)) {
        return ERR_NOT_SUPPORTED;
    }

    return SetSystemVolumeLevelInternal(streamType, volumeLevel, volumeFlag == VolumeFlag::FLAG_SHOW_SYSTEM_UI);
}

int32_t AudioPolicyServer::GetSystemVolumeLevel(AudioStreamType streamType)
{
    return GetSystemVolumeLevelInternal(streamType, false);
}

int32_t AudioPolicyServer::GetSystemVolumeLevelInternal(AudioStreamType streamType, bool isFromVolumeKey)
{
    if (streamType == STREAM_ALL) {
        streamType = STREAM_MUSIC;
        AUDIO_DEBUG_LOG("GetVolume of STREAM_ALL for streamType = %{public}d ", streamType);
    }
    return audioPolicyService_.GetSystemVolumeLevel(streamType, isFromVolumeKey);
}

int32_t AudioPolicyServer::SetLowPowerVolume(int32_t streamId, float volume)
{
    auto callerUid = IPCSkeleton::GetCallingUid();
    if (callerUid != UID_FOUNDATION_SA &&
        callerUid != UID_ROOT) {
        AUDIO_ERR_LOG("SetLowPowerVolume callerUid Error: not foundation or component_schedule_service");
        return ERROR;
    }
    return audioPolicyService_.SetLowPowerVolume(streamId, volume);
}

float AudioPolicyServer::GetLowPowerVolume(int32_t streamId)
{
    return audioPolicyService_.GetLowPowerVolume(streamId);
}

float AudioPolicyServer::GetSingleStreamVolume(int32_t streamId)
{
    return audioPolicyService_.GetSingleStreamVolume(streamId);
}

bool AudioPolicyServer::IsVolumeUnadjustable()
{
    return audioPolicyService_.IsVolumeUnadjustable();
}

int32_t AudioPolicyServer::AdjustVolumeByStep(VolumeAdjustType adjustType)
{
    if (!PermissionUtil::VerifySystemPermission()) {
        AUDIO_ERR_LOG("AdjustVolumeByStep: No system permission");
        return ERR_PERMISSION_DENIED;
    }

    AudioStreamType streamInFocus = GetVolumeTypeFromStreamType(GetStreamInFocus());
    if (streamInFocus == AudioStreamType::STREAM_DEFAULT) {
        streamInFocus = AudioStreamType::STREAM_MUSIC;
    }

    int32_t volumeLevelInInt = GetSystemVolumeLevel(streamInFocus);
    int32_t ret = ERROR;
    if (adjustType == VolumeAdjustType::VOLUME_UP) {
        ret = SetSystemVolumeLevelInternal(streamInFocus, volumeLevelInInt + volumeStep_, false);
        AUDIO_INFO_LOG("AdjustVolumeByStep Up, VolumeLevel is %{public}d", GetSystemVolumeLevel(streamInFocus));
    }

    if (adjustType == VolumeAdjustType::VOLUME_DOWN) {
        ret = SetSystemVolumeLevelInternal(streamInFocus, volumeLevelInInt - volumeStep_, false);
        AUDIO_INFO_LOG("AdjustVolumeByStep Down, VolumeLevel is %{public}d", GetSystemVolumeLevel(streamInFocus));
    }
    return ret;
}

int32_t AudioPolicyServer::AdjustSystemVolumeByStep(AudioVolumeType volumeType, VolumeAdjustType adjustType)
{
    if (!PermissionUtil::VerifySystemPermission()) {
        AUDIO_ERR_LOG("AdjustSystemVolumeByStep: No system permission");
        return ERR_PERMISSION_DENIED;
    }

    int32_t volumeLevelInInt = GetSystemVolumeLevel(volumeType);
    int32_t ret = ERROR;

    if (adjustType == VolumeAdjustType::VOLUME_UP) {
        ret = SetSystemVolumeLevelInternal(volumeType, volumeLevelInInt + volumeStep_, false);
        AUDIO_INFO_LOG("AdjustSystemVolumeByStep Up, VolumeLevel:%{public}d", GetSystemVolumeLevel(volumeType));
    }

    if (adjustType == VolumeAdjustType::VOLUME_DOWN) {
        ret = SetSystemVolumeLevelInternal(volumeType, volumeLevelInInt - volumeStep_, false);
        AUDIO_INFO_LOG("AdjustSystemVolumeByStep Down, VolumeLevel:%{public}d", GetSystemVolumeLevel(volumeType));
    }
    return ret;
}

float AudioPolicyServer::GetSystemVolumeInDb(AudioVolumeType volumeType, int32_t volumeLevel, DeviceType deviceType)
{
    if (!IsVolumeTypeValid(volumeType)) {
        return static_cast<float>(ERR_INVALID_PARAM);
    }
    if (!IsVolumeLevelValid(volumeType, volumeLevel)) {
        return static_cast<float>(ERR_INVALID_PARAM);
    }

    return audioPolicyService_.GetSystemVolumeInDb(volumeType, volumeLevel, deviceType);
}

int32_t AudioPolicyServer::SetStreamMute(AudioStreamType streamType, bool mute, API_VERSION api_v)
{
    if (api_v == API_9 && !PermissionUtil::VerifySystemPermission()) {
        AUDIO_ERR_LOG("SetStreamMute: No system permission");
        return ERR_PERMISSION_DENIED;
    }

    return SetStreamMuteInternal(streamType, mute, false);
}

int32_t AudioPolicyServer::SetStreamMuteInternal(AudioStreamType streamType, bool mute, bool isUpdateUi)
{
    AUDIO_INFO_LOG("SetStreamMuteInternal streamType: %{public}d, mute: %{public}d, updateUi: %{public}d",
        streamType, mute, isUpdateUi);

    if (streamType == STREAM_ALL) {
        for (auto audioStreamType : GET_STREAM_ALL_VOLUME_TYPES) {
            AUDIO_INFO_LOG("SetMute of STREAM_ALL for StreamType = %{public}d ", audioStreamType);
            int32_t setResult = SetSingleStreamMute(audioStreamType, mute, isUpdateUi);
            if (setResult != SUCCESS) {
                return setResult;
            }
        }
        return SUCCESS;
    }

    return SetSingleStreamMute(streamType, mute, isUpdateUi);
}

int32_t AudioPolicyServer::SetSingleStreamMute(AudioStreamType streamType, bool mute, bool isUpdateUi)
{
    if (streamType == AudioStreamType::STREAM_RING && !isUpdateUi) {
        if (!VerifyPermission(ACCESS_NOTIFICATION_POLICY_PERMISSION)) {
            AUDIO_ERR_LOG("SetStreamMute permission denied for stream type : %{public}d", streamType);
            return ERR_PERMISSION_DENIED;
        }
    }

    int result = audioPolicyService_.SetStreamMute(streamType, mute);
    VolumeEvent volumeEvent;
    volumeEvent.volumeType = streamType;
    volumeEvent.volume = GetSystemVolumeLevel(streamType);
    volumeEvent.updateUi = isUpdateUi;
    volumeEvent.volumeGroupId = 0;
    volumeEvent.networkId = LOCAL_NETWORK_ID;
    if (audioPolicyServerHandler_ != nullptr) {
        audioPolicyServerHandler_->SendVolumeKeyEventCallback(volumeEvent);
    }
    return result;
}

float AudioPolicyServer::GetSystemVolumeDb(AudioStreamType streamType)
{
    return audioPolicyService_.GetSystemVolumeDb(streamType);
}

int32_t AudioPolicyServer::SetSystemVolumeLevelInternal(AudioStreamType streamType, int32_t volumeLevel,
    bool isUpdateUi)
{
    AUDIO_INFO_LOG("SetSystemVolumeLevelInternal streamType: %{public}d, volumeLevel: %{public}d, updateUi: %{public}d",
        streamType, volumeLevel, isUpdateUi);
    if (IsVolumeUnadjustable()) {
        AUDIO_ERR_LOG("Unadjustable device, not allow set volume");
        return ERR_OPERATION_FAILED;
    }
    if (streamType == STREAM_ALL) {
        for (auto audioSteamType : GET_STREAM_ALL_VOLUME_TYPES) {
            AUDIO_INFO_LOG("SetVolume of STREAM_ALL, SteamType = %{public}d ", audioSteamType);
            int32_t setResult = SetSingleStreamVolume(audioSteamType, volumeLevel, isUpdateUi);
            if (setResult != SUCCESS) {
                return setResult;
            }
        }
        return SUCCESS;
    }
    return SetSingleStreamVolume(streamType, volumeLevel, isUpdateUi);
}

int32_t AudioPolicyServer::SetSingleStreamVolume(AudioStreamType streamType, int32_t volumeLevel, bool isUpdateUi)
{
    if ((streamType == AudioStreamType::STREAM_RING) && !isUpdateUi) {
        int32_t curRingVolumeLevel = GetSystemVolumeLevel(STREAM_RING);
        if ((curRingVolumeLevel > 0 && volumeLevel == 0) || (curRingVolumeLevel == 0 && volumeLevel > 0)) {
            if (!VerifyPermission(ACCESS_NOTIFICATION_POLICY_PERMISSION)) {
                AUDIO_ERR_LOG("Access policy permission denied for volume type : %{public}d", streamType);
                return ERR_PERMISSION_DENIED;
            }
        }
    }

    int ret = audioPolicyService_.SetSystemVolumeLevel(streamType, volumeLevel, isUpdateUi);
    VolumeEvent volumeEvent;
    volumeEvent.volumeType = streamType;
    volumeEvent.volume = GetSystemVolumeLevel(streamType);
    volumeEvent.updateUi = isUpdateUi;
    volumeEvent.volumeGroupId = 0;
    volumeEvent.networkId = LOCAL_NETWORK_ID;
    if (audioPolicyServerHandler_ != nullptr) {
        audioPolicyServerHandler_->SendVolumeKeyEventCallback(volumeEvent);
    }
    return ret;
}

bool AudioPolicyServer::GetStreamMute(AudioStreamType streamType)
{
    if (streamType == AudioStreamType::STREAM_RING) {
        bool ret = VerifyPermission(ACCESS_NOTIFICATION_POLICY_PERMISSION);
        CHECK_AND_RETURN_RET_LOG(ret, false,
            "GetStreamMute permission denied for stream type : %{public}d", streamType);
    }

    return GetStreamMuteInternal(streamType);
}

bool AudioPolicyServer::GetStreamMuteInternal(AudioStreamType streamType)
{
    if (streamType == STREAM_ALL) {
        streamType = STREAM_MUSIC;
        AUDIO_INFO_LOG("GetStreamMute of STREAM_ALL for streamType = %{public}d ", streamType);
    }
    return audioPolicyService_.GetStreamMute(streamType);
}

int32_t AudioPolicyServer::SelectOutputDevice(sptr<AudioRendererFilter> audioRendererFilter,
    std::vector<sptr<AudioDeviceDescriptor>> audioDeviceDescriptors)
{
    CHECK_AND_RETURN_RET_LOG(PermissionUtil::VerifySystemPermission(), ERR_PERMISSION_DENIED,
        "SelectOutputDevice: No system permission");

    return audioPolicyService_.SelectOutputDevice(audioRendererFilter, audioDeviceDescriptors);
}

std::string AudioPolicyServer::GetSelectedDeviceInfo(int32_t uid, int32_t pid, AudioStreamType streamType)
{
    return audioPolicyService_.GetSelectedDeviceInfo(uid, pid, streamType);
}

int32_t AudioPolicyServer::SelectInputDevice(sptr<AudioCapturerFilter> audioCapturerFilter,
    std::vector<sptr<AudioDeviceDescriptor>> audioDeviceDescriptors)
{
    CHECK_AND_RETURN_RET_LOG(PermissionUtil::VerifySystemPermission(), ERR_PERMISSION_DENIED,
        "SelectInputDevice: No system permission");
    int32_t ret = audioPolicyService_.SelectInputDevice(audioCapturerFilter, audioDeviceDescriptors);
    return ret;
}

std::vector<sptr<AudioDeviceDescriptor>> AudioPolicyServer::GetDevices(DeviceFlag deviceFlag)
{
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    switch (deviceFlag) {
        case NONE_DEVICES_FLAG:
        case DISTRIBUTED_OUTPUT_DEVICES_FLAG:
        case DISTRIBUTED_INPUT_DEVICES_FLAG:
        case ALL_DISTRIBUTED_DEVICES_FLAG:
        case ALL_L_D_DEVICES_FLAG:
            if (!hasSystemPermission) {
                AUDIO_ERR_LOG("GetDevices: No system permission");
                std::vector<sptr<AudioDeviceDescriptor>> info = {};
                return info;
            }
            break;
        default:
            break;
    }

    std::vector<sptr<AudioDeviceDescriptor>> deviceDescs = audioPolicyService_.GetDevices(deviceFlag);

    if (!hasSystemPermission) {
        for (sptr<AudioDeviceDescriptor> desc : deviceDescs) {
            desc->networkId_ = "";
            desc->interruptGroupId_ = GROUP_ID_NONE;
            desc->volumeGroupId_ = GROUP_ID_NONE;
        }
    }

    bool hasBTPermission = VerifyPermission(USE_BLUETOOTH_PERMISSION);
    if (!hasBTPermission) {
        audioPolicyService_.UpdateDescWhenNoBTPermission(deviceDescs);
    }

    return deviceDescs;
}

int32_t AudioPolicyServer::NotifyCapturerAdded(AudioCapturerInfo capturerInfo, AudioStreamInfo streamInfo,
    uint32_t sessionId)
{
    auto callerUid = IPCSkeleton::GetCallingUid();
    // Temporarily allow only media service to use non-IPC route
    CHECK_AND_RETURN_RET_LOG(callerUid == MEDIA_SERVICE_UID, ERR_PERMISSION_DENIED, "No permission");

    return audioPolicyService_.NotifyCapturerAdded(capturerInfo, streamInfo, sessionId);
}

int32_t AudioPolicyServer::VerifyVoiceCallPermission(uint64_t fullTokenId, Security::AccessToken::AccessTokenID tokenId)
{
    bool hasSystemPermission = TokenIdKit::IsSystemAppByFullTokenID(fullTokenId);
    CHECK_AND_RETURN_RET_LOG(hasSystemPermission, ERR_PERMISSION_DENIED, "No system permission");

    bool hasRecordVoiceCallPermission = VerifyPermission(RECORD_VOICE_CALL_PERMISSION, tokenId, true);
    CHECK_AND_RETURN_RET_LOG(hasRecordVoiceCallPermission, ERR_PERMISSION_DENIED, "No permission");
    return SUCCESS;
}

std::vector<sptr<AudioDeviceDescriptor>> AudioPolicyServer::GetPreferredOutputDeviceDescriptors(
    AudioRendererInfo &rendererInfo)
{
    std::vector<sptr<AudioDeviceDescriptor>> deviceDescs =
        audioPolicyService_.GetPreferredOutputDeviceDescriptors(rendererInfo);
    bool hasBTPermission = VerifyPermission(USE_BLUETOOTH_PERMISSION);
    if (!hasBTPermission) {
        audioPolicyService_.UpdateDescWhenNoBTPermission(deviceDescs);
    }

    return deviceDescs;
}

std::vector<sptr<AudioDeviceDescriptor>> AudioPolicyServer::GetPreferredInputDeviceDescriptors(
    AudioCapturerInfo &captureInfo)
{
    std::vector<sptr<AudioDeviceDescriptor>> deviceDescs =
        audioPolicyService_.GetPreferredInputDeviceDescriptors(captureInfo);
    bool hasBTPermission = VerifyPermission(USE_BLUETOOTH_PERMISSION);
    if (!hasBTPermission) {
        audioPolicyService_.UpdateDescWhenNoBTPermission(deviceDescs);
    }

    return deviceDescs;
}

bool AudioPolicyServer::IsStreamActive(AudioStreamType streamType)
{
    return audioPolicyService_.IsStreamActive(streamType);
}

int32_t AudioPolicyServer::SetDeviceActive(InternalDeviceType deviceType, bool active)
{
    return audioPolicyService_.SetDeviceActive(deviceType, active);
}

bool AudioPolicyServer::IsDeviceActive(InternalDeviceType deviceType)
{
    return audioPolicyService_.IsDeviceActive(deviceType);
}

InternalDeviceType AudioPolicyServer::GetActiveOutputDevice()
{
    return audioPolicyService_.GetActiveOutputDevice();
}

InternalDeviceType AudioPolicyServer::GetActiveInputDevice()
{
    return audioPolicyService_.GetActiveInputDevice();
}

int32_t AudioPolicyServer::SetRingerMode(AudioRingerMode ringMode, API_VERSION api_v)
{
    CHECK_AND_RETURN_RET_LOG(api_v != API_9 || PermissionUtil::VerifySystemPermission(),
        ERR_PERMISSION_DENIED, "No system permission");
    bool isPermissionRequired = false;

    if (ringMode == AudioRingerMode::RINGER_MODE_SILENT) {
        isPermissionRequired = true;
    } else {
        AudioRingerMode currentRingerMode = GetRingerMode();
        if (currentRingerMode == AudioRingerMode::RINGER_MODE_SILENT) {
            isPermissionRequired = true;
        }
    }

    if (isPermissionRequired) {
        bool result = VerifyPermission(ACCESS_NOTIFICATION_POLICY_PERMISSION);
        CHECK_AND_RETURN_RET_LOG(result, ERR_PERMISSION_DENIED,
            "Access policy permission denied for ringerMode : %{public}d", ringMode);
    }

    int32_t ret = audioPolicyService_.SetRingerMode(ringMode);
    if ((ret == SUCCESS) && (audioPolicyServerHandler_ != nullptr)) {
        audioPolicyServerHandler_->SendRingerModeUpdatedCallback(ringMode);
    }

    return ret;
}

#ifdef FEATURE_DTMF_TONE
std::shared_ptr<ToneInfo> AudioPolicyServer::GetToneConfig(int32_t ltonetype)
{
    return audioPolicyService_.GetToneConfig(ltonetype);
}

std::vector<int32_t> AudioPolicyServer::GetSupportedTones()
{
    return audioPolicyService_.GetSupportedTones();
}
#endif

void AudioPolicyServer::InitMicrophoneMute()
{
    if (system::GetBoolParameter("persist.edm.mic_disable", false)) {
        AUDIO_INFO_LOG("Entered %{public}s", __func__);
        bool isMute = true;
        bool isMicrophoneMute = audioPolicyService_.IsMicrophoneMute();
        int32_t ret = audioPolicyService_.SetMicrophoneMute(isMute);
        if (ret == SUCCESS && isMicrophoneMute != isMute && audioPolicyServerHandler_ != nullptr) {
            MicStateChangeEvent micStateChangeEvent;
            micStateChangeEvent.mute = isMute;
            audioPolicyServerHandler_->SendMicStateUpdatedCallback(micStateChangeEvent);
        }
        if (ret != SUCCESS) {
            AUDIO_ERR_LOG("InitMicrophoneMute EDM SetMicrophoneMute result %{public}d", ret);
        }
    }
}

int32_t AudioPolicyServer::SetMicrophoneMuteCommon(bool isMute, API_VERSION api_v)
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    std::lock_guard<std::mutex> lock(micStateChangeMutex_);
    auto callerUid = IPCSkeleton::GetCallingUid();
    if (!isMute && callerUid != EDM_SERVICE_UID && system::GetBoolParameter("persist.edm.mic_disable", false)) {
        AUDIO_ERR_LOG("set microphone mute failed cause feature is disabled by edm");
        return ERR_MICROPHONE_DISABLED_BY_EDM;
    }
    bool isMicrophoneMute = IsMicrophoneMute(api_v);
    int32_t ret = audioPolicyService_.SetMicrophoneMute(isMute);
    if (ret == SUCCESS && isMicrophoneMute != isMute && audioPolicyServerHandler_ != nullptr) {
        MicStateChangeEvent micStateChangeEvent;
        micStateChangeEvent.mute = isMute;
        audioPolicyServerHandler_->SendMicStateUpdatedCallback(micStateChangeEvent);
    }
    return ret;
}

int32_t AudioPolicyServer::SetMicrophoneMute(bool isMute)
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    bool ret = VerifyPermission(MICROPHONE_PERMISSION);
    CHECK_AND_RETURN_RET_LOG(ret, ERR_PERMISSION_DENIED,
        "MICROPHONE permission denied");
    return SetMicrophoneMuteCommon(isMute, API_7);
}

int32_t AudioPolicyServer::SetMicrophoneMuteAudioConfig(bool isMute)
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    bool ret = VerifyPermission(MANAGE_AUDIO_CONFIG);
    CHECK_AND_RETURN_RET_LOG(ret, ERR_PERMISSION_DENIED,
        "MANAGE_AUDIO_CONFIG permission denied");
    return SetMicrophoneMuteCommon(isMute, API_9);
}

bool AudioPolicyServer::IsMicrophoneMute(API_VERSION api_v)
{
    bool ret = VerifyPermission(MICROPHONE_PERMISSION);
    CHECK_AND_RETURN_RET_LOG(api_v != API_7 || ret, ERR_PERMISSION_DENIED,
        "MICROPHONE permission denied");

    return audioPolicyService_.IsMicrophoneMute();
}

AudioRingerMode AudioPolicyServer::GetRingerMode()
{
    return audioPolicyService_.GetRingerMode();
}

int32_t AudioPolicyServer::SetAudioScene(AudioScene audioScene)
{
    CHECK_AND_RETURN_RET_LOG(audioScene > AUDIO_SCENE_INVALID && audioScene < AUDIO_SCENE_MAX,
        ERR_INVALID_PARAM, "param is invalid");
    if (audioScene == AUDIO_SCENE_CALL_START) {
        AUDIO_INFO_LOG("SetAudioScene, AUDIO_SCENE_CALL_START means voip start.");
        isAvSessionSetVoipStart = true;
        return SUCCESS;
    }
    if (audioScene == AUDIO_SCENE_CALL_END) {
        AUDIO_INFO_LOG("SetAudioScene, AUDIO_SCENE_CALL_END means voip end, need set AUDIO_SCENE_DEFAULT.");
        isAvSessionSetVoipStart = false;
        return audioPolicyService_.SetAudioScene(AUDIO_SCENE_DEFAULT);
    }
    bool ret = PermissionUtil::VerifySystemPermission();
    CHECK_AND_RETURN_RET_LOG(ret, ERR_PERMISSION_DENIED, "No system permission");
    return audioPolicyService_.SetAudioScene(audioScene);
}

int32_t AudioPolicyServer::SetAudioSceneInternal(AudioScene audioScene)
{
    return audioPolicyService_.SetAudioScene(audioScene);
}

AudioScene AudioPolicyServer::GetAudioScene()
{
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    return audioPolicyService_.GetAudioScene(hasSystemPermission);
}

int32_t AudioPolicyServer::SetAudioInterruptCallback(const uint32_t sessionID, const sptr<IRemoteObject> &object,
    const int32_t zoneID)
{
    if (interruptService_ != nullptr) {
        return interruptService_->SetAudioInterruptCallback(zoneID, sessionID, object);
    }
    return ERR_UNKNOWN;
}

int32_t AudioPolicyServer::UnsetAudioInterruptCallback(const uint32_t sessionID, const int32_t zoneID)
{
    if (interruptService_ != nullptr) {
        return interruptService_->UnsetAudioInterruptCallback(zoneID, sessionID);
    }
    return ERR_UNKNOWN;
}

int32_t AudioPolicyServer::SetAudioManagerInterruptCallback(const int32_t /* clientId */,
                                                            const sptr<IRemoteObject> &object)
{
    if (interruptService_ != nullptr) {
        return interruptService_->SetAudioManagerInterruptCallback(object);
    }
    return ERR_UNKNOWN;
}

int32_t AudioPolicyServer::UnsetAudioManagerInterruptCallback(const int32_t /* clientId */)
{
    if (interruptService_ != nullptr) {
        return interruptService_->UnsetAudioManagerInterruptCallback();
    }
    return ERR_UNKNOWN;
}

int32_t AudioPolicyServer::RequestAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt)
{
    if (interruptService_ != nullptr) {
        return interruptService_->RequestAudioFocus(clientId, audioInterrupt);
    }
    return ERR_UNKNOWN;
}

int32_t AudioPolicyServer::AbandonAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt)
{
    if (interruptService_ != nullptr) {
        return interruptService_->AbandonAudioFocus(clientId, audioInterrupt);
    }
    return ERR_UNKNOWN;
}

int32_t AudioPolicyServer::ActivateAudioInterrupt(const AudioInterrupt &audioInterrupt, const int32_t zoneID)
{
    if (interruptService_ != nullptr) {
        return interruptService_->ActivateAudioInterrupt(zoneID, audioInterrupt);
    }
    return ERR_UNKNOWN;
}

int32_t AudioPolicyServer::DeactivateAudioInterrupt(const AudioInterrupt &audioInterrupt, const int32_t zoneID)
{
    if (interruptService_ != nullptr) {
        return interruptService_->DeactivateAudioInterrupt(zoneID, audioInterrupt);
    }
    return ERR_UNKNOWN;
}

void AudioPolicyServer::OnSessionRemoved(const uint64_t sessionID)
{
    CHECK_AND_RETURN_LOG(audioPolicyServerHandler_ != nullptr, "audioPolicyServerHandler_ is nullptr");
    audioPolicyServerHandler_->SendCapturerRemovedEvent(sessionID, false);
}

void AudioPolicyServer::ProcessSessionRemoved(const uint64_t sessionID, const int32_t zoneID)
{
    AUDIO_DEBUG_LOG("Removed SessionId: %{public}" PRIu64, sessionID);
}

void AudioPolicyServer::ProcessSessionAdded(SessionEvent sessionEvent)
{
    AUDIO_DEBUG_LOG("Added Session");
}

void AudioPolicyServer::ProcessorCloseWakeupSource(const uint64_t sessionID)
{
    audioPolicyService_.CloseWakeUpAudioCapturer();
}

void AudioPolicyServer::OnPlaybackCapturerStop()
{
    audioPolicyService_.UnloadLoopback();
}

AudioStreamType AudioPolicyServer::GetStreamInFocus(const int32_t zoneID)
{
    if (interruptService_ != nullptr) {
        return interruptService_->GetStreamInFocus(zoneID);
    }
    return STREAM_MUSIC;
}

int32_t AudioPolicyServer::GetSessionInfoInFocus(AudioInterrupt &audioInterrupt, const int32_t zoneID)
{
    if (interruptService_ != nullptr) {
        return interruptService_->GetSessionInfoInFocus(audioInterrupt, zoneID);
    }
    return ERR_UNKNOWN;
}

int32_t AudioPolicyServer::GetAudioFocusInfoList(std::list<std::pair<AudioInterrupt, AudioFocuState>> &focusInfoList,
    const int32_t zoneID)
{
    if (interruptService_ != nullptr) {
        return interruptService_->GetAudioFocusInfoList(zoneID, focusInfoList);
    }
    return ERR_UNKNOWN;
}

bool AudioPolicyServer::CheckRootCalling(uid_t callingUid, int32_t appUid)
{
    if (callingUid == UID_ROOT) {
        return true;
    }

    // check original caller if it pass
    if (std::count(RECORD_PASS_APPINFO_LIST.begin(), RECORD_PASS_APPINFO_LIST.end(), callingUid) > 0) {
        if (appUid == UID_ROOT) {
            return true;
        }
    }

    return false;
}

bool AudioPolicyServer::CheckRecordingCreate(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid,
    SourceType sourceType)
{
    uid_t callingUid = IPCSkeleton::GetCallingUid();
    uint32_t callingTokenId = IPCSkeleton::GetCallingTokenID();
    uint64_t callingFullTokenId = IPCSkeleton::GetCallingFullTokenID();

    if (CheckRootCalling(callingUid, appUid)) {
        AUDIO_INFO_LOG("root user recording");
        return true;
    }

    Security::AccessToken::AccessTokenID targetTokenId = GetTargetTokenId(callingUid, callingTokenId, appTokenId);
    uint64_t targetFullTokenId = GetTargetFullTokenId(callingUid, callingFullTokenId, appFullTokenId);
    if (sourceType == SOURCE_TYPE_VOICE_CALL) {
        if (VerifyVoiceCallPermission(targetFullTokenId, targetTokenId) != SUCCESS) {
            return false;
        }
        return true;
    }

    if (!VerifyPermission(MICROPHONE_PERMISSION, targetTokenId, true)) {
        return false;
    }

    if (!CheckAppBackgroundPermission(callingUid, targetFullTokenId, targetTokenId)) {
        return false;
    }

    return true;
}

bool AudioPolicyServer::VerifyPermission(const std::string &permissionName, uint32_t tokenId, bool isRecording)
{
    AUDIO_DEBUG_LOG("Verify permission [%{public}s]", permissionName.c_str());

    if (!isRecording) {
        // root user case for auto test
        uid_t callingUid = IPCSkeleton::GetCallingUid();
        if (callingUid == UID_ROOT) {
            return true;
        }

        tokenId = IPCSkeleton::GetCallingTokenID();
    }

    int res = Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenId, permissionName);
    CHECK_AND_RETURN_RET_LOG(res == Security::AccessToken::PermissionState::PERMISSION_GRANTED,
        false, "Permission denied [%{public}s]", permissionName.c_str());

    return true;
}

bool AudioPolicyServer::CheckRecordingStateChange(uint32_t appTokenId, uint64_t appFullTokenId, int32_t appUid,
    AudioPermissionState state)
{
    uid_t callingUid = IPCSkeleton::GetCallingUid();
    uint32_t callingTokenId = IPCSkeleton::GetCallingTokenID();
    uint64_t callingFullTokenId = IPCSkeleton::GetCallingFullTokenID();
    Security::AccessToken::AccessTokenID targetTokenId = GetTargetTokenId(callingUid, callingTokenId, appTokenId);
    uint64_t targetFullTokenId = GetTargetFullTokenId(callingUid, callingFullTokenId, appFullTokenId);

    // start recording need to check app state
    if (state == AUDIO_PERMISSION_START && !CheckRootCalling(callingUid, appUid)) {
        if (!CheckAppBackgroundPermission(callingUid, targetFullTokenId, targetTokenId)) {
            return false;
        }
    }

    NotifyPrivacy(targetTokenId, state);
    return true;
}

void AudioPolicyServer::NotifyPrivacy(uint32_t targetTokenId, AudioPermissionState state)
{
    if (state == AUDIO_PERMISSION_START) {
        int res = PrivacyKit::StartUsingPermission(targetTokenId, MICROPHONE_PERMISSION);
        if (res != 0) {
            AUDIO_WARNING_LOG("notice start using perm error");
        }
        res = PrivacyKit::AddPermissionUsedRecord(targetTokenId, MICROPHONE_PERMISSION, 1, 0);
        if (res != 0) {
            AUDIO_WARNING_LOG("add mic record error");
        }
    } else {
        int res = PrivacyKit::StopUsingPermission(targetTokenId, MICROPHONE_PERMISSION);
        if (res != 0) {
            AUDIO_WARNING_LOG("notice stop using perm error");
        }

        saveAppCapTokenIdThroughMS.erase(targetTokenId);
    }
}

bool AudioPolicyServer::CheckAppBackgroundPermission(uid_t callingUid, uint64_t targetFullTokenId,
    uint32_t targetTokenId)
{
    if (TokenIdKit::IsSystemAppByFullTokenID(targetFullTokenId)) {
        AUDIO_INFO_LOG("system app recording");
        return true;
    }
    if (std::count(RECORD_ALLOW_BACKGROUND_LIST.begin(), RECORD_ALLOW_BACKGROUND_LIST.end(), callingUid) > 0) {
        AUDIO_INFO_LOG("internal sa user directly recording");
        return true;
    }
    return PrivacyKit::IsAllowedUsingPermission(targetTokenId, MICROPHONE_PERMISSION);
}

Security::AccessToken::AccessTokenID AudioPolicyServer::GetTargetTokenId(uid_t callingUid, uint32_t callingTokenId,
    uint32_t appTokenId)
{
    if (RECORD_CHECK_FORWARD_LIST.count(callingUid)) {
        AUDIO_INFO_LOG("check forward TokenId with callingUid:%{public}d", callingUid);
        return IPCSkeleton::GetFirstTokenID();
    }
    return (std::count(RECORD_PASS_APPINFO_LIST.begin(), RECORD_PASS_APPINFO_LIST.end(), callingUid) > 0) ?
        appTokenId : callingTokenId;
}

uint64_t AudioPolicyServer::GetTargetFullTokenId(uid_t callingUid, uint64_t callingFullTokenId,
    uint64_t appFullTokenId)
{
    if (RECORD_CHECK_FORWARD_LIST.count(callingUid)) {
        AUDIO_INFO_LOG("check forward FullTokenId with callingUid:%{public}d", callingUid);
        return IPCSkeleton::GetFirstFullTokenID();
    }
    return (std::count(RECORD_PASS_APPINFO_LIST.begin(), RECORD_PASS_APPINFO_LIST.end(), callingUid) > 0) ?
        appFullTokenId : callingFullTokenId;
}

int32_t AudioPolicyServer::ReconfigureAudioChannel(const uint32_t &count, DeviceType deviceType)
{
    // Only root users should have access to this api
    if (ROOT_UID != IPCSkeleton::GetCallingUid()) {
        AUDIO_INFO_LOG("Unautorized user. Cannot modify channel");
        return ERR_PERMISSION_DENIED;
    }

    return audioPolicyService_.ReconfigureAudioChannel(count, deviceType);
}

void AudioPolicyServer::GetPolicyData(PolicyData &policyData)
{
    policyData.ringerMode = GetRingerMode();
    policyData.callStatus = GetAudioScene();

    // Get stream volumes
    for (int stream = AudioStreamType::STREAM_VOICE_CALL; stream <= AudioStreamType::STREAM_TYPE_MAX; stream++) {
        AudioStreamType streamType = (AudioStreamType)stream;

        if (AudioServiceDump::IsStreamSupported(streamType)) {
            int32_t volume = GetSystemVolumeLevel(streamType);
            policyData.streamVolumes.insert({ streamType, volume });
        }
    }

    if (interruptService_ != nullptr) {
        interruptService_->AddDumpInfo(policyData);
    }
    GetDeviceInfo(policyData);
    GetGroupInfo(policyData);
    GetStreamVolumeInfoMap(policyData.streamVolumeInfos);
    policyData.availableMicrophones = GetAvailableMicrophones();
    // Get Audio Effect Manager Information
    audioPolicyService_.GetEffectManagerInfo(policyData.oriEffectConfig, policyData.availableEffects);
    audioPolicyService_.GetAudioAdapterInfos(policyData.adapterInfoMap);
    audioPolicyService_.GetVolumeGroupData(policyData.volumeGroupData);
    audioPolicyService_.GetInterruptGroupData(policyData.interruptGroupData);
}

void AudioPolicyServer::GetStreamVolumeInfoMap(StreamVolumeInfoMap& streamVolumeInfos)
{
    audioPolicyService_.GetStreamVolumeInfoMap(streamVolumeInfos);
}

void AudioPolicyServer::GetDeviceInfo(PolicyData& policyData)
{
    DeviceFlag deviceFlag = DeviceFlag::INPUT_DEVICES_FLAG;
    std::vector<sptr<AudioDeviceDescriptor>> audioDeviceDescriptors = GetDevices(deviceFlag);

    for (auto it = audioDeviceDescriptors.begin(); it != audioDeviceDescriptors.end(); it++) {
        AudioDeviceDescriptor audioDeviceDescriptor = **it;
        DevicesInfo deviceInfo;
        deviceInfo.deviceType = audioDeviceDescriptor.deviceType_;
        deviceInfo.deviceRole = audioDeviceDescriptor.deviceRole_;
        deviceInfo.conneceType  = CONNECT_TYPE_LOCAL;
        policyData.inputDevices.push_back(deviceInfo);
    }

    deviceFlag = DeviceFlag::OUTPUT_DEVICES_FLAG;
    audioDeviceDescriptors = GetDevices(deviceFlag);

    for (auto it = audioDeviceDescriptors.begin(); it != audioDeviceDescriptors.end(); it++) {
        AudioDeviceDescriptor audioDeviceDescriptor = **it;
        DevicesInfo deviceInfo;
        deviceInfo.deviceType = audioDeviceDescriptor.deviceType_;
        deviceInfo.deviceRole = audioDeviceDescriptor.deviceRole_;
        deviceInfo.conneceType  = CONNECT_TYPE_LOCAL;
        policyData.outputDevices.push_back(deviceInfo);
    }

    deviceFlag = DeviceFlag::DISTRIBUTED_INPUT_DEVICES_FLAG;
    audioDeviceDescriptors = GetDevices(deviceFlag);

    for (auto it = audioDeviceDescriptors.begin(); it != audioDeviceDescriptors.end(); it++) {
        AudioDeviceDescriptor audioDeviceDescriptor = **it;
        DevicesInfo deviceInfo;
        deviceInfo.deviceType = audioDeviceDescriptor.deviceType_;
        deviceInfo.deviceRole = audioDeviceDescriptor.deviceRole_;
        deviceInfo.conneceType  = CONNECT_TYPE_DISTRIBUTED;
        policyData.inputDevices.push_back(deviceInfo);
    }

    deviceFlag = DeviceFlag::DISTRIBUTED_OUTPUT_DEVICES_FLAG;
    audioDeviceDescriptors = GetDevices(deviceFlag);

    for (auto it = audioDeviceDescriptors.begin(); it != audioDeviceDescriptors.end(); it++) {
        AudioDeviceDescriptor audioDeviceDescriptor = **it;
        DevicesInfo deviceInfo;
        deviceInfo.deviceType = audioDeviceDescriptor.deviceType_;
        deviceInfo.deviceRole = audioDeviceDescriptor.deviceRole_;
        deviceInfo.conneceType  = CONNECT_TYPE_DISTRIBUTED;
        policyData.outputDevices.push_back(deviceInfo);
    }

    policyData.priorityOutputDevice = GetActiveOutputDevice();
    policyData.priorityInputDevice = GetActiveInputDevice();
}

void AudioPolicyServer::GetGroupInfo(PolicyData& policyData)
{
    // Get group info
    std::vector<sptr<VolumeGroupInfo>> groupInfos = audioPolicyService_.GetVolumeGroupInfos();

    for (auto volumeGroupInfo : groupInfos) {
        GroupInfo info;
        info.groupId = volumeGroupInfo->volumeGroupId_;
        info.groupName = volumeGroupInfo->groupName_;
        info.type = volumeGroupInfo->connectType_;
        policyData.groupInfos.push_back(info);
    }
}

int32_t AudioPolicyServer::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    AUDIO_DEBUG_LOG("Dump Process Invoked");
    std::queue<std::u16string> argQue;
    for (decltype(args.size()) index = 0; index < args.size(); ++index) {
        argQue.push(args[index]);
    }
    std::string dumpString;
    PolicyData policyData;
    AudioServiceDump dumpObj;

    int32_t res = dumpObj.Initialize();
    CHECK_AND_RETURN_RET_LOG(res == AUDIO_DUMP_SUCCESS, AUDIO_DUMP_INIT_ERR,
        "Audio Service Dump Not initialised\n");

    GetPolicyData(policyData);

    dumpObj.AudioDataDump(policyData, dumpString, argQue);

    write(fd, dumpString.c_str(), dumpString.size());

    return audioPolicyService_.Dump(fd, args);
}

int32_t AudioPolicyServer::GetAudioLatencyFromXml()
{
    return audioPolicyService_.GetAudioLatencyFromXml();
}

uint32_t AudioPolicyServer::GetSinkLatencyFromXml()
{
    return audioPolicyService_.GetSinkLatencyFromXml();
}

int32_t AudioPolicyServer::RegisterTracker(AudioMode &mode, AudioStreamChangeInfo &streamChangeInfo,
    const sptr<IRemoteObject> &object)
{
    auto callerPid = IPCSkeleton::GetCallingPid();
    streamChangeInfo.audioRendererChangeInfo.callerPid = callerPid;
    streamChangeInfo.audioCapturerChangeInfo.callerPid = callerPid;

    // update the clientUid
    auto callerUid = IPCSkeleton::GetCallingUid();
    streamChangeInfo.audioRendererChangeInfo.createrUID = callerUid;
    streamChangeInfo.audioCapturerChangeInfo.createrUID = callerUid;
    AUDIO_DEBUG_LOG("RegisterTracker: [caller uid: %{public}d]", callerUid);
    if (callerUid != MEDIA_SERVICE_UID) {
        if (mode == AUDIO_MODE_PLAYBACK) {
            streamChangeInfo.audioRendererChangeInfo.clientUID = callerUid;
            AUDIO_DEBUG_LOG("Non media service caller, use the uid retrieved. ClientUID:%{public}d]",
                streamChangeInfo.audioRendererChangeInfo.clientUID);
        } else {
            streamChangeInfo.audioCapturerChangeInfo.clientUID = callerUid;
            AUDIO_DEBUG_LOG("Non media service caller, use the uid retrieved. ClientUID:%{public}d]",
                streamChangeInfo.audioCapturerChangeInfo.clientUID);
        }
    } else {
        if (mode == AUDIO_MODE_RECORD) {
            saveAppCapTokenIdThroughMS.insert(streamChangeInfo.audioCapturerChangeInfo.appTokenId);
        }
    }
    RegisterClientDeathRecipient(object, TRACKER_CLIENT);
    return audioPolicyService_.RegisterTracker(mode, streamChangeInfo, object);
}

int32_t AudioPolicyServer::UpdateTracker(AudioMode &mode, AudioStreamChangeInfo &streamChangeInfo)
{
    auto callerPid = IPCSkeleton::GetCallingPid();
    streamChangeInfo.audioRendererChangeInfo.callerPid = callerPid;
    streamChangeInfo.audioCapturerChangeInfo.callerPid = callerPid;

    // update the clientUid
    auto callerUid = IPCSkeleton::GetCallingUid();
    streamChangeInfo.audioRendererChangeInfo.createrUID = callerUid;
    streamChangeInfo.audioCapturerChangeInfo.createrUID = callerUid;
    AUDIO_DEBUG_LOG("UpdateTracker: [caller uid: %{public}d]", callerUid);
    if (callerUid != MEDIA_SERVICE_UID) {
        if (mode == AUDIO_MODE_PLAYBACK) {
            streamChangeInfo.audioRendererChangeInfo.clientUID = callerUid;
            AUDIO_DEBUG_LOG("Non media service caller, use the uid retrieved. ClientUID:%{public}d]",
                streamChangeInfo.audioRendererChangeInfo.clientUID);
        } else {
            streamChangeInfo.audioCapturerChangeInfo.clientUID = callerUid;
            AUDIO_DEBUG_LOG("Non media service caller, use the uid retrieved. ClientUID:%{public}d]",
                streamChangeInfo.audioCapturerChangeInfo.clientUID);
        }
    }
    return audioPolicyService_.UpdateTracker(mode, streamChangeInfo);
}

void AudioPolicyServer::FetchOutputDeviceForTrack(AudioStreamChangeInfo &streamChangeInfo)
{
    auto callerPid = IPCSkeleton::GetCallingPid();
    streamChangeInfo.audioRendererChangeInfo.callerPid = callerPid;

    // update the clientUid
    auto callerUid = IPCSkeleton::GetCallingUid();
    streamChangeInfo.audioRendererChangeInfo.createrUID = callerUid;
    AUDIO_DEBUG_LOG("[caller uid: %{public}d]", callerUid);
    if (callerUid != MEDIA_SERVICE_UID) {
        streamChangeInfo.audioRendererChangeInfo.clientUID = callerUid;
        AUDIO_DEBUG_LOG("Non media service caller, use the uid retrieved. ClientUID:%{public}d]",
            streamChangeInfo.audioRendererChangeInfo.clientUID);
    }
    audioPolicyService_.FetchOutputDeviceForTrack(streamChangeInfo);
}

void AudioPolicyServer::FetchInputDeviceForTrack(AudioStreamChangeInfo &streamChangeInfo)
{
    auto callerPid = IPCSkeleton::GetCallingPid();
    streamChangeInfo.audioCapturerChangeInfo.callerPid = callerPid;

    // update the clientUid
    auto callerUid = IPCSkeleton::GetCallingUid();
    streamChangeInfo.audioCapturerChangeInfo.createrUID = callerUid;
    AUDIO_DEBUG_LOG("[caller uid: %{public}d]", callerUid);
    if (callerUid != MEDIA_SERVICE_UID) {
        streamChangeInfo.audioCapturerChangeInfo.clientUID = callerUid;
        AUDIO_DEBUG_LOG("Non media service caller, use the uid retrieved. ClientUID:%{public}d]",
            streamChangeInfo.audioCapturerChangeInfo.clientUID);
    }
    audioPolicyService_.FetchInputDeviceForTrack(streamChangeInfo);
}

int32_t AudioPolicyServer::GetCurrentRendererChangeInfos(
    std::vector<unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    bool hasBTPermission = VerifyPermission(USE_BLUETOOTH_PERMISSION);
    AUDIO_DEBUG_LOG("GetCurrentRendererChangeInfos: BT use permission: %{public}d", hasBTPermission);
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    AUDIO_DEBUG_LOG("GetCurrentRendererChangeInfos: System use permission: %{public}d", hasSystemPermission);

    return audioPolicyService_.GetCurrentRendererChangeInfos(audioRendererChangeInfos,
        hasBTPermission, hasSystemPermission);
}

int32_t AudioPolicyServer::GetCurrentCapturerChangeInfos(
    std::vector<unique_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos)
{
    bool hasBTPermission = VerifyPermission(USE_BLUETOOTH_PERMISSION);
    AUDIO_DEBUG_LOG("GetCurrentCapturerChangeInfos: BT use permission: %{public}d", hasBTPermission);
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    AUDIO_DEBUG_LOG("GetCurrentCapturerChangeInfos: System use permission: %{public}d", hasSystemPermission);

    return audioPolicyService_.GetCurrentCapturerChangeInfos(audioCapturerChangeInfos,
        hasBTPermission, hasSystemPermission);
}

void AudioPolicyServer::RegisterClientDeathRecipient(const sptr<IRemoteObject> &object, DeathRecipientId id)
{
    AUDIO_DEBUG_LOG("Register clients death recipient!! RecipientId: %{public}d", id);
    std::lock_guard<std::mutex> lock(clientDiedListenerStateMutex_);
    CHECK_AND_RETURN_LOG(object != nullptr, "Client proxy obj NULL!!");

    pid_t uid = 0;
    if (id == TRACKER_CLIENT) {
        // Deliberately casting UID to pid_t
        uid = static_cast<pid_t>(IPCSkeleton::GetCallingUid());
    } else {
        uid = IPCSkeleton::GetCallingPid();
    }
    if (id == TRACKER_CLIENT && std::find(clientDiedListenerState_.begin(), clientDiedListenerState_.end(), uid)
        != clientDiedListenerState_.end()) {
        AUDIO_INFO_LOG("Tracker has been registered for %{public}d!", uid);
        return;
    }
    sptr<AudioServerDeathRecipient> deathRecipient_ = new(std::nothrow) AudioServerDeathRecipient(uid);
    if (deathRecipient_ != nullptr) {
        if (id == TRACKER_CLIENT) {
            deathRecipient_->SetNotifyCb(std::bind(&AudioPolicyServer::RegisteredTrackerClientDied,
                this, std::placeholders::_1));
        } else {
            AUDIO_INFO_LOG("RegisteredStreamListenerClientDied register!!");
            deathRecipient_->SetNotifyCb(std::bind(&AudioPolicyServer::RegisteredStreamListenerClientDied,
                this, std::placeholders::_1));
        }
        bool result = object->AddDeathRecipient(deathRecipient_);
        if (result && id == TRACKER_CLIENT) {
            clientDiedListenerState_.push_back(uid);
        }
        if (!result) {
            AUDIO_WARNING_LOG("failed to add deathRecipient");
        }
    }
}

void AudioPolicyServer::RegisteredTrackerClientDied(pid_t uid)
{
    AUDIO_INFO_LOG("RegisteredTrackerClient died: remove entry, uid %{public}d", uid);
    std::lock_guard<std::mutex> lock(clientDiedListenerStateMutex_);
    audioPolicyService_.RegisteredTrackerClientDied(uid);

    if (uid == MEDIA_SERVICE_UID) {
        for (auto iter = saveAppCapTokenIdThroughMS.begin(); iter != saveAppCapTokenIdThroughMS.end(); iter++) {
            AUDIO_DEBUG_LOG("RegisteredTrackerClient died, stop permis for appTokenId: %{public}u", *iter);
            int res = PrivacyKit::StopUsingPermission(*iter, MICROPHONE_PERMISSION);
            if (res != 0) {
                AUDIO_WARNING_LOG("media server died, notice stop using perm error");
            }
        }
        saveAppCapTokenIdThroughMS.clear();
    }
    auto filter = [&uid](int val) {
        return uid == val;
    };
    clientDiedListenerState_.erase(std::remove_if(clientDiedListenerState_.begin(), clientDiedListenerState_.end(),
        filter), clientDiedListenerState_.end());
}

void AudioPolicyServer::RegisteredStreamListenerClientDied(pid_t pid)
{
    AUDIO_INFO_LOG("RegisteredStreamListenerClient died: remove entry, uid %{public}d", pid);
    audioPolicyService_.ReduceAudioPolicyClientProxyMap(pid);
}

int32_t AudioPolicyServer::UpdateStreamState(const int32_t clientUid,
    StreamSetState streamSetState, StreamUsage streamUsage)
{
    constexpr int32_t avSessionUid = 6700; // "uid" : "av_session"
    auto callerUid = IPCSkeleton::GetCallingUid();
    // This function can only be used by av_session
    CHECK_AND_RETURN_RET_LOG(callerUid == avSessionUid, ERROR,
        "UpdateStreamState callerUid is error: not av_session");

    AUDIO_INFO_LOG("UpdateStreamState::uid:%{public}d streamSetState:%{public}d audioStreamUsage:%{public}d",
        clientUid, streamSetState, streamUsage);
    StreamSetState setState = StreamSetState::STREAM_PAUSE;
    if (streamSetState == StreamSetState::STREAM_RESUME) {
        setState  = StreamSetState::STREAM_RESUME;
    } else if (streamSetState != StreamSetState::STREAM_PAUSE) {
        AUDIO_ERR_LOG("UpdateStreamState streamSetState value is error");
        return ERROR;
    }
    StreamSetStateEventInternal setStateEvent = {};
    setStateEvent.streamSetState = setState;
    setStateEvent.streamUsage = streamUsage;

    return audioPolicyService_.UpdateStreamState(clientUid, setStateEvent);
}

int32_t AudioPolicyServer::GetVolumeGroupInfos(std::string networkId, std::vector<sptr<VolumeGroupInfo>> &infos)
{
    bool ret = PermissionUtil::VerifySystemPermission();
    CHECK_AND_RETURN_RET_LOG(ret, ERR_PERMISSION_DENIED,
        "No system permission");

    infos = audioPolicyService_.GetVolumeGroupInfos();
    auto filter = [&networkId](const sptr<VolumeGroupInfo>& info) {
        return networkId != info->networkId_;
    };
    infos.erase(std::remove_if(infos.begin(), infos.end(), filter), infos.end());

    return SUCCESS;
}

int32_t AudioPolicyServer::GetNetworkIdByGroupId(int32_t groupId, std::string &networkId)
{
    auto volumeGroupInfos = audioPolicyService_.GetVolumeGroupInfos();

    auto filter = [&groupId](const sptr<VolumeGroupInfo>& info) {
        return groupId != info->volumeGroupId_;
    };
    volumeGroupInfos.erase(std::remove_if(volumeGroupInfos.begin(), volumeGroupInfos.end(), filter),
        volumeGroupInfos.end());
    if (volumeGroupInfos.size() > 0) {
        networkId = volumeGroupInfos[0]->networkId_;
        AUDIO_INFO_LOG("GetNetworkIdByGroupId: get networkId %{public}s.", networkId.c_str());
    } else {
        AUDIO_ERR_LOG("GetNetworkIdByGroupId: has no valid group");
        return ERROR;
    }

    return SUCCESS;
}

AudioPolicyServer::RemoteParameterCallback::RemoteParameterCallback(sptr<AudioPolicyServer> server)
{
    server_ = server;
}

void AudioPolicyServer::RemoteParameterCallback::OnAudioParameterChange(const std::string networkId,
    const AudioParamKey key, const std::string& condition, const std::string& value)
{
    AUDIO_INFO_LOG("key:%{public}d, condition:%{public}s, value:%{public}s",
        key, condition.c_str(), value.c_str());
    CHECK_AND_RETURN_LOG(server_ != nullptr, "AudioPolicyServer is nullptr");
    switch (key) {
        case VOLUME:
            VolumeOnChange(networkId, condition);
            break;
        case INTERRUPT:
            InterruptOnChange(networkId, condition);
            break;
        case PARAM_KEY_STATE:
            StateOnChange(networkId, condition, value);
            break;
        default:
            AUDIO_DEBUG_LOG("[AudioPolicyServer]: No processing");
            break;
    }
}

void AudioPolicyServer::RemoteParameterCallback::VolumeOnChange(const std::string networkId,
    const std::string& condition)
{
    VolumeEvent volumeEvent;
    volumeEvent.networkId = networkId;
    char eventDes[EVENT_DES_SIZE];
    if (sscanf_s(condition.c_str(), "%[^;];AUDIO_STREAM_TYPE=%d;VOLUME_LEVEL=%d;IS_UPDATEUI=%d;VOLUME_GROUP_ID=%d;",
        eventDes, EVENT_DES_SIZE, &(volumeEvent.volumeType), &(volumeEvent.volume), &(volumeEvent.updateUi),
        &(volumeEvent.volumeGroupId)) < PARAMS_VOLUME_NUM) {
        AUDIO_ERR_LOG("[VolumeOnChange]: Failed parse condition");
        return;
    }

    volumeEvent.updateUi = false;
    CHECK_AND_RETURN_LOG(server_->audioPolicyServerHandler_ != nullptr, "audioPolicyServerHandler_ is nullptr");
    server_->audioPolicyServerHandler_->SendVolumeKeyEventCallback(volumeEvent);
}

void AudioPolicyServer::RemoteParameterCallback::InterruptOnChange(const std::string networkId,
    const std::string& condition)
{
    char eventDes[EVENT_DES_SIZE];
    InterruptType type = INTERRUPT_TYPE_BEGIN;
    InterruptForceType forceType = INTERRUPT_SHARE;
    InterruptHint hint = INTERRUPT_HINT_NONE;

    int ret = sscanf_s(condition.c_str(), "%[^;];EVENT_TYPE=%d;FORCE_TYPE=%d;HINT_TYPE=%d;", eventDes,
        EVENT_DES_SIZE, &type, &forceType, &hint);
    CHECK_AND_RETURN_LOG(ret >= PARAMS_INTERRUPT_NUM, "[InterruptOnChange]: Failed parse condition");

    InterruptEventInternal interruptEvent {type, forceType, hint, 0.2f};
    CHECK_AND_RETURN_LOG(server_->audioPolicyServerHandler_ != nullptr, "audioPolicyServerHandler_ is nullptr");
    server_->audioPolicyServerHandler_->SendInterruptEventInternalCallback(interruptEvent);
}

void AudioPolicyServer::RemoteParameterCallback::StateOnChange(const std::string networkId,
    const std::string& condition, const std::string& value)
{
    char eventDes[EVENT_DES_SIZE];
    char contentDes[ADAPTER_STATE_CONTENT_DES_SIZE];
    int ret = sscanf_s(condition.c_str(), "%[^;];%s", eventDes, EVENT_DES_SIZE, contentDes,
        ADAPTER_STATE_CONTENT_DES_SIZE);
    CHECK_AND_RETURN_LOG(ret >= PARAMS_RENDER_STATE_NUM, "StateOnChange: Failed parse condition");
    CHECK_AND_RETURN_LOG(strcmp(eventDes, "ERR_EVENT") == 0,
        "StateOnChange: Event %{public}s is not supported.", eventDes);

    std::string devTypeKey = "DEVICE_TYPE=";
    std::string contentDesStr = std::string(contentDes);
    auto devTypeKeyPos =  contentDesStr.find(devTypeKey);
    CHECK_AND_RETURN_LOG(devTypeKeyPos != std::string::npos,
        "StateOnChange: Not find daudio device type info, contentDes %{public}s.", contentDesStr.c_str());
    size_t devTypeValPos = devTypeKeyPos + devTypeKey.length();
    CHECK_AND_RETURN_LOG(devTypeValPos < contentDesStr.length(),
        "StateOnChange: Not find daudio device type value, contentDes %{public}s.", contentDesStr.c_str());

    if (contentDesStr[devTypeValPos] == DAUDIO_DEV_TYPE_SPK) {
        server_->audioPolicyService_.NotifyRemoteRenderState(networkId, contentDesStr, value);
    } else if (contentDesStr[devTypeValPos] == DAUDIO_DEV_TYPE_MIC) {
        AUDIO_INFO_LOG("StateOnChange: ERR_EVENT of DAUDIO_DEV_TYPE_MIC.");
    } else {
        AUDIO_ERR_LOG("StateOnChange: Device type is not supported, contentDes %{public}s.", contentDesStr.c_str());
    }
}

void AudioPolicyServer::PerStateChangeCbCustomizeCallback::PermStateChangeCallback(
    Security::AccessToken::PermStateChangeInfo& result)
{
    ready_ = true;
    Security::AccessToken::HapTokenInfo hapTokenInfo;
    int32_t res = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(result.tokenID, hapTokenInfo);
    if (res < 0) {
        AUDIO_ERR_LOG("Call GetHapTokenInfo fail.");
    }
    bool bSetMute;
    if (result.permStateChangeType > 0) {
        bSetMute = false;
    } else {
        bSetMute = true;
    }

    int32_t appUid = getUidByBundleName(hapTokenInfo.bundleName, hapTokenInfo.userID);
    if (appUid < 0) {
        AUDIO_ERR_LOG("fail to get uid.");
    } else {
        server_->audioPolicyService_.SetSourceOutputStreamMute(appUid, bSetMute);
        AUDIO_DEBUG_LOG("get uid value:%{public}d", appUid);
    }
}

int32_t AudioPolicyServer::PerStateChangeCbCustomizeCallback::getUidByBundleName(std::string bundle_name, int user_id)
{
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        return ERR_INVALID_PARAM;
    }

    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remoteObject == nullptr) {
        return ERR_INVALID_PARAM;
    }

    sptr<AppExecFwk::IBundleMgr> bundleMgrProxy = OHOS::iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
    if (bundleMgrProxy == nullptr) {
        return ERR_INVALID_PARAM;
    }
    int32_t iUid = bundleMgrProxy->GetUidByBundleName(bundle_name, user_id);

    return iUid;
}

void AudioPolicyServer::RegisterParamCallback()
{
    AUDIO_INFO_LOG("RegisterParamCallback");
    remoteParameterCallback_ = std::make_shared<RemoteParameterCallback>(this);
    audioPolicyService_.SetParameterCallback(remoteParameterCallback_);
    // regiest policy provider in audio server
    audioPolicyService_.RegiestPolicy();
}

void AudioPolicyServer::RegisterBluetoothListener()
{
    AUDIO_INFO_LOG("RegisterBluetoothListener");
    audioPolicyService_.RegisterBluetoothListener();
}

void AudioPolicyServer::SubscribeAccessibilityConfigObserver()
{
    AUDIO_INFO_LOG("SubscribeAccessibilityConfigObserver");
    audioPolicyService_.SubscribeAccessibilityConfigObserver();
}

bool AudioPolicyServer::IsAudioRendererLowLatencySupported(const AudioStreamInfo &audioStreamInfo)
{
    AUDIO_INFO_LOG("IsAudioRendererLowLatencySupported server call");
    return true;
}

int32_t AudioPolicyServer::SetSystemSoundUri(const std::string &key, const std::string &uri)
{
    if (!PermissionUtil::VerifySystemPermission()) {
        AUDIO_ERR_LOG("GetVolumeGroupInfos: No system permission");
        return ERR_PERMISSION_DENIED;
    }
    AUDIO_INFO_LOG("key: %{public}s, uri: %{public}s", key.c_str(), uri.c_str());
    return audioPolicyService_.SetSystemSoundUri(key, uri);
}

std::string AudioPolicyServer::GetSystemSoundUri(const std::string &key)
{
    if (!PermissionUtil::VerifySystemPermission()) {
        AUDIO_ERR_LOG("GetVolumeGroupInfos: No system permission");
        return "";
    }
    AUDIO_INFO_LOG("key: %{public}s", key.c_str());
    return audioPolicyService_.GetSystemSoundUri(key);
}

float AudioPolicyServer::GetMinStreamVolume()
{
    return audioPolicyService_.GetMinStreamVolume();
}

float AudioPolicyServer::GetMaxStreamVolume()
{
    return audioPolicyService_.GetMaxStreamVolume();
}

int32_t AudioPolicyServer::GetMaxRendererInstances()
{
    AUDIO_INFO_LOG("GetMaxRendererInstances");
    int32_t retryCount = 20; // 20 * 200000us = 4s, wait up to 4s
    while (!isFirstAudioServiceStart_ && retryCount-- > 0) {
        AUDIO_WARNING_LOG("Audio server is not start");
        usleep(200000); // Wait 200000us when audio server is not started
    }
    return audioPolicyService_.GetMaxRendererInstances();
}

void AudioPolicyServer::RegisterDataObserver()
{
    audioPolicyService_.RegisterDataObserver();
}

int32_t AudioPolicyServer::QueryEffectSceneMode(SupportedEffectConfig &supportedEffectConfig)
{
    int32_t ret = audioPolicyService_.QueryEffectManagerSceneMode(supportedEffectConfig);
    return ret;
}

int32_t AudioPolicyServer::SetPlaybackCapturerFilterInfos(const AudioPlaybackCaptureConfig &config,
    uint32_t appTokenId)
{
    for (auto &usg : config.filterOptions.usages) {
        if (usg != STREAM_USAGE_VOICE_COMMUNICATION) {
            continue;
        }

        if (!VerifyPermission(CAPTURER_VOICE_DOWNLINK_PERMISSION, appTokenId)) {
            AUDIO_ERR_LOG("downlink capturer permission check failed");
            return ERR_PERMISSION_DENIED;
        }
    }
    return audioPolicyService_.SetPlaybackCapturerFilterInfos(config);
}

int32_t AudioPolicyServer::SetCaptureSilentState(bool state)
{
    auto callerUid = IPCSkeleton::GetCallingUid();
    if (callerUid != UID_CAST_ENGINE_SA) {
        AUDIO_ERR_LOG("SetCaptureSilentState callerUid is Error: not cast_engine");
        return ERROR;
    }
    return audioPolicyService_.SetCaptureSilentState(state);
}

int32_t AudioPolicyServer::GetHardwareOutputSamplingRate(const sptr<AudioDeviceDescriptor> &desc)
{
    return audioPolicyService_.GetHardwareOutputSamplingRate(desc);
}

vector<sptr<MicrophoneDescriptor>> AudioPolicyServer::GetAudioCapturerMicrophoneDescriptors(int32_t sessionId)
{
    std::vector<sptr<MicrophoneDescriptor>> micDescs =
        audioPolicyService_.GetAudioCapturerMicrophoneDescriptors(sessionId);
    return micDescs;
}

vector<sptr<MicrophoneDescriptor>> AudioPolicyServer::GetAvailableMicrophones()
{
    std::vector<sptr<MicrophoneDescriptor>> micDescs = audioPolicyService_.GetAvailableMicrophones();
    return micDescs;
}

int32_t AudioPolicyServer::SetDeviceAbsVolumeSupported(const std::string &macAddress, const bool support)
{
    auto callerUid = IPCSkeleton::GetCallingUid();
    if (callerUid != UID_BLUETOOTH_SA) {
        AUDIO_ERR_LOG("SetDeviceAbsVolumeSupported: Error caller uid: %{public}d", callerUid);
        return ERROR;
    }
    return audioPolicyService_.SetDeviceAbsVolumeSupported(macAddress, support);
}

bool AudioPolicyServer::IsAbsVolumeScene()
{
    return audioPolicyService_.IsAbsVolumeScene();
}

int32_t AudioPolicyServer::SetA2dpDeviceVolume(const std::string &macAddress, const int32_t volume,
    const bool updateUi)
{
    auto callerUid = IPCSkeleton::GetCallingUid();
    if (callerUid != UID_BLUETOOTH_SA) {
        AUDIO_ERR_LOG("SetA2dpDeviceVolume: Error caller uid: %{public}d", callerUid);
        return ERROR;
    }

    AudioStreamType streamInFocus = AudioStreamType::STREAM_MUSIC; // use STREAM_MUSIC as default stream type
    if (volumeApplyToAll_) {
        streamInFocus = AudioStreamType::STREAM_ALL;
    } else {
        streamInFocus = GetVolumeTypeFromStreamType(GetStreamInFocus());
    }

    if (!IsVolumeLevelValid(streamInFocus, volume)) {
        return ERR_NOT_SUPPORTED;
    }
    int32_t ret = audioPolicyService_.SetA2dpDeviceVolume(macAddress, volume);

    VolumeEvent volumeEvent;
    volumeEvent.volumeType = streamInFocus;
    volumeEvent.volume = volume;
    volumeEvent.updateUi = updateUi;
    volumeEvent.volumeGroupId = 0;
    volumeEvent.networkId = LOCAL_NETWORK_ID;

    if (ret == SUCCESS && audioPolicyServerHandler_!= nullptr) {
        audioPolicyServerHandler_->SendVolumeKeyEventCallback(volumeEvent);
    }
    return ret;
}

std::vector<std::unique_ptr<AudioDeviceDescriptor>> AudioPolicyServer::GetAvailableDevices(AudioDeviceUsage usage)
{
    std::vector<unique_ptr<AudioDeviceDescriptor>> deviceDescs = {};
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    switch (usage) {
        case MEDIA_OUTPUT_DEVICES:
        case MEDIA_INPUT_DEVICES:
        case ALL_MEDIA_DEVICES:
        case CALL_OUTPUT_DEVICES:
        case CALL_INPUT_DEVICES:
        case ALL_CALL_DEVICES:
            if (!hasSystemPermission) {
                AUDIO_ERR_LOG("GetAvailableDevices: No system permission");
                return deviceDescs;
            }
            break;
        default:
            break;
    }

    deviceDescs = audioPolicyService_.GetAvailableDevices(usage);

    if (!hasSystemPermission) {
        for (auto &desc : deviceDescs) {
            desc->networkId_ = "";
            desc->interruptGroupId_ = GROUP_ID_NONE;
            desc->volumeGroupId_ = GROUP_ID_NONE;
        }
    }

    std::vector<sptr<AudioDeviceDescriptor>> deviceDevices = {};
    for (auto &desc : deviceDescs) {
        deviceDevices.push_back(new(std::nothrow) AudioDeviceDescriptor(*desc));
    }

    bool hasBTPermission = VerifyPermission(USE_BLUETOOTH_PERMISSION);
    if (!hasBTPermission) {
        audioPolicyService_.UpdateDescWhenNoBTPermission(deviceDevices);
        deviceDescs.clear();
        for (auto &dec : deviceDevices) {
            deviceDescs.push_back(make_unique<AudioDeviceDescriptor>(*dec));
        }
    }

    return deviceDescs;
}

int32_t AudioPolicyServer::SetAvailableDeviceChangeCallback(const int32_t /*clientId*/, const AudioDeviceUsage usage,
    const sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_PARAM,
        "SetAvailableDeviceChangeCallback set listener object is nullptr");
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    switch (usage) {
        case MEDIA_OUTPUT_DEVICES:
        case MEDIA_INPUT_DEVICES:
        case ALL_MEDIA_DEVICES:
        case CALL_OUTPUT_DEVICES:
        case CALL_INPUT_DEVICES:
        case ALL_CALL_DEVICES:
            if (!hasSystemPermission) {
                AUDIO_ERR_LOG("SetAvailableDeviceChangeCallback: No system permission");
                return ERR_PERMISSION_DENIED;
            }
            break;
        default:
            break;
    }

    int32_t clientPid = IPCSkeleton::GetCallingPid();
    bool hasBTPermission = VerifyPermission(USE_BLUETOOTH_PERMISSION);
    return audioPolicyService_.SetAvailableDeviceChangeCallback(clientPid, usage, object, hasBTPermission);
}

int32_t AudioPolicyServer::UnsetAvailableDeviceChangeCallback(const int32_t /*clientId*/, AudioDeviceUsage usage)
{
    int32_t clientPid = IPCSkeleton::GetCallingPid();
    return audioPolicyService_.UnsetAvailableDeviceChangeCallback(clientPid, usage);
}

int32_t AudioPolicyServer::OffloadStopPlaying(const AudioInterrupt &audioInterrupt)
{
    return audioPolicyService_.OffloadStopPlaying(std::vector<int32_t>(1, audioInterrupt.sessionId));
}

int32_t AudioPolicyServer::ConfigDistributedRoutingRole(const sptr<AudioDeviceDescriptor> descriptor, CastType type)
{
    if (!PermissionUtil::VerifySystemPermission()) {
        AUDIO_ERR_LOG("No system permission");
        return ERR_PERMISSION_DENIED;
    }
    std::lock_guard<std::mutex> lock(descLock_);
    audioPolicyService_.ConfigDistributedRoutingRole(descriptor, type);
    OnDistributedRoutingRoleChange(descriptor, type);
    return SUCCESS;
}

int32_t AudioPolicyServer::SetDistributedRoutingRoleCallback(const sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_PARAM,
        "SetDistributedRoutingRoleCallback set listener object is nullptr");
    int32_t clientPid = IPCSkeleton::GetCallingPid();
    bool hasBTPermission = VerifyPermission(USE_BLUETOOTH_PERMISSION);
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    sptr<IStandardAudioRoutingManagerListener> listener = iface_cast<IStandardAudioRoutingManagerListener>(object);
    if (listener != nullptr && audioPolicyServerHandler_ != nullptr) {
        listener->hasBTPermission_ = hasBTPermission;
        audioPolicyServerHandler_->AddDistributedRoutingRoleChangeCbsMap(clientPid, listener);
    }
    return SUCCESS;
}

int32_t AudioPolicyServer::UnsetDistributedRoutingRoleCallback()
{
    int32_t clientPid = IPCSkeleton::GetCallingPid();
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    if (audioPolicyServerHandler_ != nullptr) {
        return audioPolicyServerHandler_->RemoveDistributedRoutingRoleChangeCbsMap(clientPid);
    }
    return SUCCESS;
}

void AudioPolicyServer::OnDistributedRoutingRoleChange(const sptr<AudioDeviceDescriptor> descriptor,
    const CastType type)
{
    CHECK_AND_RETURN_LOG(audioPolicyServerHandler_ != nullptr, "audioPolicyServerHandler_ is nullptr");
    audioPolicyServerHandler_->SendDistributedRoutingRoleChange(descriptor, type);
}

void AudioPolicyServer::RegisterPowerStateListener()
{
    if (powerStateListener_ == nullptr) {
        powerStateListener_ = new (std::nothrow) PowerStateListener(this);
    }

    if (powerStateListener_ == nullptr) {
        AUDIO_ERR_LOG("create power state listener failed");
        return;
    }

    auto& powerMgrClient = OHOS::PowerMgr::PowerMgrClient::GetInstance();
    bool ret = powerMgrClient.RegisterSyncSleepCallback(powerStateListener_, SleepPriority::HIGH);
    if (!ret) {
        AUDIO_ERR_LOG("register sync sleep callback failed");
    } else {
        AUDIO_INFO_LOG("register sync sleep callback success");
    }
}

void AudioPolicyServer::UnRegisterPowerStateListener()
{
    if (powerStateListener_ == nullptr) {
        AUDIO_ERR_LOG("power state listener is null");
        return;
    }

    auto& powerMgrClient = OHOS::PowerMgr::PowerMgrClient::GetInstance();
    bool ret = powerMgrClient.UnRegisterSyncSleepCallback(powerStateListener_);
    if (!ret) {
        AUDIO_WARNING_LOG("unregister sync sleep callback failed");
    } else {
        powerStateListener_ = nullptr;
        AUDIO_INFO_LOG("unregister sync sleep callback success");
    }
}

bool AudioPolicyServer::SpatializationClientDeathRecipientExist(SpatializationEventCategory eventCategory, pid_t uid)
{
    if (eventCategory == SPATIALIZATION_ENABLED_CHANGE_EVENT) {
        std::lock_guard<std::mutex> lock(spatializationEnabledListenerStateMutex_);
        if (std::find(spatializationEnabledListenerState_.begin(), spatializationEnabledListenerState_.end(),
            uid) != spatializationEnabledListenerState_.end()) {
            AUDIO_INFO_LOG("spatializationEnabledListener has been registered for %{public}d!", uid);
            return true;
        }
    } else if (eventCategory == HEAD_TRACKING_ENABLED_CHANGE_EVENT) {
        std::lock_guard<std::mutex> lock(headTrackingEnabledListenerStateMutex_);
        if (std::find(headTrackingEnabledListenerState_.begin(), headTrackingEnabledListenerState_.end(),
            uid) != headTrackingEnabledListenerState_.end()) {
            AUDIO_INFO_LOG("headTrackingEnabledListener has been registered for %{public}d!", uid);
            return true;
        }
    }
    return false;
}

void AudioPolicyServer::RegisterSpatializationClientDeathRecipient(const sptr<IRemoteObject> &object,
    SpatializationEventCategory eventCategory)
{
    AUDIO_INFO_LOG("Register spatialization clients death recipient, eventCategory: %{public}d", eventCategory);
    CHECK_AND_RETURN_LOG(object != nullptr, "Client proxy obj NULL");

    pid_t uid = IPCSkeleton::GetCallingPid();
    if (SpatializationClientDeathRecipientExist(eventCategory, uid)) {
        return;
    }

    sptr<AudioServerDeathRecipient> deathRecipient = new(std::nothrow) AudioServerDeathRecipient(uid);
    if (deathRecipient == nullptr) {
        AUDIO_ERR_LOG("deathRecipient is nullptr, add deathRecipient fail for %{public}d", eventCategory);
        return;
    }
    if (eventCategory == SPATIALIZATION_ENABLED_CHANGE_EVENT) {
        deathRecipient->SetNotifyCb(std::bind(&AudioPolicyServer::RegisteredSpatializationEnabledClientDied,
            this, std::placeholders::_1));
        bool result = object->AddDeathRecipient(deathRecipient);
        if (!result) {
            AUDIO_ERR_LOG("failed to add DeathRecipient for %{public}d!", eventCategory);
            return;
        }
        std::lock_guard<std::mutex> lock(spatializationEnabledListenerStateMutex_);
        spatializationEnabledListenerState_.push_back(uid);
    } else if (eventCategory == HEAD_TRACKING_ENABLED_CHANGE_EVENT) {
        deathRecipient->SetNotifyCb(std::bind(&AudioPolicyServer::RegisteredHeadTrackingEnabledClientDied,
            this, std::placeholders::_1));
        bool result = object->AddDeathRecipient(deathRecipient);
        if (!result) {
            AUDIO_ERR_LOG("failed to add DeathRecipient for %{public}d!", eventCategory);
            return;
        }
        std::lock_guard<std::mutex> lock(headTrackingEnabledListenerStateMutex_);
        headTrackingEnabledListenerState_.push_back(uid);
    }
}

void AudioPolicyServer::RegisteredSpatializationEnabledClientDied(pid_t uid)
{
    AUDIO_INFO_LOG("RegisteredSpatializationEnabledClient died: remove entry, uid %{public}d", uid);

    int32_t ret = audioSpatializationService_.UnregisterSpatializationEnabledEventListener(static_cast<int32_t>(uid));
    if (ret != 0) {
        AUDIO_WARNING_LOG("UnregisterSpatializationEnabledEventListener fail, uid %{public}d", uid);
    }

    auto filter = [&uid](int val) {
        return uid == val;
    };
    std::lock_guard<std::mutex> lock(spatializationEnabledListenerStateMutex_);
    spatializationEnabledListenerState_.erase(std::remove_if(spatializationEnabledListenerState_.begin(),
        spatializationEnabledListenerState_.end(), filter), spatializationEnabledListenerState_.end());
}

void AudioPolicyServer::RegisteredHeadTrackingEnabledClientDied(pid_t uid)
{
    AUDIO_INFO_LOG("RegisteredHeadTrackingEnabledClient died: remove entry, uid %{public}d", uid);

    int32_t ret = audioSpatializationService_.UnregisterHeadTrackingEnabledEventListener(static_cast<int32_t>(uid));
    if (ret != 0) {
        AUDIO_WARNING_LOG("UnregisterHeadTrackingEnabledEventListener fail, uid %{public}d", uid);
    }

    auto filter = [&uid](int val) {
        return uid == val;
    };
    std::lock_guard<std::mutex> lock(headTrackingEnabledListenerStateMutex_);
    headTrackingEnabledListenerState_.erase(std::remove_if(headTrackingEnabledListenerState_.begin(),
        headTrackingEnabledListenerState_.end(), filter), headTrackingEnabledListenerState_.end());
}

bool AudioPolicyServer::IsSpatializationEnabled()
{
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        return false;
    }
    return audioSpatializationService_.IsSpatializationEnabled();
}

int32_t AudioPolicyServer::SetSpatializationEnabled(const bool enable)
{
    if (!VerifyPermission(MANAGE_SYSTEM_AUDIO_EFFECTS)) {
        AUDIO_ERR_LOG("MANAGE_SYSTEM_AUDIO_EFFECTS permission check failed");
        return ERR_PERMISSION_DENIED;
    }
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        return ERR_PERMISSION_DENIED;
    }
    return audioSpatializationService_.SetSpatializationEnabled(enable);
}

bool AudioPolicyServer::IsHeadTrackingEnabled()
{
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        return false;
    }
    return audioSpatializationService_.IsHeadTrackingEnabled();
}

int32_t AudioPolicyServer::SetHeadTrackingEnabled(const bool enable)
{
    if (!VerifyPermission(MANAGE_SYSTEM_AUDIO_EFFECTS)) {
        AUDIO_ERR_LOG("MANAGE_SYSTEM_AUDIO_EFFECTS permission check failed");
        return ERR_PERMISSION_DENIED;
    }
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        return ERR_PERMISSION_DENIED;
    }
    return audioSpatializationService_.SetHeadTrackingEnabled(enable);
}

int32_t AudioPolicyServer::RegisterSpatializationEnabledEventListener(const sptr<IRemoteObject> &object)
{
    int32_t clientPid = IPCSkeleton::GetCallingPid();
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    RegisterSpatializationClientDeathRecipient(object, SPATIALIZATION_ENABLED_CHANGE_EVENT);
    return audioSpatializationService_.RegisterSpatializationEnabledEventListener(
        clientPid, object, hasSystemPermission);
}

int32_t AudioPolicyServer::RegisterHeadTrackingEnabledEventListener(const sptr<IRemoteObject> &object)
{
    int32_t clientPid = IPCSkeleton::GetCallingPid();
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    RegisterSpatializationClientDeathRecipient(object, HEAD_TRACKING_ENABLED_CHANGE_EVENT);
    return audioSpatializationService_.RegisterHeadTrackingEnabledEventListener(clientPid, object, hasSystemPermission);
}

int32_t AudioPolicyServer::UnregisterSpatializationEnabledEventListener()
{
    int32_t clientPid = IPCSkeleton::GetCallingPid();
    return audioSpatializationService_.UnregisterSpatializationEnabledEventListener(clientPid);
}

int32_t AudioPolicyServer::UnregisterHeadTrackingEnabledEventListener()
{
    int32_t clientPid = IPCSkeleton::GetCallingPid();
    return audioSpatializationService_.UnregisterHeadTrackingEnabledEventListener(clientPid);
}

AudioSpatializationState AudioPolicyServer::GetSpatializationState(const StreamUsage streamUsage)
{
    return audioSpatializationService_.GetSpatializationState(streamUsage);
}

bool AudioPolicyServer::IsSpatializationSupported()
{
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        return false;
    }
    return audioSpatializationService_.IsSpatializationSupported();
}

bool AudioPolicyServer::IsSpatializationSupportedForDevice(const std::string address)
{
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        return false;
    }
    return audioSpatializationService_.IsSpatializationSupportedForDevice(address);
}

bool AudioPolicyServer::IsHeadTrackingSupported()
{
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        return false;
    }
    return audioSpatializationService_.IsHeadTrackingSupported();
}

bool AudioPolicyServer::IsHeadTrackingSupportedForDevice(const std::string address)
{
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        return false;
    }
    return audioSpatializationService_.IsHeadTrackingSupportedForDevice(address);
}

int32_t AudioPolicyServer::UpdateSpatialDeviceState(const AudioSpatialDeviceState audioSpatialDeviceState)
{
    if (!VerifyPermission(MANAGE_SYSTEM_AUDIO_EFFECTS)) {
        AUDIO_ERR_LOG("MANAGE_SYSTEM_AUDIO_EFFECTS permission check failed");
        return ERR_PERMISSION_DENIED;
    }
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        return ERR_PERMISSION_DENIED;
    }
    return audioSpatializationService_.UpdateSpatialDeviceState(audioSpatialDeviceState);
}

int32_t AudioPolicyServer::RegisterSpatializationStateEventListener(const uint32_t sessionID,
    const StreamUsage streamUsage, const sptr<IRemoteObject> &object)
{
    return audioSpatializationService_.RegisterSpatializationStateEventListener(sessionID, streamUsage, object);
}

int32_t AudioPolicyServer::UnregisterSpatializationStateEventListener(const uint32_t sessionID)
{
    return audioSpatializationService_.UnregisterSpatializationStateEventListener(sessionID);
}

int32_t AudioPolicyServer::RegisterPolicyCallbackClient(const sptr<IRemoteObject> &object, const int32_t zoneID)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_PARAM,
        "RegisterPolicyCallbackClient listener object is nullptr");

    sptr<IAudioPolicyClient> callback = iface_cast<IAudioPolicyClient>(object);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_PARAM,
        "RegisterPolicyCallbackClient listener obj cast failed");

    int32_t clientPid = IPCSkeleton::GetCallingPid();
    AUDIO_DEBUG_LOG("register clientPid: %{public}d", clientPid);

    bool hasBTPermission = VerifyPermission(USE_BLUETOOTH_PERMISSION);
    bool hasSysPermission = PermissionUtil::VerifySystemPermission();
    callback->hasBTPermission_ = hasBTPermission;
    callback->hasSystemPermission_ = hasSysPermission;
    callback->apiVersion_ = GetApiTargerVersion();
    audioPolicyService_.AddAudioPolicyClientProxyMap(clientPid, callback);

    RegisterClientDeathRecipient(object, LISTENER_CLIENT);
    return SUCCESS;
}

int32_t AudioPolicyServer::CreateAudioInterruptZone(const std::set<int32_t> &pids, const int32_t zoneID)
{
    if (interruptService_ != nullptr) {
        return interruptService_->CreateAudioInterruptZone(zoneID, pids);
    }
    return ERR_UNKNOWN;
}

int32_t AudioPolicyServer::AddAudioInterruptZonePids(const std::set<int32_t> &pids, const int32_t zoneID)
{
    if (interruptService_ != nullptr) {
        return interruptService_->AddAudioInterruptZonePids(zoneID, pids);
    }
    return ERR_UNKNOWN;
}

int32_t AudioPolicyServer::RemoveAudioInterruptZonePids(const std::set<int32_t> &pids, const int32_t zoneID)
{
    if (interruptService_ != nullptr) {
        return interruptService_->RemoveAudioInterruptZonePids(zoneID, pids);
    }
    return ERR_UNKNOWN;
}

int32_t AudioPolicyServer::ReleaseAudioInterruptZone(const int32_t zoneID)
{
    if (interruptService_ != nullptr) {
        return interruptService_->ReleaseAudioInterruptZone(zoneID);
    }
    return ERR_UNKNOWN;
}

int32_t AudioPolicyServer::SetCallDeviceActive(InternalDeviceType deviceType, bool active, std::string address)
{
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        AUDIO_ERR_LOG("No system permission");
        return ERR_SYSTEM_PERMISSION_DENIED;
    }
    switch (deviceType) {
        case EARPIECE:
        case SPEAKER:
        case BLUETOOTH_SCO:
            break;
        default:
            AUDIO_ERR_LOG("device=%{public}d not supported", deviceType);
            return ERR_NOT_SUPPORTED;
    }
    return audioPolicyService_.SetCallDeviceActive(deviceType, active, address);
}

std::unique_ptr<AudioDeviceDescriptor> AudioPolicyServer::GetActiveBluetoothDevice()
{
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        AUDIO_ERR_LOG("No system permission");
        return make_unique<AudioDeviceDescriptor>();
    }
   
    auto btdevice = audioPolicyService_.GetActiveBluetoothDevice();

    bool hasBTPermission = VerifyPermission(USE_BLUETOOTH_PERMISSION);
    if (!hasBTPermission) {
        btdevice->deviceName_ = "";
        btdevice->macAddress_ = "";
    }

    return btdevice;
}

ConverterConfig AudioPolicyServer::GetConverterConfig()
{
    return audioPolicyService_.GetConverterConfig();
}

AudioSpatializationSceneType AudioPolicyServer::GetSpatializationSceneType()
{
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        return SPATIALIZATION_SCENE_TYPE_DEFAULT;
    }
    return audioSpatializationService_.GetSpatializationSceneType();
}

int32_t AudioPolicyServer::SetSpatializationSceneType(const AudioSpatializationSceneType spatializationSceneType)
{
    if (!VerifyPermission(MANAGE_SYSTEM_AUDIO_EFFECTS)) {
        AUDIO_ERR_LOG("MANAGE_SYSTEM_AUDIO_EFFECTS permission check failed");
        return ERR_PERMISSION_DENIED;
    }
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        return ERR_PERMISSION_DENIED;
    }
    return audioSpatializationService_.SetSpatializationSceneType(spatializationSceneType);
}

int32_t AudioPolicyServer::DisableSafeMediaVolume()
{
    if (!VerifyPermission(MODIFY_AUDIO_SETTINGS_PERMISSION)) {
        AUDIO_ERR_LOG("MODIFY_AUDIO_SETTINGS_PERMISSION permission check failed");
        return ERR_PERMISSION_DENIED;
    }
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        return ERR_SYSTEM_PERMISSION_DENIED;
    }
    return audioPolicyService_.DisableSafeMediaVolume();
}

AppExecFwk::BundleInfo AudioPolicyServer::GetBundleInfoFromUid()
{
    std::string bundleName {""};
    AppExecFwk::BundleInfo bundleInfo;
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(systemAbilityManager != nullptr, bundleInfo, "systemAbilityManager is nullptr");

    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    CHECK_AND_RETURN_RET_LOG(remoteObject != nullptr, bundleInfo, "remoteObject is nullptr");

    sptr<AppExecFwk::IBundleMgr> bundleMgrProxy = OHOS::iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
    CHECK_AND_RETURN_RET_LOG(bundleMgrProxy != nullptr, bundleInfo, "bundleMgrProxy is nullptr");

    int32_t callingUid = IPCSkeleton::GetCallingUid();
    bundleMgrProxy->GetNameForUid(callingUid, bundleName);

    bundleMgrProxy->GetBundleInfoV9(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT |
        AppExecFwk::BundleFlag::GET_BUNDLE_WITH_ABILITIES |
        AppExecFwk::BundleFlag::GET_BUNDLE_WITH_REQUESTED_PERMISSION |
        AppExecFwk::BundleFlag::GET_BUNDLE_WITH_EXTENSION_INFO |
        AppExecFwk::BundleFlag::GET_BUNDLE_WITH_HASH_VALUE,
        bundleInfo,
        AppExecFwk::Constants::ALL_USERID);

    return bundleInfo;
}

int32_t AudioPolicyServer::GetApiTargerVersion()
{
    AppExecFwk::BundleInfo bundleInfo = GetBundleInfoFromUid();

    // Taking remainder of large integers
    int32_t apiTargetversion = bundleInfo.applicationInfo.apiTargetVersion % API_VERSION_REMAINDER;
    return apiTargetversion;
}

bool AudioPolicyServer::IsHighResolutionExist()
{
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        AUDIO_ERR_LOG("No system permission");
        return false;
    }
    return isHighResolutionExist_;
}

int32_t AudioPolicyServer::SetHighResolutionExist(bool highResExist)
{
    bool hasSystemPermission = PermissionUtil::VerifySystemPermission();
    if (!hasSystemPermission) {
        AUDIO_ERR_LOG("No system permission");
        return ERR_PERMISSION_DENIED;
    }
    isHighResolutionExist_ = highResExist;
    return SUCCESS;
}

float AudioPolicyServer::GetMaxAmplitude(int32_t deviceId)
{
    return audioPolicyService_.GetMaxAmplitude(deviceId);
}

bool AudioPolicyServer::IsHeadTrackingDataRequested(const std::string &macAddress)
{
    return audioSpatializationService_.IsHeadTrackingDataRequested(macAddress);
}

int32_t AudioPolicyServer::SetAudioDeviceRefinerCallback(const sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_PARAM, "SetAudioDeviceRefinerCallback object is nullptr");
    auto callerUid = IPCSkeleton::GetCallingUid();
    if (callerUid != UID_AUDIO) {
        return ERROR;
    }
    return audioRouterCenter_.SetAudioDeviceRefinerCallback(object);
}

int32_t AudioPolicyServer::UnsetAudioDeviceRefinerCallback()
{
    auto callerUid = IPCSkeleton::GetCallingUid();
    if (callerUid != UID_AUDIO) {
        return ERROR;
    }
    return audioRouterCenter_.UnsetAudioDeviceRefinerCallback();
}

int32_t AudioPolicyServer::TriggerFetchDevice()
{
    auto callerUid = IPCSkeleton::GetCallingUid();
    if (callerUid != UID_AUDIO) {
        return ERROR;
    }
    return audioPolicyService_.TriggerFetchDevice();
}
} // namespace AudioStandard
} // namespace OHOS
