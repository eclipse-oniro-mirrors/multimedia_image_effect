/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef IMAGE_EFFECT_ERROR_CODE_H
#define IMAGE_EFFECT_ERROR_CODE_H

#include <stdint.h>

namespace OHOS {
namespace Media {
namespace Effect {
enum struct ErrorCode : int32_t {
    SUCCESS = 0,
    ERR_UNKNOWN = INT32_MIN + 0,
    ERR_INPUT_NULL = ERR_UNKNOWN + 1,
    ERR_NO_VALUE = ERR_UNKNOWN + 2,
    ERR_UNSUPPORTED_IPTYPE_FOR_EFFECT = ERR_UNKNOWN + 3,
    ERR_UNSUPPORTED_FORMAT_TYPE = ERR_UNKNOWN + 4,
    ERR_ANY_CAST_TYPE_NOT_IPTYPE = ERR_UNKNOWN + 5,
    ERR_UNSUPPORTED_CONFIG_TYPE = ERR_UNKNOWN + 6,
    ERR_INVALID_SRC_PIXELMAP = ERR_UNKNOWN + 7,
    ERR_INVALID_DST_PIXELMAP = ERR_UNKNOWN + 8,
    ERR_UNSUPPORTED_PIXEL_FORMAT = ERR_UNKNOWN + 9,
    ERR_UNSUPPORTED_DATA_TYPE = ERR_UNKNOWN + 10,
    ERR_NO_DATA = ERR_UNKNOWN + 11,
    ERR_PIXELMAP_GETIMAGEINFO_FAIL = ERR_UNKNOWN + 12,
    ERR_PIXELMAP_ACCESSPIXELS_FAIL = ERR_UNKNOWN + 13,
    ERR_UNSUPPORTED_INPUT_ANYTYPE = ERR_UNKNOWN + 14,
    ERR_UNSUPPORTED_VALUE_KEY = ERR_UNKNOWN + 15,
    ERR_ANY_CAST_TYPE_NOT_FLOAT = ERR_UNKNOWN + 16,
    ERR_VALUE_OUT_OF_RANGE = ERR_UNKNOWN + 17,
    ERR_NO_VALUE_KEY = ERR_UNKNOWN + 18,
    ERR_ANY_CAST_TYPE_NOT_MATCH = ERR_UNKNOWN + 19,
    ERR_NOT_SUPPORT_SWITCH_TO_OHANY = ERR_UNKNOWN + 20,
    ERR_JSON_DATA_TYPE = ERR_UNKNOWN + 21,
    ERR_JSON_NO_TARGET = ERR_UNKNOWN + 22,
    ERR_NO_FILTER_OPERATOR_LEVEL = ERR_UNKNOWN + 23,
    ERR_PIPELINE_INVALID_FILTER = ERR_UNKNOWN + 24,
    ERR_PIPELINE_INVALID_FILTER_PORT = ERR_UNKNOWN + 25,
    ERR_PIPELINE_INVALID_FILTER_CALLBACK = ERR_UNKNOWN + 26,
    ERR_CUSTOM_EFILTER_APPLY_FAIL = ERR_UNKNOWN + 27,
    ERR_CUSTOM_EFILTER_SETVALUE_FAIL = ERR_UNKNOWN + 28,
    ERR_CUSTOM_EFILTER_SAVE_FAIL = ERR_UNKNOWN + 29,
    ERR_CUSTOM_EFILTER_RESTORE_FAIL = ERR_UNKNOWN + 30,
    ERR_ALLOC_MEMORY_SIZE_OUT_OF_RANGE = ERR_UNKNOWN + 31,
    ERR_ALLOC_MEMORY_FAIL = ERR_UNKNOWN + 32,
    ERR_FREE_MEMORY_ADDR_ABNORMAL = ERR_UNKNOWN + 33,
    ERR_MEMCPY_FAIL = ERR_UNKNOWN + 34,
    ERR_UNSUPPORTED_RUNNINGTYPE = ERR_UNKNOWN + 35,
    ERR_INVALID_SRC_SURFACEBUFFER = ERR_UNKNOWN + 36,
    ERR_INVALID_DST_SURFACEBUFFER = ERR_UNKNOWN + 37,
    ERR_PARSE_FOR_EFFECT_BUFFER_FAIL = ERR_UNKNOWN + 38,
    ERR_NOT_SUPPORT_DIFF_DATATYPE = ERR_UNKNOWN + 39,
    ERR_CREATE_IMAGESOURCE_FAIL = ERR_UNKNOWN + 40,
    ERR_CREATE_PIXELMAP_FAIL = ERR_UNKNOWN + 41,
    ERR_EXTRA_INFO_NULL = ERR_UNKNOWN + 42,
    ERR_BUFFER_INFO_NULL = ERR_UNKNOWN + 43,
    ERR_NOT_SUPPORT_DIFF_FORMAT = ERR_UNKNOWN + 44,
    ERR_IMAGE_PACKER_EXEC_FAIL = ERR_UNKNOWN + 45,
    ERR_SET_IMAGE_INFO_EXEC_FAIL = ERR_UNKNOWN + 46,
    ERR_FILE_TYPE_NOT_SUPPORT = ERR_UNKNOWN + 47,
    ERR_INVALID_INDEX = ERR_UNKNOWN + 48,
    ERR_BUFFER_NOT_ALLOW_CHANGE = ERR_UNKNOWN + 49,
    ERR_CREATE_MEMORY_FAIL = ERR_UNKNOWN + 50,
    ERR_SET_IMAGE_INFO_FAIL = ERR_UNKNOWN + 51,
    ERR_MEMORY_DATA_ABNORMAL = ERR_UNKNOWN + 52,
    ERR_NOT_FILTERS_WITH_RENDER = ERR_UNKNOWN + 53,
    ERR_NOT_SET_INPUT_DATA = ERR_UNKNOWN + 54,
    ERR_UNSUPPORTED_INOUT_WITH_DIFF_BUFFER = ERR_UNKNOWN + 55,
    ERR_UNSUPPORTED_BUFFER_TYPE = ERR_UNKNOWN + 56,
    ERR_INVALID_SURFACE_BUFFER = ERR_UNKNOWN + 57,
    ERR_INVALID_FD = ERR_UNKNOWN + 58,
    ERR_SRC_EFFECT_BUFFER_NULL = ERR_UNKNOWN + 59,
    ERR_INVALID_PARAMETER_VALUE = ERR_UNKNOWN + 100,
    ERR_INVALID_PARAMETER_TYPE = ERR_UNKNOWN + 101,
    ERR_INVALID_OPERATION = ERR_UNKNOWN + 102,
    ERR_UNIMPLEMENTED = ERR_UNKNOWN + 103,
    ERR_TIMED_OUT = ERR_UNKNOWN + 104,
    ERR_NO_MEMORY = ERR_UNKNOWN + 105,
    ERR_PERMISSION_DENIED = ERR_UNKNOWN + 106,
    ERR_IMAGE_EFFECT_RECEIVER_INIT_FAILED = ERR_UNKNOWN + 107,
    ERR_GL_FRAMEBUFFER_NOT_COMPLETE = ERR_UNKNOWN + 200,
    ERR_GL_CREATE_TEXTURE_FAILED = ERR_UNKNOWN + 201,
    ERR_GL_CREATE_PROGRAM_FAILED = ERR_UNKNOWN + 202,
    ERR_GL_INIT_MESH_FAILED = ERR_UNKNOWN + 203,
    ERR_GL_PRE_DRAW_FAILED = ERR_UNKNOWN + 204,
    ERR_GL_DRAW_FAILED = ERR_UNKNOWN + 205,
    ERR_GL_COPY_PIXELS_FAILED = ERR_UNKNOWN + 206,
};

const char *GetErrorName(ErrorCode code);

#ifndef FALSE_RETURN_MSG_W
#define FALSE_RETURN_MSG_W(exec, ret, fmt, args...) \
    do {                                            \
        bool returnValue = (exec);                  \
        if (!returnValue) {                         \
            EFFECT_LOGW(fmt, ##args);               \
            return ret;                             \
        }                                           \
    } while (0)
#endif

#ifndef FALSE_RETURN_E
#define FALSE_RETURN_E(exec, ret)               \
    do {                                        \
        bool returnValue = (exec);              \
        if (!returnValue) {                     \
            EFFECT_LOGE("FALSE_RETURN " #exec); \
            return ret;                         \
        }                                       \
    } while (0)
#endif

#ifndef FALSE_RETURN_MSG_E
#define FALSE_RETURN_MSG_E(exec, ret, fmt, args...) \
    do {                                            \
        bool returnValue = (exec);                  \
        if (!returnValue) {                         \
            EFFECT_LOGE(fmt, ##args);               \
            return ret;                             \
        }                                           \
    } while (0)
#endif

#ifndef FAIL_RETURN
#define FAIL_RETURN(exec)                                                                    \
    do {                                                                                     \
        ErrorCode returnValue = (exec);                                                      \
        if (returnValue != ErrorCode::SUCCESS) {                                             \
            EFFECT_LOGE("FAIL_RETURN on ErrorCode(%{public}s).", GetErrorName(returnValue)); \
            return returnValue;                                                              \
        }                                                                                    \
    } while (0)
#endif
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_ERROR_CODE_H
