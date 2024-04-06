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

#ifndef IE_PIPELINE_CORE_FILTER_H
#define IE_PIPELINE_CORE_FILTER_H

#include "any.h"
#include "port.h"
#include "transfer.h"

namespace OHOS {
namespace Media {
namespace Effect {
class Filter;
using PairPort = std::pair<std::string, std::string>;
using PPort = std::shared_ptr<Port>;
using POutPort = std::shared_ptr<OutPort>;
using PInPort = std::shared_ptr<InPort>;
using PFilter = std::shared_ptr<Filter>;

enum class FilterState {
    CREATED,     // Filter created
    INITIALIZED, // Init called
    PREPARING,   // Prepare called
    RUNNING,     // Start called
    READY,       // Ready Event reported
};

class Filter : public InfoTransfer {
public:
    ~Filter() override = default;

    virtual void Initialize(EventReceiver *receiver) = 0;

    virtual PInPort GetInPort(const std::string &name) = 0;

    virtual POutPort GetOutPort(const std::string &name) = 0;

    virtual ErrorCode Prepare() = 0;

    virtual ErrorCode Start() = 0;

    virtual ErrorCode SetParameter(int32_t key, const Plugin::Any &value) = 0;

    virtual ErrorCode GetParameter(int32_t key, Plugin::Any &value) = 0;

    virtual void UnlinkPrevFilters() = 0;

    virtual std::vector<Filter *> GetNextFilters() = 0;

    virtual std::vector<Filter *> GetPreFilters() = 0;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IE_PIPELINE_CORE_FILTER_H
