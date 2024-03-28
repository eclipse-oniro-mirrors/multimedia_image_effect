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

#include "native_efilter.h"

#include "common_utils.h"
#include "delegate.h"
#include "effect_log.h"
#include "efilter.h"
#include "efilter_factory.h"
#include "native_base.h"
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

static std::vector<std::shared_ptr<OH_FilterNames>> sOHFilterNames;

void OH_EFilter::SetParameter(const std::string &key, Plugin::Any &param)
{
    params_.erase(key);
    params_.emplace(key, param);
}

ErrorCode OH_EFilter::GetParameter(const std::string &key, Plugin::Any &param)
{
    auto it = params_.find(key);
    if (it == params_.end()) {
        return ErrorCode::ERR_NO_VALUE;
    }

    param = std::move(it->second);
    return ErrorCode::SUCCESS;
}

void OH_EFilter::RemoveParameter(const std::string &key)
{
    params_.erase(key);
}

class FilterDelegate : public IFilterDelegate {
public:
    FilterDelegate(const OH_EffectInfo *info, const OH_EFilterDelegate *delegate)
        : ohInfo_(*info), ohDelegate_(delegate) {}

    ~FilterDelegate() override = default;

    bool Render(void *efilter, EffectBuffer *src, EffectBuffer *dst, std::shared_ptr<EffectContext> &context) override
    {
        EFFECT_LOGI("FilterDelegate Render with src and dst.");
        OH_EFilter *ohEFilter = (OH_EFilter *)efilter;
        CHECK_AND_RETURN_RET_LOG(ohEFilter != nullptr && ohEFilter->filter_ != nullptr, false,
            "filter is null! ohEFilter=%{public}p", ohEFilter);
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
        std::unique_ptr<OH_EffectBuffer> srcBuffer = std::make_unique<OH_EffectBuffer>();
        srcBuffer->buffer = src->buffer_;
        srcBuffer->width = src->bufferInfo_->width_;
        srcBuffer->height = src->bufferInfo_->height_;
        srcBuffer->rowStride = src->bufferInfo_->rowStride_;
        NativeCommonUtils::SwitchToOHFormatType(src->bufferInfo_->formatType_, srcBuffer->format);

        OH_EFilter *ohEFilter = (OH_EFilter *)efilter;
        CHECK_AND_RETURN_RET_LOG(ohEFilter != nullptr && ohEFilter->filter_ != nullptr, false,
            "filter is null! ohEFilter=%{public}p", ohEFilter);

        Plugin::Any any = src;
        ohEFilter->SetParameter(PARA_SRC_EFFECT_BUFFER, any);
        Plugin::Any parameter = context;
        ohEFilter->SetParameter(PARA_RENDER_INFO, parameter);

        OH_EFilterDelegate_PushData pushData = [](OH_EFilter *filter, OH_EffectBuffer *dst) {
            FilterDelegate::PushData(filter, dst);
        };

        bool res = ohDelegate_->render((OH_EFilter *)efilter, srcBuffer.get(), pushData);
        ohEFilter->RemoveParameter(PARA_SRC_EFFECT_BUFFER);
        ohEFilter->RemoveParameter(PARA_RENDER_INFO);
        return res;
    }

    bool SetValue(void *efilter, const std::string &key, const Plugin::Any &value) override
    {
        EFFECT_LOGI("FilterDelegate SetValue.");
        std::unique_ptr<OH_Any> ohValue = std::make_unique<OH_Any>();
        NativeCommonUtils::SwitchToOHAny(value, ohValue.get());
        return ohDelegate_->setValue((OH_EFilter *)efilter, key.c_str(), ohValue.get());
    }

