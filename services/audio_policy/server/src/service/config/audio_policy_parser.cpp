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

#undef LOG_TAG
#define LOG_TAG "AudioPolicyParser"

#include "audio_policy_parser.h"

#include <regex>
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
bool AudioPolicyParser::LoadConfiguration()
{
    doc_ = xmlReadFile(CHIP_PROD_CONFIG_FILE, nullptr, 0);
    if (doc_ == nullptr) {
        doc_ = xmlReadFile(CONFIG_FILE, nullptr, 0);
        if (doc_ == nullptr) {
            AUDIO_ERR_LOG("xmlReadFile Failed");
            return false;
        }
    }
    return true;
}

bool AudioPolicyParser::Parse()
{
    xmlNode *root = xmlDocGetRootElement(doc_);
    if (root == nullptr) {
        AUDIO_ERR_LOG("xmlDocGetRootElement Failed");
        return false;
    }
    if (!ParseInternal(*root)) {
        AUDIO_ERR_LOG("Audio policy config xml parse failed.");
        return false;
    }

    std::unordered_map<std::string, std::string> volumeGroupMap {};
    std::unordered_map<std::string, std::string> interruptGroupMap {};

    ConvertAdapterInfoToAudioModuleInfo(volumeGroupMap, interruptGroupMap);

    volumeGroupMap_ = volumeGroupMap;
    interruptGroupMap_ = interruptGroupMap;

    portObserver_.OnAudioPolicyXmlParsingCompleted(adapterInfoMap_);
    portObserver_.OnXmlParsingCompleted(xmlParsedDataMap_);
    portObserver_.OnVolumeGroupParsed(volumeGroupMap_);
    portObserver_.OnInterruptGroupParsed(interruptGroupMap_);
    return true;
}

void AudioPolicyParser::Destroy()
{
    if (doc_ != nullptr) {
        xmlFreeDoc(doc_);
    }
}

bool AudioPolicyParser::ParseInternal(xmlNode &node)
{
    xmlNode *currNode = &node;
    for (; currNode; currNode = currNode->next) {
        if (XML_ELEMENT_NODE == currNode->type) {
            switch (GetNodeTypeAsInt(*currNode)) {
                case XmlNodeType::ADAPTERS:
                    ParseAdapters(*currNode);
                    break;
                case XmlNodeType::VOLUME_GROUPS:
                    ParseGroups(*currNode, XmlNodeType::VOLUME_GROUPS);
                    break;
                case XmlNodeType::INTERRUPT_GROUPS:
                    ParseGroups(*currNode, XmlNodeType::INTERRUPT_GROUPS);
                    break;
                case XmlNodeType::EXTENDS:
                    ParseExtends(*currNode);
                    break;
                default:
                    ParseInternal(*(currNode->children));
                    break;
            }
        }
    }
    return true;
}

void AudioPolicyParser::ConvertAdapterInfoToGroupInfo(std::unordered_map<std::string, std::string> &volumeGroupMap,
    std::unordered_map<std::string, std::string> &interruptGroupMap, ModuleInfo &moduleInfo)
{
    for (auto &[sinkName, groupName] : volumeGroupMap_) {
        if (sinkName == moduleInfo.name_) {
            volumeGroupMap[moduleInfo.devices_.front()] = groupName;
        }
    }

    for (auto &[sinkName, groupName] : interruptGroupMap_) {
        if (sinkName == moduleInfo.name_) {
            interruptGroupMap[moduleInfo.devices_.front()] = groupName;
        }
    }
}

