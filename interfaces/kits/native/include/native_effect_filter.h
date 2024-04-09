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

#ifndef NATIVE_EFFECT_FILTER_H
#define NATIVE_EFFECT_FILTER_H

#include <stdint.h>
#include "native_effect_errors.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Define the new type name OH_EffectFilter for struct OH_EffectFilter
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
typedef struct OH_EffectFilter OH_EffectFilter;

/**
 * @brief Pixelmap that from images
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
typedef struct OH_Pixelmap OH_Pixelmap;

/**
 * @brief Define the brightness filter name that contain the parameter matched with the key refer to
 * {@link OH_EFFECT_FILTER_INTENSITY_KEY} and the value refer to {@link OH_EffectAny} that contain the data type of
 * {@link EFFECT_DATA_TYPE_FLOAT}
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
extern const char *OH_EFFECT_BRIGHTNESS_FILTER;

/**
 * @brief Define the contrast filter name that contain the parameter matched with the key refer to
 * {@link OH_EFFECT_FILTER_INTENSITY_KEY} and the value refer to {@link OH_EffectAny} that contain the data type of
 * {@link EFFECT_DATA_TYPE_FLOAT}
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
extern const char *OH_EFFECT_CONTRAST_FILTER;

/**
 * @brief Define the crop filter name that contain the parameter matched with the key refer to
 * {@link OH_EFFECT_FILTER_REGION_KEY} and the value refer to {@link OH_EffectAny} that contain the data type of
 * {@link EFFECT_DATA_TYPE_PTR} for @{link OH_EffectRegion}
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
extern const char *OH_EFFECT_CROP_FILTER;

/**
 * @brief Define the key that means intensity
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
extern const char *OH_EFFECT_FILTER_INTENSITY_KEY;

/**
 * @brief Define the key that means region and matches the value ref to {@link OH_EffectAny} contain the data type of
 * {@link EFFECT_DATA_TYPE_PTR} for @{link OH_EffectRegion}
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
extern const char *OH_EFFECT_FILTER_REGION_KEY;

/**
 * @brief Enumerates the data type
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
typedef enum OH_EffectDataType {
    EFFECT_DATA_TYPE_UNKNOWN = 0,           /**< Unknown data type */
    EFFECT_DATA_TYPE_INT32 = 1,             /**< int32_t data type */
    EFFECT_DATA_TYPE_FLOAT = 2,             /**< float data type */
    EFFECT_DATA_TYPE_DOUBLE = 3,            /**< double data type */
    EFFECT_DATA_TYPE_CHAR = 4,              /**< char data type */
    EFFECT_DATA_TYPE_LONG = 5,              /**< long data type */
    EFFECT_DATA_TYPE_BOOL = 6,              /**< bool data type */
    EFFECT_DATA_TYPE_PTR = 7,               /**< point data type */
} OH_EffectDataType;

/**
 * @brief Data value for the union
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
typedef union OH_EffectDataValue {
    /** Parameter of 32-bit integer value matches with {@link EFFECT_DATA_TYPE_INT32} */
    int32_t int32Value;
    /** Parameter of float value matches with {@link EFFECT_DATA_TYPE_FLOAT */
    float floatValue;
    /** Parameter of double value matches with {@link EFFECT_DATA_TYPE_DOUBLE */
    double doubleValue;
    /** Parameter of character value matches with {@link EFFECT_DATA_TYPE_CHAR */
    char charValue;
    /** Parameter of long integer value matches with {@link EFFECT_DATA_TYPE_LONG */
    long longValue;
    /** Parameter of bool value matches with {@link EFFECT_DATA_TYPE_BOOL */
    bool boolValue;
    /** Parameter of point value matches with {@link EFFECT_DATA_TYPE_PTR */
    void *ptrValue;
} OH_EffectDataValue;

/**
 * @brief Data parameter struct information
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
typedef struct OH_EffectAny {
    /** Effect any data type */
    OH_EffectDataType dataType = OH_EffectDataType::EFFECT_DATA_TYPE_UNKNOWN;
    /** Effect any data value */
    OH_EffectDataValue dataValue = { 0 };
} OH_EffectAny;

