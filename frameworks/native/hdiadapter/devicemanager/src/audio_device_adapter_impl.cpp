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

#include "audio_device_adapter_impl.h"

#include <securec.h>

#include "audio_errors.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
int32_t AudioDeviceAdapterImpl::Init()
{
    AUDIO_INFO_LOG("Init start.");

    CHECK_AND_RETURN_RET_LOG(audioAdapter_ != nullptr, ERR_INVALID_HANDLE,
        "Init: audio adapter is null.");
    int32_t ret = audioAdapter_->InitAllPorts(audioAdapter_);
    CHECK_AND_RETURN_RET_LOG(ret == 0, ERR_DEVICE_INIT, "InitAllPorts fail, ret %{public}d.", ret);

    ret = RegExtraParamObserver();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Init: Register extra param observer fail, ret %{public}d.", ret);
    AUDIO_INFO_LOG("Init end.");
    return SUCCESS;
}


size_t AudioDeviceAdapterImpl::GetRenderPortsNum()
{
    std::lock_guard<std::mutex> lock(renderPortsMtx_);
    return renderPorts_.size();
}

size_t AudioDeviceAdapterImpl::GetCapturePortsNum()
{
    std::lock_guard<std::mutex> lock(capturePortsMtx_);
    return capturePorts_.size();
}

int32_t AudioDeviceAdapterImpl::HandleRenderParamEvent(AudioDeviceAdapterImpl* devAdapter,
    const AudioParamKey audioKey, const char *condition, const char *value)
{
    if (devAdapter == nullptr || condition == nullptr || value == nullptr) {
        AUDIO_ERR_LOG("HandleStateChangeEvent: some params are null.");
        return ERR_INVALID_HANDLE;
    }

    std::shared_ptr<IAudioDeviceAdapterCallback> sink = nullptr;
    {
        std::lock_guard<std::mutex> lock(devAdapter->renderPortsMtx_);
        CHECK_AND_BREAK_LOG(devAdapter->renderPorts_.size() == 1, "Please check renderId or other infos.");
        for (auto i = devAdapter->renderPorts_.begin(); i != devAdapter->renderPorts_.end();) {
            sink = i->second.devAdpCb.lock();
            if (sink == nullptr) {
                AUDIO_ERR_LOG("HandleStateChangeEvent: Device adapter sink callback is null.");
                devAdapter->renderPorts_.erase(i++);
                continue;
            }
            break;
        }
    }

    CHECK_AND_RETURN_RET_LOG(sink != nullptr, ERR_INVALID_HANDLE,
        "Not find daudio sink port in adapter, condition %{public}s.", condition);
    sink->OnAudioParamChange(devAdapter->adapterName_, audioKey, std::string(condition), std::string(value));
    return SUCCESS;
}

int32_t AudioDeviceAdapterImpl::HandleCaptureParamEvent(AudioDeviceAdapterImpl* devAdapter,
    const AudioParamKey audioKey, const char *condition, const char *value)
{
    if (devAdapter == nullptr || condition == nullptr || value == nullptr) {
        AUDIO_ERR_LOG("HandleStateChangeEvent: some params are null.");
        return ERR_INVALID_HANDLE;
    }

    std::shared_ptr<IAudioDeviceAdapterCallback> source = nullptr;
    {
        std::lock_guard<std::mutex> lock(devAdapter->capturePortsMtx_);
        CHECK_AND_BREAK_LOG(devAdapter->capturePorts_.size() == 1, "Please check captureId or other infos.");
        for (auto j = devAdapter->capturePorts_.begin(); j != devAdapter->capturePorts_.end();) {
            source = j->second.devAdpCb.lock();
            if (source == nullptr) {
                AUDIO_ERR_LOG("HandleStateChangeEvent: Device adapter source callback is null.");
                devAdapter->capturePorts_.erase(j++);
                continue;
            }
            break;
        }
    }

    CHECK_AND_RETURN_RET_LOG(source != nullptr, ERR_INVALID_HANDLE,
        "Not find daudio source port in adapter, condition %{public}s.", condition);
    source->OnAudioParamChange(devAdapter->adapterName_, audioKey, std::string(condition), std::string(value));
    return SUCCESS;
}

