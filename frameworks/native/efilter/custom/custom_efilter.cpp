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

#include "custom_efilter.h"

#include "efilter_factory.h"
#include "json_helper.h"

namespace OHOS {
namespace Media {
namespace Effect {
std::map<std::string, std::shared_ptr<EffectInfo>> CustomEFilter::effectInfos_;

ErrorCode CustomEFilter::Render(EffectBuffer *src, EffectBuffer *dst, std::shared_ptr<EffectContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(delegate_ != nullptr, ErrorCode::ERR_INPUT_NULL, "delegate_ is null");
    if (!delegate_->Render(handler_, src, dst, context)) {
        EFFECT_LOGE("custom efilter render with src and dst fail! name=%{public}s", name_.c_str());
        return ErrorCode::ERR_CUSTOM_EFILTER_APPLY_FAIL;
    }
    return ErrorCode::SUCCESS;
}

ErrorCode CustomEFilter::Render(EffectBuffer *buffer, std::shared_ptr<EffectContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(delegate_ != nullptr, ErrorCode::ERR_INPUT_NULL, "delegate_ is null");
    if (!delegate_->Render(handler_, buffer, context)) {
        EFFECT_LOGE("custom efilter render fail! name=%{public}s", name_.c_str());
        return ErrorCode::ERR_CUSTOM_EFILTER_APPLY_FAIL;
    }
    return ErrorCode::SUCCESS;
}

ErrorCode CustomEFilter::SetValue(const std::string &key, Plugin::Any &value)
{
    CHECK_AND_RETURN_RET_LOG(delegate_ != nullptr, ErrorCode::ERR_INPUT_NULL, "delegate_ is null");
    if (!delegate_->SetValue(handler_, key, value)) {
        EFFECT_LOGE("custom efilter setValue fail! name=%{public}s, key=%{public}s", name_.c_str(), key.c_str());
        return ErrorCode::ERR_CUSTOM_EFILTER_SETVALUE_FAIL;
    }
    return EFilter::SetValue(key, value);
}

ErrorCode CustomEFilter::Save(nlohmann::json &res)
{
    CHECK_AND_RETURN_RET_LOG(delegate_ != nullptr, ErrorCode::ERR_INPUT_NULL, "delegate_ is null");
    if (!delegate_->Save(handler_, res)) {
        EFFECT_LOGE("custom efilter save fail! name=%{public}s", name_.c_str());
        return ErrorCode::ERR_CUSTOM_EFILTER_SAVE_FAIL;
    }
    return ErrorCode::SUCCESS;
}

ErrorCode CustomEFilter::Restore(const nlohmann::json &values)
{
    CHECK_AND_RETURN_RET_LOG(delegate_ != nullptr, ErrorCode::ERR_INPUT_NULL, "delegate_ is null");
    void *result = delegate_->Restore(values);
    auto *filter = static_cast<EFilter *>(result);
    if (filter == nullptr) {
        EFFECT_LOGE("custom efilter restore fail! name=%{public}s", name_.c_str());
        return ErrorCode::ERR_CUSTOM_EFILTER_RESTORE_FAIL;
    }

    std::map<std::string, Plugin::Any> filterValues = filter->GetValues();
    for (auto &filterValue : filterValues) {
        SetValue(filterValue.first, filterValue.second);
    }

    return ErrorCode::SUCCESS;
}

void CustomEFilter::SetEffectInfo(const std::string &name, std::shared_ptr<EffectInfo> &effectInfo)
{
    effectInfos_[name] = effectInfo;
}

std::shared_ptr<EffectInfo> CustomEFilter::GetEffectInfo(const std::string &name)
{
    auto it = effectInfos_.find(name);
    if (it == effectInfos_.end()) {
        EFFECT_LOGE("effect info not find! name=%{public}s", name.c_str());
        return nullptr;
    }
    return it->second;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS