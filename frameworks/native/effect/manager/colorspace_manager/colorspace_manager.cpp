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

#include "colorspace_manager.h"

#include "effect_log.h"
#include "colorspace_converter.h"

namespace OHOS {
namespace Media {
namespace Effect {
ColorSpaceManager::ColorSpaceManager()
{
    strategy_ = std::make_shared<ColorSpaceStrategy>();
}

bool ColorSpaceManager::IsNeedConvertColorSpace(EffectColorSpace colorSpace)
{
    return ColorSpaceStrategy::IsNeedConvertColorSpace(colorSpace);
}

bool ColorSpaceManager::IsSupportedColorSpace(EffectColorSpace colorSpace)
{
    return ColorSpaceStrategy::IsSupportedColorSpace(colorSpace);
}

std::unordered_set<EffectColorSpace> ColorSpaceManager::GetAllSupportedColorSpaces()
{
    return ColorSpaceStrategy::GetAllSupportedColorSpaces();
}

void ColorSpaceManager::Init(std::shared_ptr<EffectBuffer> &src, std::shared_ptr<EffectBuffer> &dst)
{
    strategy_->Init(src, dst);
}

ErrorCode ColorSpaceManager::ApplyColorSpace(EffectBuffer *effectBuffer, const EffectColorSpace &colorSpace,
    EffectColorSpace &outputColorSpace)
{
    if (!IsNeedConvertColorSpace(colorSpace)) {
        outputColorSpace = colorSpace;
        return ErrorCode::SUCCESS;
    }

    EffectColorSpace targetColorSpace = ColorSpaceStrategy::GetTargetColorSpace(colorSpace);
    ErrorCode res = strategy_->CheckConverterColorSpace(targetColorSpace);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res,
        "ApplyColorSpace: CheckConverterColorSpace fail! res=%{public}d", res);

    res = ColorSpaceConverter::ApplyColorSpace(effectBuffer, targetColorSpace);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "ApplyColorSpace: convert fail! "
        "res=%{public}d, colorSpace=%{public}d, targetColorSpace=%{public}d", res, colorSpace, targetColorSpace);
    outputColorSpace = targetColorSpace;

    return ErrorCode::SUCCESS;
}

ErrorCode ColorSpaceManager::ChooseColorSpace(const std::unordered_set<EffectColorSpace> &filtersSupportedColorSpace,
    const EffectColorSpace &srcRealColorSpace, EffectColorSpace &outputColorSpace)
{
    return strategy_->ChooseColorSpace(filtersSupportedColorSpace, srcRealColorSpace, outputColorSpace);
}

void ColorSpaceManager::Deinit()
{
    strategy_->Deinit();
}

} // namespace Effect
} // namespace Media
} // namespace OHOS