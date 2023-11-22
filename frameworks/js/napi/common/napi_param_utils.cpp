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

#include "napi_param_utils.h"

namespace OHOS {
namespace AudioStandard {
napi_value NapiParamUtils::GetUndefinedValue(napi_env env)
{
    napi_value result {};
    napi_get_undefined(env, &result);
    return result;
}

napi_status NapiParamUtils::GetParam(const napi_env &env, napi_callback_info info, size_t &argc, napi_value *args)
{
    napi_value thisVar = nullptr;
    void *data;
    return napi_get_cb_info(env, info, &argc, args, &thisVar, &data);
}

napi_status NapiParamUtils::GetValueInt32(const napi_env &env, int32_t &value, napi_value in)
{
    napi_status status = napi_get_value_int32(env, in, &value);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "GetValueInt32 napi_get_value_int32 failed");
    return status;
}

napi_status NapiParamUtils::SetValueInt32(const napi_env &env, const int32_t &value, napi_value &result)
{
    napi_status status = napi_create_int32(env, value, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueInt32 napi_create_int32 failed");
    return status;
}

napi_status NapiParamUtils::GetValueInt32(const napi_env &env, const std::string &fieldStr,
    int32_t &value, napi_value in)
{
    napi_value jsValue = nullptr;
    napi_status status = napi_get_named_property(env, in, fieldStr.c_str(), &jsValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "GetValueInt32 napi_get_named_property failed");
    status = GetValueInt32(env, value, jsValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "GetValueInt32 napi_get_value_int32 failed");
    return status;
}

napi_status NapiParamUtils::SetValueInt32(const napi_env &env, const std::string &fieldStr,
    const int32_t value, napi_value &result)
{
    napi_value jsValue = nullptr;
    napi_status status = SetValueInt32(env, value, jsValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueInt32 napi_create_int32 failed");
    status = napi_set_named_property(env, result, fieldStr.c_str(), jsValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueInt32 napi_create_int32 failed");
    return status;
}

napi_status NapiParamUtils::GetValueUInt32(const napi_env &env, uint32_t &value, napi_value in)
{
    napi_status status = napi_get_value_uint32(env, in, &value);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "GetValueUInt32 napi_get_value_uint32 failed");
    return status;
}

napi_status NapiParamUtils::SetValueUInt32(const napi_env &env, const uint32_t &value, napi_value &result)
{
    napi_status status = napi_create_uint32(env, value, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueUInt32 napi_create_uint32 failed");
    return status;
}

napi_status NapiParamUtils::GetValueDouble(const napi_env &env, double &value, napi_value in)
{
    napi_status status = napi_get_value_double(env, in, &value);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "GetValueDouble napi_get_value_double failed");
    return status;
}

napi_status NapiParamUtils::SetValueDouble(const napi_env &env, const double &value, napi_value &result)
{
    napi_status status = napi_create_double(env, value, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueDouble napi_create_double failed");
    return status;
}

napi_status NapiParamUtils::GetValueDouble(const napi_env &env, const std::string &fieldStr,
    double &value, napi_value in)
{
    napi_value jsValue = nullptr;
    napi_status status = napi_get_named_property(env, in, fieldStr.c_str(), &jsValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "GetValueDouble napi_get_named_property failed");
    status = GetValueDouble(env, value, jsValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "GetValueDouble failed");
    return status;
}

napi_status NapiParamUtils::SetValueDouble(const napi_env &env, const std::string &fieldStr,
    const double value, napi_value &result)
{
    napi_value jsValue = nullptr;
    napi_status status = SetValueDouble(env, value, jsValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueDouble SetValueDouble failed");
    status = napi_set_named_property(env, result, fieldStr.c_str(), jsValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueDouble napi_set_named_property failed");
    return status;
}

std::string NapiParamUtils::GetPropertyString(napi_env env, napi_value value, const std::string &fieldStr)
{
    std::string invalid = "";
    bool exist = false;
    napi_status status = napi_has_named_property(env, value, fieldStr.c_str(), &exist);
    if (status != napi_ok || !exist) {
        AUDIO_ERR_LOG("can not find %{public}s property", fieldStr.c_str());
        return invalid;
    }

    napi_value item = nullptr;
    if (napi_get_named_property(env, value, fieldStr.c_str(), &item) != napi_ok) {
        AUDIO_ERR_LOG("get %{public}s property fail", fieldStr.c_str());
        return invalid;
    }

    return GetStringArgument(env, item);
}

std::string NapiParamUtils::GetStringArgument(napi_env env, napi_value value)
{
    std::string strValue = "";
    size_t bufLength = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &bufLength);
    if (status == napi_ok && bufLength > 0 && bufLength < PATH_MAX) {
        strValue.reserve(bufLength + 1);
        strValue.resize(bufLength);
        status = napi_get_value_string_utf8(env, value, strValue.data(), bufLength + 1, &bufLength);
        if (status == napi_ok) {
            AUDIO_DEBUG_LOG("argument = %{public}s", strValue.c_str());
        }
    }
    return strValue;
}

napi_status NapiParamUtils::SetValueString(const napi_env &env, const std::string stringValue, napi_value &result)
{
    napi_status status = napi_create_string_utf8(env, stringValue.c_str(), NAPI_AUTO_LENGTH, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueString napi_create_string_utf8 failed");
    return status;
}

napi_status NapiParamUtils::SetValueString(const napi_env &env, const std::string &fieldStr,
    const std::string stringValue, napi_value &result)
{
    napi_value value = nullptr;
    napi_status status = SetValueString(env, stringValue.c_str(), value);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueString failed");
    status = napi_set_named_property(env, result, fieldStr.c_str(), value);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueString napi_set_named_property failed");
    return status;
}

napi_status NapiParamUtils::GetValueBoolean(const napi_env &env, bool &boolValue, napi_value in)
{
    napi_status status = napi_get_value_bool(env, in, &boolValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "GetValueBoolean napi_get_boolean failed");
    return status;
}

napi_status NapiParamUtils::SetValueBoolean(const napi_env &env, const bool boolValue, napi_value &result)
{
    napi_status status = napi_get_boolean(env, boolValue, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueBoolean napi_get_boolean failed");
    return status;
}

napi_status NapiParamUtils::GetValueBoolean(const napi_env &env, const std::string &fieldStr,
    bool &boolValue, napi_value in)
{
    napi_value jsValue = nullptr;
    napi_status status = napi_get_named_property(env, in, fieldStr.c_str(), &jsValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "GetValueBoolean napi_get_named_property failed");
    status = GetValueBoolean(env, boolValue, jsValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "GetValueBoolean failed");
    return status;
}

napi_status NapiParamUtils::SetValueBoolean(const napi_env &env, const std::string &fieldStr,
    const bool boolValue, napi_value &result)
{
    napi_value value = nullptr;
    napi_status status = SetValueBoolean(env, boolValue, value);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueBoolean SetValueBoolean failed");
    napi_set_named_property(env, result, fieldStr.c_str(), value);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueBoolean napi_get_boolean failed");
    return status;
}

napi_status NapiParamUtils::GetValueInt64(const napi_env &env, int64_t &value, napi_value in)
{
    napi_status status = napi_get_value_int64(env, in, &value);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "GetValueInt64 napi_get_value_int64 failed");
    return status;
}

napi_status NapiParamUtils::SetValueInt64(const napi_env &env, const int64_t &value, napi_value &result)
{
    napi_status status = napi_create_int64(env, value, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueInt64 napi_create_int64 failed");
    return status;
}

napi_status NapiParamUtils::GetValueInt64(const napi_env &env, const std::string &fieldStr,
    int64_t &value, napi_value in)
{
    napi_value jsValue = nullptr;
    napi_status status = napi_get_named_property(env, in, fieldStr.c_str(), &jsValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "GetValueInt64 napi_get_named_property failed");
    status = GetValueInt64(env, value, jsValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "GetValueInt64 failed");
    return status;
}

napi_status NapiParamUtils::SetValueInt64(const napi_env &env, const std::string &fieldStr,
    const int64_t value, napi_value &result)
{
    napi_value jsValue = nullptr;
    napi_status status = SetValueInt64(env, value, jsValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueInt64 failed");
    status = napi_set_named_property(env, result, fieldStr.c_str(), jsValue);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueInt64 napi_set_named_property failed");
    return status;
}

napi_status NapiParamUtils::GetArrayBuffer(const napi_env &env, void* &data, size_t &length, napi_value in)
{
    napi_status status = napi_get_arraybuffer_info(env, in, &data, &length);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "GetArrayBuffer napi_get_arraybuffer_info failed");
    return status;
}

napi_status NapiParamUtils::CreateArrayBuffer(const napi_env &env, const std::string &fieldStr, size_t bufferLen,
    uint8_t *bufferData, napi_value &result)
{
    napi_value value = nullptr;

    napi_status status = napi_create_arraybuffer(env, bufferLen, (void**)&bufferData, &value);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "napi_create_arraybuffer failed");
    status = napi_set_named_property(env, result, fieldStr.c_str(), value);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "napi_set_named_property failed");

    return status;
}

void NapiParamUtils::ConvertDeviceInfoToAudioDeviceDescriptor(sptr<AudioDeviceDescriptor> audioDeviceDescriptor,
    const DeviceInfo &deviceInfo)
{
    CHECK_AND_RETURN_LOG(audioDeviceDescriptor != nullptr, "audioDeviceDescriptor is nullptr");
    audioDeviceDescriptor->deviceRole_ = deviceInfo.deviceRole;
    audioDeviceDescriptor->deviceType_ = deviceInfo.deviceType;
    audioDeviceDescriptor->deviceId_ = deviceInfo.deviceId;
    audioDeviceDescriptor->channelMasks_ = deviceInfo.channelMasks;
    audioDeviceDescriptor->channelIndexMasks_ = deviceInfo.channelIndexMasks;
    audioDeviceDescriptor->deviceName_ = deviceInfo.deviceName;
    audioDeviceDescriptor->macAddress_ = deviceInfo.macAddress;
    audioDeviceDescriptor->interruptGroupId_ = deviceInfo.interruptGroupId;
    audioDeviceDescriptor->volumeGroupId_ = deviceInfo.volumeGroupId;
    audioDeviceDescriptor->networkId_ = deviceInfo.networkId;
    audioDeviceDescriptor->displayName_ = deviceInfo.displayName;
    audioDeviceDescriptor->audioStreamInfo_.samplingRate = deviceInfo.audioStreamInfo.samplingRate;
    audioDeviceDescriptor->audioStreamInfo_.encoding = deviceInfo.audioStreamInfo.encoding;
    audioDeviceDescriptor->audioStreamInfo_.format = deviceInfo.audioStreamInfo.format;
    audioDeviceDescriptor->audioStreamInfo_.channels = deviceInfo.audioStreamInfo.channels;
}

napi_status NapiParamUtils::GetRendererOptions(const napi_env &env, AudioRendererOptions *opts, napi_value in)
{
    napi_value res = nullptr;

    napi_status status = napi_get_named_property(env, in, "rendererInfo", &res);
    if (status == napi_ok) {
        GetRendererInfo(env, &(opts->rendererInfo), res);
    }

    status = napi_get_named_property(env, in, "streamInfo", &res);
    if (status == napi_ok) {
        GetStreamInfo(env, &(opts->streamInfo), res);
    }

    int32_t intValue = {};
    status = GetValueInt32(env, "privacyType", intValue, in);
    if (status == napi_ok) {
        opts->privacyType = static_cast<AudioPrivacyType>(intValue);
    }

    return napi_ok;
}

napi_status NapiParamUtils::GetRendererInfo(const napi_env &env, AudioRendererInfo *rendererInfo, napi_value in)
{
    int32_t intValue = {0};
    napi_status status = GetValueInt32(env, "content", intValue, in);
    if (status == napi_ok) {
        rendererInfo->contentType = static_cast<ContentType>(intValue);
    }

    status = GetValueInt32(env, "usage", intValue, in);
    if (status == napi_ok) {
        rendererInfo->streamUsage = static_cast<StreamUsage>(intValue);
    }

    GetValueInt32(env, "rendererFlags", rendererInfo->rendererFlags, in);

    return napi_ok;
}

napi_status NapiParamUtils::SetRendererInfo(const napi_env &env, const AudioRendererInfo &rendererInfo,
    napi_value &result)
{
    napi_status status = napi_create_object(env, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetRendererInfo napi_create_object failed");
    SetValueInt32(env, "content", static_cast<int32_t>(rendererInfo.contentType), result);
    SetValueInt32(env, "usage", static_cast<int32_t>(rendererInfo.streamUsage), result);
    SetValueInt32(env, "rendererFlags", rendererInfo.rendererFlags, result);

    return napi_ok;
}

napi_status NapiParamUtils::GetStreamInfo(const napi_env &env, AudioStreamInfo *streamInfo, napi_value in)
{
    int32_t intValue = {0};
    napi_status status = GetValueInt32(env, "samplingRate", intValue, in);
    if (status == napi_ok) {
        streamInfo->samplingRate = static_cast<AudioSamplingRate>(intValue);
    }

    status = GetValueInt32(env, "channels", intValue, in);
    if (status == napi_ok) {
        streamInfo->channels = static_cast<AudioChannel>(intValue);
    }

    status = GetValueInt32(env, "sampleFormat", intValue, in);
    if (status == napi_ok) {
        streamInfo->format = static_cast<OHOS::AudioStandard::AudioSampleFormat>(intValue);
    }

    status = GetValueInt32(env, "encodingType", intValue, in);
    if (status == napi_ok) {
        streamInfo->encoding = static_cast<AudioEncodingType>(intValue);
    }

    return napi_ok;
}

napi_status NapiParamUtils::SetStreamInfo(const napi_env &env, const AudioStreamInfo &streamInfo, napi_value &result)
{
    napi_status status = napi_create_object(env, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetStreamInfo napi_create_object failed");
    SetValueInt32(env, "samplingRate", static_cast<int32_t>(streamInfo.samplingRate), result);
    SetValueInt32(env, "channels", static_cast<int32_t>(streamInfo.channels), result);
    SetValueInt32(env, "sampleFormat", static_cast<int32_t>(streamInfo.format), result);
    SetValueInt32(env, "encodingType", static_cast<int32_t>(streamInfo.encoding), result);

    return napi_ok;
}

napi_status NapiParamUtils::SetValueInt32Element(const napi_env &env, const std::string &fieldStr,
    const std::vector<int32_t> &values, napi_value &result)
{
    napi_value jsValues = nullptr;
    napi_status status = napi_create_array_with_length(env, values.size(), &jsValues);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueInt32Element napi_create_array_with_length failed");
    int32_t count = 0;
    for (auto value : values) {
        napi_value jsValue = nullptr;
        status = SetValueInt32(env, value, jsValue);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueInt32Element SetValueInt32 failed");
        status = napi_set_element(env, jsValues, count, jsValue);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueInt32Element napi_set_element failed");
        count++;
    }
    status = napi_set_named_property(env, result, fieldStr.c_str(), jsValues);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "SetValueInt32Element napi_set_named_property failed");
    return status;
}

napi_status NapiParamUtils::SetDeviceDescriptor(const napi_env &env, const AudioDeviceDescriptor &deviceInfo,
    napi_value &result)
{
    (void)napi_create_object(env, &result);
    SetValueInt32(env, "deviceRole", static_cast<int32_t>(deviceInfo.deviceRole_), result);
    SetValueInt32(env, "deviceType", static_cast<int32_t>(deviceInfo.deviceType_), result);
    SetValueInt32(env, "id", static_cast<int32_t>(deviceInfo.deviceId_), result);
    SetValueString(env, "name", deviceInfo.deviceName_, result);
    SetValueString(env, "address", deviceInfo.macAddress_, result);
    SetValueString(env, "networkId", deviceInfo.networkId_, result);
    SetValueString(env, "displayName", deviceInfo.displayName_, result);
    SetValueInt32(env, "interruptGroupId", static_cast<int32_t>(deviceInfo.interruptGroupId_), result);
    SetValueInt32(env, "volumeGroupId", static_cast<int32_t>(deviceInfo.volumeGroupId_), result);

    std::vector<int32_t> samplingRate;
    samplingRate.push_back(deviceInfo.audioStreamInfo_.samplingRate);
    SetValueInt32Element(env, "sampleRates", samplingRate, result);

    std::vector<int32_t> channels;
    channels.push_back(deviceInfo.audioStreamInfo_.channels);
    SetValueInt32Element(env, "channelCounts", channels, result);

    std::vector<int32_t> channelMasks_;
    channelMasks_.push_back(deviceInfo.channelMasks_);
    SetValueInt32Element(env, "channelMasks", channelMasks_, result);

    std::vector<int32_t> channelIndexMasks_;
    channelIndexMasks_.push_back(deviceInfo.channelIndexMasks_);
    SetValueInt32Element(env, "channelIndexMasks", channelIndexMasks_, result);

    std::vector<int32_t> encoding;
    encoding.push_back(deviceInfo.audioStreamInfo_.encoding);
    SetValueInt32Element(env, "encodingTypes", encoding, result);

    return napi_ok;
}

napi_status NapiParamUtils::SetDeviceDescriptors(const napi_env &env,
    const std::vector<sptr<AudioDeviceDescriptor>> &deviceDescriptors, napi_value &result)
{
    napi_status status = napi_create_array_with_length(env, deviceDescriptors.size(), &result);
    for (size_t i = 0; i < deviceDescriptors.size(); i++) {
        if (deviceDescriptors[i] != nullptr) {
            napi_value valueParam = nullptr;
            SetDeviceDescriptor(env, deviceDescriptors[i], valueParam);
            napi_set_element(env, result, i, valueParam);
        }
    }
    return status;
}


napi_status NapiParamUtils::SetValueDeviceInfo(const napi_env &env, const DeviceInfo &deviceInfo, napi_value &result)
{
    std::vector<sptr<AudioDeviceDescriptor>> deviceDescriptors;
    sptr<AudioDeviceDescriptor> audioDeviceDescriptor = new(std::nothrow) AudioDeviceDescriptor();
    CHECK_AND_RETURN_RET_LOG(audioDeviceDescriptor != nullptr, napi_generic_failure,
        "audioDeviceDescriptor malloc failed");
    ConvertDeviceInfoToAudioDeviceDescriptor(audioDeviceDescriptor, deviceInfo);
    deviceDescriptors.push_back(std::move(audioDeviceDescriptor));
    SetDeviceDescriptors(env, deviceDescriptors, result);
    return napi_ok;
}

napi_status NapiParamUtils::SetInterruptEvent(const napi_env &env, const InterruptEvent &interruptEvent,
    napi_value &result)
{
    napi_create_object(env, &result);
    SetValueInt32(env, "eventType", static_cast<int32_t>(interruptEvent.eventType), result);
    SetValueInt32(env, "forceType", static_cast<int32_t>(interruptEvent.forceType), result);
    SetValueInt32(env, "hintType", static_cast<int32_t>(interruptEvent.hintType), result);
    return napi_ok;
}

napi_status NapiParamUtils::SetNativeAudioRendererDataInfo(const napi_env &env,
    const AudioRendererDataInfo &audioRendererDataInfo, napi_value &result)
{
    napi_status status = napi_create_object(env, &result);

    SetValueInt32(env, "flag", static_cast<int32_t>(audioRendererDataInfo.flag), result);
    CreateArrayBuffer(env, "buffer", audioRendererDataInfo.flag, audioRendererDataInfo.buffer, result);

    return status;
}
} // namespace AudioStandard
} // namespace OHOS