int32_t AudioDeviceAdapterImpl::HandleStateChangeEvent(AudioDeviceAdapterImpl* devAdapter,
    const AudioParamKey audioKey, const char *condition, const char *value)
{
    if (devAdapter == nullptr || condition == nullptr || value == nullptr) {
        AUDIO_ERR_LOG("HandleStateChangeEvent: some params are null.");
        return ERR_INVALID_HANDLE;
    }

    char eventDes[EVENT_DES_SIZE];
    char contentDes[STATE_CONTENT_DES_SIZE];
    CHECK_AND_RETURN_RET_LOG(
        sscanf_s(condition, "%[^;];%s", eventDes, EVENT_DES_SIZE, contentDes, STATE_CONTENT_DES_SIZE)
        == PARAMS_STATE_NUM, ERR_INVALID_PARAM, "ParamEventCallback: Failed parse condition");
    CHECK_AND_RETURN_RET_LOG(strcmp(eventDes, "ERR_EVENT") == 0, ERR_NOT_SUPPORTED,
        "HandleStateChangeEvent: Event %{public}s is not supported.", eventDes);

    AUDIO_INFO_LOG("render state invalid, destroy audioRender");

    std::string devTypeKey = "DEVICE_TYPE=";
    size_t devTypeKeyPos =  std::string(contentDes).find(devTypeKey);
    CHECK_AND_RETURN_RET_LOG(devTypeKeyPos != std::string::npos, ERR_INVALID_PARAM,
        "HandleStateChangeEvent: Not find daudio device type info, contentDes %{public}s.", contentDes);
    int32_t ret = SUCCESS;
    if (contentDes[devTypeKeyPos + devTypeKey.length()] == DAUDIO_DEV_TYPE_SPK) {
        AUDIO_INFO_LOG("HandleStateChangeEvent: ERR_EVENT of DAUDIO_DEV_TYPE_SPK.");
        ret = HandleRenderParamEvent(devAdapter, audioKey, condition, value);
    } else if (contentDes[devTypeKeyPos + devTypeKey.length()] == DAUDIO_DEV_TYPE_MIC) {
        AUDIO_INFO_LOG("HandleStateChangeEvent: ERR_EVENT of DAUDIO_DEV_TYPE_MIC.");
        ret = HandleCaptureParamEvent(devAdapter, audioKey, condition, value);
    } else {
        AUDIO_ERR_LOG("HandleStateChangeEvent: Device type is not supported, contentDes %{public}s.", contentDes);
        return ERR_NOT_SUPPORTED;
    }

    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret,
        "Handle state error change event %{public}s fail.", contentDes);
    return SUCCESS;
}

int32_t AudioDeviceAdapterImpl::ParamEventCallback(AudioExtParamKey key, const char* condition, const char* value,
    void* reserved, void* cookie)
{
    AUDIO_INFO_LOG("ParamEventCallback: key %{public}d, condition %{public}s, value %{public}s",
        key, condition, value);
    AudioDeviceAdapterImpl* devAdapter = reinterpret_cast<AudioDeviceAdapterImpl*>(cookie);
    AudioParamKey audioKey = AudioParamKey(key);
    int32_t ret = SUCCESS;
    switch (audioKey) {
        case AudioParamKey::PARAM_KEY_STATE:
            ret = HandleStateChangeEvent(devAdapter, audioKey, condition, value);
            break;
        case AudioParamKey::VOLUME:
        case AudioParamKey::INTERRUPT:
            HandleRenderParamEvent(devAdapter, audioKey, condition, value);
            break;
        default:
            AUDIO_ERR_LOG("ParamEventCallback: Audio param key %{public}d is not supported.", audioKey);
            return ERR_NOT_SUPPORTED;
    }
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret,
        "ParamEventCallback: Handle audio param key %{public}d fail, ret %{public}d.", audioKey, ret);
    return SUCCESS;
}

int32_t AudioDeviceAdapterImpl::RegExtraParamObserver()
{
    AUDIO_INFO_LOG("Register extra param observer.");
#ifdef FEATURE_DISTRIBUTE_AUDIO
    std::lock_guard<std::mutex> lock(regParamCbMtx_);
    if (isParamCbReg_.load()) {
        AUDIO_INFO_LOG("Audio adapter already registered extra param observer.");
        return SUCCESS;
    }

    ParamCallback adapterCallback = &AudioDeviceAdapterImpl::ParamEventCallback;
    CHECK_AND_RETURN_RET_LOG(audioAdapter_ != nullptr, ERR_INVALID_HANDLE,
        "RegExtraParamObserver: audio adapter is null.");

    int32_t ret = audioAdapter_->RegExtraParamObserver(audioAdapter_, adapterCallback, this);
    CHECK_AND_RETURN_RET_LOG(ret == 0, ERR_OPERATION_FAILED,
        "Register extra param observer fail, ret %{public}d", ret);

    isParamCbReg_.store(true);
    AUDIO_INFO_LOG("Register extra param observer for daudio OK.");
#endif
    return SUCCESS;
}

int32_t AudioDeviceAdapterImpl::CreateRender(const struct AudioDeviceDescriptor *devDesc,
    const struct AudioSampleAttributes *attr, struct AudioRender **audioRender,
    const std::shared_ptr<IAudioDeviceAdapterCallback> &renderCb)
{
    AUDIO_INFO_LOG("Create render start.");
    CHECK_AND_RETURN_RET_LOG(audioAdapter_ != nullptr, ERR_INVALID_HANDLE,
        "CreateRender: audio adapter is null.");
    int32_t ret = audioAdapter_->CreateRender(audioAdapter_, devDesc, attr, audioRender);
    if (ret != 0 || audioRender == nullptr || *audioRender == nullptr) {
        AUDIO_ERR_LOG("AudioDeviceCreateRender failed");
        return ERR_NOT_STARTED;
    }

    std::lock_guard<std::mutex> lock(renderPortsMtx_);
    if (renderPorts_.find(*audioRender) != renderPorts_.end()) {
        AUDIO_INFO_LOG("Audio render already exit in renderPorts.");
        return SUCCESS;
    }

    DevicePortInfo renderPortInfo = {
        .devAdpCb = renderCb,
        .portId = 0,
    };
    renderPorts_[*audioRender] = renderPortInfo;
    return SUCCESS;
}

void AudioDeviceAdapterImpl::DestroyRender(struct AudioRender *audioRender)
{
    CHECK_AND_RETURN_LOG(audioRender != nullptr, "DestroyRender: Audio render is null.");
    {
        std::lock_guard<std::mutex> lock(renderPortsMtx_);
        AUDIO_INFO_LOG("Destroy render start.");
        if (renderPorts_.find(audioRender) == renderPorts_.end()) {
            AUDIO_INFO_LOG("Audio render is already destoried.");
            return;
        }
    }

    CHECK_AND_RETURN_LOG(audioAdapter_ != nullptr, "DestroyRender: Audio adapter is null.");
    audioAdapter_->DestroyRender(audioAdapter_, audioRender);

    {
        std::lock_guard<std::mutex> lock(renderPortsMtx_);
        renderPorts_.erase(audioRender);
    }
}

int32_t AudioDeviceAdapterImpl::CreateCapture(const struct AudioDeviceDescriptor *devDesc,
    const struct AudioSampleAttributes *attr, struct AudioCapture **audioCapture,
    const std::shared_ptr<IAudioDeviceAdapterCallback> &captureCb)
{
    CHECK_AND_RETURN_RET_LOG(audioAdapter_ != nullptr, ERR_INVALID_HANDLE,
        "CreateRender: audio adapter is null.");
    int32_t ret = audioAdapter_->CreateCapture(audioAdapter_, devDesc, attr, audioCapture);
    if (ret != 0 || audioCapture == nullptr || *audioCapture == nullptr) {
        AUDIO_ERR_LOG("Create capture failed, error code %{public}d.", ret);
        return ERR_NOT_STARTED;
    }

    std::lock_guard<std::mutex> lock(capturePortsMtx_);
    if (capturePorts_.find(*audioCapture) != capturePorts_.end()) {
        AUDIO_INFO_LOG("Audio capture already exit in capturePorts.");
        return SUCCESS;
    }

    DevicePortInfo capturePortInfo = {
        .devAdpCb = captureCb,
        .portId = 0,
    };
    capturePorts_[*audioCapture] = capturePortInfo;
    return SUCCESS;
}

