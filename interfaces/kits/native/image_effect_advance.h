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

#ifndef NATIVE_IMAGE_EFFECT_ADVANCE_H
#define NATIVE_IMAGE_EFFECT_ADVANCE_H

#include "image_effect_filter.h"
#include "image_effect.h"
#include "image_effect_errors.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativePixelMap_ NativePixelMap;

ImageEffect_ErrorCode OH_EFilter_Render(OH_EffectFilter *filter, NativePixelMap *inputPixelmap,
    NativePixelMap *outputPixelmap);

ImageEffect_ErrorCode OH_ImageEffect_SetInputNativePixelMap(OH_ImageEffect *imageEffect, NativePixelMap *pixelmap);

ImageEffect_ErrorCode OH_ImageEffect_SetOutputNativePixelMap(OH_ImageEffect *imageEffect, NativePixelMap *pixelmap);

#ifdef __cplusplus
}
#endif
#endif // NATIVE_IMAGE_EFFECT_ADVANCE_H
