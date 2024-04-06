/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef IE_PIPELINE_FACTORY_FILTER_FACTORY_H
#define IE_PIPELINE_FACTORY_FILTER_FACTORY_H

#include <functional>

#include "filter.h"

#define REGISTER_FILTER_FACTORY(T) static AutoRegisterFilter<T> gAutoRegister_##T(#T)

namespace OHOS {
namespace Media {
namespace Effect {
using InstanceGenerator = std::function<std::shared_ptr<Filter>(const std::string &)>;

class FilterFactory {
public:
    ~FilterFactory() = default;

    FilterFactory(const FilterFactory &) = delete;

    FilterFactory operator = (const FilterFactory &) = delete;

    static FilterFactory &Instance();

    void Init();

    void RegisterGenerator(const std::string &name, const InstanceGenerator &generator);

    template <typename T> void RegisterFilter(const std::string &name)
    {
        RegisterGenerator(name, [](const std::string &name) { return std::make_shared<T>(name); });
    }

    std::shared_ptr<Filter> CreateFilter(const std::string &filterName, const std::string &aliasName);

    template <typename T> std::shared_ptr<T> CreateFilterWithType(const std::string &filterName)
    {
        return CreateFilterWithType<T>(filterName, filterName);
    }

    template <typename T>
    std::shared_ptr<T> CreateFilterWithType(const std::string &filterName, const std::string &aliasName)
    {
        auto filter = CreateFilter(filterName, aliasName);
        auto typedFilter = Plugin::ReinterpretPointerCast<T>(filter);
        return typedFilter;
    }

private:
    FilterFactory() = default;

    std::unordered_map<std::string, InstanceGenerator> generators_;
};

template <typename T> class AutoRegisterFilter {
public:
    explicit AutoRegisterFilter(const std::string &name)
    {
        FilterFactory::Instance().RegisterFilter<T>(name);
    }

    AutoRegisterFilter(const std::string &name, const InstanceGenerator &generator)
    {
        FilterFactory::Instance().RegisterGenerator(name, generator);
    }

    ~AutoRegisterFilter() = default;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IE_PIPELINE_FACTORY_FILTER_FACTORY_H
