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

#ifndef IMAGE_EFFECT_DELEGATE_H
#define IMAGE_EFFECT_DELEGATE_H

#include <memory>

#include "any.h"
#include "effect_buffer.h"
#include "effect_info.h"
#include "nlohmann/json.hpp"
#include "effect_context.h"

namespace OHOS {
namespace Media {
namespace Effect {
class IFilterDelegate {
public:
    virtual ~IFilterDelegate() = default;

    virtual bool Render(void *efilter, EffectBuffer *src, EffectBuffer *dst,
        std::shared_ptr<EffectContext> &context) = 0;

    virtual bool Render(void *efilter, EffectBuffer *src, std::shared_ptr<EffectContext> &context) = 0;

    virtual bool SetValue(void *efilter, const std::string &key, const Plugin::Any &value) = 0;

    virtual bool Save(void *efilter, nlohmann::json &res) = 0;

    virtual void *Restore(const nlohmann::json &values) = 0;

    virtual void *GetEffectInfo() = 0;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_DELEGATE_H