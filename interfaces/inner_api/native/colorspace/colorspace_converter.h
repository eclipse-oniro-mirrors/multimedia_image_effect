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

#ifndef IMAGE_EFFECT_COLORSPACE_CONVERTER_H
#define IMAGE_EFFECT_COLORSPACE_CONVERTER_H

#include "error_code.h"
#include "effect_buffer.h"
#include "effect_memory.h"
#include "image_effect_marco_define.h"

namespace OHOS {
namespace Media {
namespace Effect {
class ColorSpaceConverter {
public:
    ColorSpaceConverter() = default;
    IMAGE_EFFECT_EXPORT ~ColorSpaceConverter();
    IMAGE_EFFECT_EXPORT
    ErrorCode ComposeHdrImage(const EffectBuffer *inputSdr, const SurfaceBuffer *inputGainmap, EffectBuffer *outputHdr);

    IMAGE_EFFECT_EXPORT
    ErrorCode DecomposeHdrImage(const EffectBuffer *inputHdr, std::shared_ptr<EffectBuffer> &outputSdr,
        SurfaceBuffer **outputGainmap);

    IMAGE_EFFECT_EXPORT
    ErrorCode ProcessHdrImage(const EffectBuffer *inputHdr, std::shared_ptr<EffectBuffer> &outputSdr);
    IMAGE_EFFECT_EXPORT std::shared_ptr<MemoryData> GetMemoryData(SurfaceBuffer *sb);

    IMAGE_EFFECT_EXPORT static ErrorCode ApplyColorSpace(EffectBuffer *effectBuffer, EffectColorSpace targetColorSpace);
private:
    static int32_t CreateVpeColorSpaceInstance();
    static void DestroyVpeColorSpaceInstance(int32_t vpeColorSpaceInstance);
    ErrorCode ComposeHdrImageInner(int32_t vpeColorSpaceInstance, const EffectBuffer *inputSdr,
        const SurfaceBuffer *inputGainmap, EffectBuffer *outputHdr);
    ErrorCode DecomposeHdrImageInner(int32_t vpeColorSpaceInstance, const EffectBuffer *inputHdr,
        std::shared_ptr<EffectBuffer> &outputSdr, SurfaceBuffer **outputGainmap);
    ErrorCode ProcessHdrImageInner(int32_t vpeColorSpaceInstance, const EffectBuffer *inputHdr,
        std::shared_ptr<EffectBuffer> &outputSdr);

    std::vector<std::shared_ptr<MemoryData>> memoryDataArray_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_COLORSPACE_CONVERTER_H
