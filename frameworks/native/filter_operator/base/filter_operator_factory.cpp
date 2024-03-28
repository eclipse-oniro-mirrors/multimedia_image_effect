/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "filter_operator_factory.h"

#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
FilterOperatorFactory *FilterOperatorFactory::Instance()
{
    static FilterOperatorFactory instance;
    return &instance;
}

void FilterOperatorFactory::RegisterFunction(const std::string &name, const std::string &filterOperatorName,
    const FilterOperatorFunction &function)
{
    EFFECT_LOGI("register filter operator. efilterName=%{public}s, filterOperatorName=%{public}s", name.c_str(),
        filterOperatorName.c_str());
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        std::vector<FilterOperatorFunction> functions;
        functions.emplace_back(function);
        auto result = functions_.emplace(name, functions);
        if (!result.second) {
            result.first->second.emplace_back(function);
        }
    } else {
        it->second.emplace_back(function);
    }
}

std::vector<std::shared_ptr<FilterOperator>> FilterOperatorFactory::Create(const std::string &name)
{
    std::vector<std::shared_ptr<FilterOperator>> result;

    auto it = functions_.find(name);
    if (it == functions_.end()) {
        EFFECT_LOGE("create filter fail! functions has no %{public}s", name.c_str());
        return result;
    }

    std::transform(it->second.begin(), it->second.end(), std::back_inserter(result),
        [](const auto &function) { return function.generator(); });
    return result;
}

void FilterOperatorFactory::GetEffectInfo(const std::string &name, const std::shared_ptr<EffectInfo> &info)
{
    auto functionIter = functions_.find(name);
    if (functionIter == functions_.end()) {
        EFFECT_LOGE("get effect info fail! functions has no %{public}s", name.c_str());
        return;
    }

    std::map<IPType, std::vector<IEffectFormat>> formatTypes;
    for (const auto &function : functionIter->second) {
        formatTypes.emplace(function.ipTypeGetter(), function.formatTypeGetter());
    }

    for (const auto &formatType : formatTypes) {
        const IPType &ipType = formatType.first;
        const std::vector<IEffectFormat> &effectFormats = formatType.second;
        for (const auto &effectFormat : effectFormats) {
            auto it = info->formats_.find(effectFormat);
            if (it == info->formats_.end()) {
                std::vector<IPType> ipTypes;
                ipTypes.emplace_back(ipType);
                info->formats_.emplace(effectFormat, ipTypes);
            } else {
                std::vector<IPType> &ipTypes = it->second;
                ipTypes.emplace_back(ipType);
            }
        }
    }
}
} // namespace Effect
} // namespace Media
} // namespace OHOS