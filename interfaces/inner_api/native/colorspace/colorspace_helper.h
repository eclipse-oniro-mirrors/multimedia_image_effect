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

#ifndef IMAGE_EFFECT_COLORSPACE_HELPER_H
#define IMAGE_EFFECT_COLORSPACE_HELPER_H

#include <memory>

#include "effect_info.h"
#include "color_space.h"
#include "error_code.h"
#include "surface_buffer.h"
#include "v1_0/cm_color_space.h"
#include "effect_buffer.h"
#include "effect_context.h"

namespace OHOS {
namespace Media {
namespace Effect {
class ColorSpaceHelper {
public:
    static bool IsHdrColorSpace(EffectColorSpace colorSpace);

    static EffectColorSpace ConvertToEffectColorSpace(OHOS::ColorManager::ColorSpaceName colorSpaceName);
    static OHOS::ColorManager::ColorSpaceName ConvertToColorSpaceName(EffectColorSpace colorSpace);
    static EffectColorSpace ConvertToEffectColorSpace(
        OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType type);
    static OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType ConvertToCMColorSpace(
        EffectColorSpace colorSpace);
    static ErrorCode SetSurfaceBufferMetadataType(SurfaceBuffer *sb,
        const OHOS::HDI::Display::Graphic::Common::V1_0::CM_HDR_Metadata_Type &type);
    static ErrorCode GetSurfaceBufferMetadataType(SurfaceBuffer *sb,
        OHOS::HDI::Display::Graphic::Common::V1_0::CM_HDR_Metadata_Type &type);
    static ErrorCode SetSurfaceBufferColorSpaceType(SurfaceBuffer *sb,
        const OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType &type);
    static ErrorCode GetSurfaceBufferColorSpaceType(SurfaceBuffer *sb,
        OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType &type);
    static ErrorCode SetHDRDynamicMetadata(SurfaceBuffer *sb, const std::vector<uint8_t> &hdrDynamicMetadata);
    static ErrorCode SetHDRStaticMetadata(SurfaceBuffer *sb, const std::vector<uint8_t> &hdrStaticMetadata);

    static ErrorCode UpdateMetadata(EffectBuffer *input);
    static ErrorCode UpdateMetadata(SurfaceBuffer *input, const EffectColorSpace &colorSpace);
    static ErrorCode ConvertColorSpace(std::shared_ptr<EffectBuffer> &srcBuffer,
        std::shared_ptr<EffectContext> &context);
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_COLORSPACE_HELPER_H
