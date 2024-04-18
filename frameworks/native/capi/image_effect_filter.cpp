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

#include "image_effect_filter.h"

#include "common_utils.h"
#include "delegate.h"
#include "effect_log.h"
#include "efilter.h"
#include "efilter_factory.h"
#include "native_effect_base.h"
#include "native_common_utils.h"
#include "memcpy_helper.h"
#include "format_helper.h"

using namespace OHOS::Media;
using namespace OHOS::Media::Effect;

namespace {
    constexpr char const *PARA_SRC_EFFECT_BUFFER = "PARA_SRC_EFFECT_BUFFER";
    constexpr char const *PARA_RENDER_WITH_SRC_AND_DST = "PARA_RENDER_WITH_SRC_AND_DST";
    constexpr char const *PARA_RENDER_INFO = "PARA_RENDER_INFO";
}

static std::vector<std::shared_ptr<ImageEffect_FilterNames>> sOHFilterNames;

void OH_EffectFilter::SetParameter(const std::string &key, Plugin::Any &param)
{
    params_.erase(key);
    params_.emplace(key, param);
}

ErrorCode OH_EffectFilter::GetParameter(const std::string &key, Plugin::Any &param)
{
    auto it = params_.find(key);
    if (it == params_.end()) {
        return ErrorCode::ERR_NO_VALUE;
    }

    param = std::move(it->second);
    return ErrorCode::SUCCESS;
}

void OH_EffectFilter::RemoveParameter(const std::string &key)
{
    params_.erase(key);
}

class FilterDelegate : public IFilterDelegate {
public:
    FilterDelegate(const OH_EffectFilterInfo *info, const ImageEffect_FilterDelegate *delegate)
        : ohInfo_(*info), ohDelegate_(delegate) {}

    ~FilterDelegate() override = default;

    bool Render(void *efilter, EffectBuffer *src, EffectBuffer *dst, std::shared_ptr<EffectContext> &context) override
    {
        EFFECT_LOGI("FilterDelegate Render with src and dst.");
        OH_EffectFilter *ohEFilter = (OH_EffectFilter *)efilter;
        CHECK_AND_RETURN_RET_LOG(ohEFilter != nullptr && ohEFilter->filter_ != nullptr, false,
            "FilterDelegateRender: filter is null! ohEFilter=%{public}p", ohEFilter);
        Plugin::Any param = true;
        ohEFilter->SetParameter(PARA_RENDER_WITH_SRC_AND_DST, param);
        Plugin::Any any = context;
        ohEFilter->SetParameter(PARA_RENDER_INFO, any);

        MemcpyHelper::CopyData(src, dst);
        bool res = Render(efilter, dst, context);
        ohEFilter->RemoveParameter(PARA_RENDER_WITH_SRC_AND_DST);
        ohEFilter->RemoveParameter(PARA_RENDER_INFO);
        return res;
    }

    bool Render(void *efilter, EffectBuffer *src, std::shared_ptr<EffectContext> &context) override
    {
        EFFECT_LOGI("FilterDelegate Render.");
        std::unique_ptr<OH_EffectBufferInfo> srcBuffer = std::make_unique<OH_EffectBufferInfo>();
        srcBuffer->addr = src->buffer_;
        srcBuffer->width = src->bufferInfo_->width_;
        srcBuffer->height = src->bufferInfo_->height_;
        srcBuffer->rowSize = src->bufferInfo_->rowStride_;
        NativeCommonUtils::SwitchToOHFormatType(src->bufferInfo_->formatType_, srcBuffer->format);

        OH_EffectFilter *ohEFilter = (OH_EffectFilter *)efilter;
        CHECK_AND_RETURN_RET_LOG(ohEFilter != nullptr && ohEFilter->filter_ != nullptr, false,
            "FilterDelegateRender: filter is null! ohEFilter=%{public}p", ohEFilter);

        Plugin::Any any = src;
        ohEFilter->SetParameter(PARA_SRC_EFFECT_BUFFER, any);
        Plugin::Any parameter = context;
        ohEFilter->SetParameter(PARA_RENDER_INFO, parameter);

        OH_EffectFilterDelegate_PushData pushData = [](OH_EffectFilter *filter, OH_EffectBufferInfo *dst) {
            FilterDelegate::PushData(filter, dst);
        };

        bool res = ohDelegate_->render((OH_EffectFilter *)efilter, srcBuffer.get(), pushData);
        ohEFilter->RemoveParameter(PARA_SRC_EFFECT_BUFFER);
        ohEFilter->RemoveParameter(PARA_RENDER_INFO);
        return res;
    }

    bool SetValue(void *efilter, const std::string &key, const Plugin::Any &value) override
    {
        EFFECT_LOGI("FilterDelegate SetValue.");
        std::unique_ptr<ImageEffect_Any> ohValue = std::make_unique<ImageEffect_Any>();
        NativeCommonUtils::SwitchToOHAny(value, ohValue.get());
        return ohDelegate_->setValue((OH_EffectFilter *)efilter, key.c_str(), ohValue.get());
    }

    bool Save(void *efilter, nlohmann::json &res) override
    {
        EFFECT_LOGI("FilterDelegate Save.");
        char *result = nullptr;
        if (!ohDelegate_->save((OH_EffectFilter *)efilter, &result)) {
            return false;
        }
        if (result == nullptr) {
            return true;
        }
        std::string content = result;
        res = nlohmann::json::parse(content, nullptr, false);
        if (res.is_discarded()) {
            EFFECT_LOGW("json object is null");
        }
        return true;
    }

    void *Restore(const nlohmann::json &values) override
    {
        EFFECT_LOGI("FilterDelegate Restore.");
        std::string valueStr;
        try {
            valueStr = values.dump();
        } catch (nlohmann::json::type_error &exception) {
            EFFECT_LOGE("FilterDelegate Restore: json dump fail! error:%{public}s", exception.what());
            return nullptr;
        }
        return ohDelegate_->restore(valueStr.c_str());
    }

    void *GetEffectInfo() override
    {
        EFFECT_LOGI("FilterDelegate GetEffectInfo.");
        return &ohInfo_;
    }
protected:
    static void PushData(OH_EffectFilter *filter, OH_EffectBufferInfo *dst)
    {
        CHECK_AND_RETURN_LOG(dst != nullptr && filter != nullptr && filter->filter_ != nullptr,
            "FilterDelegatePushData: filter is null! ohEFilter=%{public}p, dst=%{public}p", filter, dst);
        Plugin::Any param;
        if (filter->GetParameter(PARA_RENDER_WITH_SRC_AND_DST, param) == ErrorCode::SUCCESS) {
            return;
        }

        Plugin::Any value;
        ErrorCode res = filter->GetParameter(PARA_SRC_EFFECT_BUFFER, value);
        CHECK_AND_RETURN_LOG(res == ErrorCode::SUCCESS, "FilterDelegatePushData: get param fail! key=%{public}s",
            PARA_SRC_EFFECT_BUFFER);

        EffectBuffer *src = nullptr;
        res = CommonUtils::ParseAny(value, src);
        CHECK_AND_RETURN_LOG(res == ErrorCode::SUCCESS && src != nullptr,
            "FilterDelegatePushData: parse EffectBuffer ptr fail! res=%{public}d, src=%{public}p", res, src);

        std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
        bufferInfo->width_ = dst->width;
        bufferInfo->height_ = dst->height;
        bufferInfo->rowStride_ = dst->rowSize;
        NativeCommonUtils::SwitchToFormatType(dst->format, bufferInfo->formatType_);
        bufferInfo->len_ =
            FormatHelper::CalculateDataRowCount(bufferInfo->height_, bufferInfo->formatType_) * bufferInfo->rowStride_;
        std::shared_ptr<ExtraInfo> extraInfo = std::make_unique<ExtraInfo>();
        *extraInfo = *src->extraInfo_;
        extraInfo->bufferType = BufferType::DEFAULT;
        extraInfo->surfaceBuffer = nullptr;
        std::shared_ptr<EffectBuffer> effectBuffer = std::make_shared<EffectBuffer>(bufferInfo, dst->addr, extraInfo);

        Plugin::Any any;
        res = filter->GetParameter(PARA_RENDER_INFO, any);
        CHECK_AND_RETURN_LOG(res == ErrorCode::SUCCESS, "FilterDelegatePushData: get param fail! key=%{public}s",
            PARA_RENDER_INFO);

        auto &context = Plugin::AnyCast<std::shared_ptr<EffectContext> &>(any);
        filter->filter_->PushData(effectBuffer.get(), context);
    }
private:
    OH_EffectFilterInfo ohInfo_;
    const ImageEffect_FilterDelegate *ohDelegate_;
};

