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

#ifndef AUDIO_POLICY_PARSER_H
#define AUDIO_POLICY_PARSER_H

#include <list>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <unordered_map>
#include <string>
#include <regex>

#include "audio_adapter_info.h"
#include "audio_device_info.h"
#include "iport_observer.h"
#include "parser.h"

namespace OHOS {
namespace AudioStandard {

class AudioPolicyParser : public Parser {
public:
    static constexpr char CHIP_PROD_CONFIG_FILE[] = "/chip_prod/etc/audio/audio_policy_config.xml";
    static constexpr char CONFIG_FILE[] = "/vendor/etc/audio/audio_policy_config_new.xml";

    bool LoadConfiguration() final;
    bool Parse() final;
    void Destroy() final;

    explicit AudioPolicyParser(IPortObserver &observer)
        : portObserver_(observer),
          doc_(nullptr)
    {
    }

    virtual ~AudioPolicyParser()
    {
        Destroy();
    }

private:
    XmlNodeType GetNodeTypeAsInt(xmlNode &node);
    AdaptersType GetAdaptersType(const std::string &adapterClass);
    AdapterType GetAdapterTypeAsInt(xmlNode &node);
    ModulesType GetModulesTypeAsInt(xmlNode &node);
    ModuleType GetModuleTypeAsInt(xmlNode &node);
    ExtendType GetExtendTypeAsInt(xmlNode &node);

    bool ParseInternal(xmlNode &node);
    void ParseAdapters(xmlNode &node);
    void ParseAdapter(xmlNode &node);
    void ParseModule(xmlNode &node, ModuleInfo &moduleInfo);
    void ParseModuleProperty(xmlNode &node, ModuleInfo &moduleInfo, std::string adapterName);
    void ParseModules(xmlNode &node, AudioAdapterInfo &adapterInfo);
    void ParseAudioAdapterDevices(xmlNode &node, AudioAdapterInfo &adapterInfo);
    void ParseConfigs(xmlNode &node, ModuleInfo &moduleInfo);
    void ParseProfiles(xmlNode &node, ModuleInfo &moduleInfo);
    void ParseModuleDevices(xmlNode &node, ModuleInfo &moduleInfo);
    void ParseGroups(xmlNode& node, XmlNodeType type);
    void ParseGroup(xmlNode& node, XmlNodeType type);
    void ParseGroupSink(xmlNode& node, XmlNodeType type, std::string groupName);
    void ParseExtends(xmlNode& node);
    void ParseUpdateRouteSupport(xmlNode &node);
    void ConvertAdapterInfoToAudioModuleInfo();
    void ConvertAdapterInfoToAudioModuleInfo(std::unordered_map<std::string, std::string> &volumeGroupMap,
        std::unordered_map<std::string, std::string> &interruptGroupMap);
    void ConvertAdapterInfoToGroupInfo(std::unordered_map<std::string, std::string> &volumeGroupMap,
        std::unordered_map<std::string, std::string> &interruptGroupMap, ModuleInfo &moduleInfo);
    void GetCommontAudioModuleInfo(ModuleInfo &moduleInfo, AudioModuleInfo &audioModuleInfo);
    ClassType GetClassTypeByAdapterType(AdaptersType adapterType);
    void GetOffloadAndOpenMicState(AudioAdapterInfo &adapterInfo, bool &shouldEnableOffload);
    std::string ExtractPropertyValue(const std::string &propName, xmlNode &node);

    IPortObserver &portObserver_;
    xmlDoc *doc_;
    std::map<AdaptersType, AudioAdapterInfo> adapterInfoMap_ {};
    std::unordered_map<ClassType, std::list<AudioModuleInfo>> xmlParsedDataMap_ {};
    std::unordered_map<std::string, std::string> volumeGroupMap_;
    std::unordered_map<std::string, std::string> interruptGroupMap_;
    bool shouldOpenMicSpeaker_;
};
} // namespace AudioStandard
} // namespace OHOS

#endif // AUDIO_POLICY_PARSER_H