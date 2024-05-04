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

#include "pipeline_core.h"

#include <algorithm>
#include <queue>

#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
void PipelineCore::Init(EventReceiver *receiver)
{
    eventReceiver_ = receiver;
    state_ = FilterState::INITIALIZED;
}

ErrorCode PipelineCore::Prepare()
{
    state_ = FilterState::PREPARING;

    // Set source buffer to source filter
    auto sourceFilter = filters_.at(0);
    FALSE_RETURN_MSG_W(sourceFilter, ErrorCode::ERR_PIPELINE_INVALID_FILTER, "Prepare, sourceFilter is not found");
    EFFECT_LOGI("Prepare sourceFilter(%{public}s)", sourceFilter->GetName().c_str());
    ErrorCode res = sourceFilter->Prepare();
    FALSE_RETURN_MSG_W(res == ErrorCode::SUCCESS, res, "Prepare, sourceFilter prepare fail! res=%{public}d", res);
    return ErrorCode::SUCCESS;
}

ErrorCode PipelineCore::Start()
{
    state_ = FilterState::RUNNING;
    auto sourceEffect = filters_[0];
    if (sourceEffect) {
        return sourceEffect->Start();
    }
    EFFECT_LOGE("Start, sourceEffect is null");
    return ErrorCode::ERR_PIPELINE_INVALID_FILTER;
}

ErrorCode PipelineCore::AddFilter(Filter *filterIn)
{
    if (filterIn == nullptr) {
        EFFECT_LOGW("add filter is null");
        return ErrorCode::ERR_PIPELINE_INVALID_FILTER;
    }

    this->filters_.push_back(filterIn);
    return ErrorCode::SUCCESS;
}

ErrorCode PipelineCore::AddFilters(std::vector<Filter *> filtersIn)
{
    EFFECT_LOGI("AddFilters");
    if (filtersIn.empty()) {
        EFFECT_LOGI("add filters is empty");
        return ErrorCode::ERR_INVALID_PARAMETER_VALUE;
    }

    this->filters_.insert(this->filters_.end(), filtersIn.begin(), filtersIn.end());
    InitFilters(filtersIn);
    return ErrorCode::SUCCESS;
}

ErrorCode PipelineCore::RemoveFilter(Filter *filter)
{
    auto it = std::find_if(filters_.begin(), filters_.end(),
        [&filter](const Filter *filterPtr) { return filterPtr == filter; });
    if (it != filters_.end()) {
        EFFECT_LOGI("RemoveFilter %{public}s", (*it)->GetName().c_str());
        filters_.erase(it);
    }
    return ErrorCode::SUCCESS;
}

ErrorCode PipelineCore::RemoveFilterChain(Filter *firstFilter)
{
    if (!firstFilter) {
        return ErrorCode::ERR_PIPELINE_INVALID_FILTER;
    }
    std::queue<Filter *> filters;
    filters.push(firstFilter);
    while (!filters.empty()) {
        auto filter = filters.front();
        filters.pop();
        filter->UnlinkPrevFilters();
        filtersToRemove_.push_back(filter);
        for (auto &&nextFilter : filter->GetNextFilters()) {
            filters.push(nextFilter);
        }
    }
    return ErrorCode::SUCCESS;
}

void PipelineCore::InitFilters(const std::vector<Filter *> &filters)
{
    EFFECT_LOGI("InitFilters");
    for (auto &filter : filters) {
        filter->Initialize(this);
    }
}

ErrorCode PipelineCore::LinkFilters(std::vector<Filter *> filters)
{
    EFFECT_LOGI("LinkFilters");
    FALSE_RETURN_MSG_E(!filters.empty(), ErrorCode::ERR_INVALID_PARAMETER_VALUE, "link filters is empty");

    int count = std::max((int)(filters.size()) - 1, 0);
    for (int i = 0; i < count; i++) {
        filters[i]->GetOutPort(PORT_NAME_DEFAULT)->Connect(filters[i + 1]->GetInPort(PORT_NAME_DEFAULT));
        filters[i + 1]->GetInPort(PORT_NAME_DEFAULT)->Connect(filters[i]->GetOutPort(PORT_NAME_DEFAULT));
    }
    return ErrorCode::SUCCESS;
}

ErrorCode PipelineCore::LinkPorts(std::shared_ptr<OutPort> outPort, std::shared_ptr<InPort> inPort)
{
    FAIL_RETURN(outPort->Connect(inPort));
    FAIL_RETURN(inPort->Connect(outPort));
    return ErrorCode::SUCCESS;
}

void PipelineCore::OnEvent(const Event &event)
{
    if (eventReceiver_) {
        eventReceiver_->OnEvent(event);
    } else {
        EFFECT_LOGI("no event receiver when receive type %{public}d", event.type);
    }
}
} // namespace Effect
} // namespace Media
} // namespace OHOS
