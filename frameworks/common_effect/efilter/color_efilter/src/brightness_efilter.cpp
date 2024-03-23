/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "brightness_efilter.h"

#include "effect_log.h"
#include "efilter_factory.h"
#include "json_helper.h"
#include "common_utils.h"

namespace OHOS {
namespace Media {
namespace Effect {
REGISTER_EFILTER_FACTORY(BrightnessEFilter, "Brightness");

const float BrightnessEFilter::Paramter::RANGE[] = {-100.f, 100.f};

ErrorCode BrightnessEFilter::SetValue(const std::string &key, Plugin::Any &value)
{
    if (Paramter::KEY_INTENSITY.compare(key) != 0) {
        EFFECT_LOGE("key is not support! key=%{public}s", key.c_str());
        return ErrorCode::ERR_UNSUPPORTED_VALUE_KEY;
    }

    auto brightnessPtr = Plugin::AnyCast<float>(&value);
    if (brightnessPtr == nullptr) {
        EFFECT_LOGE("the type is not float! key=%{public}s", key.c_str());
        return ErrorCode::ERR_ANY_CAST_TYPE_NOT_FLOAT;
    }

    float brightness = *brightnessPtr;
    if (brightness < Paramter::RANGE[0] || brightness > Paramter::RANGE[1]) {
        EFFECT_LOGW("the value is out of range! key=%{public}s, value=%{public}f, range=[%{public}f, %{public}f]",
            key.c_str(), brightness, Paramter::RANGE[0], Paramter::RANGE[1]);
            *brightnessPtr = CLIP(brightness, Paramter::RANGE[0], Paramter::RANGE[1]);
    }

    return EFilter::SetValue(key, value);
}

ErrorCode BrightnessEFilter::Restore(const nlohmann::json &values)
{
    float brightness;

    //If the developer does not set parameters, the function returns a failure, but it is a normal case.
    ErrorCode result = JsonHelper::GetFloatValue(values, Paramter::KEY_INTENSITY, brightness);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGE("not set value! key=%{public}s", Paramter::KEY_INTENSITY.c_str());
        return ErrorCode::SUCCESS;
    }
    if (brightness < Paramter::RANGE[0] || brightness > Paramter::RANGE[1]) {
        return ErrorCode::ERR_VALUE_OUT_OF_RANGE;
    }
    Plugin::Any any = brightness;
    return SetValue(Paramter::KEY_INTENSITY, any);
}
} // namespace Effect
} // namespace Media
} // namespace OHOS