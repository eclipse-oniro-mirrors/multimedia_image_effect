/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef IMAGE_EFFECT_FILTER_OPERATOR_UTILS_H
#define IMAGE_EFFECT_FILTER_OPERATOR_UTILS_H

#include <cstdint>

#define UNSIGHED_CHAR_MAX 255
#define CLIP(a, aMin, aMax) (a) > (aMax) ? (aMax) : ((a) < (aMin) ? (aMin) : (a))

namespace OHOS {
namespace Media {
namespace Effect {
class FilterOperatorUtils {
public:
    static inline uint8_t RGBToY(uint8_t r, uint8_t g, uint8_t b)
    {
        int y = (54 * r + 183 * g + 18 * b) >> 8;
        return CLIP(y, 0, UNSIGHED_CHAR_MAX);
    }

    static inline uint8_t RGBToU(uint8_t r, uint8_t g, uint8_t b)
    {
        int u = ((-29 * r - 99 * g + 128 * b) >> 8) + 128;
        return CLIP(u, 0, UNSIGHED_CHAR_MAX);
    }

    static inline uint8_t RGBToV(uint8_t r, uint8_t g, uint8_t b)
    {
        int v = ((128 * r - 116 * g - 12 * b) >> 8) + 128;
        return CLIP(v, 0, UNSIGHED_CHAR_MAX);
    }

    static inline uint8_t YuvToR(uint8_t y, uint8_t u, uint8_t v)
    {
        int r = (y + ((403 * (v - 128)) >> 8));
        return CLIP(r, 0, UNSIGHED_CHAR_MAX);
    }

    static inline uint8_t YuvToG(uint8_t y, uint8_t u, uint8_t v)
    {
        int g = (y - ((48 * (u - 128) + 120 * (v - 128)) >> 8));
        return CLIP(g, 0, UNSIGHED_CHAR_MAX);
    }

    static inline uint8_t YuvToB(uint8_t y, uint8_t u, uint8_t v)
    {
        int b = (y + ((475 * (u - 128)) >> 8));
        return CLIP(b, 0, UNSIGHED_CHAR_MAX);
    }
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_FILTER_OPERATOR_UTILS_H