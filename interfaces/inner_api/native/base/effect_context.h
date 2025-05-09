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

#ifndef IMAGE_EFFECT_EFFECT_CONTEXT_H
#define IMAGE_EFFECT_EFFECT_CONTEXT_H

#include <unordered_set>

#include "effect_info.h"
#include "effect_memory_manager.h"
#include "render_strategy.h"
#include "capability_negotiate.h"
#include "colorspace_manager.h"
#include "efilter_cache_negotiate.h"
#include "image_effect_marco_define.h"
#include "efilter_metainfo_negotiate.h"

namespace OHOS {
namespace Media {
namespace Effect {
class RenderEnvironment;
struct EffectContext {
public:
    std::shared_ptr<EffectMemoryManager> memoryManager_;
    std::shared_ptr<RenderStrategy> renderStrategy_;
    std::shared_ptr<CapabilityNegotiate> capNegotiate_;
    std::shared_ptr<RenderEnvironment> renderEnvironment_;
    std::shared_ptr<ColorSpaceManager> colorSpaceManager_;
    std::shared_ptr<EFilterCacheNegotiate> cacheNegotiate_;
    std::shared_ptr<EfilterMetaInfoNegotiate> metaInfoNegotiate_;

    IPType ipType_ = IPType::DEFAULT;
    std::unordered_set<EffectColorSpace> filtersSupportedColorSpace_;
    std::unordered_set<HdrFormat> filtersSupportedHdrFormat_;
    LOG_STRATEGY logStrategy_ = LOG_STRATEGY::NORMAL;

    IMAGE_EFFECT_EXPORT std::shared_ptr<ExifMetadata> GetExifMetadata();

    std::shared_ptr<ExifMetadata> exifMetadata_ = nullptr;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_EFFECT_CONTEXT_H
