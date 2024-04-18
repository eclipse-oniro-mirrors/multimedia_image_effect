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

#include "native_common_utils.h"

#include <set>
#include <algorithm>

#include "effect_log.h"
#include "efilter_factory.h"
#include "native_effect_base.h"
#include "pixelmap_native_impl.h"

namespace OHOS {
namespace Media {
namespace Effect {
static const std::map<IEffectFormat, ImageEffect_Format> FORMAT_TABLE = {
    { IEffectFormat::RGBA8888, ImageEffect_Format::EFFECT_PIXEL_FORMAT_RGBA8888 },
    { IEffectFormat::YUVNV12, ImageEffect_Format::EFFECT_PIXEL_FORMAT_NV12 },
    { IEffectFormat::YUVNV21, ImageEffect_Format::EFFECT_PIXEL_FORMAT_NV21 }
};

static const std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>> LOOK_UP_CAPABILITY = {
    { "Format",
        {
            { "default", static_cast<uint32_t>(IEffectFormat::DEFAULT) },
            { "rgba_8888", static_cast<uint32_t>(IEffectFormat::RGBA8888) },
            { "nv21", static_cast<uint32_t>(IEffectFormat::YUVNV21) },
            { "nv12", static_cast<uint32_t>(IEffectFormat::YUVNV12) },
        }
    },
};

static const std::map<IPType, ImageEffect_BufferType> IPTYPE_TABLE = {
    { IPType::CPU, ImageEffect_BufferType::EFFECT_BUFFER_TYPE_PIXEL },
    { IPType::GPU, ImageEffect_BufferType::EFFECT_BUFFER_TYPE_TEXTURE },
};

template <class ValueType>
ErrorCode AnyCastOHAny(const Plugin::Any &any, ImageEffect_DataType &ohDataType, ImageEffect_DataType ohDataTypeValue,
    ValueType &value)
{
    auto result = Plugin::AnyCast<ValueType>(&any);
    if (result == nullptr) {
        return ErrorCode::ERR_ANY_CAST_TYPE_NOT_MATCH;
    }
    ohDataType = ohDataTypeValue;
    value = *result;
    return ErrorCode::SUCCESS;
}

ErrorCode NativeCommonUtils::ParseOHAny(const ImageEffect_Any *value, Plugin::Any &any)
{
    switch (value->dataType) {
        case ImageEffect_DataType::EFFECT_DATA_TYPE_INT32:
            any = value->dataValue.int32Value;
            break;
        case ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT:
            any = value->dataValue.floatValue;
            break;
        case ImageEffect_DataType::EFFECT_DATA_TYPE_DOUBLE:
            any = value->dataValue.doubleValue;
            break;
        case ImageEffect_DataType::EFFECT_DATA_TYPE_CHAR:
            any = value->dataValue.charValue;
            break;
        case ImageEffect_DataType::EFFECT_DATA_TYPE_LONG:
            any = value->dataValue.longValue;
            break;
        case ImageEffect_DataType::EFFECT_DATA_TYPE_BOOL:
            any = value->dataValue.boolValue;
            break;
        case ImageEffect_DataType::EFFECT_DATA_TYPE_PTR:
            any = value->dataValue.ptrValue;
            break;
        default:
            EFFECT_LOGE("input any data type not support! dataType=%{public}d", value->dataType);
            return ErrorCode::ERR_UNSUPPORTED_INPUT_ANYTYPE;
    }
    return ErrorCode::SUCCESS;
}

ErrorCode NativeCommonUtils::SwitchToOHAny(const Plugin::Any &any, ImageEffect_Any *value)
{
    CHECK_AND_RETURN_RET(AnyCastOHAny(any, value->dataType, ImageEffect_DataType::EFFECT_DATA_TYPE_INT32,
        value->dataValue.int32Value) != ErrorCode::SUCCESS, ErrorCode::SUCCESS);
    CHECK_AND_RETURN_RET(AnyCastOHAny(any, value->dataType, ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT,
        value->dataValue.floatValue) != ErrorCode::SUCCESS, ErrorCode::SUCCESS);
    CHECK_AND_RETURN_RET(AnyCastOHAny(any, value->dataType, ImageEffect_DataType::EFFECT_DATA_TYPE_DOUBLE,
        value->dataValue.doubleValue) != ErrorCode::SUCCESS, ErrorCode::SUCCESS);
    CHECK_AND_RETURN_RET(AnyCastOHAny(any, value->dataType, ImageEffect_DataType::EFFECT_DATA_TYPE_CHAR,
        value->dataValue.charValue) != ErrorCode::SUCCESS, ErrorCode::SUCCESS);
    CHECK_AND_RETURN_RET(AnyCastOHAny(any, value->dataType, ImageEffect_DataType::EFFECT_DATA_TYPE_LONG,
        value->dataValue.longValue) != ErrorCode::SUCCESS, ErrorCode::SUCCESS);
    CHECK_AND_RETURN_RET(AnyCastOHAny(any, value->dataType, ImageEffect_DataType::EFFECT_DATA_TYPE_PTR,
        value->dataValue.ptrValue) != ErrorCode::SUCCESS, ErrorCode::SUCCESS);
    CHECK_AND_RETURN_RET(AnyCastOHAny(any, value->dataType, ImageEffect_DataType::EFFECT_DATA_TYPE_BOOL,
        value->dataValue.boolValue) != ErrorCode::SUCCESS, ErrorCode::SUCCESS);

#ifndef HST_ANY_WITH_NO_RTTI
    EFFECT_LOGE("inner any type not support switch to oh_any! type:%{public}s", any.Type().name());
#else
    EFFECT_LOGE("inner any type not support switch to oh_any! type:%{public}s", std::string(any.TypeName()).c_str());
#endif
    return ErrorCode::ERR_NOT_SUPPORT_SWITCH_TO_OHANY;
}

void NativeCommonUtils::SwitchToOHFormatType(const IEffectFormat &formatType, ImageEffect_Format &ohFormatType)
{
    auto it = FORMAT_TABLE.find(formatType);
    if (it != FORMAT_TABLE.end()) {
        ohFormatType = it->second;
    } else {
        ohFormatType = ImageEffect_Format::EFFECT_PIXEL_FORMAT_UNKNOWN;
    }
}

void NativeCommonUtils::SwitchToFormatType(const ImageEffect_Format &ohFormatType, IEffectFormat &formatType)
{
    auto formatIter = std::find_if(FORMAT_TABLE.begin(), FORMAT_TABLE.end(),
        [&ohFormatType](const std::pair<IEffectFormat, ImageEffect_Format> &format) {
            return format.second == ohFormatType;
    });
    if (formatIter != FORMAT_TABLE.end()) {
        formatType = formatIter->first;
    } else {
        formatType = IEffectFormat::DEFAULT;
    }
}

void SwitchToOHBufferType(const IPType &ipType, ImageEffect_BufferType &ohBufferType)
{
    auto it = IPTYPE_TABLE.find(ipType);
    if (it != IPTYPE_TABLE.end()) {
        ohBufferType = it->second;
    } else {
        ohBufferType = ImageEffect_BufferType::EFFECT_BUFFER_TYPE_UNKNOWN;
    }
}

void NativeCommonUtils::SwitchToOHEffectInfo(const EffectInfo *effectInfo, OH_EffectFilterInfo *ohFilterInfo)
{
    CHECK_AND_RETURN_LOG(effectInfo != nullptr, "effectInfo is null!");
    CHECK_AND_RETURN_LOG(ohFilterInfo != nullptr, "ohFilterInfo is null!");

    ohFilterInfo->supportedBufferTypes.clear();
    ohFilterInfo->supportedFormats.clear();
    for (const auto &format : effectInfo->formats_) {
        for (auto ipType : format.second) {
            ImageEffect_BufferType bufferType = ImageEffect_BufferType::EFFECT_BUFFER_TYPE_UNKNOWN;
            SwitchToOHBufferType(ipType, bufferType);
            if (bufferType == ImageEffect_BufferType::EFFECT_BUFFER_TYPE_UNKNOWN) {
                continue;
            }
            ohFilterInfo->supportedBufferTypes.emplace(bufferType);
        }

        ImageEffect_Format ohFormatType = ImageEffect_Format::EFFECT_PIXEL_FORMAT_UNKNOWN;
        SwitchToOHFormatType(format.first, ohFormatType);
        ohFilterInfo->supportedFormats.emplace(ohFormatType);
    }
}

PixelMap *NativeCommonUtils::GetPixelMapFromOHPixelmap(OH_PixelmapNative *pixelmapNative)
{
    CHECK_AND_RETURN_RET_LOG(pixelmapNative != nullptr, nullptr, "input pixelmapNative is null!");

    std::shared_ptr<PixelMap> pixelMap = pixelmapNative->GetInnerPixelmap();
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, nullptr, "pixelMap is null!");

    return pixelMap.get();
}

bool IsMatchLookupCondition(std::shared_ptr<EffectInfo> &effectInfo, std::string &type, uint32_t enumValue)
{
    if (type.compare("Format") == 0) {
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
    auto effectInfo = static_cast<OH_EffectFilterInfo *>(filterDelegate->GetEffectInfo());

    if (type.compare("Format") == 0) {
        auto formatType = static_cast<IEffectFormat>(enumValue);
        if (formatType == IEffectFormat::DEFAULT) {
            return true;
        }
        ImageEffect_Format ohFormatType = ImageEffect_Format::EFFECT_PIXEL_FORMAT_UNKNOWN;
        NativeCommonUtils::SwitchToOHFormatType(formatType, ohFormatType);
        return ohFormatType != ImageEffect_Format::EFFECT_PIXEL_FORMAT_UNKNOWN && effectInfo != nullptr &&
            effectInfo->supportedFormats.find(ohFormatType) != effectInfo->supportedFormats.end();
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

void NativeCommonUtils::SwitchToEffectInfo(const OH_EffectFilterInfo *info, std::shared_ptr<EffectInfo> &effectInfo)
{
    CHECK_AND_RETURN_LOG(info != nullptr, "SwitchToEffectInfo: info is null!");
    effectInfo->category_ = Category::DEFAULT;
    for (const auto &format: FORMAT_TABLE) {
        ImageEffect_Format ohFormat = format.second;
        if (ohFormat != ImageEffect_Format::EFFECT_PIXEL_FORMAT_UNKNOWN &&
            info->supportedFormats.find(ohFormat) != info->supportedFormats.end()) {
            effectInfo->formats_.emplace(format.first, std::vector<IPType>{ IPType::CPU });
        }
    }
}
} // namespace Effect
} // namespace Media
} // namespace OHOS