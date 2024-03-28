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

#ifndef IMAGE_EFFECT_FILTER_OPERATOR_FACTORY_H
#define IMAGE_EFFECT_FILTER_OPERATOR_FACTORY_H

#include <functional>
#include <memory>
#include <string>

#include "filter_operator.h"

#define REGISTER_FILTER_OPERATOR(T, U) static AutoRegisterFilterOperator<U> gAutoRegster_##U(#T, #U)

namespace OHOS {
namespace Media {
namespace Effect {
using Generator = std::function<std::shared_ptr<FilterOperator>()>;
using IPTypeGetter = std::function<IPType()>;
using FormatTypeGetter = std::function<std::vector<IEffectFormat>()>;

struct FilterOperatorFunction {
    Generator generator;
    IPTypeGetter ipTypeGetter;
    FormatTypeGetter formatTypeGetter;
};

class FilterOperatorFactory {
public:
    ~FilterOperatorFactory() = default;

    FilterOperatorFactory(const FilterOperatorFactory &) = delete;

    FilterOperatorFactory operator = (const FilterOperatorFactory &) = delete;

    static FilterOperatorFactory *Instance();

    void RegisterFunction(const std::string &name, const std::string &filterOperatorName,
        const FilterOperatorFunction &function);

    std::vector<std::shared_ptr<FilterOperator>> Create(const std::string &name);

    template <class U> void ResisterFilterOperator(const std::string &name, const std::string &filterOperatorName)
    {
        FilterOperatorFunction function = {
            .generator = []() { return std::make_shared<U>(); },
            .ipTypeGetter = []() { return U::GetIPType(); },
            .formatTypeGetter = []() { return U::GetFormatTypes(); },
        };

        RegisterFunction(name, filterOperatorName, function);
    }

    void GetEffectInfo(const std::string &name, const std::shared_ptr<EffectInfo> &info);

private:
    FilterOperatorFactory() = default;

    std::map<std::string, std::vector<FilterOperatorFunction>> functions_;
};

template <typename U> class AutoRegisterFilterOperator {
public:
    explicit AutoRegisterFilterOperator(const std::string &name, const std::string &filterOperatorName)
    {
        FilterOperatorFactory::Instance()->ResisterFilterOperator<U>(name, filterOperatorName);
    }

    ~AutoRegisterFilterOperator() = default;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_FILTER_OPERATOR_FACTORY_H