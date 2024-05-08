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

#ifndef IE_PIPELINE_CORE_PIPELINE_CORE_H
#define IE_PIPELINE_CORE_PIPELINE_CORE_H

#include "pipeline.h"

namespace OHOS {
namespace Media {
namespace Effect {
class PipelineCore : public Pipeline {
public:
    ~PipelineCore() override = default;

    void Init(EventReceiver *receiver) override;

    ErrorCode Prepare() override;

    ErrorCode Start() override;

    void OnEvent(const Event &event) override;

    ErrorCode AddFilter(Filter *filterIn) override;

    ErrorCode AddFilters(std::vector<Filter *> filtersIn) override;

    ErrorCode RemoveFilter(Filter *filter) override;

    ErrorCode RemoveFilterChain(Filter *firstFilter) override;

    ErrorCode LinkFilters(std::vector<Filter *> filters) override;

    ErrorCode LinkPorts(std::shared_ptr<OutPort> outPort, std::shared_ptr<InPort> inPort) override;

private:
    void InitFilters(const std::vector<Filter *> &filters);

    FilterState state_{ FilterState::CREATED };

    std::vector<Filter *> filters_{};

    std::vector<Filter *> filtersToRemove_{};

    EventReceiver *eventReceiver_{ nullptr };
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IE_PIPELINE_CORE_PIPELINE_CORE_H
