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

#ifndef IM_EFILTER_CACHE_NEGOTIATE_H
#define IM_EFILTER_CACHE_NEGOTIATE_H

#include "efilter_cache_config.h"

#include <memory>

namespace OHOS {
namespace Media {
namespace Effect {
class EFilterCacheNegotiate {
public:
    EFilterCacheNegotiate() = default;

    ~EFilterCacheNegotiate() = default;

    void NegotiateConfig(std::shared_ptr<EFilterCacheConfig> config)
    {
        if (config == nullptr) {
            return;
        }
        if (config->GetStatus() == CacheStatus::NO_CACHE) {
            return;
        }
        needCache_ = true;
        if (config->GetStatus() == CacheStatus::CACHE_START) {
            return;
        }
        if (config_ == nullptr) {
            config_ = config;
            config_->SetStatus(CacheStatus::CACHE_USED);
            return;
        }
        if (config_->GetStatus() == CacheStatus::CACHE_ENABLED) {
            config_->SetStatus(CacheStatus::CACHE_ENABLED);
            config_ = config;
            config_->SetStatus(CacheStatus::CACHE_USED);
        }
    }

    void UseCache()
    {
        hasUsedCache_ = true;
    }

    bool HasUseCache()
    {
        return hasUsedCache_;
    }

    bool needCache()
    {
        return needCache_;
    }

    bool HasCached()
    {
        return config_ != nullptr;
    }

    void ClearConfig()
    {
        if (config_ != nullptr && config_->GetStatus() == CacheStatus::CACHE_USED) {
            config_->SetStatus(CacheStatus::CACHE_ENABLED);
        }
        hasUsedCache_ = false;
        needCache_ = false;
        config_ = nullptr;
    }

private:
    std::shared_ptr<EFilterCacheConfig> config_ = nullptr;
    bool hasUsedCache_ = false;
    bool needCache_ = false;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IM_EFILTER_CACHE_NEGOTIATE_H