    bool Save(void *efilter, nlohmann::json &res) override
    {
        EFFECT_LOGI("FilterDelegate Save.");
        char *result = nullptr;
        if (!ohDelegate_->save((OH_EFilter *)efilter, &result)) {
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
    static void PushData(OH_EFilter *filter, OH_EffectBuffer *dst)
    {
        CHECK_AND_RETURN_LOG(dst != nullptr && filter != nullptr && filter->filter_ != nullptr,
            "filter is null! ohEFilter=%{public}p, dst=%{public}p", filter, dst);
        Plugin::Any param;
        if (filter->GetParameter(PARA_RENDER_WITH_SRC_AND_DST, param) == ErrorCode::SUCCESS) {
            return;
        }

        Plugin::Any value;
        ErrorCode res = filter->GetParameter(PARA_SRC_EFFECT_BUFFER, value);
        CHECK_AND_RETURN_LOG(res == ErrorCode::SUCCESS, "get param fail! key=%{public}s", PARA_SRC_EFFECT_BUFFER);

        EffectBuffer *src = nullptr;
        res = CommonUtils::ParseAny(value, src);
        CHECK_AND_RETURN_LOG(res == ErrorCode::SUCCESS && src != nullptr,
            "parse EffectBuffer ptr fail! res=%{public}d, src=%{public}p", res, src);

        std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
        bufferInfo->width_ = dst->width;
        bufferInfo->height_ = dst->height;
        bufferInfo->rowStride_ = dst->rowStride;
        NativeCommonUtils::SwitchToFormatType(dst->format, bufferInfo->formatType_);
        bufferInfo->len_ =
            FormatHelper::CalculateDataRowCount(bufferInfo->height_, bufferInfo->formatType_) * bufferInfo->rowStride_;
        std::shared_ptr<ExtraInfo> extraInfo = std::make_unique<ExtraInfo>();
        *extraInfo = *src->extraInfo_;
        extraInfo->bufferType = BufferType::DEFAULT;
        extraInfo->surfaceBuffer = nullptr;
        std::shared_ptr<EffectBuffer> effectBuffer = std::make_shared<EffectBuffer>(bufferInfo, dst->buffer, extraInfo);

        Plugin::Any any;
        res = filter->GetParameter(PARA_RENDER_INFO, any);
        CHECK_AND_RETURN_LOG(res == ErrorCode::SUCCESS, "get param fail! key=%{public}s", PARA_RENDER_INFO);

        auto &context = Plugin::AnyCast<std::shared_ptr<EffectContext> &>(any);
        filter->filter_->PushData(effectBuffer.get(), context);
    }
private:
    OH_EffectInfo ohInfo_;
    const OH_EFilterDelegate *ohDelegate_;
};

#ifdef __cplusplus
extern "C" {
#endif

EFFECT_EXPORT
OH_EFilter *OH_EFilter_Create(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "input parameter name is null!");
    EFFECT_LOGI("creat efilter. name=%{public}s", name);
    std::unique_ptr<OH_EFilter> nativeEFilter = std::make_unique<OH_EFilter>();
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(name, nativeEFilter.get());
    if (efilter == nullptr) {
        EFFECT_LOGW("creat efilter fail. name=%{public}s not exist!", name);
        return nullptr;
    }
    nativeEFilter->filter_ = efilter;
    return nativeEFilter.release();
}

EFFECT_EXPORT
OH_EffectErrorCode OH_EFilter_SetValue(OH_EFilter *filter, const char *key, const OH_Any *value)
{
    EFFECT_LOGI("filter set value.");
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, OH_EffectErrorCode::EFFECT_ERR_INPUT_NULL,
        "input parameter nativeEFilter is null!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, OH_EffectErrorCode::EFFECT_ERR_INPUT_NULL, "input parameter key is null!");
    CHECK_AND_RETURN_RET_LOG(value != nullptr, OH_EffectErrorCode::EFFECT_ERR_INPUT_NULL,
        "input parameter value is null!");

    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(value, any);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGE("parse oh any fail! result=%{public}d, dataType=%{public}d", result, value->dataType);
        return OH_EffectErrorCode::EFFECT_ERR_SET_VALUE_FAIL;
    }

    result = filter->filter_->SetValue(key, any);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGE("set value fail! result=%{public}d, key=%{public}s, dataType=%{public}d", result, key,
            value->dataType);
        return OH_EffectErrorCode::EFFECT_ERR_SET_VALUE_FAIL;
    }

    return OH_EffectErrorCode::EFFECT_ERR_SUCCESS;
}

EFFECT_EXPORT
OH_EffectErrorCode OH_EFilter_GetValue(OH_EFilter *nativeEFilter, const char *key, OH_Any *value)
{
    EFFECT_LOGI("filter get value.");
    CHECK_AND_RETURN_RET_LOG(nativeEFilter != nullptr, OH_EffectErrorCode::EFFECT_ERR_INPUT_NULL,
        "input parameter nativeEFilter is null!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, OH_EffectErrorCode::EFFECT_ERR_INPUT_NULL, "input parameter key is null!");
    CHECK_AND_RETURN_RET_LOG(value != nullptr, OH_EffectErrorCode::EFFECT_ERR_INPUT_NULL,
        "input parameter value is null!");

    if (strcmp("FILTER_NAME", key) == 0) {
        value->dataType = OH_DataType::TYPE_PTR;
        value->dataValue.ptrValue = static_cast<void *>(const_cast<char *>(nativeEFilter->filter_->GetName().c_str()));
        return OH_EffectErrorCode::EFFECT_ERR_SUCCESS;
    }

    Plugin::Any any;
    ErrorCode result = nativeEFilter->filter_->GetValue(key, any);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGE("get value fail! result=%{public}d, key=%{public}s", result, key);
        return OH_EffectErrorCode::EFFECT_ERR_GET_VALUE_FAIL;
    }

    result = NativeCommonUtils::SwitchToOHAny(any, value);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGE("get value fail! result=%{public}d, key=%{public}s", result, key);
        return OH_EffectErrorCode::EFFECT_ERR_GET_VALUE_FAIL;
    }

    return OH_EffectErrorCode::EFFECT_ERR_SUCCESS;
}

EFFECT_EXPORT
OH_EffectErrorCode OH_EFilter_Register(const OH_EffectInfo *info, const OH_EFilterDelegate *delegate)
{
    EFFECT_LOGI("efilter register.");
    CHECK_AND_RETURN_RET_LOG(info != nullptr, OH_EffectErrorCode::EFFECT_ERR_INPUT_NULL,
        "input parameter info is null!");
    CHECK_AND_RETURN_RET_LOG(delegate != nullptr, OH_EffectErrorCode::EFFECT_ERR_INPUT_NULL,
        "input parameter delegate is null!");
    std::shared_ptr<FilterDelegate> effectDelegate = std::make_shared<FilterDelegate>(info, delegate);

    std::shared_ptr<EffectInfo> effectInfo = std::make_shared<EffectInfo>();
    NativeCommonUtils::SwitchToEffectInfo(info, effectInfo);
    EFilterFactory::Instance()->RegisterDelegate(info->name, effectDelegate, effectInfo);
    return OH_EffectErrorCode::EFFECT_ERR_SUCCESS;
}

EFFECT_EXPORT
OH_FilterNames *OH_EFilter_LookupFilters(const char *key)
{
    EFFECT_LOGI("efilter Lookup filters.");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, nullptr, "input parameter key is null!");

    std::string lookupKey = key;
    std::vector<const char *> matchEFilter;
    NativeCommonUtils::ParseLookupKey(lookupKey, matchEFilter);

    std::shared_ptr<OH_FilterNames> filterNames = std::make_shared<OH_FilterNames>();
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
void OH_EFilter_ReleaseFilterNames()
{
    for (const auto &filterNames : sOHFilterNames) {
        if (filterNames == nullptr) {
            continue;
        }
        free(filterNames->nameList);
    }
    sOHFilterNames.clear();
}

