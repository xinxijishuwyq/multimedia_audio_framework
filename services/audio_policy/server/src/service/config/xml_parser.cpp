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
#define LOG_TAG "XMLParser"

#include "audio_log.h"

#include "xml_parser.h"

#include <regex>

namespace OHOS {
namespace AudioStandard {
bool XMLParser::LoadConfiguration()
{
    AUDIO_INFO_LOG("start LoadConfiguration");
    mDoc = xmlReadFile(CONFIG_FILE, nullptr, 0);
    CHECK_AND_RETURN_RET_LOG(mDoc != nullptr, false, "xmlReadFile Failed");

    return true;
}

bool XMLParser::Parse()
{
    xmlNode *root = xmlDocGetRootElement(mDoc);
    CHECK_AND_RETURN_RET_LOG(root != nullptr, false, "xmlDocGetRootElement Failed");

    if (!ParseInternal(*root)) {
        return false;
    }

    return true;
}

void XMLParser::Destroy()
{
    if (mDoc != nullptr) {
        xmlFreeDoc(mDoc);
    }
}

bool XMLParser::ParseInternal(xmlNode &node)
{
    xmlNode *currNode = &node;
    for (; currNode; currNode = currNode->next) {
        if (XML_ELEMENT_NODE == currNode->type) {
            switch (GetNodeNameAsInt(*currNode)) {
                case DEVICE_CLASS:
                    ParseDeviceClass(*currNode);
                    break;
                case AUDIO_INTERRUPT_ENABLE:
                    ParseAudioInterrupt(*currNode);
                    break;
                case UPDATE_ROUTE_SUPPORT:
                    ParseUpdateRouteSupport(*currNode);
                    break;
                case AUDIO_LATENCY:
                    ParseAudioLatency(*currNode);
                    break;
                case SINK_LATENCY:
                    ParseSinkLatency(*currNode);
                    break;
                case VOLUME_GROUP_CONFIG:
                    ParseGroups(*currNode, VOLUME_GROUP_CONFIG);
                    break;
                case INTERRUPT_GROUP_CONFIG:
                    ParseGroups(*currNode, INTERRUPT_GROUP_CONFIG);
                    break;
                default:
                    ParseInternal(*(currNode->children));
                    break;
            }
        }
    }

    mPortObserver.OnXmlParsingCompleted(xmlParsedDataMap_);
    mPortObserver.OnVolumeGroupParsed(volumeGroupMap_);
    mPortObserver.OnInterruptGroupParsed(interruptGroupMap_);
    return true;
}

void XMLParser::ParseDeviceClass(xmlNode &node)
{
    xmlNode *modulesNode = nullptr;
    modulesNode = node.xmlChildrenNode;

    std::string className = ExtractPropertyValue("name", node);
    CHECK_AND_RETURN_LOG(!className.empty(), "No name provided for the device class %{public}s", node.name);

    deviceClassType_ = GetDeviceClassType(className);
    xmlParsedDataMap_[deviceClassType_] = {};

    while (modulesNode != nullptr) {
        if (modulesNode->type == XML_ELEMENT_NODE) {
            ParseModules(*modulesNode, className);
        }
        modulesNode = modulesNode->next;
    }
}

void XMLParser::ParseModules(xmlNode &node, std::string &className)
{
    xmlNode *moduleNode = nullptr;
    std::list<AudioModuleInfo> moduleList = {};
    moduleNode = node.xmlChildrenNode;

    while (moduleNode != nullptr) {
        if (moduleNode->type == XML_ELEMENT_NODE) {
            AudioModuleInfo moduleInfo = {};
            moduleInfo.className = className;
            moduleInfo.name = ExtractPropertyValue("name", *moduleNode);
            moduleInfo.lib = ExtractPropertyValue("lib", *moduleNode);
            moduleInfo.role = ExtractPropertyValue("role", *moduleNode);

            std::regex regexDelimiter(",");
            std::string rates = ExtractPropertyValue("rate", *moduleNode);
            const std::sregex_token_iterator itEnd;
            for (std::sregex_token_iterator it(rates.begin(), rates.end(), regexDelimiter, -1); it != itEnd; ++it) {
                moduleInfo.supportedRate_.insert(atoi(it->str().c_str()));
            }
            moduleInfo.rate = std::to_string(*(moduleInfo.supportedRate_.rbegin()));

            moduleInfo.format = ExtractPropertyValue("format", *moduleNode);

            std::string channels = ExtractPropertyValue("channels", *moduleNode);
            for (std::sregex_token_iterator it(channels.begin(), channels.end(), regexDelimiter, -1);
                it != itEnd; ++it) {
                moduleInfo.supportedChannels_.insert(atoi(it->str().c_str()));
            }
            moduleInfo.channels = std::to_string(*(moduleInfo.supportedChannels_.rbegin()));

            moduleInfo.bufferSize = ExtractPropertyValue("buffer_size", *moduleNode);
            moduleInfo.fileName = ExtractPropertyValue("file", *moduleNode);
            moduleInfo.ports = {};

            ParsePorts(*moduleNode, moduleInfo);
            moduleList.push_back(moduleInfo);
        }
        moduleNode = moduleNode->next;
    }

    xmlParsedDataMap_[deviceClassType_] = moduleList;
}

void XMLParser::ParsePorts(xmlNode &node, AudioModuleInfo &moduleInfo)
{
    xmlNode *portsNode = nullptr;
    portsNode = node.xmlChildrenNode;

    while (portsNode != nullptr) {
        if (portsNode->type == XML_ELEMENT_NODE) {
            ParsePort(*portsNode, moduleInfo);
        }
        portsNode = portsNode->next;
    }
}

void XMLParser::ParsePort(xmlNode &node, AudioModuleInfo &moduleInfo)
{
    xmlNode *portNode = nullptr;
    std::list<AudioModuleInfo> portInfoList = {};
    portNode = node.xmlChildrenNode;

    while (portNode != nullptr) {
        if (portNode->type == XML_ELEMENT_NODE) {
            moduleInfo.adapterName = ExtractPropertyValue("adapter_name", *portNode);
            moduleInfo.id = ExtractPropertyValue("id", *portNode);

            // if some parameter is not configured inside <Port>, take data from moduleinfo
            std::string value = ExtractPropertyValue("rate", *portNode);
            if (!value.empty()) {
                SeparatedListParserAndFetchMaxValue(value, moduleInfo.supportedRate_, moduleInfo.rate);
            }

            value = ExtractPropertyValue("format", *portNode);
            if (!value.empty()) {
                moduleInfo.format = value;
            }

            value = ExtractPropertyValue("channels", *portNode);
            if (!value.empty()) {
                SeparatedListParserAndFetchMaxValue(value, moduleInfo.supportedChannels_, moduleInfo.channels);
            }

            value = ExtractPropertyValue("buffer_size", *portNode);
            if (!value.empty()) {
                moduleInfo.bufferSize = value;
            }

            value = ExtractPropertyValue("fixed_latency", *portNode);
            if (!value.empty()) {
                moduleInfo.fixedLatency = value;
            }

            value = ExtractPropertyValue("render_in_idle_state", *portNode);
            if (!value.empty()) {
                moduleInfo.renderInIdleState = value;
            }

            if (!((value = ExtractPropertyValue("open_mic_speaker", *portNode)).empty())) {
                moduleInfo.OpenMicSpeaker = value;
            }

            if (!((value = ExtractPropertyValue("source_type", *portNode)).empty())) {
                moduleInfo.sourceType = value;
            }

            if (!((value = ExtractPropertyValue("file", *portNode)).empty())) {
                moduleInfo.fileName = value;
            }

            if (!((value = ExtractPropertyValue("offload_enable", *portNode)).empty())) {
                moduleInfo.offloadEnable = value;
            }

            portInfoList.push_back(moduleInfo);
        }
        portNode = portNode->next;
    }

    moduleInfo.ports.assign(portInfoList.begin(), portInfoList.end());
}

NodeName XMLParser::GetNodeNameAsInt(xmlNode &node)
{
    if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("deviceclass"))) {
        return DEVICE_CLASS;
    } else if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("AudioInterruptEnable"))) {
        return AUDIO_INTERRUPT_ENABLE;
    } else if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("UpdateRouteSupport"))) {
        return UPDATE_ROUTE_SUPPORT;
    } else if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("AudioLatency"))) {
        return AUDIO_LATENCY;
    } else if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("SinkLatency"))) {
        return SINK_LATENCY;
    } else if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("VolumeGroupConfig"))) {
        return VOLUME_GROUP_CONFIG;
    } else if (!xmlStrcmp(node.name, reinterpret_cast<const xmlChar*>("InterruptGroupConfig"))) {
        return INTERRUPT_GROUP_CONFIG;
    } else {
        return UNKNOWN;
    }
}

void XMLParser::ParseUpdateRouteSupport(xmlNode &node)
{
    xmlNode *child = node.children;
    xmlChar *supportFlag = xmlNodeGetContent(child);

    if (!xmlStrcmp(supportFlag, reinterpret_cast<const xmlChar*>("true"))) {
        mPortObserver.OnUpdateRouteSupport(true);
    } else {
        mPortObserver.OnUpdateRouteSupport(false);
    }

    xmlFree(supportFlag);
}

void XMLParser::ParseAudioInterrupt(xmlNode &node)
{
    xmlNode *child = node.children;
    xmlChar *enableFlag = xmlNodeGetContent(child);

    xmlStrcmp(enableFlag, reinterpret_cast<const xmlChar*>("true"));
    xmlFree(enableFlag);
}

ClassType XMLParser::GetDeviceClassType(const std::string &deviceClass)
{
    if (deviceClass == PRIMARY_CLASS)
        return ClassType::TYPE_PRIMARY;
    else if (deviceClass == A2DP_CLASS)
        return ClassType::TYPE_A2DP;
    else if (deviceClass == USB_CLASS)
        return ClassType::TYPE_USB;
    else if (deviceClass == FILE_CLASS)
        return ClassType::TYPE_FILE_IO;
    else
        return ClassType::TYPE_INVALID;
}

