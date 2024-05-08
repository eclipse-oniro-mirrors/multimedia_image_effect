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

#ifndef IE_PIPELINE_CORE_FILTER_BASE_H
#define IE_PIPELINE_CORE_FILTER_BASE_H

#include <atomic>

#include "filter.h"
#include "filter_type.h"

#define GET_FILTER_NAME(T) #T

namespace OHOS {
namespace Media {
namespace Effect {
class FilterBase : public Filter {
public:
    explicit FilterBase(std::string name);

    ~FilterBase() override;

    void Initialize(EventReceiver *receiver) override;

    PInPort GetInPort(const std::string &name) override;

    POutPort GetOutPort(const std::string &name) override;

    const std::string &GetName() override
    {
        return name_;
    }

    std::vector<WorkMode> GetWorkModes() override
    {
        return { WorkMode::PUSH };
    }

    void Negotiate(const std::string& inPort, const std::shared_ptr<Capability> &capability,
        std::shared_ptr<EffectContext> &context) override;

    ErrorCode PushData(const std::string &inPort, const std::shared_ptr<EffectBuffer> &buffer,
        std::shared_ptr<EffectContext> &context) override;

    ErrorCode PullData(const std::string &outPort, std::shared_ptr<EffectBuffer> &data) override;

    const EventReceiver *GetOwnerPipeline() const override
    {
        return eventReceiver_;
    }

    ErrorCode Prepare() override;

    ErrorCode Start() override;

    ErrorCode SetParameter(int32_t key, const Media::Plugin::Any &value) override
    {
        return ErrorCode::ERR_UNIMPLEMENTED;
    }

    ErrorCode GetParameter(int32_t key, Media::Plugin::Any &value) override
    {
        return ErrorCode::ERR_UNIMPLEMENTED;
    }

    void UnlinkPrevFilters() override;

    std::vector<Filter *> GetNextFilters() override;

    std::vector<Filter *> GetPreFilters() override;

    // Port调用此方法向Filter报告事件
    void OnEvent(const Event &event) override;

    template <typename T> static T FindPort(const std::vector<T> &ports, const std::string &name);

protected:
    virtual void InitPorts();

    std::string NamePort(const std::string &mime);

    /**
     * Get in-port from routeMap_ by outPortName.
     *
     * @param outPortName out-port name
     * @return null if not exists
     */
    PInPort GetRouteInPort(const std::string &outPortName);

    /**
     * Get out-port from routeMap_ by inPortName.
     *
     * @param inPortName in-port name
     * @return null if not exists
     */
    POutPort GetRouteOutPort(const std::string &inPortName);

    std::string name_;

    std::atomic<FilterState> state_;

    EventReceiver *eventReceiver_;

    std::vector<PFilter> children_{};

    std::vector<PInPort> inPorts_{};

    std::vector<POutPort> outPorts_{};

    std::vector<PairPort> routeMap_{}; // inport -> outport

    std::map<std::string, uint32_t> portTypeCntMap_{};

    FilterType filterType_{ FilterType::NONE };

    std::shared_ptr<EffectBuffer> sinkBuffer_{ nullptr };
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif
