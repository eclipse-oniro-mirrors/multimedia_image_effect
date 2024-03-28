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

#ifndef IMAGE_EFFECT_CPU_CONTRAST_FILTER_OPERATOR_H
#define IMAGE_EFFECT_CPU_CONTRAST_FILTER_OPERATOR_H

#include <vector>

#include "cpu_filter_operator.h"

namespace OHOS {
namespace Media {
namespace Effect {
class CpuContrastFilterOperator : public CpuFilterOperator {
public:
    CpuContrastFilterOperator() : CpuFilterOperator(FORMAT_TYPES){};

    static std::vector<IEffectFormat> GetFormatTypes()
    {
        return FORMAT_TYPES;
    }

    static IPType GetIPType()
    {
        return IP_TYPE;
    }

    ErrorCode OnApplyRGBA8888(EffectBuffer *src, EffectBuffer *dst, std::map<std::string, Plugin::Any> &value) override;

    ErrorCode OnApplyYUVNV21(EffectBuffer *src, EffectBuffer *dst, std::map<std::string, Plugin::Any> &value) override;

    ErrorCode OnApplyYUVNV12(EffectBuffer *src, EffectBuffer *dst, std::map<std::string, Plugin::Any> &value) override;

private:
    float ParseContrast(std::map<std::string, Plugin::Any> &value);

    static const std::vector<IEffectFormat> FORMAT_TYPES;
    static const IPType IP_TYPE = IPType::CPU;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_CPU_CONTRAST_FILTER_OPERATOR_H