std::string XMLParser::ExtractPropertyValue(const std::string &propName, xmlNode &node)
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

void XMLParser::ParseAudioLatency(xmlNode &node)
{
    xmlNode *child = node.children;
    xmlChar *audioLatency = xmlNodeGetContent(child);
    std::string sAudioLatency(reinterpret_cast<char *>(audioLatency));
    mPortObserver.OnAudioLatencyParsed((uint64_t)std::stoi(sAudioLatency));

    xmlFree(audioLatency);
}

void XMLParser::ParseSinkLatency(xmlNode &node)
{
    xmlNode *child = node.children;
    xmlChar *latency = xmlNodeGetContent(child);
    std::string sLatency(reinterpret_cast<char *>(latency));
    mPortObserver.OnSinkLatencyParsed((uint64_t)std::stoi(sLatency));

    xmlFree(latency);
}

void XMLParser::ParseGroups(xmlNode& node, NodeName type)
{
    xmlNode* groupsNode = nullptr;
    groupsNode = node.xmlChildrenNode; // get <groups>
    while (groupsNode != nullptr) {
        if (groupsNode->type == XML_ELEMENT_NODE) {
            ParseGroup(*groupsNode, type);
        }
        groupsNode = groupsNode->next;
    }
}

void XMLParser::ParseGroup(xmlNode& node, NodeName type)
{
    xmlNode* groupNode = nullptr;
    groupNode = node.xmlChildrenNode;

    while (groupNode != nullptr) {
        if (groupNode->type == XML_ELEMENT_NODE) {
            std::string groupName = ExtractPropertyValue("name", *groupNode);
            ParseGroupModule(*groupNode, type, groupName);
        }
        groupNode = groupNode->next;
    }
}

void XMLParser::ParseGroupModule(xmlNode& node, NodeName type, std::string groupName)
{
    xmlNode* moduleNode = nullptr;
    moduleNode = node.xmlChildrenNode;
    while (moduleNode != nullptr) {
        if (moduleNode->type == XML_ELEMENT_NODE) {
            std::string moduleName = ExtractPropertyValue("name", *moduleNode);
            if (type == VOLUME_GROUP_CONFIG) {
                volumeGroupMap_[moduleName] = groupName;
            } else if (type == INTERRUPT_GROUP_CONFIG) {
                interruptGroupMap_[moduleName] = groupName;
            }
        }
        moduleNode = moduleNode->next;
    }
}
} // namespace AudioStandard
} // namespace OHOS
