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

#ifndef IMAGE_EFFECT_EVENT_REPORT_H
#define IMAGE_EFFECT_EVENT_REPORT_H

#include <string>
#include <unordered_map>
#include "image_effect_marco_define.h"

namespace OHOS {
namespace Media {
namespace Effect {
constexpr const char* const REGISTER_CUSTOM_FILTER_STATISTIC = "REGISTER_CUSTOM_FILTER";
constexpr const char* const ADD_FILTER_STATISTIC = "ADD_FILTER";
constexpr const char* const REMOVE_FILTER_STATISTIC = "REMOVE_FILTER";
constexpr const char* const INPUT_DATA_TYPE_STATISTIC = "INPUT_DATA_TYPE";
constexpr const char* const OUTPUT_DATA_TYPE_STATISTIC = "OUTPUT_DATA_TYPE";
constexpr const char* const RENDER_FAILED_FAULT = "RENDER_FAILED";
constexpr const char* const SAVE_IMAGE_EFFECT_BEHAVIOR = "SAVE_IMAGE_EFFECT";
constexpr const char* const RESTORE_IMAGE_EFFECT_BEHAVIOR = "RESTORE_IMAGE_EFFECT";

enum class EventDataType {
    PIXEL_MAP = 0,
    URI,
    SURFACE,
    SURFACE_BUFFER,
    PICTURE,
    TEX,
};

struct EventErrorInfo {
    int32_t errorCode = 0;
    std::string errorMsg;
};

struct EventInfo {
    std::string filterName;
    uint32_t supportedFormats = 0;
    int32_t filterNum = 0;
    EventDataType dataType;
    EventErrorInfo errorInfo;
};

class EventReport {
public:
    IMAGE_EFFECT_EXPORT static void ReportHiSysEvent(const std::string &eventName, const EventInfo &eventInfo);

private:
    static void ReportRegisterCustomFilterEvent(const EventInfo &eventInfo);
    static void ReportAddFilterEvent(const EventInfo &eventInfo);
    static void ReportRemoveFilterEvent(const EventInfo &eventInfo);
    static void ReportInputDataTypeEvent(const EventInfo &eventInfo);
    static void ReportOutputDataTypeEvent(const EventInfo &eventInfo);
    static void ReportRenderFailedEvent(const EventInfo &eventInfo);
    static void ReportSaveImageEffectEvent(const EventInfo &eventInfo);
    static void ReportRestoreImageEffectEvent(const EventInfo &eventInfo);

    static std::string ConvertDataType(const EventDataType &dataType);

    static std::unordered_map<std::string, void (*)(const EventInfo &eventInfo)> sysEventFuncMap_;
    static std::unordered_map<EventDataType, std::string> sysEventDataTypeMap_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_EVENT_REPORT_H
