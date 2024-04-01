/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef IMAGE_EFFECT_BRIGHTNESS_EFILTER_H
#define IMAGE_EFFECT_BRIGHTNESS_EFILTER_H

#include "efilter.h"

namespace OHOS {
namespace Media {
namespace Effect {
class BrightnessEFilter : public EFilter {
public:
    class Parameter : public EFilter::Parameter {
    public:
        static const float RANGE[];
        static const std::string KEY_INTENSITY;
    };

    explicit BrightnessEFilter(const std::string &name) : EFilter(name) {}

    ~BrightnessEFilter() override = default;

    ErrorCode Render(EffectBuffer *buffer, std::shared_ptr<EffectContext> &context) override;

    ErrorCode Render(EffectBuffer *src, EffectBuffer *dst, std::shared_ptr<EffectContext> &context) override;

    ErrorCode SetValue(const std::string &key, Plugin::Any &value) override;

    ErrorCode Restore(const nlohmann::json &values) override;

    static std::shared_ptr<EffectInfo> GetEffectInfo(const std::string &name);

private:
    static std::shared_ptr<EffectInfo> info_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_BRIGHTNESS_EFILTER_H