EFFECT_EXPORT
OH_EffectErrorCode OH_EFilter_LookupFilterInfo(const char *name, OH_EffectInfo *info)
{
    EFFECT_LOGI("efilter Lookup filter.");
    CHECK_AND_RETURN_RET_LOG(name != nullptr, OH_EffectErrorCode::EFFECT_ERR_INPUT_NULL,
        "input parameter name is null!");
    CHECK_AND_RETURN_RET_LOG(info != nullptr, OH_EffectErrorCode::EFFECT_ERR_INPUT_NULL,
        "input parameter info is null!");

    std::shared_ptr<IFilterDelegate> filterDelegate = EFilterFactory::Instance()->GetDelegate(name);
    if (filterDelegate != nullptr) {
        auto effectInfo = static_cast<OH_EffectInfo *>(filterDelegate->GetEffectInfo());
        CHECK_AND_RETURN_RET_LOG(effectInfo != nullptr, OH_EffectErrorCode::EFFECT_ERR_LOOKUP_EFFECT_INFO_FAIL,
            "filter delegate get effect info is null! name=%{public}s", name);
        *info = *effectInfo;
        return OH_EffectErrorCode::EFFECT_ERR_SUCCESS;
    }

    std::shared_ptr<EffectInfo> effectInfo = EFilterFactory::Instance()->GetEffectInfo(name);
    CHECK_AND_RETURN_RET_LOG(effectInfo != nullptr, OH_EffectErrorCode::EFFECT_ERR_LOOKUP_EFFECT_INFO_FAIL,
        "LookupFilter info fail! name=%{public}s", name);

    info->name = const_cast<char *>(name);
    NativeCommonUtils::SwitchToOHEffectInfo(effectInfo.get(), info);
    return OH_EffectErrorCode::EFFECT_ERR_SUCCESS;
}

EFFECT_EXPORT
OH_EffectErrorCode OH_EFilter_Render(OH_EFilter *filter, NativePixelMap *inputPixel, NativePixelMap *outputPixel)
{
    EFFECT_LOGI("efilter render.");
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, OH_EffectErrorCode::EFFECT_ERR_INPUT_NULL,
        "input parameter filter is null!");
    CHECK_AND_RETURN_RET_LOG(inputPixel != nullptr, OH_EffectErrorCode::EFFECT_ERR_INPUT_NULL,
        "input parameter inputPixel is null!");
    CHECK_AND_RETURN_RET_LOG(outputPixel != nullptr, OH_EffectErrorCode::EFFECT_ERR_INPUT_NULL,
        "input parameter outputPixel is null!");

    PixelMap *input = NativeCommonUtils::GetPixelMapFromNativePixelMap(inputPixel);
    PixelMap *output = NativeCommonUtils::GetPixelMapFromNativePixelMap(outputPixel);

    std::shared_ptr<EffectBuffer> inEffectBuffer = nullptr;
    ErrorCode result = CommonUtils::LockPixelMap(input, inEffectBuffer);
    if (result != ErrorCode::SUCCESS || inEffectBuffer == nullptr) {
        EFFECT_LOGE("lock input native pixelMap error! errorCode:%{public}d", result);
        CommonUtils::UnlockPixelMap(input);
        return OH_EffectErrorCode::EFFECT_ERR_EFILTER_RENDER_FAIL;
    }

    std::shared_ptr<EffectBuffer> outEffectBuffer = nullptr;
    result = CommonUtils::LockPixelMap(output, outEffectBuffer);
    if (result != ErrorCode::SUCCESS || outEffectBuffer == nullptr) {
        EFFECT_LOGE("lock output native pixelMap error! errorCode:%{public}d", result);
        CommonUtils::UnlockPixelMap(output);
        return OH_EffectErrorCode::EFFECT_ERR_EFILTER_RENDER_FAIL;
    }

    filter->filter_->PreRender(inEffectBuffer->bufferInfo_->formatType_);
    result = filter->filter_->Render(inEffectBuffer, outEffectBuffer);
    CommonUtils::UnlockPixelMap(input);
    CommonUtils::UnlockPixelMap(output);
    CHECK_AND_RETURN_RET_LOG(result == ErrorCode::SUCCESS, OH_EffectErrorCode::EFFECT_ERR_EFILTER_RENDER_FAIL,
        "efilter render fail! errorCode:%{public}d", result);

    return OH_EffectErrorCode::EFFECT_ERR_SUCCESS;
}

EFFECT_EXPORT
OH_EffectErrorCode OH_EFilter_Release(OH_EFilter *filter)
{
    EFFECT_LOGI("efilter release.");
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, OH_EffectErrorCode::EFFECT_ERR_INPUT_NULL,
        "input parameter imageEffect is null!");
    delete (filter);
    return OH_EffectErrorCode::EFFECT_ERR_SUCCESS;
}

#ifdef __cplusplus
}
#endif