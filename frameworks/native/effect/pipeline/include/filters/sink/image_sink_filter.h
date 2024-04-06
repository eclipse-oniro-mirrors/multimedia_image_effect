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

#ifndef IE_PIPELINE_FILTERS_IMAGE_SINK_FILTER_H
#define IE_PIPELINE_FILTERS_IMAGE_SINK_FILTER_H

#include "filter_base.h"

namespace OHOS {
namespace Media {
namespace Effect {
class ImageSinkFilter : public FilterBase {
public:
    explicit ImageSinkFilter(const std::string &name) : FilterBase(name)
    {
        filterType_ = FilterType::OUTPUT_SINK;
    }

    ~ImageSinkFilter() override = default;

    virtual ErrorCode SetSink(const std::shared_ptr<EffectBuffer> &sink);

    ErrorCode SetParameter(int32_t key, const Media::Plugin::Any &value) override
    {
        return FilterBase::SetParameter(key, value);
    }

    ErrorCode GetParameter(int32_t key, Media::Plugin::Any &value) override
    {
        return FilterBase::GetParameter(key, value);
    }

    ErrorCode Start() override;

    void Negotiate(const std::string& inPort, const std::shared_ptr<Capability> &capability,
        std::shared_ptr<EffectContext> &context) override;

    ErrorCode PushData(const std::string &inPort, const std::shared_ptr<EffectBuffer> &buffer,
        std::shared_ptr<EffectContext> &context) override;

private:
    void OnEvent(const Event &event) override {}
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif