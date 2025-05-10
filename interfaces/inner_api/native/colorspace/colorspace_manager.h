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

#ifndef IMAGE_EFFECT_COLORSPACE_MANAGER_H
#define IMAGE_EFFECT_COLORSPACE_MANAGER_H

#include <memory>

#include "colorspace_strategy.h"
#include "image_effect_marco_define.h"
#include "metadata_processor.h"
#include "colorspace_processor.h"

namespace OHOS {
namespace Media {
namespace Effect {
class ColorSpaceManager {
public:
    IMAGE_EFFECT_EXPORT ColorSpaceManager();
    ~ColorSpaceManager() = default;

    IMAGE_EFFECT_EXPORT static bool IsNeedConvertColorSpace(EffectColorSpace colorSpace);
    IMAGE_EFFECT_EXPORT static bool IsSupportedColorSpace(EffectColorSpace colorSpace);
    IMAGE_EFFECT_EXPORT static std::unordered_set<EffectColorSpace> GetAllSupportedColorSpaces();

    IMAGE_EFFECT_EXPORT void Init(std::shared_ptr<EffectBuffer> &src, std::shared_ptr<EffectBuffer> &dst);
    IMAGE_EFFECT_EXPORT ErrorCode ApplyColorSpace(EffectBuffer *effectBuffer, const EffectColorSpace &colorSpace,
        EffectColorSpace &outputColorSpace);

    IMAGE_EFFECT_EXPORT
    ErrorCode ChooseColorSpace(const std::unordered_set<EffectColorSpace> &filtersSupportedColorSpace,
        const EffectColorSpace &srcRealColorSpace, EffectColorSpace &outputColorSpace);
    IMAGE_EFFECT_EXPORT void Deinit();
    std::shared_ptr<ColorSpaceProcessor> GetColorSpaceProcessor();
    std::shared_ptr<MetadataProcessor> GetMetaDataProcessor();
private:
    std::shared_ptr<ColorSpaceStrategy> strategy_;
    std::shared_ptr<MetadataProcessor> metadataGenerator_ = nullptr;
    std::shared_ptr<ColorSpaceProcessor> colorSpaceConverter_ = nullptr;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_COLORSPACE_MANAGER_H
