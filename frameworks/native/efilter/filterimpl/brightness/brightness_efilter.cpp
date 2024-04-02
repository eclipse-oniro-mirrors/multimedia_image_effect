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

#include "brightness_efilter.h"

#include <unordered_map>

#include "effect_log.h"
#include "json_helper.h"
#include "common_utils.h"
#include "cpu_brightness_algo.h"
#include "gpu_brightness_algo.h"
#include "efilter_factory.h"

namespace OHOS {
namespace Media {
namespace Effect {
REGISTER_EFILTER_FACTORY(BrightnessEFilter, "Brightness");
std::shared_ptr<EffectInfo> BrightnessEFilter::info_ = nullptr;
using ApplyFunc =
    std::function<ErrorCode(EffectBuffer *src, EffectBuffer *dst, std::map<std::string, Plugin::Any> &value)>;

const float BrightnessEFilter::Parameter::RANGE[] = { -100.f, 100.f };
const std::string BrightnessEFilter::Parameter::KEY_INTENSITY = "FILTER_INTENSITY";

static const std::unordered_map<IPType, std::unordered_map<IEffectFormat, ApplyFunc>> BRIGHTNESS_APPLY_FUNCS = {
    { IPType::CPU,
        {
            { IEffectFormat::RGBA8888, CpuBrightnessAlgo::OnApplyRGBA8888 },
            { IEffectFormat::YUVNV12, CpuBrightnessAlgo::OnApplyYUVNV12 },
            { IEffectFormat::YUVNV21, CpuBrightnessAlgo::OnApplyYUVNV21 },
        }
    },
    {
        IPType::GPU,
        {
            { IEffectFormat::RGBA8888, GpuBrightnessAlgo::OnApplyRGBA8888 },
        }
    }
};

ErrorCode BrightnessEFilter::Render(EffectBuffer *buffer, std::shared_ptr<EffectContext> &context)
{
    ErrorCode res = Render(buffer, buffer, context);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "filter(%{public}s) render fail", name_.c_str());
    return PushData(buffer, context);
}

ErrorCode BrightnessEFilter::Render(EffectBuffer *src, EffectBuffer *dst, std::shared_ptr<EffectContext> &context)
{
    IPType ipType = context->ipType_;
    auto it = BRIGHTNESS_APPLY_FUNCS.find(ipType);
    CHECK_AND_RETURN_RET_LOG(it != BRIGHTNESS_APPLY_FUNCS.end(), ErrorCode::ERR_UNSUPPORTED_IPTYPE_FOR_EFFECT,
        "ipType=%{public}d is not support! filter=%{public}s", ipType, name_.c_str());

    IEffectFormat formatType = src->bufferInfo_->formatType_;
    std::unordered_map<IEffectFormat, ApplyFunc> formatFuncs = it->second;
    auto formatIter = formatFuncs.find(formatType);
    CHECK_AND_RETURN_RET_LOG(formatIter != formatFuncs.end(), ErrorCode::ERR_UNSUPPORTED_FORMAT_TYPE,
        "format=%{public}d is not support! filter=%{public}s", formatType, name_.c_str());

    return formatIter->second(src, dst, values_);
}

ErrorCode BrightnessEFilter::SetValue(const std::string &key, Plugin::Any &value)
{
    if (Parameter::KEY_INTENSITY.compare(key) != 0) {
        EFFECT_LOGE("key is not support! key=%{public}s", key.c_str());
        return ErrorCode::ERR_UNSUPPORTED_VALUE_KEY;
    }

    auto brightnessPtr = Plugin::AnyCast<float>(&value);
    if (brightnessPtr == nullptr) {
        EFFECT_LOGE("the type is not float! key=%{public}s", key.c_str());
        return ErrorCode::ERR_ANY_CAST_TYPE_NOT_FLOAT;
    }

    float brightness = *brightnessPtr;
    if (brightness < Parameter::RANGE[0] || brightness > Parameter::RANGE[1]) {
        EFFECT_LOGW("the value is out of range! key=%{public}s, value=%{public}f, range=[%{public}f, %{public}f]",
            key.c_str(), brightness, Parameter::RANGE[0], Parameter::RANGE[1]);
        *brightnessPtr = CLIP(brightness, Parameter::RANGE[0], Parameter::RANGE[1]);
    }

    return EFilter::SetValue(key, value);
}

ErrorCode BrightnessEFilter::Restore(const nlohmann::json &values)
{
    float brightness;

    // If the developer does not set parameters, the function returns a failure, but it is a normal case.
    ErrorCode result = JsonHelper::GetFloatValue(values, Parameter::KEY_INTENSITY, brightness);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGW("not set value! key=%{public}s", Parameter::KEY_INTENSITY.c_str());
        return ErrorCode::SUCCESS;
    }
    if (brightness < Parameter::RANGE[0] || brightness > Parameter::RANGE[1]) {
        return ErrorCode::ERR_VALUE_OUT_OF_RANGE;
    }
    Plugin::Any any = brightness;
    return SetValue(Parameter::KEY_INTENSITY, any);
}

std::shared_ptr<EffectInfo> BrightnessEFilter::GetEffectInfo(const std::string &name)
{
    if (info_ != nullptr) {
        return info_;
    }
    info_ = std::make_unique<EffectInfo>();
    std::map<IPType, std::vector<IEffectFormat>> formatTypes;
    for (const auto &applyFunc : BRIGHTNESS_APPLY_FUNCS) {
        const IPType &ipType = applyFunc.first;
        const std::unordered_map<IEffectFormat, ApplyFunc> &effectFormats = applyFunc.second;
        for (const auto &effectFormat : effectFormats) {
            const IEffectFormat &format = effectFormat.first;
            auto it = info_->formats_.find(format);
            if (it == info_->formats_.end()) {
                std::vector<IPType> ipTypes;
                ipTypes.emplace_back(ipType);
                info_->formats_.emplace(format, ipTypes);
            } else {
                std::vector<IPType> &ipTypes = it->second;
                ipTypes.emplace_back(ipType);
            }
        }
    }

    info_->category_ = Category::COLOR_ADJUST;
    return info_;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS