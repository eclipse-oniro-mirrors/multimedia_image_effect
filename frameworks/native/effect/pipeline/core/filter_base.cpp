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

#include "filter_base.h"

#include <algorithm>

#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
FilterBase::FilterBase(std::string name) : name_(std::move(name)), state_(FilterState::CREATED), eventReceiver_(nullptr)
{
    inPorts_.reserve(1);
    outPorts_.reserve(1);
    routeMap_.reserve(1);
    EFFECT_LOGI("pipeline filter(name=%{public}s) Constructor", name_.c_str());
}

FilterBase::~FilterBase()
{
    portTypeCntMap_.clear();
}

void FilterBase::Initialize(EventReceiver *receiver)
{
    this->eventReceiver_ = receiver;
    InitPorts();
    state_ = FilterState::INITIALIZED;
}

std::shared_ptr<InPort> FilterBase::GetInPort(const std::string &name)
{
    EFFECT_LOGI("GetInPort, name=%{public}s", name.c_str());
    FALSE_RETURN_E(!inPorts_.empty(), nullptr);
    return FindPort(inPorts_, name);
}

std::shared_ptr<OutPort> FilterBase::GetOutPort(const std::string &name)
{
    EFFECT_LOGI("GetOutPort, name=%{public}s", name.c_str());
    FALSE_RETURN_E(!outPorts_.empty(), nullptr);
    return FindPort(outPorts_, name);
}

ErrorCode FilterBase::Prepare()
{
    EFFECT_LOGI("Prepare called");
    state_ = FilterState::PREPARING;

    // Filter默认InPort按Push方式获取数据
    WorkMode mode;
    return GetInPort(PORT_NAME_DEFAULT)->Activate({ WorkMode::PUSH }, mode);
}

ErrorCode FilterBase::Start()
{
    state_ = FilterState::RUNNING;
    return ErrorCode::SUCCESS;
}

void FilterBase::UnlinkPrevFilters()
{
    for (auto &&port : inPorts_) {
        auto peerPort = port->GetPeerPort();
        port->Disconnect();
        if (peerPort) {
            peerPort->Disconnect();
        }
    }
}

std::vector<Filter *> FilterBase::GetNextFilters()
{
    std::vector<Filter *> nextFilters;
    nextFilters.reserve(outPorts_.size());
    for (auto &&port : outPorts_) {
        auto peerPort = port->GetPeerPort();
        if (!peerPort) {
            EFFECT_LOGI("Filter %{public}s outport %{public}s has no peer port (%{public}zu).", name_.c_str(),
                port->GetName().c_str(), outPorts_.size());
            continue;
        }
        auto filter = const_cast<Filter *>(reinterpret_cast<const Filter *>(peerPort->GetOwnerFilter()));
        if (filter) {
            nextFilters.emplace_back(filter);
        }
    }
    return nextFilters;
}

std::vector<Filter *> FilterBase::GetPreFilters()
{
    std::vector<Filter *> preFilters;
    preFilters.reserve(inPorts_.size());
    for (auto &&inPort : inPorts_) {
        auto peerPort = inPort->GetPeerPort();
        if (!peerPort) {
            EFFECT_LOGI("Filter %{public}s inport %{public}s has no peer port (%{public}zu).", name_.c_str(),
                inPort->GetName().c_str(), inPorts_.size());
            continue;
        }
        auto filter = const_cast<Filter *>(reinterpret_cast<const Filter *>(peerPort->GetOwnerFilter()));
        if (filter) {
            preFilters.emplace_back(filter);
        }
    }
    return preFilters;
}

void FilterBase::Negotiate(const std::string& inPort, const std::shared_ptr<Capability> &capability,
    std::shared_ptr<EffectContext> &context) {}

ErrorCode FilterBase::PushData(const std::string &inPort, const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    return ErrorCode::ERR_INVALID_OPERATION;
}

ErrorCode FilterBase::PullData(const std::string &outPort, std::shared_ptr<EffectBuffer> &data)
{
    EFFECT_LOGI("PullData");
    return ErrorCode::ERR_INVALID_OPERATION;
}

void FilterBase::OnEvent(const Event &event)
{
    // Receive event from port and pass it to pipeline
    if (eventReceiver_ != nullptr) {
        eventReceiver_->OnEvent(event);
    }
}

template <typename T> T FilterBase::FindPort(const std::vector<T> &ports, const std::string &name)
{
    auto find = std::find_if(ports.begin(), ports.end(), [name](const T &item) {
        if (item == nullptr) {
            return false;
        }
        return name == item->GetName();
    });
    if (find != ports.end()) {
        return *find;
    }
    EFFECT_LOGE("Find port(%{public}s) failed.", name.c_str());
    return nullptr;
}

void FilterBase::InitPorts()
{
    inPorts_.clear();
    auto inPort = std::make_shared<InPort>(this);
    inPorts_.push_back(inPort);

    outPorts_.clear();
    auto outPort = std::make_shared<OutPort>(this);
    outPorts_.push_back(outPort);

    routeMap_.emplace_back(inPort->GetName(), outPort->GetName());
}

std::string FilterBase::NamePort(const std::string &mime)
{
    auto type = mime.substr(0, mime.find_first_of('/'));
    if (type.empty()) {
        type = "default";
    }
    auto count = ++(portTypeCntMap_[name_ + type]);
    auto portName = type + "_" + std::to_string(count);
    return portName;
}

PInPort FilterBase::GetRouteInPort(const std::string &outPortName)
{
    auto it = std::find_if(routeMap_.begin(), routeMap_.end(), [&outPortName](const PairPort &pp) {
        return outPortName == pp.second;
    });
    if (it == routeMap_.end()) {
        EFFECT_LOGW("out port %{public}s has no route map port", outPortName.c_str());
        return nullptr;
    }
    return GetInPort(it->first);
}

POutPort FilterBase::GetRouteOutPort(const std::string &inPortName)
{
    auto it = std::find_if(routeMap_.begin(), routeMap_.end(), [&inPortName](const PairPort &pp) {
        return inPortName == pp.first;
    });
    if (it == routeMap_.end()) {
        EFFECT_LOGW("in port %{public}s has no route map port", inPortName.c_str());
        return nullptr;
    }
    return GetOutPort(it->second);
}
} // namespace Effect
} // namespace Media
} // namespace OHOS
