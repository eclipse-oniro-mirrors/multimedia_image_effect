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

#ifndef IE_PIPELINE_CORE_PIPELINE_H
#define IE_PIPELINE_CORE_PIPELINE_H

#include "filter.h"

namespace OHOS {
namespace Media {
namespace Effect {
class Pipeline : public EventReceiver {
public:
    ~Pipeline() override = default;

    virtual void Init(EventReceiver *receiver) = 0;

    virtual ErrorCode Prepare() = 0;

    virtual ErrorCode Start() = 0;

    virtual ErrorCode AddFilter(Filter *filterIn) = 0;

    /**
     * Add filter to Pipeline.
     * Note: Only added filters can be initialed.
     *
     * @param filters filters to add
     * @return ErrorCode
     */
    virtual ErrorCode AddFilters(std::vector<Filter *> filtersIn) = 0;

    virtual ErrorCode RemoveFilter(Filter *filter) = 0;

    virtual ErrorCode RemoveFilterChain(Filter *firstFilter) = 0;

    /**
     * Link filters.
     * Link the default out-port of pre-filter with the default in-port of next-filter.
     *
     * @param filters filters to link
     * @return ErrorCode
     */
    virtual ErrorCode LinkFilters(std::vector<Filter *> filters) = 0;

    /**
     * Link ports.
     * Note: The owner filters of ports must have been added to the Pipeline, otherwise it will return error.
     *
     * @param port the out-port of the pre-filter
     * @param nextPort the in-port of the next-filter
     * @return ErrorCode
     */
    virtual ErrorCode LinkPorts(std::shared_ptr<OutPort> port, std::shared_ptr<InPort> nextPort) = 0;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IE_PIPELINE_CORE_PIPELINE_H