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

#include "effect.h"
#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
void Effect::AddEFilter(const std::shared_ptr<EFilter> &efilter)
{
    efilters_.emplace_back(efilter);
}

ErrorCode Effect::InsertEFilter(const std::shared_ptr<EFilter> &efilter, uint32_t index)
{
    if (index > static_cast<uint32_t>(efilters_.size())) {
        EFFECT_LOGE("index is invalid! index=%{public}d, efilterSize=%{public}zu", index, efilters_.size());
        return ErrorCode::ERR_INVALID_INDEX;
    }
    efilters_.emplace(efilters_.begin() + index, efilter);
    return ErrorCode::SUCCESS;
}

void Effect::RemoveEFilter(const std::shared_ptr<EFilter> &efilter)
{
    for (auto it = efilters_.begin(); it != efilters_.end();) {
        if (*it == efilter) {
            it = efilters_.erase(it);
        } else {
            ++it;
        }
    }
}
} // namespace Effect
} // namespace Media
} // namespace OHOS