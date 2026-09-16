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

#include "cpu_contrast_algo.h"

#include <cmath>

#include "common_utils.h"
#include "effect_log.h"
#include "format_helper.h"
#include "securec.h"
#include "effect_trace.h"

namespace OHOS {
namespace Media {
namespace Effect {

constexpr float ESP = 1e-5;
constexpr uint32_t SCALE_FACTOR = 100;
constexpr uint32_t UNSIGHED_CHAR_DATA_RECORDS = 256;
constexpr uint32_t BYTES_PER_INT = 4;
constexpr uint32_t RGBA_ALPHA_INDEX = 3;
constexpr double PI = 3.14159265;
constexpr uint32_t ALGORITHM_PARAMTER_FACTOR = 2;
const int RGBA_SIZE = 4;

ErrorCode ContrastCheckBufferInfolen(EffectBuffer *src, EffectBuffer *dst, uint32_t src_width, uint32_t src_height)
{
    uint32_t dst_width = dst->bufferInfo_->width_;
    uint32_t dst_height = dst->bufferInfo_->height_;
    uint64_t dst_required = 0;
    uint64_t src_required = 0;
    if (!SafeMul3(dst_width, dst_height, RGBA_SIZE, dst_required) ||
        !SafeMul3(src_width, src_height, RGBA_SIZE, src_required) ||
        dst->bufferInfo_->len_ < dst_required || src->bufferInfo_->len_ < src_required ||
        dst->bufferInfo_->len_ < src->bufferInfo_->len_) {
        EFFECT_LOGE("ContrastCheckBufferInfolen: buffer check fail! dstLen=%{public}u, srcLen=%{public}u",
            dst->bufferInfo_->len_, src->bufferInfo_->len_);
        return ErrorCode::ERR_INVALID_PARAMETER_VALUE;
    }
    return ErrorCode::SUCCESS;
}

bool ContrastCheckStride(EffectBuffer *src, EffectBuffer *dst, uint32_t height)
{
    uint32_t srcRowStride = src->bufferInfo_->rowStride_;
    uint32_t dstRowStride = dst->bufferInfo_->rowStride_;
    uint64_t srcTotalSize = 0;
    uint64_t dstTotalSize = 0;
    if (srcRowStride == 0 || dstRowStride == 0 ||
        !SafeMul(srcRowStride, height, srcTotalSize) || srcTotalSize > src->bufferInfo_->len_ ||
        !SafeMul(dstRowStride, height, dstTotalSize) || dstTotalSize > dst->bufferInfo_->len_) {
        EFFECT_LOGE("ContrastCheckStride: invalid stride! srcRowStride=%{public}u, dstRowStride=%{public}u, "
            "height=%{public}u, srcLen=%{public}u, dstLen=%{public}u",
            srcRowStride, height, dstRowStride, src->bufferInfo_->len_, dst->bufferInfo_->len_);
        return false;
    }
    return true;
}

bool ContrastCheckYUVOffset(uint32_t width, uint32_t height, uint64_t &uvOffset)
{
    if (!SafeMul(width, height, uvOffset)) {
        EFFECT_LOGE("ContrastCheckYUVOffset: width*height overflow! width=%{public}u, height=%{public}u",
            width, height);
        return false;
    }
    return true;
}

void ContrastBuildLUT(float scale, unsigned char lut[])
{
    for (uint32_t idx = 0; idx < UNSIGHED_CHAR_DATA_RECORDS; idx++) {
        float current = (float)idx / UNSIGHED_CHAR_MAX;
        current = current - scale * 0.1f * sin(ALGORITHM_PARAMTER_FACTOR * PI * current);
        current = CommonUtils::Clip(current, 0, 1);
        lut[idx] = (unsigned char)(current * UNSIGHED_CHAR_MAX);
    }
}

bool ContrastCopyIfZero(float contrast, EffectBuffer *src, EffectBuffer *dst)
{
    if (fabs(contrast) < ESP) {
        if (src != dst) {
            errno_t result = memcpy_s(dst->buffer_, dst->bufferInfo_->len_, src->buffer_, src->bufferInfo_->len_);
            CHECK_AND_RETURN_RET_LOG(result == 0, false, "memory copy failed: %{public}d", result);
        }
        return true;
    }
    return false;
}

ErrorCode CpuContrastAlgo::OnApplyRGBA8888(EffectBuffer *src, EffectBuffer *dst,
    std::map<std::string, Any> &value, std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGI("CpuContrastAlgo::OnApplyRGBA8888 enter!");
    CHECK_AND_RETURN_RET_LOG(src != nullptr && dst != nullptr, ErrorCode::ERR_INPUT_NULL, "input para is null!");
    float contrast = ParseContrast(value);
    auto *srcRgb = static_cast<unsigned char *>(src->buffer_);
    auto *dstRgb = static_cast<unsigned char *>(dst->buffer_);

    uint32_t width = src->bufferInfo_->width_;
    uint32_t height = src->bufferInfo_->height_;

    CHECK_AND_RETURN_RET(ContrastCheckBufferInfolen(src, dst, width, height) == ErrorCode::SUCCESS,
        ErrorCode::ERR_INVALID_PARAMETER_VALUE);
    if (ContrastCopyIfZero(contrast, src, dst)) {
        return ErrorCode::SUCCESS;
    }
    float scale = contrast / SCALE_FACTOR;

    unsigned char lut[UNSIGHED_CHAR_DATA_RECORDS] = {0};
    ContrastBuildLUT(scale, lut);

    uint32_t srcRowStride = src->bufferInfo_->rowStride_;
    uint32_t dstRowStride = dst->bufferInfo_->rowStride_;
    if (!ContrastCheckStride(src, dst, height)) {
        return ErrorCode::ERR_INVALID_PARAMETER_VALUE;
    }
    
    if (srcRowStride * (height - 1) + (width - 1) * BYTES_PER_INT + BYTES_PER_INT >  dst->bufferInfo_->len_ ||
    dstRowStride * (height - 1) + (width - 1) * BYTES_PER_INT + BYTES_PER_INT > src->bufferInfo_->len_) {
        return ErrorCode::ERR_INVALID_PARAMETER_VALUE;
    }
#pragma omp parallel for default(none) shared(height, width, dstRgb, srcRgb, lut, srcRowStride, dstRowStride)
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            for (uint32_t i = 0; i < BYTES_PER_INT; ++i) {
                uint32_t srcIndex = srcRowStride * y + x * BYTES_PER_INT + i;
                uint32_t dstIndex = dstRowStride * y + x * BYTES_PER_INT + i;
                dstRgb[dstIndex] = (i == RGBA_ALPHA_INDEX) ? srcRgb[srcIndex] : lut[srcRgb[srcIndex]];
            }
        }
    }

