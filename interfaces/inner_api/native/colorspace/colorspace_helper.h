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
#include "image_effect_marco_define.h"

namespace OHOS {
namespace Media {
namespace Effect {
class ColorSpaceHelper {
public:
    IMAGE_EFFECT_EXPORT static bool IsHdrColorSpace(EffectColorSpace colorSpace);

    IMAGE_EFFECT_EXPORT
    static EffectColorSpace ConvertToEffectColorSpace(OHOS::ColorManager::ColorSpaceName colorSpaceName);
    IMAGE_EFFECT_EXPORT static OHOS::ColorManager::ColorSpaceName ConvertToColorSpaceName(EffectColorSpace colorSpace);
    IMAGE_EFFECT_EXPORT static EffectColorSpace ConvertToEffectColorSpace(
        OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType type);
    IMAGE_EFFECT_EXPORT static OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType ConvertToCMColorSpace(
        EffectColorSpace colorSpace);
    IMAGE_EFFECT_EXPORT static OH_NativeBuffer_ColorSpace ConvertToNativeBufferColorSpace(
        EffectColorSpace effectColorSpace);
    IMAGE_EFFECT_EXPORT static ErrorCode SetSurfaceBufferMetadataType(SurfaceBuffer *sb,
        const OHOS::HDI::Display::Graphic::Common::V1_0::CM_HDR_Metadata_Type &type);
    IMAGE_EFFECT_EXPORT static ErrorCode GetSurfaceBufferMetadataType(SurfaceBuffer *sb,
        OHOS::HDI::Display::Graphic::Common::V1_0::CM_HDR_Metadata_Type &type);
    IMAGE_EFFECT_EXPORT static ErrorCode SetSurfaceBufferColorSpaceType(SurfaceBuffer *sb,
        const OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType &type);
    IMAGE_EFFECT_EXPORT static ErrorCode GetSurfaceBufferColorSpaceType(SurfaceBuffer *sb,
        OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType &type);

    IMAGE_EFFECT_EXPORT
    static ErrorCode SetHDRDynamicMetadata(SurfaceBuffer *sb, const std::vector<uint8_t> &hdrDynamicMetadata);

    IMAGE_EFFECT_EXPORT
    static ErrorCode SetHDRStaticMetadata(SurfaceBuffer *sb, const std::vector<uint8_t> &hdrStaticMetadata);

    IMAGE_EFFECT_EXPORT static ErrorCode UpdateMetadata(EffectBuffer *input);
    IMAGE_EFFECT_EXPORT static ErrorCode UpdateMetadata(SurfaceBuffer *input, const EffectColorSpace &colorSpace);
    IMAGE_EFFECT_EXPORT static ErrorCode ConvertColorSpace(std::shared_ptr<EffectBuffer> &srcBuffer,
        std::shared_ptr<EffectContext> &context);
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_COLORSPACE_HELPER_H
