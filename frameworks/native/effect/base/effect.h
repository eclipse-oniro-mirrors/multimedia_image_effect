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

#ifndef IMAGE_EFFECT_EFFECT_H
#define IMAGE_EFFECT_EFFECT_H

#include "efilter.h"

namespace OHOS {
namespace Media {
namespace Effect {
class Effect {
public:
    Effect() = default;

    virtual ~Effect() = default;

    virtual void AddEFilter(const std::shared_ptr<EFilter> &efilter);

    virtual ErrorCode InsertEFilter(const std::shared_ptr<EFilter> &efilter, uint32_t index);

    virtual void RemoveEFilter(const std::shared_ptr<EFilter> &efilter);

    virtual ErrorCode Start() = 0;

    virtual ErrorCode Save(nlohmann::json &res) = 0;

    std::vector<std::shared_ptr<EFilter>> &GetEFilters()
    {
        return efilters_;
    }
protected:
    std::vector<std::shared_ptr<EFilter>> efilters_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_EFFECT_H
