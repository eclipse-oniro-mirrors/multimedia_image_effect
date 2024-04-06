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

#include "port.h"

#include <algorithm>

#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
std::shared_ptr<InPort> EmptyInPort::port = std::make_shared<EmptyInPort>();
std::shared_ptr<OutPort> EmptyOutPort::port = std::make_shared<EmptyOutPort>();

const std::string &Port::GetName()
{
    return name;
}

const InfoTransfer *Port::GetOwnerFilter() const
{
    return filter;
}

std::shared_ptr<Port> Port::GetPeerPort()
{
    return nullptr;
}

ErrorCode InPort::Connect(const std::shared_ptr<Port> &port)
{
    prevPort = port;
    return ErrorCode::SUCCESS;
}

ErrorCode InPort::Disconnect()
{
    prevPort.reset();
    return ErrorCode::SUCCESS;
}

ErrorCode InPort::Activate(const std::vector<WorkMode> &modes, WorkMode &outMode)
{
    if (auto ptr = prevPort.lock()) {
        FAIL_RETURN(ptr->Activate(modes, workMode));
        outMode = workMode;
        return ErrorCode::SUCCESS;
    }
    EFFECT_LOGE("[Filter %{public}s] InPort %{public}s Activate error: prevPort destructed", filter->GetName().c_str(),
        name.c_str());
    return ErrorCode::ERR_INVALID_PARAMETER_VALUE;
}

std::shared_ptr<Port> InPort::GetPeerPort()
{
    return prevPort.lock();
}

void InPort::Negotiate(const std::shared_ptr<Capability> &capability, std::shared_ptr<EffectContext> &context)
{
    if (filter) {
        return filter->Negotiate(name, capability, context);
    } else {
        EFFECT_LOGE("InPort::Negotiate filter is invalid! name=%{public}s", name.c_str());
    }
}

void InPort::PushData(const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context)
{
    if (filter) {
        filter->PushData(name, buffer, context);
    } else {
        EFFECT_LOGE("InPort::PushData filter is invalid! name=%{public}s", name.c_str());
    }
}

ErrorCode InPort::PullData(std::shared_ptr<EffectBuffer> &data)
{
    EFFECT_LOGI("InPort::PullData");
    FALSE_RETURN_MSG_E(!prevPort.expired(), ErrorCode::ERR_PIPELINE_INVALID_FILTER_PORT,
        "prevPort is null! name=%{public}s", name.c_str());
    if (auto ptr = prevPort.lock()) {
        FALSE_RETURN_MSG_E(ptr != nullptr, ErrorCode::ERR_PIPELINE_INVALID_FILTER_PORT,
            "prevPort.lock is null! name=%{public}s", name.c_str());
        return ptr->PullData(data);
    }
    EFFECT_LOGE("prevPort destructed! name=%{public}s", name.c_str());
    return ErrorCode::ERR_INVALID_PARAMETER_VALUE;
}

ErrorCode OutPort::Connect(const std::shared_ptr<Port> &port)
{
    if (InSamePipeline(port)) {
        nextPort = port;
        return ErrorCode::SUCCESS;
    }
    EFFECT_LOGE("Connect filters that are not in the same pipeline. name=%{public}s", name.c_str());
    return ErrorCode::ERR_INVALID_PARAMETER_VALUE;
}

ErrorCode OutPort::Disconnect()
{
    nextPort.reset();
    return ErrorCode::SUCCESS;
}

bool OutPort::InSamePipeline(const std::shared_ptr<Port> &port) const
{
    auto filter1 = GetOwnerFilter();
    FALSE_RETURN_E(filter1 != nullptr, false);
    auto filter2 = port->GetOwnerFilter();
    FALSE_RETURN_E(filter2 != nullptr, false);
    auto pipeline1 = filter1->GetOwnerPipeline();
    FALSE_RETURN_E(pipeline1 != nullptr, false);
    auto pipeline2 = filter2->GetOwnerPipeline();
    FALSE_RETURN_E(pipeline2 != nullptr, false);
    return pipeline1 == pipeline2;
}

ErrorCode OutPort::Activate(const std::vector<WorkMode> &modes, WorkMode &outMode)
{
    if (filter) {
        auto supportedModes = filter->GetWorkModes();
        for (auto mode : modes) {
            auto found = std::find(supportedModes.cbegin(), supportedModes.cend(), mode);
            if (found != supportedModes.cend()) {
                outMode = mode;
                workMode = mode;
                return ErrorCode::SUCCESS; // 最先找到的兼容的mode，作为最后结果
            }
        }
    } else {
        EFFECT_LOGE("no valid filter! name=%{public}s", name.c_str());
    }
    EFFECT_LOGE("activate failed! name=%{public}s", name.c_str());
    return ErrorCode::ERR_UNKNOWN;
}

std::shared_ptr<Port> OutPort::GetPeerPort()
{
    return nextPort;
}

void OutPort::Negotiate(const std::shared_ptr<Capability> &capability, std::shared_ptr<EffectContext> &context)
{
    nextPort->Negotiate(capability, context);
}

void OutPort::PushData(const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context)
{
    nextPort->PushData(buffer, context);
}

ErrorCode OutPort::PullData(std::shared_ptr<EffectBuffer> &data)
{
    if (filter) {
        EFFECT_LOGI("OutPort::PullData for filter(%{public}s)", filter->GetName().c_str());
        return filter->PullData(name, data);
    }
    EFFECT_LOGE("filter destructed! name=%{public}s", name.c_str());
    return ErrorCode::ERR_INVALID_PARAMETER_VALUE;
}

ErrorCode EmptyInPort::Connect(const std::shared_ptr<Port> &another)
{
    EFFECT_LOGE("Connect in EmptyInPort");
    return ErrorCode::ERR_UNIMPLEMENTED;
}

ErrorCode EmptyInPort::Activate(const std::vector<WorkMode> &modes, WorkMode &outMode)
{
    EFFECT_LOGE("Activate in EmptyInPort");
    return ErrorCode::ERR_UNIMPLEMENTED;
}

void EmptyInPort::Negotiate(const std::shared_ptr<Capability> &capability, std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGE("Negotiate in EmptyInPort");
}

void EmptyInPort::PushData(const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGE("PushData in EmptyInPort");
}
ErrorCode EmptyInPort::PullData(std::shared_ptr<EffectBuffer> &data)
{
    EFFECT_LOGE("PullData in EmptyInPort");
    return ErrorCode::ERR_UNIMPLEMENTED;
}

ErrorCode EmptyOutPort::Connect(const std::shared_ptr<Port> &another)
{
    EFFECT_LOGE("Connect in EmptyOutPort");
    return ErrorCode::ERR_UNIMPLEMENTED;
}

ErrorCode EmptyOutPort::Activate(const std::vector<WorkMode> &modes, WorkMode &outMode)
{
    EFFECT_LOGE("Activate in EmptyOutPort");
    return ErrorCode::ERR_UNIMPLEMENTED;
}

void EmptyOutPort::Negotiate(const std::shared_ptr<Capability> &capability, std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGE("Negotiate in EmptyOutPort");
}

void EmptyOutPort::PushData(const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGE("PushData in EmptyOutPort");
}

ErrorCode EmptyOutPort::PullData(std::shared_ptr<EffectBuffer> &data)
{
    EFFECT_LOGE("PullData in EmptyOutPort");
    return ErrorCode::ERR_UNIMPLEMENTED;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS
