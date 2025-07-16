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

#ifndef IMAGE_EFFECT_EFFECT_INFO_H
#define IMAGE_EFFECT_EFFECT_INFO_H

#include <cstdint>
#include <map>
#include <vector>

namespace OHOS {
namespace Media {
namespace Effect {
enum class IPType : uint32_t {
    DEFAULT = 0,
    GPU,
    CPU,
};

enum class IEffectFormat : uint32_t {
    DEFAULT = 0,
    RGBA8888,
    YUVNV21,
    YUVNV12,
    RGBA_1010102,
    YCBCR_P010,
    YCRCB_P010,
    RGBA_F16,
};

enum class Category : uint32_t {
    DEFAULT = 0,
    COLOR_ADJUST,
    SHAPE_ADJUST,
    LAYER_BLEND,
    OTHER,
};

enum class EffectColorSpace : uint32_t {
    NOT_SUPPORTED = UINT32_MAX,
    DEFAULT = 0,
    SRGB,
    DISPLAY_P3,
    BT2020_HLG,
    BT2020_PQ,
    ADOBE_RGB,
    SRGB_LIMIT,
    DISPLAY_P3_LIMIT,
    BT2020_HLG_LIMIT,
    BT2020_PQ_LIMIT,
};

enum class HdrFormat : uint32_t {
    DEFAULT = 0,
    SDR,
    HDR10,
    HDR8_GAINMAP,
};

enum class LOG_STRATEGY {
    NORMAL = 0,
    LIMITED
};

class EffectInfo {
public:
    std::map<IEffectFormat, std::vector<IPType>> formats_;
    std::vector<EffectColorSpace> colorSpaces_;
    std::vector<HdrFormat> hdrFormats_;
    Category category_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_EFFECT_INFO_H
