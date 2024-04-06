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

#ifndef IE_PIPELINE_CORE_TRANSFER_H
#define IE_PIPELINE_CORE_TRANSFER_H

#include <map>
#include <string>
#include <vector>

#include "any.h"
#include "effect_buffer.h"
#include "effect_type.h"
#include "error_code.h"
#include "capability.h"


namespace OHOS {
namespace Media {
namespace Effect {
// 各个组件向Pipeline报告的事件类型
enum struct EventType : uint32_t {
    EVENT_READY = 0,
    EVENT_START,
    EVENT_COMPLETE,
    EVENT_ERROR
};

struct Event {
    std::string srcFilter;
    EventType type;
    Media::Plugin::Any param;
};

/**
 * EventReceiver:
 * 1. Port使用此接口传递事件给Filter
 * 2. Filter使用此接口传递事件给Pipeline
 * 3. Sub Filter使用此接口传递事件给 Parent Filter
 */
class EventReceiver {
public:
    virtual ~EventReceiver() = default;

    virtual void OnEvent(const Event &event) = 0;
};

enum class WorkMode {
    PUSH,
    PULL
};

/**
 * Port使用此接口从Filter获取信息，或传递信息给Filter.
 */
class InfoTransfer : public EventReceiver {
public:
    virtual const std::string &GetName() = 0;

    // OutPort调用
    virtual std::vector<WorkMode> GetWorkModes() = 0;

    virtual void Negotiate(const std::string& inPort, const std::shared_ptr<Capability> &capability,
        std::shared_ptr<EffectContext> &context) = 0;

    // InPort调用
    virtual ErrorCode PushData(const std::string &inPort, const std::shared_ptr<EffectBuffer> &buffer,
        std::shared_ptr<EffectContext> &context) = 0;

    // OutPort调用
    virtual ErrorCode PullData(const std::string &outPort, std::shared_ptr<EffectBuffer> &data) = 0;

    virtual const EventReceiver *GetOwnerPipeline() const = 0;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif
