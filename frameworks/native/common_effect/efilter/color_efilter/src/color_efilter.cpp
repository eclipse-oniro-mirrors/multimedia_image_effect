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

#include "color_efilter.h"

#include <string>

#include "effect_log.h"
#include "IMRenderContext.h"

namespace OHOS {
namespace Media {
namespace Effect {
const std::string ColorEFilter::Parameter::KEY_INTENSITY = "FILTER_INTENSITY";

IMRenderContext* filterContext_ = nullptr;

ColorEFilter::~ColorEFilter()
{
    if (filterContext_ != nullptr) {
        filterContext_->ReleaseCurrent();
        filterContext_->Release();
    }

    for (const auto &filterOperator_ : filterOperators_) {
        filterOperator_->Release();
    }
}

ErrorCode ColorEFilter::Render(EffectBuffer *buffer, std::shared_ptr<EffectContext> &context)
{
    ErrorCode res = Render(buffer, buffer, context);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "filter(%{public}s) render fail", name_.c_str());
    return PushData(buffer, context);
}

ErrorCode ColorEFilter::Render(EffectBuffer *src, EffectBuffer *dst, std::shared_ptr<EffectContext> &context)
{
    IPType ipType = context->ipType_;

    std::shared_ptr<FilterOperator> filterOperator = nullptr;
    auto it = std::find_if(filterOperators_.begin(), filterOperators_.end(),
        [&ipType](const auto &filterOperator) { return filterOperator->GetIpType() == ipType; });
    if (it != filterOperators_.end()) {
        filterOperator = *it;
    }

    if (filterOperator == nullptr) {
        EFFECT_LOGE("ipType not support for effect! name=%{public}s, ipType=%{public}d", name_.c_str(), ipType);
        return ErrorCode::ERR_UNSUPPORTED_IPTYPE_FOR_EFFECT;
    }

    IEffectFormat formatType = src->bufferInfo_->formatType_;
    switch (formatType) {
        case IEffectFormat::RGBA8888:
            return filterOperator->OnApplyRGBA8888(src, dst, values_);
        case IEffectFormat::YUVNV21:
            return filterOperator->OnApplyYUVNV21(src, dst, values_);
        case IEffectFormat::YUVNV12:
            return filterOperator->OnApplyYUVNV12(src, dst, values_);
        default:
            EFFECT_LOGE("format type not support! name=%{public}s, ipType=%{public}d, formatType=%{public}d",
                name_.c_str(), ipType, formatType);
            return ErrorCode::ERR_UNSUPPORTED_FORMAT_TYPE;
    }
}

std::shared_ptr<EffectInfo> ColorEFilter::GetEffectInfo(const std::string &name)
{
    std::shared_ptr<EffectInfo> info = std::make_unique<EffectInfo>();
    FilterOperatorFactory::Instance()->GetEffectInfo(name, info);
    info->category_ = Category::COLOR_ADJUST;
    return info;
}

ErrorCode ColorEFilter::PreRender(IEffectFormat &format)
{
    IPType ipType = IPType::DEFAULT;
    ErrorCode res = CalculateEFilterIPType(format, ipType);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res,
        "PreRender: CalculateEFilterIPType fail! name=%{public}s", name_.c_str());
    if (ipType == IPType::GPU) {
        filterContext_ = new IMRenderContext();
        filterContext_->Init();
        filterContext_->MakeCurrent(nullptr);
    }
    return ErrorCode::SUCCESS;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS