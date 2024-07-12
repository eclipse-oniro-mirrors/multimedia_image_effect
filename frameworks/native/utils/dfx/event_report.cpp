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

#include <vector>

#include "effect_log.h"
#include "hisysevent_c.h"
#include "file_ex.h"

namespace OHOS {
namespace Media {
namespace Effect {
namespace {
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
    std::string cmdline;
    if (OHOS::LoadStringFromFile(PROC_SELF_CMDLINE_PATH, cmdline)) {
        size_t pos = cmdline.find_first_of('\0');
        if (pos != std::string::npos) {
            procName = cmdline.substr(0, pos);
        }
    }
    return procName;
}

void EventWrite(const char *func, int64_t line, const char *name, HiSysEventEventType type,
    std::vector<HiSysEventParam> &params)
{
    std::string processName = GetCurrentProcessName();
    HiSysEventParam processNamePara = {
        .name = "PNAMEID",
        .t = HISYSEVENT_STRING,
        .v = { .s = processName.empty() ? const_cast<char *>(DEFAULT_PACKAGE_NAME.c_str()) :
            const_cast<char *>(processName.c_str()) },
        .arraySize = 0,
    };
    HiSysEventParam versionPara = {
        .name = "PVERSIONID",
        .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char *>(DEFAULT_VERSION_ID.c_str()) },
        .arraySize = 0,
    };
    params.insert(params.begin(), processNamePara);
    params.insert(params.begin() + 1, versionPara);
    HiSysEvent_Write(func, line, IMAGE_EFFECT_UE, name, type, params.data(), params.size());
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
    HiSysEventParam processNamePara = {
        .name = { "FILTER_NAME" },
        .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char *>(eventInfo.filterName.c_str()) },
        .arraySize = 0,
    };
    HiSysEventParam supportedFormatsPara = {
        .name = { "SUPPORTED_FORMATS" },
        .t = HISYSEVENT_UINT32,
        .v = { .ui32 = eventInfo.supportedFormats },
        .arraySize = 0,
    };
    std::vector<HiSysEventParam> params = { processNamePara, supportedFormatsPara };
    EventWrite(__FUNCTION__, __LINE__, REGISTER_CUSTOM_FILTER_STATISTIC, HiSysEventEventType::HISYSEVENT_BEHAVIOR,
        params);
}

void EventReport::ReportAddFilterEvent(const EventInfo &eventInfo)
{
    HiSysEventParam processNamePara = {
        .name = { "FILTER_NAME" },
        .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char *>(eventInfo.filterName.c_str()) },
        .arraySize = 0,
    };
    std::vector<HiSysEventParam> params = { processNamePara };
    EventWrite(__FUNCTION__, __LINE__, ADD_FILTER_STATISTIC, HiSysEventEventType::HISYSEVENT_BEHAVIOR,
        params);
}

void EventReport::ReportRemoveFilterEvent(const EventInfo &eventInfo)
{
    HiSysEventParam processNamePara = {
        .name = { "FILTER_NAME" },
        .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char *>(eventInfo.filterName.c_str()) },
        .arraySize = 0,
    };
    HiSysEventParam filterNumPara = {
        .name = { "FILTER_NUMBER" },
        .t = HISYSEVENT_INT32,
        .v = { .i32 = eventInfo.filterNum },
        .arraySize = 0,
    };
    std::vector<HiSysEventParam> params = { processNamePara, filterNumPara };
    EventWrite(__FUNCTION__, __LINE__, REMOVE_FILTER_STATISTIC, HiSysEventEventType::HISYSEVENT_BEHAVIOR,
        params);
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
    std::string dataType = ConvertDataType(eventInfo.dataType);
    HiSysEventParam dataTypePara = {
        .name = { "DATA_TYPE" },
        .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char *>(dataType.c_str()) },
        .arraySize = 0,
    };
    std::vector<HiSysEventParam> params = { dataTypePara };
    EventWrite(__FUNCTION__, __LINE__, INPUT_DATA_TYPE_STATISTIC, HiSysEventEventType::HISYSEVENT_BEHAVIOR,
        params);
}

void EventReport::ReportOutputDataTypeEvent(const EventInfo &eventInfo)
{
    std::string dataType = ConvertDataType(eventInfo.dataType);
    HiSysEventParam dataTypePara = {
        .name = { "DATA_TYPE" },
        .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char *>(dataType.c_str()) },
        .arraySize = 0,
    };
    std::vector<HiSysEventParam> params = { dataTypePara };
    EventWrite(__FUNCTION__, __LINE__, OUTPUT_DATA_TYPE_STATISTIC, HiSysEventEventType::HISYSEVENT_BEHAVIOR,
        params);
}

void EventReport::ReportRenderFailedEvent(const EventInfo &eventInfo)
{
    HiSysEventParam errorCodePara = {
        .name = { "ERROR_TYPE" },
        .t = HISYSEVENT_INT32,
        .v = { .i32 = eventInfo.errorInfo.errorCode },
        .arraySize = 0,
    };
    HiSysEventParam errorMsgPara = {
        .name = { "ERROR_MSG" },
        .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char *>(eventInfo.errorInfo.errorMsg.c_str()) },
        .arraySize = 0,
    };

    std::vector<HiSysEventParam> params = { errorCodePara, errorMsgPara };
    EventWrite(__FUNCTION__, __LINE__, RENDER_FAILED_FAULT, HiSysEventEventType::HISYSEVENT_BEHAVIOR,
        params);
}

void EventReport::ReportSaveImageEffectEvent(const EventInfo &eventInfo)
{
    std::vector<HiSysEventParam> params;
    EventWrite(__FUNCTION__, __LINE__, SAVE_IMAGE_EFFECT_BEHAVIOR, HiSysEventEventType::HISYSEVENT_BEHAVIOR,
        params);
}

void EventReport::ReportRestoreImageEffectEvent(const EventInfo &eventInfo)
{
    std::vector<HiSysEventParam> params;
    EventWrite(__FUNCTION__, __LINE__, RESTORE_IMAGE_EFFECT_BEHAVIOR, HiSysEventEventType::HISYSEVENT_BEHAVIOR,
        params);
}

} // namespace Effect
} // namespace Media
} // namespace OHOS