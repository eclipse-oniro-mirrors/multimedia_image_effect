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

#ifndef IMAGE_EFFECT_EFILTER_H
#define IMAGE_EFFECT_EFILTER_H

#include <map>
#include <string>

#include "any.h"
#include "effect_buffer.h"
#include "effect_type.h"
#include "efilter_base.h"
#include "error_code.h"
#include "nlohmann/json.hpp"

namespace OHOS {
namespace Media {
namespace Effect {
class EFilter : public EFilterBase {
public:
    class Parameter {
    public:
        static const std::string KEY_DEFAULT_VALUE;
    };

    explicit EFilter(const std::string &name) : EFilterBase(name) {}

    ~EFilter() override = default;

    virtual ErrorCode Render(EffectBuffer *buffer, std::shared_ptr<EffectContext> &context) = 0;

    ErrorCode Render(std::shared_ptr<EffectBuffer> &src, std::shared_ptr<EffectBuffer> &dst);

    virtual ErrorCode PreRender(IEffectFormat &format);

    virtual ErrorCode Render(EffectBuffer *src, EffectBuffer *dst, std::shared_ptr<EffectContext> &context) = 0;

    virtual ErrorCode SetValue(const std::string &key, Plugin::Any &value);

    virtual ErrorCode GetValue(const std::string &key, Plugin::Any &value);

    virtual ErrorCode Save(nlohmann::json &res);

    virtual ErrorCode Restore(const nlohmann::json &values) = 0;

    static std::shared_ptr<EffectInfo> GetEffectInfo(const std::string &name)
    {
        return nullptr;
    }

    ErrorCode PushData(const std::string &inPort, const std::shared_ptr<EffectBuffer> &buffer,
        std::shared_ptr<EffectContext> &context) override;

    ErrorCode PushData(EffectBuffer *buffer, std::shared_ptr<EffectContext> &context);

    std::map<std::string, Plugin::Any> &GetValues()
    {
        return values_;
    }

    virtual std::shared_ptr<MemNegotiatedCap> Negotiate(const std::shared_ptr<MemNegotiatedCap> &input);
protected:
    ErrorCode CalculateEFilterIPType(IEffectFormat &formatType, IPType &ipType);

    std::map<std::string, Plugin::Any> values_;

private:
    void Negotiate(const std::string &inPort, const std::shared_ptr<Capability> &capability,
        std::shared_ptr<EffectContext> &context) override;

    std::shared_ptr<Capability> outputCap_ = nullptr;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_EFILTER_H
