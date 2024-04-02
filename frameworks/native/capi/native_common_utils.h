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

#ifndef IMAGE_EFFECT_NATIVE_COMMON_UTILS_H
#define IMAGE_EFFECT_NATIVE_COMMON_UTILS_H

#include <vector>

#include "any.h"
#include "effect_info.h"
#include "error_code.h"
#include "native_effect_filter.h"
#include "image_type.h"
#include "pixel_map_napi.h"

#define EFFECT_EXPORT __attribute__((visibility("default")))

namespace OHOS {
namespace Media {
namespace Effect {
class NativeCommonUtils {
public:
    static ErrorCode ParseOHAny(const OH_Any *value, Plugin::Any &any);

    static ErrorCode SwitchToOHAny(const Plugin::Any &any, OH_Any *value);

    static void SwitchToOHFormatType(const IEffectFormat &formatType, OH_IEffectFormat &ohFormatType);

    static void SwitchToFormatType(const OH_IEffectFormat &ohFormatType, IEffectFormat &formatType);

    static void SwitchToOHFormatType(const IEffectFormat &formatType, uint32_t &ohFormatType);

    static void SwitchToFormatType(const uint32_t &ohFormatType, IEffectFormat &formatType);

    static void SwitchToOHCategory(const Category &category, OH_Category &ohCategory);

    static void SwitchToCategory(const OH_Category &ohCategory, Category &category);

    static void SwitchToCategory(const uint32_t &ohCategory, Category &category);

    static void SwitchToOHEffectInfo(const EffectInfo *effectInfo, OH_EffectInfo *ohEffectInfo);

    static PixelMap *GetPixelMapFromNativePixelMap(NativePixelMap *nativePixelMap);

    static void ParseLookupKey(std::string &key, std::vector<const char *> &matchEFilter);

    static void SwitchToEffectInfo(const OH_EffectInfo *info, std::shared_ptr<EffectInfo> &effectInfo);
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_NATIVE_COMMON_UTILS_H