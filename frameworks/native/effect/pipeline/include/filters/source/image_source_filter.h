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

#ifndef IE_PIPELINE_FILTERS_IMAGE_SOURCE_FILTER_H
#define IE_PIPELINE_FILTERS_IMAGE_SOURCE_FILTER_H

#include <string>

#include "filter_base.h"

namespace OHOS {
namespace Media {
namespace Effect {
class ImageSourceFilter : public FilterBase {
public:
    explicit ImageSourceFilter(const std::string &name) : FilterBase(name)
    {
        filterType_ = FilterType::INPUT_SOURCE;
    }

    ~ImageSourceFilter() override = default;

    virtual ErrorCode SetSource(const std::shared_ptr<EffectBuffer> &source, std::shared_ptr<EffectContext> &context);

    ErrorCode SetParameter(int32_t key, const Media::Plugin::Any &value) override
    {
        return FilterBase::SetParameter(key, value);
    }

    ErrorCode GetParameter(int32_t key, Media::Plugin::Any &value) override
    {
        return FilterBase::GetParameter(key, value);
    }

    ErrorCode Prepare() override;

    ErrorCode Start() override;

private:
    void OnEvent(const Event &event) override {}
    ErrorCode DoNegotiate();

    std::shared_ptr<EffectBuffer> srcBuffer_;

    std::shared_ptr<EffectContext> context_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif