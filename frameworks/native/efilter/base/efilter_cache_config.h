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

#ifndef IM_EFILTER_CACHE_CONFIG_H
#define IM_EFILTER_CACHE_CONFIG_H

namespace OHOS {
namespace Media {
namespace Effect {
enum class CacheStatus {
    NO_CACHE = 0,
    CACHE_START,
    CACHE_ENABLED,
    CACHE_USED,
};
class EFilterCacheConfig {
public:
    EFilterCacheConfig() = default;

    ~EFilterCacheConfig() = default;

    void SetStatus(CacheStatus status)
    {
        status_ = status;
    }

    CacheStatus GetStatus()
    {
        return status_;
    }

    void SetIPType(IPType type)
    {
        type_ = type;
    }

    IPType GetIPType()
    {
        return type_;
    }

private:
    CacheStatus status_ = CacheStatus::NO_CACHE;
    IPType type_ = IPType::DEFAULT;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IM_EFILTER_CACHE_CONFIG_H
