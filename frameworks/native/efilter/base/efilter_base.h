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

#ifndef IE_PIPELINE_FILTERS_EFILTER_BASE_H
#define IE_PIPELINE_FILTERS_EFILTER_BASE_H

#include "filter_base.h"

namespace OHOS {
namespace Media {
namespace Effect {
class EFilterBase : public FilterBase {
public:
    explicit EFilterBase(const std::string &name) : FilterBase(name)
    {
        filterType_ = FilterType::IMAGE_EFFECT;
    }

    ~EFilterBase() override = default;

    std::vector<WorkMode> GetWorkModes() override;

    ErrorCode PullData(const std::string &outPort, std::shared_ptr<EffectBuffer> &data) override;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IE_PIPELINE_FILTERS_EFILTER_BASE_H