/**
 * @brief Enumerates the pixel format type
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
typedef enum OH_EffectFormat {
    EFFECT_PIXEL_FORMAT_UNKNOWN = 0,            /**< Unknown pixel format */
    EFFECT_PIXEL_FORMAT_RGBA8888 = 1,           /**< RGBA8888 pixel format */
    EFFECT_PIXEL_FORMAT_NV21 = 2,               /**< NV21 pixel format */
    EFFECT_PIXEL_FORMAT_NV12 = 3,               /**< NV12 pixel format */
    EFFECT_PIXEL_FORMAT_RGBA1010102 = 4,        /**< RGBA 10bit pixel format */
    EFFECT_PIXEL_FORMAT_YCBCR_P010 = 5,         /**< YCBCR420 semi-planar 10bit pixel format */
    EFFECT_PIXEL_FORMAT_YCRCB_P010 = 6,         /**< YCRCB420 semi-planar 10bit pixel format */
} OH_EffectFormat;

/**
 * @brief Enumerates the effect buffer type
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
typedef enum OH_EffectBufferType {
    EFFECT_BUFFER_TYPE_UNKNOWN = 0,             /**< Unknown buffer type */
    EFFECT_BUFFER_TYPE_PIXEL = 1,               /**< Pixel buffer type */
    EFFECT_BUFFER_TYPE_TEXTURE = 2,             /**< Texture buffer type */
} OH_EffectBufferType;

/**
 * @brief Define the new type name OH_EffectFilterInfo for struct OH_EffectFilterInfo
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
typedef struct OH_EffectFilterInfo OH_EffectFilterInfo;

/**
 * @brief Create an OH_EffectFilterInfo instance. It should be noted that the life cycle of the OH_EffectFilterInfo
 * instance pointed to by the return value * needs to be manually released by {@link OH_EffectFilter_ReleaseInfo}
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @return Returns a pointer to an OH_EffectFilterInfo instance if the execution is successful, otherwise returns
 * nullptr
 * @since 12
 */
OH_EffectFilterInfo *OH_EffectFilter_CreateInfo();

/**
 * @brief Set the filter name for OH_EffectFilterInfo structure
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectFilterInfo structure instance pointer
 * @param name Indicates the filter name
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_InfoSetFilterName(OH_EffectFilterInfo *info, const char *name);

/**
 * @brief Get the filter name from OH_EffectFilterInfo structure
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectFilterInfo structure instance pointer
 * @param name Indicates the filter name
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_InfoGetFilterName(OH_EffectFilterInfo *info, char **name);

/**
 * @brief Set the supported buffer types for OH_EffectFilterInfo structure
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectFilterInfo structure instance pointer
 * @param size The size of {@link OH_EffectBufferType} that can be supported
 * @param bufferTypeArray Array of {@link OH_EffectBufferType} that can be supported
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_InfoSetSupportedBufferTypes(OH_EffectFilterInfo *info, uint32_t size,
    OH_EffectBufferType *bufferTypeArray);

/**
 * @brief Get the supported buffer types from OH_EffectFilterInfo structure
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectFilterInfo structure instance pointer
 * @param size The size of {@link OH_EffectBufferInfoType} that can be supported
 * @param bufferTypeArray Array of {@link OH_EffectBufferInfoType} that can be supported
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_InfoGetSupportedBufferTypes(OH_EffectFilterInfo *info, uint32_t *size,
    OH_EffectBufferType **bufferTypeArray);

/**
 * @brief Set the supported formats for OH_EffectFilterInfo structure
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectFilterInfo structure instance pointer
 * @param size The size of {@link OH_EffectFormat} that can be supported
 * @param formatArray Array of {@link OH_EffectFormat} that can be supported
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_InfoSetSupportedFormats(OH_EffectFilterInfo *info, uint32_t size,
    OH_EffectFormat *formatArray);

/**
 * @brief Get the supported formats from OH_EffectFilterInfo structure
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectFilterInfo structure instance pointer
 * @param size The size of {@link OH_EffectFormat} that can be supported
 * @param formatArray Array of {@link OH_EffectFormat} that can be supported
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_InfoGetSupportedFormats(OH_EffectFilterInfo *info, uint32_t *size,
    OH_EffectFormat **formatArray);

/**
 * @brief Clear the internal resources of the OH_EffectFilterInfo and destroy the OH_EffectFilterInfo instance
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectFilterInfo structure instance pointer
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_ReleaseInfo(OH_EffectFilterInfo *info);

/**
 * @brief EffectFilter names information
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
typedef struct OH_EffectFilterNames {
    /** EffectFilter names array size */
    uint32_t size = 0;
    /** EffectFilter names memory block */
    const char **nameList = nullptr;
} OH_EffectFilterNames;

