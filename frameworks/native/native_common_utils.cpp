/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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

#include "native_common_utils.h"

#include <set>
#include <algorithm>

#include "effect_log.h"
#include "efilter_factory.h"
#include "native_pixel_map.h"

namespace OHOS {
namespace Media {
namespace Effect {
static const std::map<IEffectFormat, OH_IEffectFormat> FORMAT_TABLE = {
    { IEffectFormat::RGBA8888, OH_IEffectFormat::RGBA8888 },
    { IEffectFormat::YUVNV12, OH_IEffectFormat::NV12 },
    { IEffectFormat::YUVNV21, OH_IEffectFormat::NV21 }
};

static const std::map<Category, OH_Category> CATEGORY_TABLE = {
    { Category::COLOR_ADJUST, OH_Category::COLOR_ADJUST },
    { Category::SHAPE_ADJUST, OH_Category::SHAPE_ADJUST },
    { Category::LAYER_BLEND, OH_Category::LAYER_BLEND },
    { Category::OTHER, OH_Category::OTHER }
};

static const std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>> LOOK_UP_CAPABILITY = {
    { "Category",
        {
            { "default", static_cast<uint32_t>(Category::DEFAULT) },
            { "color_adjust", static_cast<uint32_t>(Category::COLOR_ADJUST) },
            { "shape_adjust", static_cast<uint32_t>(Category::SHAPE_ADJUST) },
            { "layer_blend", static_cast<uint32_t>(Category::LAYER_BLEND) },
            { "other", static_cast<uint32_t>(Category::OTHER) },
        }
    },
    { "Format",
        {
            { "default", static_cast<uint32_t>(IEffectFormat::DEFAULT) },
            { "rgba_8888", static_cast<uint32_t>(IEffectFormat::RGBA8888) },
            { "nv21", static_cast<uint32_t>(IEffectFormat::YUVNV21) },
            { "nv12", static_cast<uint32_t>(IEffectFormat::YUVNV12) },
        }
    },
};

static const std::map<IPType, OH_BufferType> IPTYPE_TABLE = {
    { IPType::CPU, OH_BufferType::PIXEL },
    { IPType::GPU, OH_BufferType::TEXTURE },
};

template <class ValueType>
ErrorCode AnyCastOHAny(const Plugin::Any &any, OH_DataType &ohDataType, OH_DataType ohDataTypeValue, ValueType &value)
{
    auto result = Plugin::AnyCast<ValueType>(&any);
    if (result == nullptr) {
        return ErrorCode::ERR_ANY_CAST_TYPE_NOT_MATCH;
    }
    ohDataType = ohDataTypeValue;
    value = *result;
    return ErrorCode::SUCCESS;
}

ErrorCode NativeCommonUtils::ParseOHAny(const OH_Any *value, Plugin::Any &any)
{
    switch (value->dataType) {
        case OH_DataType::TYPE_INT32:
            any = value->dataValue.int32Value;
            break;
        case OH_DataType::TYPE_FLOAT:
            any = value->dataValue.floatValue;
            break;
        case OH_DataType::TYPE_DOUBLE:
            any = value->dataValue.doubleValue;
            break;
        case OH_DataType::TYPE_CHAR:
            any = value->dataValue.charValue;
            break;
        case OH_DataType::TYPE_LONG:
            any = value->dataValue.longValue;
            break;
        case OH_DataType::TYPE_PTR:
            any = value->dataValue.ptrValue;
            break;
        case OH_DataType::TYPE_BOOL:
            any = value->dataValue.boolValue;
            break;
        default:
            EFFECT_LOGE("input any data type not support! dataType=%{public}d", value->dataType);
            return ErrorCode::ERR_UNSUPPORTED_INPUT_ANYTYPE;
    }
    return ErrorCode::SUCCESS;
}

ErrorCode NativeCommonUtils::SwitchToOHAny(const Plugin::Any &any, OH_Any *value)
{
    CHECK_AND_RETURN_RET(AnyCastOHAny(any, value->dataType, OH_DataType::TYPE_INT32, value->dataValue.int32Value) !=
        ErrorCode::SUCCESS, ErrorCode::SUCCESS);
    CHECK_AND_RETURN_RET(AnyCastOHAny(any, value->dataType, OH_DataType::TYPE_FLOAT, value->dataValue.floatValue) !=
        ErrorCode::SUCCESS, ErrorCode::SUCCESS);
    CHECK_AND_RETURN_RET(AnyCastOHAny(any, value->dataType, OH_DataType::TYPE_DOUBLE, value->dataValue.doubleValue) !=
        ErrorCode::SUCCESS, ErrorCode::SUCCESS);
    CHECK_AND_RETURN_RET(AnyCastOHAny(any, value->dataType, OH_DataType::TYPE_CHAR, value->dataValue.charValue) !=
        ErrorCode::SUCCESS, ErrorCode::SUCCESS);
    CHECK_AND_RETURN_RET(AnyCastOHAny(any, value->dataType, OH_DataType::TYPE_LONG, value->dataValue.longValue) !=
        ErrorCode::SUCCESS, ErrorCode::SUCCESS);
    CHECK_AND_RETURN_RET(AnyCastOHAny(any, value->dataType, OH_DataType::TYPE_PTR, value->dataValue.ptrValue) !=
        ErrorCode::SUCCESS, ErrorCode::SUCCESS);
    CHECK_AND_RETURN_RET(AnyCastOHAny(any, value->dataType, OH_DataType::TYPE_BOOL, value->dataValue.boolValue) !=
        ErrorCode::SUCCESS, ErrorCode::SUCCESS);

#ifndef HST_ANY_WITH_NO_RTTI
    EFFECT_LOGE("inner any type not support switch to oh_any! type:%{public}s", any.Type().name());
#else
    EFFECT_LOGE("inner any type not support switch to oh_any! type:%{public}s", std::string(any.TypeName()).c_str());
#endif
    return ErrorCode::ERR_NOT_SUPPORT_SWITCH_TO_OHANY;
}

void NativeCommonUtils::SwitchToOHFormatType(const IEffectFormat &formatType, OH_IEffectFormat &ohFormatType)
{
    auto it = FORMAT_TABLE.find(formatType);
    if (it != FORMAT_TABLE.end()) {
        ohFormatType = it->second;
    } else {
        ohFormatType = OH_IEffectFormat::UNKNOWN_FORMAT;
    }
}

void NativeCommonUtils::SwitchToOHFormatType(const IEffectFormat &formatType, uint32_t &ohFormatType)
{
    OH_IEffectFormat ohFormatTypeEnum;
    SwitchToOHFormatType(formatType, ohFormatTypeEnum);
    ohFormatType = static_cast<uint32_t>(ohFormatTypeEnum);
}

void NativeCommonUtils::SwitchToFormatType(const OH_IEffectFormat &ohFormatType, IEffectFormat &formatType)
{
    auto formatIter = std::find_if(FORMAT_TABLE.begin(), FORMAT_TABLE.end(),
        [&ohFormatType](const std::pair<IEffectFormat, OH_IEffectFormat> &format) {
            return format.second == ohFormatType;
    });
    if (formatIter != FORMAT_TABLE.end()) {
        formatType = formatIter->first;
    } else {
        formatType = IEffectFormat::DEFAULT;
    }
}

void NativeCommonUtils::SwitchToFormatType(const uint32_t &ohFormatType, IEffectFormat &formatType)
{
    OH_IEffectFormat ohFormatTypeEnum = static_cast<OH_IEffectFormat>(ohFormatType);
    SwitchToFormatType(ohFormatTypeEnum, formatType);
}

void NativeCommonUtils::SwitchToOHCategory(const Category &category, OH_Category &ohCategory)
{
    auto it = CATEGORY_TABLE.find(category);
    if (it != CATEGORY_TABLE.end()) {
        ohCategory = it->second;
    } else {
        ohCategory = OH_Category::UNKNOWN_CATEGORY;
    }
}

void NativeCommonUtils::SwitchToCategory(const OH_Category &ohCategory, Category &category)
{
    auto categoryIter = std::find_if(CATEGORY_TABLE.begin(), CATEGORY_TABLE.end(),
        [&ohCategory](const std::pair<Category, OH_Category> &category) {
            return category.second == ohCategory;
        });
    if (categoryIter != CATEGORY_TABLE.end()) {
        category = categoryIter->first;
    } else {
        category = Category::DEFAULT;
    }
}

void NativeCommonUtils::SwitchToCategory(const uint32_t &ohCategory, Category &category)
{
    OH_Category ohCategoryEnum = static_cast<OH_Category>(ohCategory);
    SwitchToCategory(ohCategoryEnum, category);
}

void SwitchToOHBufferType(const IPType &ipType, OH_BufferType &ohBufferType)
{
    auto it = IPTYPE_TABLE.find(ipType);
    if (it != IPTYPE_TABLE.end()) {
        ohBufferType = it->second;
    } else {
        ohBufferType = OH_BufferType::UNKNOWN_BUFFERTYPE;
    }
}

void NativeCommonUtils::SwitchToOHEffectInfo(const EffectInfo *effectInfo, OH_EffectInfo *ohEffectInfo)
{
    CHECK_AND_RETURN_LOG(effectInfo != nullptr, "effectInfo is null!");
    CHECK_AND_RETURN_LOG(ohEffectInfo != nullptr, "ohEffectInfo is null!");
    OH_Category ohCategory = OH_Category::UNKNOWN_CATEGORY;
    SwitchToOHCategory(effectInfo->category_, ohCategory);
    ohEffectInfo->category = ohCategory;

    uint32_t formats = 0;
    uint32_t bufferTypes = 0;
    for (const auto &format : effectInfo->formats_) {
        for (auto ipType : format.second) {
            OH_BufferType bufferType;
            SwitchToOHBufferType(ipType, bufferType);
            bufferTypes |= bufferType;
        }

        uint32_t ohFormatType;
        SwitchToOHFormatType(format.first, ohFormatType);
        formats |= ohFormatType;
    }
    ohEffectInfo->bufferTypes = bufferTypes;
    ohEffectInfo->formats = formats;
}

PixelMap *NativeCommonUtils::GetPixelMapFromNativePixelMap(NativePixelMap *nativePixelMap)
{
    CHECK_AND_RETURN_RET_LOG(nativePixelMap != nullptr, nullptr, "input nativePixelMap is null!");

    std::shared_ptr<PixelMap> pixelMap = PixelMapNative_GetPixelMap(nativePixelMap);
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, nullptr, "pixelMap is null!");

    return pixelMap.get();
}

bool IsMatchLookupCondition(std::shared_ptr<EffectInfo> &effectInfo, std::string &type, uint32_t enumValue)
{
    if (type.compare("Category") == 0) {
        return effectInfo->category_ == static_cast<Category>(enumValue) ||
            static_cast<Category>(enumValue) == Category::DEFAULT;
    } else if (type.compare("Format") == 0) {
        auto formatType = static_cast<IEffectFormat>(enumValue);
        if (formatType == IEffectFormat::DEFAULT) {
            return true;
        }

        auto it = std::find_if(effectInfo->formats_.begin(), effectInfo->formats_.end(),
            [&formatType](const std::pair<IEffectFormat, std::vector<IPType>> &format) {
                return formatType == format.first;
            });
        return it != effectInfo->formats_.end();
    } else {
        return false;
    }
}

bool IsMatchLookupCondition(std::shared_ptr<IFilterDelegate> &filterDelegate, std::string &type, uint32_t enumValue)
{
    auto effectInfo = static_cast<OH_EffectInfo *>(filterDelegate->GetEffectInfo());

    if (type.compare("Category") == 0) {
        auto category = static_cast<Category>(enumValue);
        if (category == Category::DEFAULT) {
            return true;
        }

        OH_Category ohCategory;
        NativeCommonUtils::SwitchToOHCategory(category, ohCategory);
        return effectInfo != nullptr && effectInfo->category != OH_Category::UNKNOWN_CATEGORY &&
            effectInfo->category == ohCategory;
    } else if (type.compare("Format") == 0) {
        auto formatType = static_cast<IEffectFormat>(enumValue);
        if (formatType == IEffectFormat::DEFAULT) {
            return true;
        }
        uint32_t ohFormatType;
        NativeCommonUtils::SwitchToOHFormatType(formatType, ohFormatType);
        return effectInfo != nullptr && effectInfo->formats != OH_IEffectFormat::UNKNOWN_FORMAT &&
            (effectInfo->formats & ohFormatType);
    } else {
        return false;
    }
}

void NativeCommonUtils::ParseLookupKey(std::string &key, std::vector<const char *> &matchEFilter)
{
    auto pos = key.find(':');
    CHECK_AND_RETURN_LOG(pos != std::string::npos, "key is invalid! key=%{public}s", key.c_str());

    std::string type = key.substr(0, pos);
    auto it = LOOK_UP_CAPABILITY.find(type);
    CHECK_AND_RETURN_LOG(it != LOOK_UP_CAPABILITY.end(),
        "type is invalid! key=%{public}s, type=%{public}s", key.c_str(), type.c_str());

    std::string value = key.substr(pos + 1);
    auto valueIterator = it->second.find(value);
    CHECK_AND_RETURN_LOG(valueIterator != it->second.end(),
        "value is invalid! key=%{public}s, type=%{public}s, value=%{public}s",
        key.c_str(), type.c_str(), value.c_str());

    std::vector<const char *> efilterNames;
    EFilterFactory::Instance()->GetAllEffectNames(efilterNames);
    std::shared_ptr<EffectInfo> effectInfo;
    std::shared_ptr<IFilterDelegate> filterDelegate;
    for (const auto &efilterName : efilterNames) {
        // custom efilter
        filterDelegate = EFilterFactory::Instance()->GetDelegate(efilterName);
        if (filterDelegate != nullptr) {
            if (IsMatchLookupCondition(filterDelegate, type, valueIterator->second)) {
                matchEFilter.emplace_back(efilterName);
            }
            continue;
        }

        effectInfo = EFilterFactory::Instance()->GetEffectInfo(efilterName);
        if (effectInfo == nullptr) {
            EFFECT_LOGW("effectInfo is null! efilterName=%{public}s", efilterName);
            continue;
        }
        if (IsMatchLookupCondition(effectInfo, type, valueIterator->second)) {
            matchEFilter.emplace_back(efilterName);
        }
    }
}

void NativeCommonUtils::SwitchToEffectInfo(const OH_EffectInfo *info, std::shared_ptr<EffectInfo> &effectInfo)
{
    SwitchToCategory(info->category, effectInfo->category_);
    for (const auto &formats: FORMAT_TABLE) {
        if (formats.second != OH_IEffectFormat::UNKNOWN_FORMAT &&
            static_cast<uint32_t>(formats.second) & info->formats) {
            effectInfo->formats_.emplace(formats.first, std::vector<IPType>{ IPType::CPU });
        }
    }
}
} // namespace Effect
} // namespace Media
} // namespace OHOS