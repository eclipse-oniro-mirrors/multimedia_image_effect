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

#ifndef IMAGE_EFFECT_NATIVE_EFFECT_ERRORS_H
#define IMAGE_EFFECT_NATIVE_EFFECT_ERRORS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum OH_EffectErrorCode {
    EFFECT_ERR_SUCCESS = 0,
    EFFECT_ERR_INPUT_NULL,
    EFFECT_ERR_SET_OUTPUT_PIXELMAP_FAIL,
    EFFECT_ERR_SET_INPUT_PIXELMAP_FAIL,
    EFFECT_ERR_RENDER_FAIL,
    EFFECT_ERR_VALUE_TYPE_NOT_SUPPORT,
    EFFECT_ERR_CONFIG_FAIL,
    EFFECT_ERR_SET_VALUE_FAIL,
    EFFECT_ERR_GET_VALUE_FAIL,
    EFFECT_ERR_EFFECT_NOT_SUPPORT,
    EFFECT_ERR_EFILTER_RENDER_FAIL,
    EFFECT_ERR_GET_INPUT_SURFACE_FAIL,
    EFFECT_ERR_SET_OUTPUT_SURFACE_FAIL,
    EFFECT_ERR_LOOKUP_EFFECT_INFO_FAIL,
    EFFECT_ERR_STR_COPY_FAIL,
    EFFECT_ERR_SET_INPUT_SURFACEBUFFER_FAIL,
    EFFECT_ERR_SET_OUTPUT_SURFACEBUFFER_FAIL,
    EFFECT_ERR_FILE_TYPE_NOT_SUPPORT,
    EFFECT_ERR_JSON_DUMP_FAIL,
} OH_EffectErrorCode;

#ifdef __cplusplus
}
#endif
#endif // IMAGE_EFFECT_NATIVE_EFFECT_ERRORS_H
