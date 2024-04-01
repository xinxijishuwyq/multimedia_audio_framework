/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ST_AUDIO_POLICY_CONFIG_H
#define ST_AUDIO_POLICY_CONFIG_H

#include <list>
#include <set>
#include <unordered_map>
#include <string>

#include "audio_module_info.h"
#include "audio_info.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
static const std::string STR_INIT = "";

static const std::string ADAPTER_PRIMARY_TYPE = "primary";
static const std::string ADAPTER_A2DP_TYPE = "a2dp";
static const std::string ADAPTER_REMOTE_TYPE = "remote";
static const std::string ADAPTER_FILE_TYPE = "file";
static const std::string ADAPTER_USB_TYPE = "usb";

static const std::string ADAPTER_DEVICE_PRIMARY_SPEAKER = "Speaker";
static const std::string ADAPTER_DEVICE_PRIMARY_EARPIECE = "Earpicece";
static const std::string ADAPTER_DEVICE_PRIMARY_MIC = "Built-In Mic";
static const std::string ADAPTER_DEVICE_PRIMARY_WIRE_HEADSET = "Wired Headset";
static const std::string ADAPTER_DEVICE_PRIMARY_WIRE_HEADPHONE = "Wired Headphones";
static const std::string ADAPTER_DEVICE_PRIMARY_BT_SCO = "Bt Sco";
static const std::string ADAPTER_DEVICE_PRIMARY_BT_OFFLOAD = "Bt Offload";
static const std::string ADAPTER_DEVICE_PRIMARY_BT_HEADSET_HIFI = "Usb Headset Hifi";
static const std::string ADAPTER_DEVICE_A2DP_BT_A2DP = "Bt A2dp";
static const std::string ADAPTER_DEVICE_REMOTE_SINK = "Remote Sink";
static const std::string ADAPTER_DEVICE_REMOTE_SOURCE = "Remote Source";
static const std::string ADAPTER_DEVICE_FILE_SINK = "File Sink";
static const std::string ADAPTER_DEVICE_FILE_SOURCE = "File Source";
static const std::string ADAPTER_DEVICE_USB_HEADSET_ARM = "Usb Headset Arm";
static const std::string ADAPTER_DEVICE_USB_SPEAKER = "Usb_arm_speaker";
static const std::string ADAPTER_DEVICE_USB_MIC = "Usb_arm_mic";
static const std::string ADAPTER_DEVICE_PIPE_SINK = "fifo_output";
static const std::string ADAPTER_DEVICE_PIPE_SOURCE = "fifo_input";
static const std::string ADAPTER_DEVICE_WAKEUP = "Built_in_wakeup";
static const std::string ADAPTER_DEVICE_NONE = "none";

static const std::string MODULE_TYPE_SINK = "sink";
static const std::string MODULE_TYPE_SOURCE = "source";
static const std::string MODULE_SINK_OFFLOAD = "offload";
static const std::string MODULE_SINK_LIB = "libmodule-hdi-sink.z.so";
static const std::string MODULE_SOURCE_LIB = "libmodule-hdi-source.z.so";
static const std::string MODULE_FILE_SINK_FILE = "/data/data/.pulse_dir/file_sink.pcm";
static const std::string MODULE_FILE_SOURCE_FILE = "/data/data/.pulse_dir/file_source.pcm";

static const std::string CONFIG_TYPE_PRELOAD = "preload";
static const std::string CONFIG_TYPE_MAXINSTANCES = "maxinstances";

enum class XmlNodeType {
    ADAPTERS,
    VOLUME_GROUPS,
    INTERRUPT_GROUPS,
    EXTENDS,
    XML_UNKNOWN
};

enum class AdaptersType {
    TYPE_PRIMARY,
    TYPE_A2DP,
    TYPE_REMOTE,
    TYPE_FILE,
    TYPE_USB,
    TYPE_INVALID
};

enum class AdapterType {
    DEVICES,
    MODULES,
    UNKNOWN
};

enum class ModulesType {
    SINK,
    SOURCE,
    UNKNOWN
};

enum class ModuleType {
    CONFIGS,
    PROFILES,
    DEVICES,
    UNKNOWN
};

enum class ExtendType {
    UPDATE_ROUTE_SUPPORT,
    AUDIO_LATENCY,
    SINK_LATENCY,
    UNKNOWN
};

class ConfigInfo {
public:
    ConfigInfo() = default;
    virtual ~ConfigInfo() = default;

    std::string name_ = STR_INIT;
    std::string valu_ = STR_INIT;
};

class ProfileInfo {
public:
    ProfileInfo() = default;
    virtual ~ProfileInfo() = default;

    std::string rate_ = STR_INIT;
    std::string channels_ = STR_INIT;
    std::string format_ = STR_INIT;
    std::string bufferSize_ = STR_INIT;
};

class AudioAdapterDeviceInfo {
public:
    AudioAdapterDeviceInfo() = default;
    virtual ~AudioAdapterDeviceInfo() = default;

    std::string name_ = STR_INIT;
    std::string type_ = STR_INIT;
    std::string role_ = STR_INIT;
};

class ModuleInfo {
public:
    ModuleInfo() = default;
    virtual ~ModuleInfo() = default;

    std::string moduleType_ = STR_INIT;

    std::string name_ = STR_INIT;
    std::string lib_ = STR_INIT;
    std::string role_ = STR_INIT;
    std::string fixedLatency_ = STR_INIT;
    std::string renderInIdleState_ = STR_INIT;
    std::string profile_ = STR_INIT;
    std::string file_ = STR_INIT;

    std::list<ConfigInfo> configInfos_ {};
    std::list<ProfileInfo> profileInfos_ {};
    std::list<std::string> devices_ {};
};

class AudioAdapterInfo {
public:
    AudioAdapterInfo() = default;
    virtual ~AudioAdapterInfo() = default;

    std::string adapterName_ = STR_INIT;
    std::list<AudioAdapterDeviceInfo> deviceInfos_ {};
    std::list<ModuleInfo> moduleInfos_ {};
};
} // namespace AudioStandard
} // namespace OHOS

#endif // ST_AUDIO_POLICY_CONFIG_H
