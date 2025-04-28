/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef IMAGE_EFFECT_COLORSPACE_PROCESSPR_H
#define IMAGE_EFFECT_COLORSPACE_PROCESSPR_H

#include <securec.h>
#include "surface_buffer.h"
#include "error_code.h"
#include "effect_buffer.h"
#include "effect_memory.h"
#include "image_effect_marco_define.h"

namespace OHOS {
namespace Media {
namespace VideoProcessingEngine {
class ColorSpaceConverter;
}
namespace Effect {
using namespace OHOS::Media::VideoProcessingEngine;
class ColorSpaceProcessor {
public:
    IMAGE_EFFECT_EXPORT ColorSpaceProcessor();
    IMAGE_EFFECT_EXPORT ~ColorSpaceProcessor();

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
    std::shared_ptr<ColorSpaceConverter> converter = nullptr;
    std::vector<std::shared_ptr<MemoryData>> memoryDataArray_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_COLORSPACE_PROCESSPR_H