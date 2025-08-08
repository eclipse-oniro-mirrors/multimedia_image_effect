/*
* Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "filter_delegate.h"

#include "common_utils.h"
#include "effect_log.h"
#include "efilter.h"
#include "native_effect_base.h"
#include "native_common_utils.h"
#include "memcpy_helper.h"
#include "format_helper.h"
#include "event_report.h"
#include "graphic/render_texture.h"
#include "render_environment.h"

namespace OHOS {
namespace Media {
namespace Effect {
constexpr char const *PARA_SRC_EFFECT_BUFFER = "PARA_SRC_EFFECT_BUFFER";
constexpr char const *PARA_RENDER_WITH_SRC_AND_DST = "PARA_RENDER_WITH_SRC_AND_DST";
constexpr char const *PARA_RENDER_INFO = "PARA_RENDER_INFO";

bool FilterDelegate::Render(void *efilter, EffectBuffer *src, EffectBuffer *dst,
    std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGD("FilterDelegate Render with src and dst.");
    OH_EffectFilter *ohEFilter = (OH_EffectFilter *)efilter;
    CHECK_AND_RETURN_RET_LOG(ohEFilter != nullptr && ohEFilter->filter_ != nullptr, false,
        "FilterDelegateRender: filter is null!");
    Any param = true;
    ohEFilter->SetParameter(PARA_RENDER_WITH_SRC_AND_DST, param);
    Any any = context;
    ohEFilter->SetParameter(PARA_RENDER_INFO, any);

    if (src->extraInfo_->dataType != DataType::TEX) {
        MemcpyHelper::CopyData(src, dst);
    } else {
        CommonUtils::CopyTexture(context, src->bufferInfo_->tex_, dst->bufferInfo_->tex_);
    }
    bool res = Render(efilter, dst, context);
    ohEFilter->RemoveParameter(PARA_RENDER_WITH_SRC_AND_DST);
    ohEFilter->RemoveParameter(PARA_RENDER_INFO);
    return res;
}

bool FilterDelegate::Render(void *efilter, EffectBuffer *src, std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGD("FilterDelegate Render.");
    std::unique_ptr<OH_EffectBufferInfo> srcBuffer = std::make_unique<OH_EffectBufferInfo>();
    srcBuffer->addr = src->buffer_;
    if (src->bufferInfo_->tex_ != nullptr) {
        srcBuffer->textureId = static_cast<int32_t>(src->bufferInfo_->tex_->GetName());
    }

    srcBuffer->width = static_cast<int32_t>(src->bufferInfo_->width_);
    srcBuffer->height = static_cast<int32_t>(src->bufferInfo_->height_);
    srcBuffer->rowSize = static_cast<int32_t>(src->bufferInfo_->rowStride_);
    NativeCommonUtils::SwitchToOHFormatType(src->bufferInfo_->formatType_, srcBuffer->format);
    srcBuffer->timestamp = src->extraInfo_->timestamp;

    OH_EffectFilter *ohEFilter = static_cast<OH_EffectFilter *>(efilter);
    CHECK_AND_RETURN_RET_LOG(ohEFilter != nullptr && ohEFilter->filter_ != nullptr, false,
        "FilterDelegateRender: filter is null!");

    Any any = src;
    ohEFilter->SetParameter(PARA_SRC_EFFECT_BUFFER, any);
    Any parameter = context;
    ohEFilter->SetParameter(PARA_RENDER_INFO, parameter);

    OH_EffectFilterDelegate_PushData pushData = [](OH_EffectFilter *filter, OH_EffectBufferInfo *dst) {
        FilterDelegate::PushData(filter, dst);
    };

    bool res = ohDelegate_->render((OH_EffectFilter *)efilter, srcBuffer.get(), pushData);
    ohEFilter->RemoveParameter(PARA_SRC_EFFECT_BUFFER);
    ohEFilter->RemoveParameter(PARA_RENDER_INFO);
    return res;
}

bool FilterDelegate::SetValue(void *efilter, const std::string &key, const Any &value)
{
    EFFECT_LOGD("FilterDelegate SetValue.");
    std::unique_ptr<ImageEffect_Any> ohValue = std::make_unique<ImageEffect_Any>();
    NativeCommonUtils::SwitchToOHAny(value, ohValue.get());
    return ohDelegate_->setValue((OH_EffectFilter *)efilter, key.c_str(), ohValue.get());
}

bool FilterDelegate::Save(void *efilter, EffectJsonPtr &res)
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
    res = EffectJsonHelper::ParseJsonData(content);
    return true;
}

void *FilterDelegate::Restore(const EffectJsonPtr &values)
{
    EFFECT_LOGI("FilterDelegate Restore.");
    std::string valueStr = values->ToString();
    return ohDelegate_->restore(valueStr.c_str());
}

void *FilterDelegate::GetEffectInfo()
{
    EFFECT_LOGI("FilterDelegate GetEffectInfo.");
    return &ohInfo_;
}

void FilterDelegate::PushData(OH_EffectFilter *filter, OH_EffectBufferInfo *dst)
{
    CHECK_AND_RETURN_LOG(dst != nullptr && filter != nullptr && filter->filter_ != nullptr,
        "FilterDelegatePushData: filter is null!");
    Any param;
    if (filter->GetParameter(PARA_RENDER_WITH_SRC_AND_DST, param) == ErrorCode::SUCCESS) {
        return;
    }

    Any value;
    ErrorCode res = filter->GetParameter(PARA_SRC_EFFECT_BUFFER, value);
    CHECK_AND_RETURN_LOG(res == ErrorCode::SUCCESS, "FilterDelegatePushData: get param fail! key=%{public}s",
        PARA_SRC_EFFECT_BUFFER);

    EffectBuffer *src = nullptr;
    res = CommonUtils::ParseAny(value, src);
    CHECK_AND_RETURN_LOG(res == ErrorCode::SUCCESS && src != nullptr,
        "FilterDelegatePushData: parse EffectBuffer ptr fail! res=%{public}d", res);

    CHECK_AND_RETURN_LOG(src->bufferInfo_ != nullptr && src->extraInfo_ != nullptr,
        "FilterDelegatePushData: bufferInfo of src is null or extraInfo of src is null!");

    CHECK_AND_RETURN_LOG(src->buffer_ != nullptr || src->bufferInfo_->tex_ != nullptr,
        "FilterDelegatePushData: buffer of src is null!");

    if (dst->addr == src->buffer_ && src->extraInfo_->dataType != DataType::TEX) {
        std::shared_ptr<EffectBuffer> effectBuffer = std::make_shared<EffectBuffer>(src->bufferInfo_,
            dst->addr, src->extraInfo_);
        Any any;
        res = filter->GetParameter(PARA_RENDER_INFO, any);
        CHECK_AND_RETURN_LOG(res == ErrorCode::SUCCESS, "FilterDelegatePushData: get param fail! key=%{public}s",
            PARA_RENDER_INFO);

        auto &context = AnyCast<std::shared_ptr<EffectContext> &>(any);
        filter->filter_->PushData(effectBuffer.get(), context);
        return;
    }

    std::shared_ptr<EffectBuffer> effectBuffer = GenDstEffectBuffer(dst, src);

    Any any;
    res = filter->GetParameter(PARA_RENDER_INFO, any);
    CHECK_AND_RETURN_LOG(res == ErrorCode::SUCCESS, "FilterDelegatePushData: get param fail! key=%{public}s",
        PARA_RENDER_INFO);

    auto &context = AnyCast<std::shared_ptr<EffectContext> &>(any);
    filter->filter_->PushData(effectBuffer.get(), context);
}

std::shared_ptr<EffectBuffer> FilterDelegate::GenDstEffectBuffer(const OH_EffectBufferInfo *dst,
    const EffectBuffer *src)
{
    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    bufferInfo->width_ = static_cast<uint32_t>(dst->width);
    bufferInfo->height_ = static_cast<uint32_t>(dst->height);
    bufferInfo->rowStride_ = static_cast<uint32_t>(dst->rowSize);
    NativeCommonUtils::SwitchToFormatType(dst->format, bufferInfo->formatType_);
    bufferInfo->len_ =
        FormatHelper::CalculateDataRowCount(bufferInfo->height_, bufferInfo->formatType_) * bufferInfo->rowStride_;
    bufferInfo->surfaceBuffer_ = nullptr;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_unique<ExtraInfo>();
    *extraInfo = *src->extraInfo_;
    extraInfo->bufferType = BufferType::DEFAULT;
    std::shared_ptr<EffectBuffer> effectBuffer = std::make_shared<EffectBuffer>(bufferInfo, dst->addr, extraInfo);
    if (src->extraInfo_->dataType == DataType::TEX) {
        effectBuffer->bufferInfo_->tex_ = src->bufferInfo_->tex_;
        extraInfo->bufferType = BufferType::DMA_BUFFER;
        extraInfo->dataType = DataType::TEX;
    }
    return effectBuffer;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS