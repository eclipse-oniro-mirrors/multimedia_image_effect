/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "event_report.h"

#include <fstream>

#include "effect_log.h"
#include "hisysevent.h"

namespace OHOS {
namespace Media {
namespace Effect {
namespace {
const std::string EVENT_PARAM_FILTER_NAME = "FILTER_NAME";
const std::string EVENT_PARAM_SUPPORTED_FORMATS = "SUPPORTED_FORMATS";
const std::string EVENT_PARAM_FILTER_NUMBER = "FILTER_NUMBER";
const std::string EVENT_PARAM_DATA_TYPE = "DATA_TYPE";
const std::string EVENT_PARAM_ERROR_TYPE = "ERROR_TYPE";
const std::string EVENT_PARAM_ERROR_MSG = "ERROR_MSG";
constexpr char IMAGE_EFFECT_UE[] = "IMAGE_EFFECT_UE";
const std::string DEFAULT_STR = "default";
const std::string DEFAULT_PACKAGE_NAME = "entry";
const std::string DEFAULT_VERSION_ID = "1";
const std::string PROC_SELF_CMDLINE_PATH = "/proc/self/cmdline";
}

std::unordered_map<std::string, void (*)(const EventInfo &eventInfo)> EventReport::sysEventFuncMap_ = {
    { REGISTER_CUSTOM_FILTER_STATISTIC, [](const EventInfo &eventInfo) {
        ReportRegisterCustomFilterEvent(eventInfo);
    }},
    { ADD_FILTER_STATISTIC, [](const EventInfo &eventInfo) {
        ReportAddFilterEvent(eventInfo);
    }},
    { REMOVE_FILTER_STATISTIC, [](const EventInfo &eventInfo) {
        ReportRemoveFilterEvent(eventInfo);
    }},
    { INPUT_DATA_TYPE_STATISTIC, [](const EventInfo &eventInfo) {
        ReportInputDataTypeEvent(eventInfo);
    }},
    { OUTPUT_DATA_TYPE_STATISTIC, [](const EventInfo &eventInfo) {
        ReportOutputDataTypeEvent(eventInfo);
    }},
    { RENDER_FAILED_FAULT, [](const EventInfo &eventInfo) {
        ReportRenderFailedEvent(eventInfo);
    }},
    { SAVE_IMAGE_EFFECT_BEHAVIOR, [](const EventInfo &eventInfo) {
        ReportSaveImageEffectEvent(eventInfo);
    }},
    { RESTORE_IMAGE_EFFECT_BEHAVIOR, [](const EventInfo &eventInfo) {
        ReportRestoreImageEffectEvent(eventInfo);
    }},
};

std::unordered_map<EventDataType, std::string> EventReport::sysEventDataTypeMap_ = {
    { EventDataType::PIXEL_MAP, "pixelmap" },
    { EventDataType::URI, "uri" },
    { EventDataType::SURFACE, "surface" },
    { EventDataType::SURFACE_BUFFER, "surfacebuffer" },
};

std::string GetCurrentProcessName()
{
    std::string procName;
    std::ifstream procFile(PROC_SELF_CMDLINE_PATH);
    if (procFile.is_open()) {
        std::ostringstream oss;
        oss << procFile.rdbuf();
        procFile.close();

        //Extract proc name from the command line
        std::string cmdline = oss.str();
        size_t pos = cmdline.find_first_of('\0');
        if (pos != std::string::npos) {
            procName = cmdline.substr(0, pos);
        }
    }
    return procName;
}

template<typename... Types>
void EventWrite(const std::string &eventName, HiviewDFX::HiSysEvent::EventType type, Types... keyValues)
{
    std::string processName = GetCurrentProcessName();
    HiSysEventWrite(IMAGE_EFFECT_UE, eventName, type,
        "PNAMEID", processName.empty() ? DEFAULT_PACKAGE_NAME : processName,
        "PVERSIONID", DEFAULT_VERSION_ID,
        keyValues...);
}

void EventReport::ReportHiSysEvent(const std::string &eventName, const EventInfo &eventInfo)
{
    auto iter = sysEventFuncMap_.find(eventName);
    if (iter == sysEventFuncMap_.end()) {
        EFFECT_LOGE("ReportHiSysEvent: eventName is not supported! eventName=%{public}s", eventName.c_str());
        return;
    }

    iter->second(eventInfo);
}

void EventReport::ReportRegisterCustomFilterEvent(const EventInfo &eventInfo)
{
    EventWrite(
        REGISTER_CUSTOM_FILTER_STATISTIC,
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        EVENT_PARAM_FILTER_NAME, eventInfo.filterName,
        EVENT_PARAM_SUPPORTED_FORMATS, eventInfo.supportedFormats);
}

void EventReport::ReportAddFilterEvent(const EventInfo &eventInfo)
{
    EventWrite(
        ADD_FILTER_STATISTIC,
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        EVENT_PARAM_FILTER_NAME, eventInfo.filterName);
}

void EventReport::ReportRemoveFilterEvent(const EventInfo &eventInfo)
{
    EventWrite(
        REMOVE_FILTER_STATISTIC,
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        EVENT_PARAM_FILTER_NAME, eventInfo.filterName,
        EVENT_PARAM_FILTER_NUMBER, eventInfo.filterNum);
}

std::string EventReport::ConvertDataType(const EventDataType &dataType)
{
    auto iter = sysEventDataTypeMap_.find(dataType);
    if (iter == sysEventDataTypeMap_.end()) {
        return DEFAULT_STR;
    }

    return iter->second;
}

void EventReport::ReportInputDataTypeEvent(const EventInfo &eventInfo)
{
    EventWrite(
        INPUT_DATA_TYPE_STATISTIC,
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        EVENT_PARAM_DATA_TYPE, ConvertDataType(eventInfo.dataType));
}

void EventReport::ReportOutputDataTypeEvent(const EventInfo &eventInfo)
{
    EventWrite(
        OUTPUT_DATA_TYPE_STATISTIC,
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        EVENT_PARAM_DATA_TYPE, ConvertDataType(eventInfo.dataType));
}

void EventReport::ReportRenderFailedEvent(const EventInfo &eventInfo)
{
    EventWrite(
        RENDER_FAILED_FAULT,
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        EVENT_PARAM_ERROR_TYPE, eventInfo.errorInfo.errorCode,
        EVENT_PARAM_ERROR_MSG, eventInfo.errorInfo.errorMsg);
}

void EventReport::ReportSaveImageEffectEvent(const EventInfo &eventInfo)
{
    EventWrite(
        SAVE_IMAGE_EFFECT_BEHAVIOR,
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR);
}

void EventReport::ReportRestoreImageEffectEvent(const EventInfo &eventInfo)
{
    EventWrite(
        RESTORE_IMAGE_EFFECT_BEHAVIOR,
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR);
}

} // namespace Effect
} // namespace Media
} // namespace OHOS