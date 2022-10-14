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

#include "audio_common_napi.h"
#include "audio_log.h"
#include "audio_manager_napi.h"
#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {
std::string AudioCommonNapi::GetStringArgument(napi_env env, napi_value value)
{
    std::string strValue = "";
    size_t bufLength = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &bufLength);
    if (status == napi_ok && bufLength > 0 && bufLength < PATH_MAX) {
        char *buffer = (char *)malloc((bufLength + 1) * sizeof(char));
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, strValue, "no memory");
        status = napi_get_value_string_utf8(env, value, buffer, bufLength + 1, &bufLength);
        if (status == napi_ok) {
            AUDIO_DEBUG_LOG("argument = %{public}s", buffer);
            strValue = buffer;
        }
        free(buffer);
        buffer = nullptr;
    }
    return strValue;
}

std::string AudioCommonNapi::getMessageByCode(int32_t &code){
    std::string err_message;
    switch (code) {
        case ERR_NUMBER101:
            err_message = ERR_MESSAGE101;
            break;
        case ERR_NUMBER102:
            err_message = ERR_MESSAGE102;
            break;
        case ERR_NUMBER103:
            err_message = ERR_MESSAGE103;
            break;
        case ERR_NUMBER104:
            err_message = ERR_MESSAGE104;
            break;
        case ERR_NUMBER105:
            err_message = ERR_MESSAGE105;
            break;
        case ERR_NUMBER201:
            err_message = ERR_MESSAGE201;
            break;   
        case ERR_NUMBER301:
            err_message = ERR_MESSAGE301;
            break;
        case ERR_NUMBER_401:
            err_message = ERR_MESSAGE_401;
            break;
        default:
            err_message = ERR_MESSAGE301;
            code = ERR_NUMBER301;
            break;
    }
    return err_message;
}

void AudioCommonNapi::throwError(napi_env env,int32_t code){
    std::string messageValue = AudioCommonNapi::getMessageByCode(code);
    napi_throw_error(env, (std::to_string(code)).c_str(), messageValue.c_str());
}

bool AudioCommonNapi::IsLegalInputArgumentVolLevel(int32_t volLevel)
{
    return (volLevel < 0 || volLevel > 15) ? false : true;
}

bool AudioCommonNapi::IsLegalInputArgumentVolType(int32_t inputType)
{
    bool result = false;
    switch (inputType) {
        case AudioManagerNapi::RINGTONE:
        case AudioManagerNapi::MEDIA:
        case AudioManagerNapi::VOICE_CALL:
        case AudioManagerNapi::VOICE_ASSISTANT:
        case AudioManagerNapi::ALL:
            result = true;
            break;
        default:
            result = false;
            break;
    }
    return result;
}

bool AudioCommonNapi::IsLegalInputArgumentDeviceFlag(int32_t deviceFlag)
{
    bool result = false;
    switch (deviceFlag) {
        case DeviceFlag::NONE_DEVICES_FLAG:
        case DeviceFlag::OUTPUT_DEVICES_FLAG:
        case DeviceFlag::INPUT_DEVICES_FLAG:
        case DeviceFlag::ALL_DEVICES_FLAG:
        case DeviceFlag::DISTRIBUTED_OUTPUT_DEVICES_FLAG:
        case DeviceFlag::DISTRIBUTED_INPUT_DEVICES_FLAG:
        case DeviceFlag::ALL_DISTRIBUTED_DEVICES_FLAG:
        case DeviceFlag::ALL_L_D_DEVICES_FLAG:
            result = true;
            break;
        default:
            result = false;
            break;
    }
    return result;
}

bool AudioCommonNapi::IsLegalInputArgumentActiveDeviceType(int32_t activeDeviceFlag)
{
    bool result = false;
    switch (activeDeviceFlag) {

        case ActiveDeviceType::SPEAKER:
        case ActiveDeviceType::BLUETOOTH_SCO:
    
            result = true;
            break;
        default:
            result = false;
            break;
    }
    return result;
}

bool AudioCommonNapi::IsLegalInputArgumentCommunicationDeviceType(int32_t communicationDeviceType)
{
    bool result = false;
    switch (communicationDeviceType) {

        case CommunicationDeviceType::COMMUNICATION_SPEAKER:    
            result = true;
            break;
        default:
            result = false;
            break;
    }
    return result;
}

bool AudioCommonNapi::IsLegalInputArgumentRingMode(int32_t ringerMode)
{
    bool result = false;
    switch (ringerMode) {
        case AudioRingerMode::RINGER_MODE_SILENT:
        case AudioRingerMode::RINGER_MODE_VIBRATE:
        case AudioRingerMode::RINGER_MODE_NORMAL:
            result = true;
            break;
        default:
            result = false;
            break;
    }
    return result;
}

}  // namespace AudioStandard
}  // namespace OHOS