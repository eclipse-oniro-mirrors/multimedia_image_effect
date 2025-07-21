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

#ifndef IMAGE_EFFECT_COLORSPACE_STRATEGY_H
#define IMAGE_EFFECT_COLORSPACE_STRATEGY_H

#include <unordered_set>

#include "effect_buffer.h"
#include "error_code.h"
#include "image_effect_marco_define.h"

namespace OHOS {
namespace Media {
namespace Effect {

class ColorSpaceStrategy {
public:
    ColorSpaceStrategy() = default;
    ~ColorSpaceStrategy() = default;

    /**
     * @brief Checks if the given color space is supported by the image effect.
     *
     * @param colorSpace The color space to be checked.
     * @return true If the color space is supported.
     * @return false If the color space is not supported.
     */
    IMAGE_EFFECT_EXPORT static bool IsSupportedColorSpace(EffectColorSpace colorSpace);

    /**
     * @brief Determines if color space conversion is needed.
     *
     * @param colorSpace The color space to be evaluated.
     * @return True if color space conversion is needed, false otherwise.
     */
    static bool IsNeedConvertColorSpace(EffectColorSpace colorSpace);

    /**
     * @brief Get the target color space for the given source color space.
     *
     * @param src The source color space.
     * @return EffectColorSpace The target color space.
     */
    static EffectColorSpace GetTargetColorSpace(EffectColorSpace src);

    /**
     * @brief Retrieves all supported color spaces.
     *
     * @return A set containing all supported color spaces.
     */
    static std::unordered_set<EffectColorSpace> GetAllSupportedColorSpaces();

    /**
     * @brief Initializes the strategy with source and destination buffers.
     *
     * @param src A shared pointer to the source effect buffer.
     * @param dst A shared pointer to the destination effect buffer.
     */
    void Init(const std::shared_ptr<EffectBuffer> &src, const std::shared_ptr<EffectBuffer> &dst);

    /**
     * Choose the appropriate color space based on filter support and source color space.
     *
     * This function takes a set of supported color spaces, the real color space of the source image,
     * and an output parameter to store the chosen color space.
     *
     * @param filtersSupportedColorSpace A set of color spaces supported by the filter.
     * @param srcRealColorSpace The real color space of the source image.
     * @param outputColorSpace An output parameter to store the chosen color space.
     * @return ErrorCode An error code indicating the success or failure of the function.
     */
    IMAGE_EFFECT_EXPORT
    ErrorCode ChooseColorSpace(const std::unordered_set<EffectColorSpace> &filtersSupportedColorSpace,
        const EffectColorSpace &srcRealColorSpace, EffectColorSpace &outputColorSpace);

    // Checks and validates the target color space for conversion
    IMAGE_EFFECT_EXPORT ErrorCode CheckConverterColorSpace(const EffectColorSpace &targetColorSpace);

    /**
     * Deinitializes the strategy
     */
    void Deinit();
private:
    std::shared_ptr<EffectBuffer> src_ = nullptr; // Source buffer
    std::shared_ptr<EffectBuffer> dst_ = nullptr; // Destination buffer
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_COLORSPACE_STRATEGY_H