/**
 * @brief Define the new type name OH_EffectBufferInfo for struct OH_EffectBufferInfo
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
typedef struct OH_EffectBufferInfo OH_EffectBufferInfo;

/**
 * @brief Create an OH_EffectBufferInfo instance. It should be noted that the life cycle of the OH_EffectBufferInfo
 * instance pointed to by the return value * needs to be manually released by {@link OH_EffectFilter_ReleaseBufferInfo}
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @return Returns a pointer to an OH_EffectBufferInfo instance if the execution is successful, otherwise returns
 * nullptr
 * @since 12
 */
OH_EffectBufferInfo *OH_EffectFilter_CreateBufferInfo();

/**
 * @brief Set access to the address of the image in memory
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectBufferInfo structure instance pointer
 * @param addr Indicates the address of the image in memory
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_BufferInfoSetAddr(OH_EffectBufferInfo *info, void *addr);

/**
 * @brief Provide direct access to the address of the image in memory for rendering the filter effects
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectBufferInfo structure instance pointer
 * @param addr Indicates the address of the image in memory
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_BufferInfoGetAddr(OH_EffectBufferInfo *info, void **addr);

/**
 * @brief Set the width of the image in pixels
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectBufferInfo structure instance pointer
 * @param width Indicates the width of the image
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_BufferInfoSetWidth(OH_EffectBufferInfo *info, int32_t width);

/**
 * @brief Get the width of the image in pixels
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectBufferInfo structure instance pointer
 * @param width Indicates the width of the image
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_BufferInfoGetWidth(OH_EffectBufferInfo *info, int32_t *width);

/**
 * @brief Set the height of the image in pixels
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectBufferInfo structure instance pointer
 * @param height Indicates the height of the image
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_BufferInfoSetHeight(OH_EffectBufferInfo *info, int32_t height);

/**
 * @brief Get the height of the image in pixels
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectBufferInfo structure instance pointer
 * @param height Indicates the height of the image
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_BufferInfoGetHeight(OH_EffectBufferInfo *info, int32_t *height);

/**
 * @brief Set number of bytes per row for the image
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectBufferInfo structure instance pointer
 * @param rowSize Indicates number of bytes per row
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_BufferInfoSetRowSize(OH_EffectBufferInfo *info, int32_t rowSize);

/**
 * @brief Get number of bytes per row for the image
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectBufferInfo structure instance pointer
 * @param rowSize Indicates number of bytes per row
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_BufferInfoGetRowSize(OH_EffectBufferInfo *info, int32_t *rowSize);

/**
 * @brief Set the format of the image for OH_EffectBufferInfo
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectBufferInfo structure instance pointer
 * @param format Indicates {@link OH_EffectFormat} of the image
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_BufferInfoSetEffectFormat(OH_EffectBufferInfo *info, OH_EffectFormat format);

/**
 * @brief Get the format of the image from OH_EffectBufferInfo
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectBufferInfo structure instance pointer
 * @param format Indicates {@link OH_EffectFormat} of the image
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_BufferInfoGetEffectFormat(OH_EffectBufferInfo *info, OH_EffectFormat *format);

/**
 * @brief Clear the internal resources of the OH_EffectBufferInfo and destroy the OH_EffectBufferInfo instance
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Encapsulate OH_EffectBufferInfo structure instance pointer
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_ReleaseBufferInfo(OH_EffectBufferInfo *info);

/**
 * @brief When executing the method of {@link OH_EffectFilter_SetValue} for the delegate filter, the function pointer
 * will be called for checking the parameters is valid for the delegate filter
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param filter Encapsulate OH_EffectFilter structure instance pointer
 * @param key Indicates the key of the filter
 * @param value Indicates the value corresponding to the key of the filter
 * @return Returns true if the parameter is valid, otherwise returns false
 * @since 12
 */