void AudioPolicyParser::GetCommontAudioModuleInfo(ModuleInfo &moduleInfo, AudioModuleInfo &audioModuleInfo)
{
    for (auto profileInfo : moduleInfo.profileInfos_) {
        audioModuleInfo.supportedRate_.insert(atoi(profileInfo.rate_.c_str()));
        audioModuleInfo.supportedChannels_.insert(atoi(profileInfo.channels_.c_str()));
    }
    audioModuleInfo.role = moduleInfo.moduleType_;
    audioModuleInfo.name = moduleInfo.devices_.front();

    if (audioModuleInfo.name == ADAPTER_DEVICE_PRIMARY_MIC) {
        audioModuleInfo.name = PRIMARY_MIC;
    } else if (audioModuleInfo.name == ADAPTER_DEVICE_A2DP_BT_A2DP) {
        audioModuleInfo.name = BLUETOOTH_SPEAKER;
    } else if (audioModuleInfo.name == ADAPTER_DEVICE_USB_SPEAKER ||
        (audioModuleInfo.role == MODULE_TYPE_SINK && audioModuleInfo.name == ADAPTER_DEVICE_USB_HEADSET_ARM)) {
        audioModuleInfo.name = USB_SPEAKER;
    } else if (audioModuleInfo.name == ADAPTER_DEVICE_USB_MIC ||
        (audioModuleInfo.role == MODULE_TYPE_SOURCE && audioModuleInfo.name == ADAPTER_DEVICE_USB_HEADSET_ARM)) {
        audioModuleInfo.name = USB_MIC;
    } else if (audioModuleInfo.name == ADAPTER_DEVICE_FILE_SINK) {
        audioModuleInfo.name = FILE_SINK;
    } else if (audioModuleInfo.name == ADAPTER_DEVICE_FILE_SOURCE) {
        audioModuleInfo.name = FILE_SOURCE;
    } else if (audioModuleInfo.name == ADAPTER_DEVICE_PIPE_SINK) {
        audioModuleInfo.name = PIPE_SINK;
    } else if (audioModuleInfo.name == ADAPTER_DEVICE_PIPE_SOURCE) {
        audioModuleInfo.name = PIPE_SOURCE;
    } else if (audioModuleInfo.name == ADAPTER_DEVICE_DP) {
        audioModuleInfo.name = DP_SINK;
    }

    audioModuleInfo.lib = moduleInfo.lib_;
    if (moduleInfo.profileInfos_.begin() != moduleInfo.profileInfos_.end()) {
        audioModuleInfo.rate = moduleInfo.profileInfos_.begin()->rate_;
        audioModuleInfo.format = moduleInfo.profileInfos_.begin()->format_;
        audioModuleInfo.channels = moduleInfo.profileInfos_.begin()->channels_;
        audioModuleInfo.bufferSize = moduleInfo.profileInfos_.begin()->bufferSize_;
    }
    audioModuleInfo.fileName = moduleInfo.file_;

    audioModuleInfo.fixedLatency = moduleInfo.fixedLatency_;
    audioModuleInfo.renderInIdleState = moduleInfo.renderInIdleState_;
}

ClassType AudioPolicyParser::GetClassTypeByAdapterType(AdaptersType adapterType)
{
    if (adapterType == AdaptersType::TYPE_PRIMARY) {
        return ClassType::TYPE_PRIMARY;
    } else if (adapterType == AdaptersType::TYPE_A2DP) {
        return ClassType::TYPE_A2DP;
    } else if (adapterType == AdaptersType::TYPE_REMOTE) {
        return ClassType::TYPE_REMOTE_AUDIO;
    } else if (adapterType == AdaptersType::TYPE_FILE) {
        return ClassType::TYPE_FILE_IO;
    } else if (adapterType == AdaptersType::TYPE_USB) {
        return ClassType::TYPE_USB;
    }  else if (adapterType == AdaptersType::TYPE_DP) {
        return ClassType::TYPE_DP;
    } else {
        return ClassType::TYPE_INVALID;
    }
}

void AudioPolicyParser::GetOffloadAndOpenMicState(AudioAdapterInfo &adapterInfo,
    bool &shouldEnableOffload)
{
    for (auto &moduleInfo : adapterInfo.moduleInfos_) {
        if (moduleInfo.moduleType_ == MODULE_TYPE_SINK &&
            moduleInfo.name_.find(MODULE_SINK_OFFLOAD) != std::string::npos) {
            shouldEnableOffload = true;
        }
    }
}

void AudioPolicyParser::ConvertAdapterInfoToAudioModuleInfo(
    std::unordered_map<std::string, std::string> &volumeGroupMap,
    std::unordered_map<std::string, std::string> &interruptGroupMap)
{
    for (auto &[adapterType, adapterInfo] : adapterInfoMap_) {
        std::list<AudioModuleInfo> audioModuleList = {};
        bool shouldEnableOffload = false;
        if (adapterType == AdaptersType::TYPE_PRIMARY) {
            GetOffloadAndOpenMicState(adapterInfo, shouldEnableOffload);
        }

        for (auto &moduleInfo : adapterInfo.moduleInfos_) {
            ConvertAdapterInfoToGroupInfo(volumeGroupMap, interruptGroupMap, moduleInfo);
            CHECK_AND_CONTINUE_LOG(moduleInfo.name_.find(MODULE_SINK_OFFLOAD) == std::string::npos,
                "skip offload out sink.");
            AudioModuleInfo audioModuleInfo = {};
            GetCommontAudioModuleInfo(moduleInfo, audioModuleInfo);

            audioModuleInfo.className = adapterInfo.adapterName_;
            audioModuleInfo.adapterName = adapterInfo.adapterName_;
            if (adapterType == AdaptersType::TYPE_FILE) {
                audioModuleInfo.adapterName = STR_INIT;
                audioModuleInfo.format = STR_INIT;
                audioModuleInfo.className = FILE_CLASS;
            }

            shouldOpenMicSpeaker_ ? audioModuleInfo.OpenMicSpeaker = "1" : audioModuleInfo.OpenMicSpeaker = "0";
            if (adapterType == AdaptersType::TYPE_PRIMARY &&
                shouldEnableOffload && moduleInfo.moduleType_ == MODULE_TYPE_SINK) {
                audioModuleInfo.offloadEnable = "1";
            }
            audioModuleList.push_back(audioModuleInfo);
        }
        std::list<AudioModuleInfo> audioModuleListTmp = audioModuleList;
        std::list<AudioModuleInfo> audioModuleListData = {};
        for (auto audioModuleInfo : audioModuleList) {
            audioModuleInfo.ports = audioModuleListTmp;
            audioModuleListData.push_back(audioModuleInfo);
        }
        ClassType classType = GetClassTypeByAdapterType(adapterType);
        xmlParsedDataMap_[classType] = audioModuleListData;
    }
}

void AudioPolicyParser::ParseAdapters(xmlNode &node)
{
    xmlNode *adapterNode = nullptr;
    adapterNode = node.xmlChildrenNode;

    while (adapterNode != nullptr) {
        if (adapterNode->type == XML_ELEMENT_NODE) {
            ParseAdapter(*adapterNode);
        }
        adapterNode = adapterNode->next;
    }
}

void AudioPolicyParser::ParseAdapter(xmlNode &node)
{
    std::string adapterName = ExtractPropertyValue("name", node);
    if (adapterName.empty()) {
        AUDIO_ERR_LOG("No name provided for the adapter class %{public}s", node.name);
        return;
    }

    AdaptersType adaptersType = GetAdaptersType(adapterName);
    adapterInfoMap_[adaptersType] = {};

    xmlNode *currNode = node.xmlChildrenNode;
    AudioAdapterInfo adapterInfo = {};
    adapterInfo.adapterName_ = adapterName;
    for (; currNode; currNode = currNode->next) {
        if (XML_ELEMENT_NODE == currNode->type) {
            switch (GetAdapterTypeAsInt(*currNode)) {
                case AdapterType::DEVICES:
                    ParseAudioAdapterDevices(*currNode, adapterInfo);
                    break;
                case AdapterType::MODULES:
                    ParseModules(*currNode, adapterInfo);
                    break;
                default:
                    ParseAdapter(*(currNode->children));
                    break;
            }
        }
    }
    adapterInfoMap_[adaptersType] = adapterInfo;
}

