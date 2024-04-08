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

#ifndef IMAGE_EFFECT_EFILTER_FACTORY_H
#define IMAGE_EFFECT_EFILTER_FACTORY_H

#include <functional>
#include <memory>

#include "delegate.h"
#include "efilter.h"

#define REGISTER_EFILTER_FACTORY(T, U) static AutoRegisterEFilter<T> gAutoRegster_##T(U)

namespace OHOS {
namespace Media {
namespace Effect {
using EFilterGenerator = std::function<std::shared_ptr<EFilter>(const std::string &name)>;
using EFilterInfoGetter = std::function<std::shared_ptr<EffectInfo>(const std::string &name)>;

struct EFilterFunction {
    EFilterGenerator generator_;
    EFilterInfoGetter infoGetter_;
};

class EFilterFactory {
public:
    ~EFilterFactory() = default;

    EFilterFactory(const EFilterFactory &) = delete;

    EFilterFactory operator = (const EFilterFactory &) = delete;

    static EFilterFactory *Instance();

    void RegisterFunction(const std::string &name, const EFilterFunction &function);

    void RegisterDelegate(const std::string &name, const std::shared_ptr<IFilterDelegate> &delegate,
        std::shared_ptr<EffectInfo> &effectInfo);

    std::shared_ptr<IFilterDelegate> GetDelegate(const std::string &name);

    std::shared_ptr<EFilter> Create(const std::string &name, void *handler);

    inline std::shared_ptr<EFilter> Create(const std::string &name)
    {
        return Create(name, nullptr);
    }

    std::shared_ptr<EFilter> Restore(const std::string &name, const nlohmann::json &root, void *handler);

    template <class T> void RegisterEFilter(const std::string &name)
    {
        EFilterFunction function = {
            .generator_ = [](const std::string &name) { return std::make_shared<T>(name); },
            .infoGetter_ = [](const std::string &name) { return T::GetEffectInfo(name); }
        };

        RegisterFunction(name, function);
    }

    std::shared_ptr<EffectInfo> GetEffectInfo(const std::string &name);

    void GetAllEffectNames(std::vector<const char *> &names);
private:
    EFilterFactory() = default;

    std::map<std::string, EFilterFunction> functions_;

    std::map<std::string, std::shared_ptr<IFilterDelegate>> delegates_;
};

template <typename T> class AutoRegisterEFilter {
public:
    explicit AutoRegisterEFilter(const std::string &name)
    {
        EFilterFactory::Instance()->RegisterEFilter<T>(name);
    }

    ~AutoRegisterEFilter() = default;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_EFILTER_FACTORY_H
