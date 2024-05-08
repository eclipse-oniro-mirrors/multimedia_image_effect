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
std::shared_ptr<InPort> EmptyInPort::port_ = std::make_shared<EmptyInPort>();
std::shared_ptr<OutPort> EmptyOutPort::port_ = std::make_shared<EmptyOutPort>();

std::shared_ptr<Port> Port::GetPeerPort()
{
    return nullptr;
}

WorkMode Port::GetWorkMode()
{
    return workMode_;
}

ErrorCode InPort::Connect(const std::shared_ptr<Port> &port)
{
    prevPort_ = port;
    return ErrorCode::SUCCESS;
}

ErrorCode InPort::Disconnect()
{
    prevPort_.reset();
    return ErrorCode::SUCCESS;
}

ErrorCode InPort::Activate(const std::vector<WorkMode> &modes, WorkMode &outMode)
{
    if (auto ptr = prevPort_.lock()) {
        FAIL_RETURN(ptr->Activate(modes, workMode_));
        outMode = workMode_;
        return ErrorCode::SUCCESS;
    }
    EFFECT_LOGE("[Filter %{public}s] InPort %{public}s Activate error: prevPort destructed", filter_->GetName().c_str(),
        name_.c_str());
    return ErrorCode::ERR_INVALID_PARAMETER_VALUE;
}

std::shared_ptr<Port> InPort::GetPeerPort()
{
    return prevPort_.lock();
}

void InPort::Negotiate(const std::shared_ptr<Capability> &capability, std::shared_ptr<EffectContext> &context)
{
    if (filter_) {
        return filter_->Negotiate(name_, capability, context);
    } else {
        EFFECT_LOGE("InPort::Negotiate filter is invalid! name=%{public}s", name_.c_str());
    }
}

void InPort::PushData(const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context)
{
    if (filter_) {
        filter_->PushData(name_, buffer, context);
    } else {
        EFFECT_LOGE("InPort::PushData filter is invalid! name=%{public}s", name_.c_str());
    }
}

ErrorCode InPort::PullData(std::shared_ptr<EffectBuffer> &data)
{
    EFFECT_LOGI("InPort::PullData");
    FALSE_RETURN_MSG_E(!prevPort_.expired(), ErrorCode::ERR_PIPELINE_INVALID_FILTER_PORT,
        "prevPort is null! name=%{public}s", name_.c_str());
    if (auto ptr = prevPort_.lock()) {
        FALSE_RETURN_MSG_E(ptr != nullptr, ErrorCode::ERR_PIPELINE_INVALID_FILTER_PORT,
            "prevPort.lock is null! name=%{public}s", name_.c_str());
        return ptr->PullData(data);
    }
    EFFECT_LOGE("prevPort destructed! name=%{public}s", name_.c_str());
    return ErrorCode::ERR_INVALID_PARAMETER_VALUE;
}

ErrorCode OutPort::Connect(const std::shared_ptr<Port> &port)
{
    if (InSamePipeline(port)) {
        nextPort_ = port;
        return ErrorCode::SUCCESS;
    }
    EFFECT_LOGE("Connect filters that are not in the same pipeline. name=%{public}s", name_.c_str());
    return ErrorCode::ERR_INVALID_PARAMETER_VALUE;
}

ErrorCode OutPort::Disconnect()
{
    nextPort_.reset();
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
    if (filter_) {
        auto supportedModes = filter_->GetWorkModes();
        for (auto mode : modes) {
            auto found = std::find(supportedModes.cbegin(), supportedModes.cend(), mode);
            if (found != supportedModes.cend()) {
                outMode = mode;
                workMode_ = mode;
                return ErrorCode::SUCCESS; // 最先找到的兼容的mode，作为最后结果
            }
        }
    } else {
        EFFECT_LOGE("no valid filter! name=%{public}s", name_.c_str());
    }
    EFFECT_LOGE("activate failed! name=%{public}s", name_.c_str());
    return ErrorCode::ERR_UNKNOWN;
}

std::shared_ptr<Port> OutPort::GetPeerPort()
{
    return nextPort_;
}

void OutPort::Negotiate(const std::shared_ptr<Capability> &capability, std::shared_ptr<EffectContext> &context)
{
    nextPort_->Negotiate(capability, context);
}

void OutPort::PushData(const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context)
{
    nextPort_->PushData(buffer, context);
}

ErrorCode OutPort::PullData(std::shared_ptr<EffectBuffer> &data)
{
    if (filter_) {
        EFFECT_LOGI("OutPort::PullData for filter(%{public}s)", filter_->GetName().c_str());
        return filter_->PullData(name_, data);
    }
    EFFECT_LOGE("filter destructed! name=%{public}s", name_.c_str());
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