OH_EffectFilterInfo::~OH_EffectFilterInfo()
{
    if (effectBufferType != nullptr) {
        delete[] effectBufferType;
    }
    if (effectFormat != nullptr) {
        delete[] effectFormat;
    }
}

#ifdef __cplusplus
extern "C" {
#endif

EFFECT_EXPORT
OH_EffectFilterInfo *OH_EffectFilterInfo_Create()
{
    std::unique_ptr<OH_EffectFilterInfo> info = std::make_unique<OH_EffectFilterInfo>();
    return info.release();
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectFilterInfo_SetFilterName(OH_EffectFilterInfo *info, const char *name)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoSetFilterName: input parameter info is null!");
    CHECK_AND_RETURN_RET_LOG(name != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoSetFilterName: input parameter name is null!");
    EFFECT_LOGD("Set filter name. name=%{public}s", name);
    info->filterName = name;
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectFilterInfo_GetFilterName(OH_EffectFilterInfo *info, char **name)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoGetFilterName: input parameter info is null!");
    CHECK_AND_RETURN_RET_LOG(name != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoGetFilterName: input parameter name is null!");

    *name = const_cast<char *>(info->filterName.c_str());
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectFilterInfo_SetSupportedBufferTypes(OH_EffectFilterInfo *info, uint32_t size,
    ImageEffect_BufferType *bufferTypeArray)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoSetSupportedBufferTypes: input parameter info is null!");
    CHECK_AND_RETURN_RET_LOG(size > 0, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoSetSupportedBufferTypes: input parameter size is invalid! size=%{public}d", size);
    CHECK_AND_RETURN_RET_LOG(bufferTypeArray != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoSetSupportedBufferTypes: input parameter bufferTypeArray is null!");
    EFFECT_LOGD("Set supported buffer types. size=%{public}d", size);
    info->supportedBufferTypes.clear();
    for (auto index = 0; index < size; index++) {
        info->supportedBufferTypes.emplace(bufferTypeArray[index]);
    }
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectFilterInfo_GetSupportedBufferTypes(OH_EffectFilterInfo *info, uint32_t *size,
    ImageEffect_BufferType **bufferTypeArray)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoGetSupportedBufferTypes: input parameter info is null!");
    CHECK_AND_RETURN_RET_LOG(size != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoGetSupportedBufferTypes: input parameter size is invalid!");
    CHECK_AND_RETURN_RET_LOG(bufferTypeArray != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoGetSupportedBufferTypes: input parameter bufferTypeArray is null!");

    if (info->supportedBufferTypes.empty()) {
        *size = 0;
        *bufferTypeArray = nullptr;
        return ImageEffect_ErrorCode::EFFECT_SUCCESS;
    }

    auto bufferTypeRealSize = static_cast<uint32_t>(info->supportedBufferTypes.size());
    if (bufferTypeRealSize > info->bufferTypeArraySize && info->effectBufferType != nullptr) {
        delete[] info->effectBufferType;
        info->effectBufferType = nullptr;
        info->bufferTypeArraySize = 0;
    }

    if (info->effectBufferType == nullptr) {
        std::unique_ptr<ImageEffect_BufferType[]> bufferType =
            std::make_unique<ImageEffect_BufferType[]>(bufferTypeRealSize);
        info->effectBufferType = bufferType.release();
        info->bufferTypeArraySize = bufferTypeRealSize;
    }

    uint32_t index = 0;
    for (auto &bufferType : info->supportedBufferTypes) {
        if (index >= info->bufferTypeArraySize) {
            EFFECT_LOGW("supportedBufferTypes size over bufferTypeArraySize! supportedBufferTypesSize=%{public}zu, "
                "bufferTypeArraySize=%{public}d", info->supportedBufferTypes.size(), info->bufferTypeArraySize);
            break;
        }
        info->effectBufferType[index] = bufferType;
        index++;
    }
    *size = index;
    *bufferTypeArray = info->effectBufferType;

    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectFilterInfo_SetSupportedFormats(OH_EffectFilterInfo *info, uint32_t size,
    ImageEffect_Format *formatArray)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoSetSupportedFormats: input parameter info is null!");
    CHECK_AND_RETURN_RET_LOG(size > 0, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoSetSupportedFormats: input parameter size is invalid! size=%{public}d", size);
    CHECK_AND_RETURN_RET_LOG(formatArray != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoSetSupportedFormats: input parameter formatArray is null!");
    EFFECT_LOGD("Set supported formats. size=%{public}d", size);
    info->supportedFormats.clear();
    for (auto index = 0; index < size; index++) {
        info->supportedFormats.emplace(formatArray[index]);
    }
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectFilterInfo_GetSupportedFormats(OH_EffectFilterInfo *info, uint32_t *size,
    ImageEffect_Format **formatArray)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoGetSupportedFormats: input parameter info is null!");
    CHECK_AND_RETURN_RET_LOG(size != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoGetSupportedFormats: input parameter size is invalid!");
    CHECK_AND_RETURN_RET_LOG(formatArray != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InfoGetSupportedFormats: input parameter formatArray is null!");

    if (info->supportedFormats.empty()) {
        *size = 0;
        *formatArray = nullptr;
        return ImageEffect_ErrorCode::EFFECT_SUCCESS;
    }

    auto formatsRealSize = static_cast<uint32_t>(info->supportedFormats.size());
    if (formatsRealSize > info->formatArraySize && info->effectFormat != nullptr) {
        delete[] info->effectFormat;
        info->effectFormat = nullptr;
        info->formatArraySize = 0;
    }

    if (info->effectFormat == nullptr) {
        std::unique_ptr<ImageEffect_Format []> bufferType = std::make_unique<ImageEffect_Format[]>(formatsRealSize);
        info->effectFormat = bufferType.release();
        info->formatArraySize = formatsRealSize;
    }

    uint32_t index = 0;
    for (auto &format : info->supportedFormats) {
        if (index >= info->formatArraySize) {
            EFFECT_LOGW("supportedFormats size over formatArraySize! supportedFormatsSize=%{public}zu, "
                "formatArraySize=%{public}d", info->supportedFormats.size(), info->formatArraySize);
            break;
        }
        info->effectFormat[index] = format;
        index++;
    }
    *size = index;
    *formatArray = info->effectFormat;

    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectFilterInfo_Release(OH_EffectFilterInfo *info)
{
    EFFECT_LOGD("Filter release info.");
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "ReleaseInfo: input parameter info is null!");
    delete (info);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
OH_EffectBufferInfo *OH_EffectBufferInfo_Create()
{
    std::unique_ptr<OH_EffectBufferInfo> info = std::make_unique<OH_EffectBufferInfo>();
    return info.release();
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectBufferInfo_SetAddr(OH_EffectBufferInfo *info, void *addr)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoSetAddr: input parameter info is null!");
    CHECK_AND_RETURN_RET_LOG(addr != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoSetAddr: input parameter addr is null!");
    EFFECT_LOGD("Set buffer info addr.");
    info->addr = addr;
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectBufferInfo_GetAddr(OH_EffectBufferInfo *info, void **addr)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoGetAddr: input parameter info is null!");
    CHECK_AND_RETURN_RET_LOG(addr != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoGetAddr: input parameter addr is null!");

    *addr = info->addr;
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectBufferInfo_SetWidth(OH_EffectBufferInfo *info, int32_t width)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoSetWidth: input parameter info is null!");
    EFFECT_LOGD("BufferInfoSetWidth: width=%{public}d", width);
    info->width = width;
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectBufferInfo_GetWidth(OH_EffectBufferInfo *info, int32_t *width)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoGetWidth: input parameter info is null!");
    CHECK_AND_RETURN_RET_LOG(width != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoGetWidth: input parameter width is null!");

    *width = info->width;
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectBufferInfo_SetHeight(OH_EffectBufferInfo *info, int32_t height)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoSetHeight: input parameter info is null!");
    EFFECT_LOGD("BufferInfoSetHeight: height=%{public}d", height);
    info->height = height;
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectBufferInfo_GetHeight(OH_EffectBufferInfo *info, int32_t *height)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoGetHeight: input parameter info is null!");
    CHECK_AND_RETURN_RET_LOG(height != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoGetHeight: input parameter height is null!");

    *height = info->height;
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectBufferInfo_SetRowSize(OH_EffectBufferInfo *info, int32_t rowSize)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoSetRowSize: input parameter info is null!");
    EFFECT_LOGD("BufferInfoSetRowSize: rowSize=%{public}d", rowSize);
    info->rowSize = rowSize;
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectBufferInfo_GetRowSize(OH_EffectBufferInfo *info, int32_t *rowSize)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoGetRowSize: input parameter info is null!");
    CHECK_AND_RETURN_RET_LOG(rowSize != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoGetRowSize: input parameter rowSize is null!");

    *rowSize = info->rowSize;
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectBufferInfo_SetEffectFormat(OH_EffectBufferInfo *info, ImageEffect_Format format)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoSetEffectFormat: input parameter info is null!");
    EFFECT_LOGD("BufferInfoSetEffectFormat: format=%{public}d", format);
    info->format = format;
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectBufferInfo_GetEffectFormat(OH_EffectBufferInfo *info, ImageEffect_Format *format)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoGetEffectFormat: input parameter info is null!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "BufferInfoGetEffectFormat: input parameter format is null!");

    *format = info->format;
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectBufferInfo_Release(OH_EffectBufferInfo *info)
{
    EFFECT_LOGD("Filter release buffer info.");
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "EffectFilter ReleaseBufferInfo: input parameter info is null!");
    delete (info);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
OH_EffectFilter *OH_EffectFilter_Create(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "FilterCreate: input parameter name is null!");
    EFFECT_LOGI("Filter create. name=%{public}s", name);
    std::unique_ptr<OH_EffectFilter> nativeEFilter = std::make_unique<OH_EffectFilter>();
    std::shared_ptr<EFilter> filter = EFilterFactory::Instance()->Create(name, nativeEFilter.get());
    if (filter == nullptr) {
        EFFECT_LOGW("FilterCreate: create filter fail. name=%{public}s not exist!", name);
        return nullptr;
    }
    nativeEFilter->filter_ = filter;
    return nativeEFilter.release();
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectFilter_SetValue(OH_EffectFilter *filter, const char *key, const ImageEffect_Any *value)
{
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "FilterSetValue: input parameter filter is null!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "FilterSetValue: input parameter key is null!");
    CHECK_AND_RETURN_RET_LOG(value != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "FilterSetValue: input parameter value is null!");
    EFFECT_LOGI("Effect filter set value. key=%{public}s", key);

    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(value, any);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGE("FilterSetValue: parse any fail! result=%{public}d, dataType=%{public}d", result, value->dataType);
        return ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID;
    }

    result = filter->filter_->SetValue(key, any);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGE("FilterSetValue: set value fail! result=%{public}d, key=%{public}s, dataType=%{public}d", result,
            key, value->dataType);
        return ImageEffect_ErrorCode::EFFECT_KEY_ERROR;
    }

    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectFilter_GetValue(OH_EffectFilter *nativeEFilter, const char *key, ImageEffect_Any *value)
{
    CHECK_AND_RETURN_RET_LOG(nativeEFilter != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "FilterGetValue: input parameter nativeEFilter is null!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "FilterGetValue: input parameter key is null!");
    CHECK_AND_RETURN_RET_LOG(value != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "FilterGetValue: input parameter value is null!");
    EFFECT_LOGD("Effect filter get value. key=%{public}s", key);

    if (strcmp("FILTER_NAME", key) == 0) {
        value->dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_PTR;
        value->dataValue.ptrValue = static_cast<void *>(const_cast<char *>(nativeEFilter->filter_->GetName().c_str()));
        return ImageEffect_ErrorCode::EFFECT_SUCCESS;
    }

    Plugin::Any any;
    ErrorCode result = nativeEFilter->filter_->GetValue(key, any);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGE("FilterGetValue: get value fail! result=%{public}d, key=%{public}s", result, key);
        return ImageEffect_ErrorCode::EFFECT_KEY_ERROR;
    }

    result = NativeCommonUtils::SwitchToOHAny(any, value);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGE("FilterGetValue: get value fail! result=%{public}d, key=%{public}s", result, key);
        return ImageEffect_ErrorCode::EFFECT_UNKNOWN;
    }

    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectFilter_Register(const OH_EffectFilterInfo *info,
    const ImageEffect_FilterDelegate *delegate)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "RegisterFilter: input parameter info is null!");
    CHECK_AND_RETURN_RET_LOG(delegate != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "RegisterFilter: input parameter delegate is null!");
    EFFECT_LOGI("Filter register. filterName=%{public}s", info->filterName.c_str());
    std::shared_ptr<FilterDelegate> effectDelegate = std::make_shared<FilterDelegate>(info, delegate);

    std::shared_ptr<EffectInfo> effectInfo = std::make_shared<EffectInfo>();
    NativeCommonUtils::SwitchToEffectInfo(info, effectInfo);
    EFilterFactory::Instance()->RegisterDelegate(info->filterName, effectDelegate, effectInfo);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_FilterNames *OH_EffectFilter_LookupFilters(const char *key)
{
    CHECK_AND_RETURN_RET_LOG(key != nullptr, nullptr, "LookupFilters: input parameter key is null!");
    EFFECT_LOGD("Lookup filters. key=%{public}s", key);

    std::string lookupKey = key;
    std::vector<const char *> matchEFilter;
    NativeCommonUtils::ParseLookupKey(lookupKey, matchEFilter);

    std::shared_ptr<ImageEffect_FilterNames> filterNames = std::make_shared<ImageEffect_FilterNames>();
    filterNames->size = matchEFilter.size();
    if (filterNames->size != 0) {
        const char **buffer = (const char **)malloc(matchEFilter.size() * sizeof(const char *));
        if (buffer != nullptr) {
            for (size_t i = 0; i < matchEFilter.size(); i++) {
                buffer[i] = matchEFilter[i];
            }
            filterNames->nameList = buffer;
        }
    }
    sOHFilterNames.emplace_back(filterNames);

    return filterNames.get();
}

EFFECT_EXPORT
void OH_EffectFilter_ReleaseFilterNames()
{
    EFFECT_LOGI("Release filter names.");
    for (const auto &filterNames : sOHFilterNames) {
        if (filterNames == nullptr) {
            continue;
        }
        free(filterNames->nameList);
    }
    sOHFilterNames.clear();
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectFilter_LookupFilterInfo(const char *name, OH_EffectFilterInfo *info)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "LookupFilterInfo: input parameter name is null!");
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "LookupFilterInfo: input parameter info is null!");
    EFFECT_LOGD("Lookup filter info. name=%{public}s", name);

    std::shared_ptr<IFilterDelegate> filterDelegate = EFilterFactory::Instance()->GetDelegate(name);
    if (filterDelegate != nullptr) {
        auto effectInfo = static_cast<OH_EffectFilterInfo *>(filterDelegate->GetEffectInfo());
        CHECK_AND_RETURN_RET_LOG(effectInfo != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
            "LookupFilterInfo: filter delegate get effect info is null! name=%{public}s", name);
        *info = *effectInfo;
        return ImageEffect_ErrorCode::EFFECT_SUCCESS;
    }

    std::shared_ptr<EffectInfo> effectInfo = EFilterFactory::Instance()->GetEffectInfo(name);
    CHECK_AND_RETURN_RET_LOG(effectInfo != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "LookupFilterInfo: lookup fail! name=%{public}s", name);

    info->filterName = name;
    NativeCommonUtils::SwitchToOHEffectInfo(effectInfo.get(), info);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectFilter_Render(OH_EffectFilter *filter, OH_PixelmapNative *inputPixelmap,
    OH_PixelmapNative *outputPixelmap)
{
    EFFECT_LOGI("Filter render.");
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "FilterRender: input parameter filter is null!");
    CHECK_AND_RETURN_RET_LOG(inputPixelmap != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "FilterRender: input parameter inputPixelmap is null!");
    CHECK_AND_RETURN_RET_LOG(outputPixelmap != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "FilterRender: input parameter outputPixelmap is null!");

    PixelMap *input = NativeCommonUtils::GetPixelMapFromOHPixelmap(inputPixelmap);
    PixelMap *output = NativeCommonUtils::GetPixelMapFromOHPixelmap(outputPixelmap);

    std::shared_ptr<EffectBuffer> inEffectBuffer = nullptr;
    ErrorCode result = CommonUtils::LockPixelMap(input, inEffectBuffer);
    if (result != ErrorCode::SUCCESS || inEffectBuffer == nullptr) {
        EFFECT_LOGE("FilterRender: lock input native pixelMap error! errorCode:%{public}d", result);
        CommonUtils::UnlockPixelMap(input);
        return ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID;
    }

    std::shared_ptr<EffectBuffer> outEffectBuffer = nullptr;
    result = CommonUtils::LockPixelMap(output, outEffectBuffer);
    if (result != ErrorCode::SUCCESS || outEffectBuffer == nullptr) {
        EFFECT_LOGE("FilterRender: lock output native pixelMap error! errorCode:%{public}d", result);
        CommonUtils::UnlockPixelMap(output);
        return ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID;
    }

    filter->filter_->PreRender(inEffectBuffer->bufferInfo_->formatType_);
    result = filter->filter_->Render(inEffectBuffer, outEffectBuffer);
    CommonUtils::UnlockPixelMap(input);
    CommonUtils::UnlockPixelMap(output);
    CHECK_AND_RETURN_RET_LOG(result == ErrorCode::SUCCESS, ImageEffect_ErrorCode::EFFECT_UNKNOWN,
        "FilterRender: filter render fail! errorCode:%{public}d", result);

    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_EffectFilter_Release(OH_EffectFilter *filter)
{
    EFFECT_LOGI("Effect filter release.");
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "FilterRelease: input parameter imageEffect is null!");
    delete (filter);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}
#ifdef __cplusplus
}
#endif