typedef bool (*OH_EffectFilterDelegate_SetValue)(OH_EffectFilter *filter, const char *key, const OH_EffectAny *value);

/**
 * @brief Actively execute this callback function at the end of invoking the method of
 * {@link OH_EffectFilterDelegate_Render}
 * for passing possible new OH_EffectBufferInfo to the next filter. It should be noted that when passing new
 * OH_EffectBufferInfo, the buffer in OH_EffectBufferInfo needs to be manually released after the execution of the
 * function ends
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param filter Encapsulate OH_EffectFilter structure instance pointer
 * @param info Indicates the information of the image, such as width, height, etc. See {@link OH_EffectBufferInfo}
 * @since 12
 */
typedef void (*OH_EffectFilterDelegate_PushData)(OH_EffectFilter *filter, OH_EffectBufferInfo *info);

/**
 * @brief When the method of OH_ImageEffect_Start is executed on delegate filter that is contained in OH_ImageEffect,
 * the function pointer will be called for rendering the delegate filter effects
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param filter Encapsulate OH_EffectFilter structure instance pointer
 * @param info Indicates the information of the image, such as width, height, etc. See {@link OH_EffectBufferInfo}
 * @param pushData Indicates the callback function for passing possible new OH_EffectBufferInfo to the next filter. See
 * {@link OH_EffectFilterDelegate_PushData}
 * @return Returns true if this function point is executed successfully, otherwise returns false
 * @since 12
 */
typedef bool (*OH_EffectFilterDelegate_Render)(OH_EffectFilter *filter, OH_EffectBufferInfo *info,
    OH_EffectFilterDelegate_PushData pushData);

/**
 * @brief When the method of OH_ImageEffect_Save is executed on delegate filter that is contained in OH_ImageEffect,
 * the function pointer will be called for serializing the delegate filter parameters
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param filter Encapsulate OH_EffectFilter structure instance pointer
 * @param info Indicates the serialized information that is obtained by converting the delegate filter parameters to
 * JSON string
 * @return Returns true if this function point is executed successfully, otherwise returns false
 * @since 12
 */
typedef bool (*OH_EffectFilterDelegate_Save)(OH_EffectFilter *filter, char **info);

/**
 * @brief When the method of OH_ImageEffect_Restore is executed on delegate filter that is contained in OH_ImageEffect,
 * the function pointer will be called for deserializing the delegate filter parameters
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Indicates the serialized information that is obtained by converting the delegate filter parameters to
 * JSON string
 * @return Returns a pointer to an OH_EffectFilter instance if the execution is successful, otherwise returns nullptr
 * @since 12
 */
typedef OH_EffectFilter *(*OH_EffectFilterDelegate_Restore)(const char *info);

/**
 * @brief A collection of all callback function pointers in OH_EffectFilter. Register an instance of this structure to
 * the OH_EffectFilter instance by invoking {@link OH_EffectFilter_Register}, and perform related rendering operations
 * through the callback
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param setValue Monitor checking parameters, refer to {@link OH_EffectFilterDelegate_SetValue}
 * @param render Monitor render, refer to {@link OH_EffectFilterDelegate_Render}
 * @param save Monitor serialize, refer to {@link OH_EffectFilterDelegate_Save}
 * @param restore Monitor deserialize, refer to {@link OH_EffectFilterDelegate_Restore}
 * @since 12
 */
typedef struct OH_EffectFilterDelegate {
    OH_EffectFilterDelegate_SetValue setValue;
    OH_EffectFilterDelegate_Render render;
    OH_EffectFilterDelegate_Save save;
    OH_EffectFilterDelegate_Restore restore;
} OH_EffectFilterDelegate;

