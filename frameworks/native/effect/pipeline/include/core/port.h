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

#ifndef IE_PIPELINE_CORE_PORT_H
#define IE_PIPELINE_CORE_PORT_H

#include <string>
#include <vector>

#include "any.h"
#include "effect_buffer.h"
#include "effect_context.h"
#include "error_code.h"
#include "transfer.h"

#define PORT_NAME_DEFAULT "default"

namespace OHOS {
namespace Media {
namespace Effect {
enum class PortType { IN, OUT };

struct PortDesc {
    std::string name;
    bool isPcm;
};

struct PortInfo {
    PortType type;
    std::vector<PortDesc> ports;
};

class Port {
public:
    Port(InfoTransfer *ownerFilter, std::string portName) : filter_(ownerFilter), name_(std::move(portName)) {}

    virtual ~Port() = default;

    const std::string &GetName()
    {
        return name_;
    }

    const InfoTransfer *GetOwnerFilter() const
    {
        return filter_;
    }

    virtual std::shared_ptr<Port> GetPeerPort();

    WorkMode GetWorkMode();

    virtual ErrorCode Connect(const std::shared_ptr<Port> &port) = 0;

    virtual ErrorCode Disconnect() = 0;

    virtual ErrorCode Activate(const std::vector<WorkMode> &modes, WorkMode &outMode) = 0;

    virtual void Negotiate(const std::shared_ptr<Capability> &capability, std::shared_ptr<EffectContext> &context) = 0;

    virtual void PushData(const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context) = 0;

    virtual ErrorCode PullData(std::shared_ptr<EffectBuffer> &data) = 0;

protected:
    InfoTransfer *filter_;

    WorkMode workMode_{ WorkMode::PUSH };

    const std::string name_;
};

class InPort : public Port {
public:
    explicit InPort(InfoTransfer *filter, std::string name = PORT_NAME_DEFAULT) : Port(filter, std::move(name)) {}

    ~InPort() override = default;

    ErrorCode Connect(const std::shared_ptr<Port> &port) override;

    ErrorCode Disconnect() override;

    ErrorCode Activate(const std::vector<WorkMode> &modes, WorkMode &outMode) override;

    std::shared_ptr<Port> GetPeerPort() override;

    void Negotiate(const std::shared_ptr<Capability> &capability, std::shared_ptr<EffectContext> &context) override;

    void PushData(const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context) override;

    ErrorCode PullData(std::shared_ptr<EffectBuffer> &data) override;

private:
    std::weak_ptr<Port> prevPort_;
};

class OutPort : public Port {
public:
    explicit OutPort(InfoTransfer *filter, std::string name = PORT_NAME_DEFAULT) : Port(filter, std::move(name)) {}

    ~OutPort() override = default;

    ErrorCode Connect(const std::shared_ptr<Port> &port) override;

    ErrorCode Disconnect() override;

    ErrorCode Activate(const std::vector<WorkMode> &modes, WorkMode &outMode) override;

    std::shared_ptr<Port> GetPeerPort() override;

    void Negotiate(const std::shared_ptr<Capability> &capability, std::shared_ptr<EffectContext> &context) override;

    void PushData(const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context) override;

    ErrorCode PullData(std::shared_ptr<EffectBuffer> &data) override;

private:
    bool InSamePipeline(const std::shared_ptr<Port> &port) const;

    std::shared_ptr<Port> nextPort_;
};

class EmptyInPort : public InPort {
public:
    static std::shared_ptr<InPort> GetInstance()
    {
        return port_;
    }

    EmptyInPort() : InPort(nullptr, "emptyInPort") {}

    ~EmptyInPort() override = default;

    ErrorCode Connect(const std::shared_ptr<Port> &another) override;

    ErrorCode Activate(const std::vector<WorkMode> &modes, WorkMode &outMode) override;

    void Negotiate(const std::shared_ptr<Capability> &capability, std::shared_ptr<EffectContext> &context) override;

    void PushData(const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context) override;

    ErrorCode PullData(std::shared_ptr<EffectBuffer> &data) override;

private:
    static std::shared_ptr<InPort> port_;
};

class EmptyOutPort : public OutPort {
public:
    static std::shared_ptr<OutPort> GetInstance()
    {
        return port_;
    }

    EmptyOutPort() : OutPort(nullptr, "emptyOutPort") {}

    ~EmptyOutPort() override = default;

    ErrorCode Connect(const std::shared_ptr<Port> &another) override;

    ErrorCode Activate(const std::vector<WorkMode> &modes, WorkMode &outMode) override;

    void Negotiate(const std::shared_ptr<Capability> &capability, std::shared_ptr<EffectContext> &context) override;

    void PushData(const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context) override;

    ErrorCode PullData(std::shared_ptr<EffectBuffer> &data) override;

private:
    static std::shared_ptr<OutPort> port_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IE_PIPELINE_CORE_PORT_H
