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
};

enum class Category : uint32_t {
    DEFAULT = 0,
    COLOR_ADJUST,
    SHAPE_ADJUST,
    LAYER_BLEND,
    OTHER,
};

class EffectInfo {
public:
    std::map<IEffectFormat, std::vector<IPType>> formats_;
    Category category_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_EFFECT_INFO_H
