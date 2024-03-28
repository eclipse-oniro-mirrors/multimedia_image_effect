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

#include "contrast_efilter.h"

#include "effect_log.h"
#include "efilter_factory.h"
#include "json_helper.h"
#include "common_utils.h"

namespace OHOS {
namespace Media {
namespace Effect {
REGISTER_EFILTER_FACTORY(ContrastEFilter, "Contrast");
const float ContrastEFilter::Parameter::RANGE[] = {-100.f, 100.f};

ErrorCode ContrastEFilter::SetValue(const std::string &key, Plugin::Any &value)
{
    if (Parameter::KEY_INTENSITY.compare(key) != 0) {
        EFFECT_LOGE("key is not support! key=%{public}s", key.c_str());
        return ErrorCode::ERR_UNSUPPORTED_VALUE_KEY;
    }

    auto contrastPtr = Plugin::AnyCast<float>(&value);
    if (contrastPtr == nullptr) {
        EFFECT_LOGE("the type is not float! key=%{public}s", key.c_str());
        return ErrorCode::ERR_ANY_CAST_TYPE_NOT_FLOAT;
    }

    float contrast = *contrastPtr;
    if (contrast < Parameter::RANGE[0] || contrast > Parameter::RANGE[1]) {
        EFFECT_LOGW("the value is out of range! key=%{public}s, value=%{public}f, range=[%{public}f, %{public}f]",
            key.c_str(), contrast, Parameter::RANGE[0], Parameter::RANGE[1]);
        *contrastPtr = CLIP(contrast, Parameter::RANGE[0], Parameter::RANGE[1]);
    }

    return EFilter::SetValue(key, value);
}

ErrorCode ContrastEFilter::Restore(const nlohmann::json &values)
{
    float contrast;

    // If the developer does not set parameters, the function returns a failure, but it is a normal case.
    ErrorCode result = JsonHelper::GetFloatValue(values, Parameter::KEY_INTENSITY, contrast);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGW("not set value! key=%{public}s", Parameter::KEY_INTENSITY.c_str());
        return ErrorCode::SUCCESS;
    }
    if (contrast < Parameter::RANGE[0] || contrast > Parameter::RANGE[1]) {
        return ErrorCode::ERR_VALUE_OUT_OF_RANGE;
    }
    Plugin::Any any = contrast;
    return SetValue(Parameter::KEY_INTENSITY, any);
}
} // namespace Effect
} // namespace Media
} // namespace OHOS