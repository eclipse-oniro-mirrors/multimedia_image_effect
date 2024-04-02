/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef IM_CAPABILITY_NEGOTIATE_H
#define IM_CAPABILITY_NEGOTIATE_H

#include "capability.h"

namespace OHOS {
namespace Media {
namespace Effect {
class CapabilityNegotiate {
public:
    CapabilityNegotiate() = default;

    ~CapabilityNegotiate() = default;

    std::vector<std::shared_ptr<Capability>> &GetCapabilityList();

    void AddCapability(std::shared_ptr<Capability> &capability);

    void ClearNegotiateResult();
private:
    std::vector<std::shared_ptr<Capability>> caps_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IM_CAPABILITY_NEGOTIATE_H