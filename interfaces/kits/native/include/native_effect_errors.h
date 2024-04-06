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

#ifndef NATIVE_EFFECT_ERRORS_H
#define NATIVE_EFFECT_ERRORS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Effect error code
* @syscap SystemCapability.Multimedia.ImageEffect.Core
* @since 12
*/

/**
 * Basic error mask for image effect.
 */
#define EFFECT_ERROR_BASE 50100000

typedef enum OH_EffectErrorCode {
    /**
     * The operation completed successfully.
     */
    EFFECT_SUCCESS = 0,
    /**
     * Permission denied.
     */
    EFFECT_ERROR_PERMISSION_DENIED = 201,
    /**
     * Invalid parameter.
     */
    EFFECT_ERROR_PARAM_INVALID = 401,
    /**
     * Warning code if input and output buffer size is not match, it will be rendered through output buffer size.
     */
    EFFECT_BUFFER_SIZE_NOT_MATCH = EFFECT_ERROR_BASE + 1,
    /**
     * Warning code if input and output color space is not match, it will be rendered by modifying the color space of
     * output image.
     */
    EFFECT_COLOR_SPACE_NOT_MATCH = EFFECT_ERROR_BASE + 2,
    /**
     * The input and output image type is not match. For example, set input OH_Pixelmap and set output NativeBuffer.
     */
    EFFECT_INPUT_OUTPUT_NOT_MATCH = EFFECT_ERROR_BASE + 101,
    /**
     * Over the max number of the filters that can be added.
     */
    EFFECT_EFFECT_NUMBER_LIMITED = EFFECT_ERROR_BASE + 102,
    /**
     * The input or output image type is not supported. For example, the pixel format beyond the current definition.
     */
    EFFECT_INPUT_OUTPUT_NOT_SUPPORT = EFFECT_ERROR_BASE + 103,
    /**
     * Allocate memory fail. For example, over sized image resource.
     */
    EFFECT_ALLOCATE_MEMORY_FAILED = EFFECT_ERROR_BASE + 104,
    /**
     * Parameter error. For example, the invalid value set for filter.
     */
    EFFECT_PARAM_ERROR = EFFECT_ERROR_BASE + 121,
    /**
     * Key error. For example, the invalid key set for filter.
     */
    EFFECT_KEY_ERROR = EFFECT_ERROR_BASE + 122,
    /**
     * Unknown error.
     */
    EFFECT_UNKNOWN = EFFECT_ERROR_BASE + 199,
} OH_EffectErrorCode;

#ifdef __cplusplus
}
#endif
#endif // NATIVE_EFFECT_ERRORS_H
