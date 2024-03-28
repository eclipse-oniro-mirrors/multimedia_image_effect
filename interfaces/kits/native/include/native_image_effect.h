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

#ifndef IMAGE_EFFECT_NATIVE_IMAGE_EFFECT_H
#define IMAGE_EFFECT_NATIVE_IMAGE_EFFECT_H

#include "native_effect_errors.h"
#include "native_efilter.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OH_ImageEffect OH_ImageEffect;

typedef struct OH_NativeBuffer OH_NativeBuffer;

OH_ImageEffect *OH_ImageEffect_Create(const char *name);

OH_EFilter *OH_ImageEffect_AddFilter(OH_ImageEffect *imageEffect, const char *filterName);

OH_EFilter *OH_ImageEffect_InsertFilter(OH_ImageEffect *imageEffect, uint32_t index, const char *filterName);

int32_t OH_ImageEffect_RemoveFilter(OH_ImageEffect *imageEffect, const char *filterName);

OH_EffectErrorCode OH_ImageEffect_Configure(OH_ImageEffect *imageEffect, const char *key, const OH_Any *value);

OH_EffectErrorCode OH_ImageEffect_SetOutputSurface(OH_ImageEffect *imageEffect, const char *surfaceId);

OH_EffectErrorCode OH_ImageEffect_GetInputSurface(OH_ImageEffect *imageEffect, char **surfaceId);

OH_EffectErrorCode OH_ImageEffect_SetInputPixelMap(OH_ImageEffect *imageEffect, NativePixelMap *pixelMap);

OH_EffectErrorCode OH_ImageEffect_SetOutputPixelMap(OH_ImageEffect *imageEffect, NativePixelMap *pixelMap);

OH_EffectErrorCode OH_ImageEffect_SetInputNativeBuffer(OH_ImageEffect *imageEffect, OH_NativeBuffer *nativeBuffer);

OH_EffectErrorCode OH_ImageEffect_SetOutputNativeBuffer(OH_ImageEffect *imageEffect, OH_NativeBuffer *nativeBuffer);

OH_EffectErrorCode OH_ImageEffect_SetInputUri(OH_ImageEffect *imageEffect, const char *uri);

OH_EffectErrorCode OH_ImageEffect_SetOutputUri(OH_ImageEffect *imageEffect, const char *uri);

OH_EffectErrorCode OH_ImageEffect_SetInputPath(OH_ImageEffect *imageEffect, const char *path);

OH_EffectErrorCode OH_ImageEffect_SetOutputPath(OH_ImageEffect *imageEffect, const char *path);

OH_EffectErrorCode OH_ImageEffect_Start(OH_ImageEffect *imageEffect);

OH_EffectErrorCode OH_ImageEffect_Stop(OH_ImageEffect *imageEffect);

OH_EffectErrorCode OH_ImageEffect_Release(OH_ImageEffect *imageEffect);

OH_EffectErrorCode OH_ImageEffect_Save(OH_ImageEffect *imageEffect, char **info);

OH_ImageEffect *OH_ImageEffect_Restore(const char *info);

int32_t OH_ImageEffect_FiltersSize(OH_ImageEffect *imageEffect);

OH_EFilter *OH_ImageEffect_GetFilter(OH_ImageEffect *imageEffect, uint32_t index);

#ifdef __cplusplus
}
#endif
#endif // IMAGE_EFFECT_NATIVE_IMAGE_EFFECT_H
