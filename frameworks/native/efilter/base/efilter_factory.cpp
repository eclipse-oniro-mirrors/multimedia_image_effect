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

#include "efilter_factory.h"

#include "custom_efilter.h"
#include "effect_log.h"
#include "external_loader.h"
#include "json_helper.h"

namespace OHOS {
namespace Media {
namespace Effect {
EFilterFactory *EFilterFactory::Instance()
{
    static EFilterFactory instance;
    return &instance;
}

void EFilterFactory::RegisterFunction(const std::string &name, const EFilterFunction &function)
{
    EFFECT_LOGI("register efilter. name=%{public}s", name.c_str());

    auto it = functions_.find(name);
    if (it == functions_.end()) {
        auto result = functions_.emplace(name, function);
        if (!result.second) {
            result.first->second = function;
        }
    } else {
        functions_[name] = function;
    }
}

void EFilterFactory::RegisterDelegate(const std::string &name, const std::shared_ptr<IFilterDelegate> &delegate,
    std::shared_ptr<EffectInfo> &effectInfo)
{
    EFFECT_LOGI("register delegate. name=%{public}s", name.c_str());
    ExternLoader::Instance()->InitExt();
    RegisterEFilter<CustomEFilter>(name);

    CustomEFilter::SetEffectInfo(name, effectInfo);

    auto it = delegates_.find(name);
    if (it == delegates_.end()) {
        auto result = delegates_.emplace(name, delegate);
        if (!result.second) {
            result.first->second = delegate;
        }
    } else {
        delegates_[name] = delegate;
    }
}

std::shared_ptr<IFilterDelegate> EFilterFactory::GetDelegate(const std::string &name)
{
    auto it = delegates_.find(name);
    if (it != delegates_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<EFilter> EFilterFactory::Restore(const std::string &name, const nlohmann::json &root, void *handler)
{
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(name, handler);
    CHECK_AND_RETURN_RET_LOG(JsonHelper::CheckElementExitstence(root, "values") == ErrorCode::SUCCESS, efilter,
        "[values] not exist!");
    const nlohmann::json values = *(root.find("values"));
    CHECK_AND_RETURN_RET_LOG(efilter->Restore(values) == ErrorCode::SUCCESS, efilter, "values restore fail!");
    return efilter;
}

std::shared_ptr<EFilter> EFilterFactory::Create(const std::string &name, void *handler)
{
    ExternLoader::Instance()->InitExt();
    auto it = functions_.find(name);
    if (it != functions_.end()) {
        std::shared_ptr<EFilter> efilter = it->second.generator_(name);
        if (GetDelegate(name)) {
            static_cast<CustomEFilter *>(efilter.get())->SetHandler(handler);
        }
        return efilter;
    }
    EFFECT_LOGE("create effect fail! functions has no %{public}s", name.c_str());
    return nullptr;
}

std::shared_ptr<EffectInfo> EFilterFactory::GetEffectInfo(const std::string &name)
{
    ExternLoader::Instance()->InitExt();
    auto it = functions_.find(name);
    if (it != functions_.end()) {
        return it->second.infoGetter_(name);
    }
    EFFECT_LOGE("get effect info fail! functions has no %{public}s", name.c_str());
    return nullptr;
}

void EFilterFactory::GetAllEffectNames(std::vector<const char *> &names)
{
    std::transform(functions_.begin(), functions_.end(), std::back_inserter(names),
        [](const auto &pair) { return pair.first.c_str(); });
}
} // namespace Effect
} // namespace Media
} // namespace OHOS