    return ErrorCode::SUCCESS;
}

ErrorCode CpuContrastAlgo::OnApplyYUVNV21(EffectBuffer *src, EffectBuffer *dst,
    std::map<std::string, Any> &value, std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGI("CpuContrastAlgo::OnApplyYUVNV21 enter!");
    CHECK_AND_RETURN_RET_LOG(src != nullptr && dst != nullptr, ErrorCode::ERR_INPUT_NULL, "input para is null!");
    float contrast = ParseContrast(value);
    auto *srcNV21 = static_cast<unsigned char *>(src->buffer_);
    auto *dstNV21 = static_cast<unsigned char *>(dst->buffer_);

    uint32_t width = src->bufferInfo_->width_;
    uint32_t height = src->bufferInfo_->height_;

    if (ContrastCopyIfZero(contrast, src, dst)) {
        return ErrorCode::SUCCESS;
    }
    float scale = contrast / SCALE_FACTOR;

    unsigned char lut[UNSIGHED_CHAR_DATA_RECORDS] = {0};
    ContrastBuildLUT(scale, lut);

    uint64_t uvOffset = 0;
    if (!ContrastCheckYUVOffset(width, height, uvOffset)) {
        return ErrorCode::ERR_INVALID_PARAMETER_VALUE;
    }
    uint8_t *srcNV21UV = srcNV21 + uvOffset;
    uint8_t *dstNV21UV = dstNV21 + uvOffset;

#pragma omp parallel for default(none) shared(height, width, srcNV21, dstNV21, lut)
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            uint32_t y_index = i * width + j;
            uint32_t nv_index = i / 2 * width + j - j % 2; // 2 mean u/v split factor

            uint8_t y = srcNV21[y_index];
            uint8_t v = srcNV21UV[nv_index];
            uint8_t u = srcNV21UV[nv_index + 1];
            uint8_t r = FormatHelper::YuvToR(y, u, v);
            uint8_t g = FormatHelper::YuvToG(y, u, v);
            uint8_t b = FormatHelper::YuvToB(y, u, v);
            r = lut[r];
            g = lut[g];
            b = lut[b];
            dstNV21[y_index] = FormatHelper::RGBToY(r, g, b);
            dstNV21UV[nv_index] = FormatHelper::RGBToV(r, g, b);
            dstNV21UV[nv_index + 1] = FormatHelper::RGBToU(r, g, b);
        }
    }
    return ErrorCode::SUCCESS;
}

ErrorCode CpuContrastAlgo::OnApplyYUVNV12(EffectBuffer *src, EffectBuffer *dst,
    std::map<std::string, Any> &value, std::shared_ptr<EffectContext> &context)
{
    EFFECT_TRACE_NAME("CpuContrastAlgo::OnApplyYUVNV12");
    EFFECT_LOGI("CpuContrastAlgo::OnApplyYUVNV12 enter!");
    CHECK_AND_RETURN_RET_LOG(src != nullptr && dst != nullptr, ErrorCode::ERR_INPUT_NULL, "input para is null!");
    float contrast = ParseContrast(value);
    auto *srcNV12 = static_cast<unsigned char *>(src->buffer_);
    auto *dstNV12 = static_cast<unsigned char *>(dst->buffer_);

    uint32_t width = src->bufferInfo_->width_;
    uint32_t height = src->bufferInfo_->height_;

    if (ContrastCopyIfZero(contrast, src, dst)) {
        return ErrorCode::SUCCESS;
    }
    float scale = contrast / SCALE_FACTOR;

    unsigned char lut[UNSIGHED_CHAR_DATA_RECORDS] = {0};
    ContrastBuildLUT(scale, lut);

    uint64_t uvOffset = 0;
    if (!ContrastCheckYUVOffset(width, height, uvOffset)) {
        return ErrorCode::ERR_INVALID_PARAMETER_VALUE;
    }
    uint8_t *srcNV12UV = srcNV12 + uvOffset;
    uint8_t *dstNV12UV = dstNV12 + uvOffset;

#pragma omp parallel for default(none) shared(height, width, srcNV12, dstNV12, lut)
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            uint32_t y_index = i * width + j;
            uint32_t nv_index = i / 2 * width + j - j % 2; // 2 mean u/v split factor

            uint8_t y = srcNV12[y_index];
            uint8_t u = srcNV12UV[nv_index];
            uint8_t v = srcNV12UV[nv_index + 1];
            uint8_t r = FormatHelper::YuvToR(y, u, v);
            uint8_t g = FormatHelper::YuvToG(y, u, v);
            uint8_t b = FormatHelper::YuvToB(y, u, v);
            r = lut[r];
            g = lut[g];
            b = lut[b];

            dstNV12[y_index] = FormatHelper::RGBToY(r, g, b);
            dstNV12UV[nv_index] = FormatHelper::RGBToU(r, g, b);
            dstNV12UV[nv_index + 1] = FormatHelper::RGBToV(r, g, b);
        }
    }
    return ErrorCode::SUCCESS;
}

float CpuContrastAlgo::ParseContrast(std::map<std::string, Any> &value)
{
    float contrast = 0.f;
    ErrorCode res = CommonUtils::GetValue("FilterIntensity", value, contrast);
    if (res != ErrorCode::SUCCESS) {
        EFFECT_LOGW("get value fail! res=%{public}d. use default value: %{public}f", res, contrast);
    }
    EFFECT_LOGI("get value success! contrast=%{public}f", contrast);
    return contrast;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS