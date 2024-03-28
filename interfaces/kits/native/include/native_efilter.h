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

#ifndef IMAGE_EFFECT_NATIVE_EFILTER_H
#define IMAGE_EFFECT_NATIVE_EFILTER_H

#include <stdint.h>

#include "native_effect_errors.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OH_EFilter OH_EFilter;

typedef struct NativePixelMap_ NativePixelMap;

typedef enum OH_DataType {
    TYPE_UNKNOWN = 0,
    TYPE_INT32,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_CHAR,
    TYPE_LONG,
    TYPE_PTR,
    TYPE_BOOL,
} OH_DataType;

typedef union OH_DataValue {
    int32_t int32Value;
    float floatValue;
    double doubleValue;
    char charValue;
    long longValue;
    void *ptrValue;
    bool boolValue;
} OH_DataValue;

typedef struct OH_Any {
    OH_DataType dataType = OH_DataType::TYPE_UNKNOWN;
    OH_DataValue dataValue = { 0 };
} OH_Any;

typedef enum OH_IEffectFormat {
    UNKNOWN_FORMAT = 0,
    RGBA8888 = 1 << 0,
    NV21 = 1 << 1,
    NV12 = 1 << 2,
} OH_IEffectFormat;

typedef enum OH_Category {
    UNKNOWN_CATEGORY = 0,
    COLOR_ADJUST = 1 << 0,
    SHAPE_ADJUST = 1 << 1,
    LAYER_BLEND = 1 << 2,
    OTHER = 1 << 3,
} OH_Category;

typedef enum OH_BufferType {
    UNKNOWN_BUFFERTYPE = 0,
    PIXEL = 1 << 0,
    TEXTURE = 1 << 1,
} OH_BufferType;

typedef struct OH_EffectInfo {
    char *name = nullptr;
    uint32_t category = OH_Category::UNKNOWN_CATEGORY;
    uint32_t bufferTypes = OH_BufferType::UNKNOWN_BUFFERTYPE;
    uint32_t formats = OH_IEffectFormat::UNKNOWN_FORMAT;
    int64_t flag = 0;
} OH_EffectInfo;

typedef struct OH_FilterNames {
    uint32_t size = 0;
    const char **nameList = nullptr;
} OH_FilterNames;

typedef struct OH_EffectBuffer {
    void *buffer = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t rowStride = 0;
    uint32_t format = 0;
    int64_t flag = 0;
} OH_EffectBuffer;

typedef bool (*OH_EFilterDelegate_SetValue)(OH_EFilter *filter, const char *key, const OH_Any *value);

typedef void (*OH_EFilterDelegate_PushData)(OH_EFilter *filter, OH_EffectBuffer *dst);

typedef bool (*OH_EFilterDelegate_Render)(OH_EFilter *filter, OH_EffectBuffer *src,
    OH_EFilterDelegate_PushData pushData);

typedef bool (*OH_EFilterDelegate_Save)(OH_EFilter *filter, char **info);

typedef OH_EFilter *(*OH_EFilterDelegate_Restore)(const char *info);

typedef struct OH_EFilterDelegate {
    OH_EFilterDelegate_SetValue setValue;
    OH_EFilterDelegate_Render render;
    OH_EFilterDelegate_Save save;
    OH_EFilterDelegate_Restore restore;
} OH_EFilterDelegate;

typedef struct OH_AreaInfo {
    int32_t x0;
    int32_t y0;
    int32_t x1;
    int32_t y1;
} OH_AreaInfo;

typedef struct OH_Size {
    uint32_t width;
    uint32_t height;
} OH_Size;

OH_EFilter *OH_EFilter_Create(const char *name);

OH_EffectErrorCode OH_EFilter_SetValue(OH_EFilter *filter, const char *key, const OH_Any *value);

OH_EffectErrorCode OH_EFilter_GetValue(OH_EFilter *filter, const char *key, OH_Any *value);

OH_EffectErrorCode OH_EFilter_Register(const OH_EffectInfo *info, const OH_EFilterDelegate *delegate);

OH_FilterNames *OH_EFilter_LookupFilters(const char *key);

void OH_EFilter_ReleaseFilterNames();

OH_EffectErrorCode OH_EFilter_LookupFilterInfo(const char *name, OH_EffectInfo *info);

OH_EffectErrorCode OH_EFilter_Render(OH_EFilter *filter, NativePixelMap *inputPixel, NativePixelMap *outputPixel);

OH_EffectErrorCode OH_EFilter_Release(OH_EFilter *filter);

#ifdef __cplusplus
}
#endif
#endif // IMAGE_EFFECT_NATIVE_EFILTER_H
