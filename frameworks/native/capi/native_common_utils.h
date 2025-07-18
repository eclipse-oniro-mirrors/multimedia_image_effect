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

#ifndef IMAGE_EFFECT_NATIVE_COMMON_UTILS_H
#define IMAGE_EFFECT_NATIVE_COMMON_UTILS_H

#include <vector>

#include "any.h"
#include "effect_info.h"
#include "error_code.h"
#include "image_effect_filter.h"
#include "image_type.h"
#include "pixel_map_napi.h"
#include "picture_native_impl.h"

#define EFFECT_EXPORT __attribute__((visibility("default")))

namespace OHOS {
namespace Media {
namespace Effect {
class NativeCommonUtils {
public:
    /**
     * Parses an ImageEffect_Any object and converts it to Plugin::Any.
     *
     * @param value The input ImageEffect_Any object.
     * @param any The output Plugin::Any object.
     * @return ErrorCode indicating the result of the operation.
     */
    static ErrorCode ParseOHAny(const ImageEffect_Any *value, Plugin::Any &any);

    /**
     * Converts a Plugin::Any object to an ImageEffect_Any object.
     *
     * @param any The input Plugin::Any object.
     * @param value The output ImageEffect_Any object.
     * @return ErrorCode indicating the result of the operation.
     */
    static ErrorCode SwitchToOHAny(const Plugin::Any &any, ImageEffect_Any *value);

    /**
     * Converts an IEffectFormat object to an ImageEffect_Format object.
     *
     * @param formatType The input IEffectFormat object.
     * @param ohFormatType The output ImageEffect_Format object.
     */
    static void SwitchToOHFormatType(const IEffectFormat &formatType, ImageEffect_Format &ohFormatType);

    /**
     * Converts an ImageEffect_Format object to an IEffectFormat object.
     *
     * @param ohFormatType The input ImageEffect_Format object.
     * @param formatType The output IEffectFormat object.
     */
    static void SwitchToFormatType(const ImageEffect_Format &ohFormatType, IEffectFormat &formatType);

    /**
     * Converts an EffectInfo object to an OH_EffectFilterInfo object.
     *
     * @param effectInfo The input EffectInfo object.
     * @param ohFilterInfo The output OH_EffectFilterInfo object.
     */
    static void SwitchToOHEffectInfo(const EffectInfo *effectInfo, OH_EffectFilterInfo *ohFilterInfo);

    /**
     * Retrieves a PixelMap object from an OH_PixelmapNative object.
     *
     * @param pixelmapNative The input OH_PixelmapNative object.
     * @return A pointer to the retrieved PixelMap object.
     */
    static PixelMap *GetPixelMapFromOHPixelmap(OH_PixelmapNative *pixelmapNative);

    /**
     * Retrieves a Picture object from an OH_PictureNative object.
     *
     * @param pictureNative The input OH_PictureNative object.
     * @return A pointer to the retrieved Picture object.
     */
    static Picture *GetPictureFromNativePicture(OH_PictureNative *pictureNative);

    /**
     * Parses a lookup key and populates a vector with matching filter names.
     *
     * @param key The input lookup key string.
     * @param matchEFilter The output vector containing matching filter names.
     */
    static void ParseLookupKey(std::string &key, std::vector<const char *> &matchEFilter);

    /**
     * Converts an OH_EffectFilterInfo object to an EffectInfo object.
     *
     * @param info The input OH_EffectFilterInfo object.
     * @param effectInfo The output EffectInfo object.
     */
    static void SwitchToEffectInfo(const OH_EffectFilterInfo *info, const std::shared_ptr<EffectInfo> &effectInfo);

    /**
     * Retrieves supported formats from an OH_EffectFilterInfo object.
     *
     * @param ohFilterInfo The input OH_EffectFilterInfo object.
     * @return A uint32_t representing supported formats.
     */
    static uint32_t GetSupportedFormats(const OH_EffectFilterInfo *ohFilterInfo);

    /**
     * Reports a start failure event with an error code and message.
     *
     * @param errorCode The error code indicating the type of failure.
     * @param errorMsg The error message describing the failure.
     */
    static void ReportEventStartFailed(ImageEffect_ErrorCode errorCode, const char *errorMsg);

    /**
     * Converts an ErrorCode to an ImageEffect_ErrorCode.
     *
     * @param errorCode The input ErrorCode.
     * @return The converted ImageEffect_ErrorCode.
     */
    static ImageEffect_ErrorCode ConvertStartResult(ErrorCode errorCode);
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_NATIVE_COMMON_UTILS_H