void AudioDeviceAdapterImpl::DestroyCapture(struct AudioCapture *audioCapture)
{
    CHECK_AND_RETURN_LOG(audioCapture != nullptr, "DestroyCapture: Audio capture is null.");
    {
        std::lock_guard<std::mutex> lock(capturePortsMtx_);
        AUDIO_INFO_LOG("Destroy capture start.");
        if (capturePorts_.find(audioCapture) == capturePorts_.end()) {
            AUDIO_INFO_LOG("Audio capture is already destoried.");
            return;
        }
    }

    CHECK_AND_RETURN_LOG(audioAdapter_ != nullptr, "DestroyCapture: Audio adapter is null.");
    audioAdapter_->DestroyCapture(audioAdapter_, audioCapture);

    {
        std::lock_guard<std::mutex> lock(capturePortsMtx_);
        capturePorts_.erase(audioCapture);
    }
}

void AudioDeviceAdapterImpl::SetAudioParameter(const AudioParamKey key, const std::string &condition,
    const std::string &value)
{
#ifdef FEATURE_DISTRIBUTE_AUDIO
    AUDIO_INFO_LOG("SetAudioParameter: key %{public}d, condition: %{public}s, value: %{public}s.",
        key, condition.c_str(), value.c_str());
    CHECK_AND_RETURN_LOG(audioAdapter_ != nullptr, "SetAudioParameter: Audio adapter is null.");

    enum AudioExtParamKey hdiKey = AudioExtParamKey(key);
    int32_t ret = audioAdapter_->SetExtraParams(audioAdapter_, hdiKey, condition.c_str(), value.c_str());
    CHECK_AND_RETURN_LOG(ret == 0, "Set audio parameter fail, ret %{public}d.", ret);
#else
    AUDIO_INFO_LOG("SetAudioParameter is not supported.");
#endif
}

std::string AudioDeviceAdapterImpl::GetAudioParameter(const AudioParamKey key, const std::string& condition)
{
#ifdef FEATURE_DISTRIBUTE_AUDIO
    AUDIO_INFO_LOG("GetParameter: key %{public}d, condition: %{public}s", key,
        condition.c_str());
    CHECK_AND_RETURN_RET_LOG(audioAdapter_ != nullptr, "", "GetAudioParameter: Audio adapter is null.");
    enum AudioExtParamKey hdiKey = AudioExtParamKey(key);
    char value[PARAM_VALUE_LENTH];

    int32_t ret = audioAdapter_->GetExtraParams(audioAdapter_, hdiKey, condition.c_str(), value, PARAM_VALUE_LENTH);
    CHECK_AND_RETURN_RET_LOG(ret == 0, "", "Get audio parameter fail, ret %{public}d", ret);
    return value;
#else
    AUDIO_INFO_LOG("GetAudioParameter is not supported.");
    return "";
#endif
}

int32_t AudioDeviceAdapterImpl::UpdateAudioRoute(const struct AudioRoute *route, int32_t *routeHandle_)
{
    AUDIO_INFO_LOG("UpdateAudioRoute enter.");
    CHECK_AND_RETURN_RET_LOG(audioAdapter_ != nullptr, ERR_INVALID_HANDLE,
        "UpdateAudioRoute: Audio adapter is null.");
    int32_t ret = audioAdapter_->UpdateAudioRoute(audioAdapter_, route, routeHandle_);
    CHECK_AND_RETURN_RET_LOG(ret == 0, ERR_OPERATION_FAILED, "Update audio route fail, ret %{public}d", ret);
    return SUCCESS;
}

int32_t AudioDeviceAdapterImpl::Release()
{
    AUDIO_INFO_LOG("Release enter.");
    if (audioAdapter_ == nullptr) {
        AUDIO_INFO_LOG("Audio adapter is already released.");
        return SUCCESS;
    }

    size_t capturePortsNum = GetCapturePortsNum();
    size_t renderPortsNum = GetRenderPortsNum();
    if (GetCapturePortsNum() + GetRenderPortsNum() != 0) {
        AUDIO_ERR_LOG("Audio adapter has some ports busy, capturePortsNum %zu, renderPortsNum %zu.",
            capturePortsNum, renderPortsNum);
        return ERR_ILLEGAL_STATE;
    }
    if (routeHandle_ != INVALID_ROUT_HANDLE) {
        audioAdapter_->ReleaseAudioRoute(audioAdapter_, routeHandle_);
    }
    audioAdapter_ = nullptr;
    AUDIO_INFO_LOG("Release end.");
    return SUCCESS;
}
}  // namespace AudioStandard
}  // namespace OHOS
