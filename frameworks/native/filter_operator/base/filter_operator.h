/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IMAGE_EFFECT_FILTER_OPERATOR_H
#define IMAGE_EFFECT_FILTER_OPERATOR_H

#include <string>
#include <utility>

#include "any.h"
#include "effect_buffer.h"
#include "effect_info.h"
#include "error_code.h"

namespace OHOS {
namespace Media {
namespace Effect {
class FilterOperator {
public:
    FilterOperator(IPType ipType, const std::vector<IEffectFormat> &formats) : mIpType(ipType), formatTypes_(formats) {}

    virtual ~FilterOperator() = default;

    IPType GetIpType() const
    {
        return mIpType;
    }

    std::vector<IEffectFormat> GetFormatTypes() const
    {
        return formatTypes_;
    }

    virtual ErrorCode Release()
    {
        return ErrorCode::SUCCESS;
    }

    virtual ErrorCode OnApplyRGBA8888(EffectBuffer *src, EffectBuffer *dst,
        std::map<std::string, Plugin::Any> &value) = 0;

    virtual ErrorCode OnApplyYUVNV21(EffectBuffer *src, EffectBuffer *dst,
        std::map<std::string, Plugin::Any> &value) = 0;

    virtual ErrorCode OnApplyYUVNV12(EffectBuffer *src, EffectBuffer *dst,
        std::map<std::string, Plugin::Any> &value) = 0;

private:
    IPType mIpType = IPType::DEFAULT;
    const std::vector<IEffectFormat> formatTypes_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // FILTER_H