void AudioPolicyParser::ParseAudioAdapterDevices(xmlNode &node, AudioAdapterInfo &adapterInfo)
{
    xmlNode *deviceNode = nullptr;
    deviceNode = node.xmlChildrenNode;
    std::list<AudioAdapterDeviceInfo> deviceInfos;

    while (deviceNode != nullptr) {
        if (deviceNode->type == XML_ELEMENT_NODE) {
            AudioAdapterDeviceInfo deviceInfo {};
            deviceInfo.name_ = ExtractPropertyValue("name", *deviceNode);
            deviceInfo.type_ = ExtractPropertyValue("type", *deviceNode);
            deviceInfo.role_ = ExtractPropertyValue("role", *deviceNode);
            deviceInfos.push_back(deviceInfo);
        }
        deviceNode = deviceNode->next;
    }
    adapterInfo.deviceInfos_ = deviceInfos;
}

void AudioPolicyParser::ParseModules(xmlNode &node, AudioAdapterInfo &adapterInfo)
{
    xmlNode *moduleNode = nullptr;
    moduleNode = node.xmlChildrenNode;

    std::list<ModuleInfo> moduleInfos;

    for (; moduleNode; moduleNode = moduleNode->next) {
        if (XML_ELEMENT_NODE == moduleNode->type) {
            ModuleInfo moduleInfo {};
            ParseModuleProperty(*moduleNode, moduleInfo, adapterInfo.adapterName_);
            ParseModule(*moduleNode, moduleInfo);
            moduleInfos.push_back(moduleInfo);
        }
    }

    adapterInfo.moduleInfos_ = moduleInfos;
}

void AudioPolicyParser::ParseModuleProperty(xmlNode &node, ModuleInfo &moduleInfo, std::string adapterName)
{
    moduleInfo.moduleType_ = reinterpret_cast<const char*>(node.name);
    if (moduleInfo.moduleType_ == MODULE_TYPE_SINK) {
        moduleInfo.lib_ =  MODULE_SINK_LIB;
        if (adapterName == ADAPTER_FILE_TYPE) {
            moduleInfo.file_ = MODULE_FILE_SINK_FILE;
        }
    }
    if (moduleInfo.moduleType_ == MODULE_TYPE_SOURCE) {
        moduleInfo.lib_ =  MODULE_SOURCE_LIB;
        if (adapterName == ADAPTER_FILE_TYPE) {
            moduleInfo.file_ = MODULE_FILE_SOURCE_FILE;
        }
    }

    moduleInfo.name_ = ExtractPropertyValue("name", node);
    moduleInfo.role_ = ExtractPropertyValue("role", node);
    moduleInfo.fixedLatency_ = ExtractPropertyValue("fixed_latency", node);
    moduleInfo.renderInIdleState_ = ExtractPropertyValue("render_in_idle_state", node);
    moduleInfo.profile_ = ExtractPropertyValue("profile", node);
}

void AudioPolicyParser::ParseModule(xmlNode &node, ModuleInfo &moduleInfo)
{
    xmlNode *currNode = nullptr;
    currNode = node.xmlChildrenNode;

    for (; currNode; currNode = currNode->next) {
        if (currNode->type == XML_ELEMENT_NODE) {
            switch (GetModuleTypeAsInt(*currNode)) {
                case ModuleType::CONFIGS:
                    ParseConfigs(*currNode, moduleInfo);
                    break;
                case ModuleType::PROFILES:
                    ParseProfiles(*currNode, moduleInfo);
                    break;
                case ModuleType::DEVICES:
                    ParseModuleDevices(*currNode, moduleInfo);
                    break;
                default:
                    ParseModule(*(currNode->children), moduleInfo);
                    break;
            }
        }
    }
}

void AudioPolicyParser::ParseConfigs(xmlNode &node, ModuleInfo &moduleInfo)
{
    xmlNode *configNode = nullptr;
    configNode = node.xmlChildrenNode;
    std::list<ConfigInfo> configInfos;

    while (configNode != nullptr) {
        if (configNode->type == XML_ELEMENT_NODE) {
            ConfigInfo configInfo = {};
            configInfo.name_ = ExtractPropertyValue("name", *configNode);
            configInfo.valu_ = ExtractPropertyValue("valu", *configNode);
            configInfos.push_back(configInfo);
        }
        configNode = configNode->next;
    }
    moduleInfo.configInfos_ = configInfos;
}

void AudioPolicyParser::ParseProfiles(xmlNode &node, ModuleInfo &moduleInfo)
{
    xmlNode *profileNode = nullptr;
    profileNode = node.xmlChildrenNode;
    std::list<ProfileInfo> profileInfoList = {};

    while (profileNode != nullptr) {
        if (profileNode->type == XML_ELEMENT_NODE) {
            ProfileInfo profileInfo = {};
            profileInfo.rate_ = ExtractPropertyValue("rate", *profileNode);
            profileInfo.channels_ = ExtractPropertyValue("channels", *profileNode);
            profileInfo.format_ = ExtractPropertyValue("format", *profileNode);
            profileInfo.bufferSize_ = ExtractPropertyValue("buffer_size", *profileNode);

            profileInfoList.push_back(profileInfo);
        }
        profileNode = profileNode->next;
    }
    moduleInfo.profileInfos_ = profileInfoList;
}

void AudioPolicyParser::ParseModuleDevices(xmlNode &node, ModuleInfo &moduleInfo)
{
    xmlNode *deviceNode = nullptr;
    deviceNode = node.xmlChildrenNode;

    std::list<std::string> devices {};

    while (deviceNode != nullptr) {
        if (deviceNode->type == XML_ELEMENT_NODE) {
            std::string device = ExtractPropertyValue("name", *deviceNode);
            devices.push_back(device);
        }
        deviceNode = deviceNode->next;
    }
    moduleInfo.devices_ = devices;
}

void AudioPolicyParser::ParseGroups(xmlNode& node, XmlNodeType type)
{
    xmlNode* groupsNode = nullptr;
    groupsNode = node.xmlChildrenNode;
    while (groupsNode != nullptr) {
        if (groupsNode->type == XML_ELEMENT_NODE) {
            ParseGroup(*groupsNode, type);
        }
        groupsNode = groupsNode->next;
    }
}

void AudioPolicyParser::ParseGroup(xmlNode& node, XmlNodeType type)
{
    xmlNode* groupNode = nullptr;
    groupNode = node.xmlChildrenNode;

    while (groupNode != nullptr) {
        if (groupNode->type == XML_ELEMENT_NODE) {
            std::string groupName = ExtractPropertyValue("name", *groupNode);
            ParseGroupSink(*groupNode, type, groupName);
        }
        groupNode = groupNode->next;
    }
}

void AudioPolicyParser::ParseGroupSink(xmlNode& node, XmlNodeType type, std::string groupName)
{
    xmlNode* sinkNode = nullptr;
    sinkNode = node.xmlChildrenNode;
    while (sinkNode != nullptr) {
        if (sinkNode->type == XML_ELEMENT_NODE) {
            std::string sinkName = ExtractPropertyValue("name", *sinkNode);
            if (type == XmlNodeType::VOLUME_GROUPS) {
                volumeGroupMap_[sinkName] = groupName;
            } else if (type == XmlNodeType::INTERRUPT_GROUPS) {
                interruptGroupMap_[sinkName] = groupName;
            }
        }
        sinkNode = sinkNode->next;
    }
}

void AudioPolicyParser::ParseExtends(xmlNode& node)
{
    xmlNode *currNode = node.xmlChildrenNode;
    for (; currNode; currNode = currNode->next) {
        if (XML_ELEMENT_NODE == currNode->type) {
            xmlChar *extendInfo = xmlNodeGetContent(currNode);
            std::string sExtendInfo(reinterpret_cast<char *>(extendInfo));
            switch (GetExtendTypeAsInt(*currNode)) {
                case ExtendType::UPDATE_ROUTE_SUPPORT:
                    ParseUpdateRouteSupport(*currNode);
                    break;
                case ExtendType::AUDIO_LATENCY:
                    portObserver_.OnAudioLatencyParsed((uint64_t)std::stoi(sExtendInfo));
                    break;
                case ExtendType::SINK_LATENCY:
                    portObserver_.OnSinkLatencyParsed((uint64_t)std::stoi(sExtendInfo));
                    break;
                default:
                    ParseExtends(*(currNode->children));
                    break;
            }
        }
    }
}