/**
 * @brief Describes the region information
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
typedef struct OH_EffectRegion {
    /** X coordinate of the start point of a line */
    int32_t x0;
    /** Y coordinate of the start point of a line */
    int32_t y0;
    /** X coordinate of the end point of a line */
    int32_t x1;
    /** Y coordinate of the end point of a line */
    int32_t y1;
} OH_EffectRegion;

/**
 * @brief Describes the image size information
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
typedef struct OH_EffectSize {
    /** Image width, in pixels */
    int32_t width;
    /** Image height, in pixels */
    int32_t height;
} OH_EffectSize;

/**
 * @brief Create an OH_EffectFilter instance. It should be noted that the life cycle of the OH_EffectFilter instance
 * pointed to by the return value * needs to be manually released by {@link OH_EffectFilter_Release}
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param name Indicates the filter name. For example, see {@link OH_EFFECT_BRIGHTNESS_FILTER}
 * @return Returns a pointer to an OH_EffectFilter instance if the execution is successful, otherwise returns nullptr
 * @since 12
 */
OH_EffectFilter *OH_EffectFilter_Create(const char *name);

/**
 * @brief Set the filter parameter. It can be set multiple parameters by invoking this function multiple times
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param filter Encapsulate OH_EffectFilter structure instance pointer
 * @param key Indicates the key of the filter. For example, see {@link OH_EFFECT_FILTER_INTENSITY_KEY}
 * @param value Indicates the value corresponding to the key of the filter
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_SetValue(OH_EffectFilter *filter, const char *key, const OH_EffectAny *value);

/**
 * @brief Get the filter parameter
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param filter Encapsulate OH_EffectFilter structure instance pointer
 * @param key Indicates the key of the filter
 * @param value Indicates the value corresponding to the key of the filter
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_GetValue(OH_EffectFilter *filter, const char *key, OH_EffectAny *value);

/**
 * @brief Register the delegate filter
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param info Indicates the capabilities supported by delegate filter, see {@link OH_EffectFilterInfo}
 * @param delegate A collection of all callback functions, see {@link OH_EffectFilterDelegate}
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_Register(const OH_EffectFilterInfo *info, const OH_EffectFilterDelegate *delegate);

/**
 * @brief Lookup for the filter names that matches the lookup condition. It should be noted that the allocated memory of
 * OH_EffectFilterNames can be manually released by invoking {@link OH_EffectFilter_ReleaseFilterNames} if need
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param key Indicates the lookup condition
 * @return Returns Filter name array that matches the key, see {@link OH_EffectFilterNames}
 * @since 12
 */
OH_EffectFilterNames *OH_EffectFilter_LookupFilters(const char *key);

/**
 * @brief Clear the internal cached resources of the OH_EffectFilterNames
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @since 12
 */
void OH_EffectFilter_ReleaseFilterNames();

/**
 * @brief Lookup for the capabilities that supported by the filter
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param name Indicates the filter name
 * @param info Indicates the capabilities supported by the filter, see {@link OH_EffectFilterInfo}
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_LookupFilterInfo(const char *name, OH_EffectFilterInfo *info);

/**
 * @brief Render the filter effects. The function is designed to support the same input and output image
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param filter Encapsulate OH_EffectFilter structure instance pointer
 * @param inputPixelmap Indicates the input image
 * @param outputPixelmap Indicates the output image
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_Render(OH_EffectFilter *filter, OH_Pixelmap *inputPixelmap,
    OH_Pixelmap *outputPixelmap);

/**
 * @brief Clear the internal resources of the OH_EffectFilter and destroy the OH_EffectFilter instance
 * @syscap SystemCapability.Multimedia.ImageEffect.Core
 * @param filter Encapsulate OH_EffectFilter structure instance pointer
 * @return Returns EFFECT_SUCCESS if the execution is successful, otherwise returns a specific error code, refer to
 * {@link OH_EffectErrorCode}
 * @since 12
 */
OH_EffectErrorCode OH_EffectFilter_Release(OH_EffectFilter *filter);

#ifdef __cplusplus
}
#endif
#endif // NATIVE_EFFECT_FILTER_H