void AudioPolicyParser::ParseUpdateRouteSupport(xmlNode &node)
{
    xmlNode *child = node.children;
    xmlChar *supportFlag = xmlNodeGetContent(child);

    if (!xmlStrcmp(supportFlag, reinterpret_cast<const xmlChar*>("true"))) {
        portObserver_.OnUpdateRouteSupport(true);
        shouldOpenMicSpeaker_ = true;
    } else {
        portObserver_.OnUpdateRouteSupport(false);
        shouldOpenMicSpeaker_ = false;
    }
    xmlFree(supportFlag);
}

XmlNodeType AudioPolicyParser::GetNodeTypeAsInt(xmlNode &node)
{
    if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("adapters"))) {
        return XmlNodeType::ADAPTERS;
    } else if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("volumeGroups"))) {
        return XmlNodeType::VOLUME_GROUPS;
    } else if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("interruptGroups"))) {
        return XmlNodeType::INTERRUPT_GROUPS;
    } else if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("extends"))) {
        return XmlNodeType::EXTENDS;
    } else {
        return XmlNodeType::XML_UNKNOWN;
    }
}

std::string AudioPolicyParser::ExtractPropertyValue(const std::string &propName, xmlNode &node)
{
    std::string propValue = "";
    xmlChar *tempValue = nullptr;

    if (xmlHasProp(&node, reinterpret_cast<const xmlChar*>(propName.c_str()))) {
        tempValue = xmlGetProp(&node, reinterpret_cast<const xmlChar*>(propName.c_str()));
    }

    if (tempValue != nullptr) {
        propValue = reinterpret_cast<const char*>(tempValue);
        xmlFree(tempValue);
    }

    return propValue;
}

AdaptersType AudioPolicyParser::GetAdaptersType(const std::string &adapterName)
{
    if (adapterName == ADAPTER_PRIMARY_TYPE)
        return AdaptersType::TYPE_PRIMARY;
    else if (adapterName == ADAPTER_A2DP_TYPE)
        return AdaptersType::TYPE_A2DP;
    else if (adapterName == ADAPTER_REMOTE_TYPE)
        return AdaptersType::TYPE_REMOTE;
    else if (adapterName == ADAPTER_FILE_TYPE)
        return AdaptersType::TYPE_FILE;
    else if (adapterName == ADAPTER_USB_TYPE)
        return AdaptersType::TYPE_USB;
    else if (adapterName == ADAPTER_DP_TYPE)
        return AdaptersType::TYPE_DP;
    else
        return AdaptersType::TYPE_INVALID;
}

AdapterType AudioPolicyParser::GetAdapterTypeAsInt(xmlNode &node)
{
    if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("devices"))) {
        return AdapterType::DEVICES;
    } else if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("modules"))) {
        return AdapterType::MODULES;
    } else {
        return AdapterType::UNKNOWN;
    }
}

ModuleType AudioPolicyParser::GetModuleTypeAsInt(xmlNode &node)
{
    if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("configs"))) {
        return ModuleType::CONFIGS;
    } else if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("profiles"))) {
        return ModuleType::PROFILES;
    } else if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("devices"))) {
        return ModuleType::DEVICES;
    } else {
        return ModuleType::UNKNOWN;
    }
}

ExtendType AudioPolicyParser::GetExtendTypeAsInt(xmlNode &node)
{
    if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("updateRouteSupport"))) {
        return ExtendType::UPDATE_ROUTE_SUPPORT;
    } else if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("audioLatency"))) {
        return ExtendType::AUDIO_LATENCY;
    } else if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("sinkLatency"))) {
        return ExtendType::SINK_LATENCY;
    } else {
        return ExtendType::UNKNOWN;
    }
}
} // namespace AudioStandard
} // namespace OHOS