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

ErrorCode CpuContrastAlgo::OnApplyRGBA8888(EffectBuffer *src, EffectBuffer *dst,
    std::map<std::string, Plugin::Any> &value)
{
    EFFECT_LOGI("CpuContrastAlgo::OnApplyRGBA8888 enter!");
    CHECK_AND_RETURN_RET_LOG(src != nullptr && dst != nullptr, ErrorCode::ERR_INPUT_NULL,
        "input para is null! src=%{public}p, dst=%{public}p", src, dst);
    float contrast = ParseContrast(value);
    auto *srcRgb = static_cast<unsigned char *>(src->buffer_);
    auto *dstRgb = static_cast<unsigned char *>(dst->buffer_);

    uint32_t width = src->bufferInfo_->width_;
    uint32_t height = src->bufferInfo_->height_;

    float eps = ESP;
    if (fabs(contrast) < eps) {
        if (src != dst) {
            errno_t result = memcpy_s(dstRgb, dst->bufferInfo_->len_, srcRgb, src->bufferInfo_->len_);
            CHECK_AND_RETURN_RET_LOG(result == 0, ErrorCode::ERR_MEMCPY_FAIL, "memory copy failed: %{public}d", result);
        }
        return ErrorCode::SUCCESS;
    }
    float scale = contrast / SCALE_FACTOR;

    unsigned char lut[UNSIGHED_CHAR_DATA_RECORDS] = {0};
    for (uint32_t idx = 0; idx < UNSIGHED_CHAR_DATA_RECORDS; idx++) {
        float current = (float)idx / UNSIGHED_CHAR_MAX;
        current = current - scale * 0.1f * sin(ALGORITHM_PARAMTER_FACTOR * PI * current);
        current = CommonUtils::Clip(current, 0, 1);
        lut[idx] = (unsigned char)(current * UNSIGHED_CHAR_MAX);
    }

    uint32_t srcRowStride = src->bufferInfo_->rowStride_;
    uint32_t dstRowStride = dst->bufferInfo_->rowStride_;

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
    std::map<std::string, Plugin::Any> &value)
{
    EFFECT_LOGI("CpuContrastAlgo::OnApplyYUVNV21 enter!");
    CHECK_AND_RETURN_RET_LOG(src != nullptr && dst != nullptr, ErrorCode::ERR_INPUT_NULL,
        "input para is null! src=%{public}p, dst=%{public}p", src, dst);
    float contrast = ParseContrast(value);
    auto *srcNV21 = static_cast<unsigned char *>(src->buffer_);
    auto *dstNV21 = static_cast<unsigned char *>(dst->buffer_);

    uint32_t width = src->bufferInfo_->width_;
    uint32_t height = src->bufferInfo_->height_;

    float eps = ESP;
    if (fabs(contrast) < eps) {
        if (src != dst) {
            errno_t result = memcpy_s(dstNV21, dst->bufferInfo_->len_, srcNV21, src->bufferInfo_->len_);
            CHECK_AND_RETURN_RET_LOG(result == 0, ErrorCode::ERR_MEMCPY_FAIL, "memory copy failed: %{public}d", result);
        }
        return ErrorCode::SUCCESS;
    }
    float scale = contrast / SCALE_FACTOR;

    unsigned char lut[UNSIGHED_CHAR_DATA_RECORDS] = {0};
    for (uint32_t i = 0; i < UNSIGHED_CHAR_DATA_RECORDS; i++) {
        float current = (float)(i) / UNSIGHED_CHAR_MAX;
        current = current - scale * 0.1f * sin(ALGORITHM_PARAMTER_FACTOR * PI * current);
        current = CommonUtils::Clip(current, 0, 1);
        lut[i] = (unsigned char)(current * UNSIGHED_CHAR_MAX);
    }

    uint8_t *srcNV21UV = srcNV21 + width * height;
    uint8_t *dstNV21UV = dstNV21 + width * height;

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
    std::map<std::string, Plugin::Any> &value)
{
    EFFECT_TRACE_NAME("CpuContrastAlgo::OnApplyYUVNV12");
    EFFECT_LOGI("CpuContrastAlgo::OnApplyYUVNV12 enter!");
    CHECK_AND_RETURN_RET_LOG(src != nullptr && dst != nullptr, ErrorCode::ERR_INPUT_NULL,
        "input para is null! src=%{public}p, dst=%{public}p", src, dst);
    float contrast = ParseContrast(value);
    auto *srcNV12 = static_cast<unsigned char *>(src->buffer_);
    auto *dstNV12 = static_cast<unsigned char *>(dst->buffer_);

    uint32_t width = src->bufferInfo_->width_;
    uint32_t height = src->bufferInfo_->height_;

    float eps = ESP;
    if (fabs(contrast) < eps) {
        if (src != dst) {
            errno_t result = memcpy_s(dstNV12, dst->bufferInfo_->len_, srcNV12, src->bufferInfo_->len_);
            CHECK_AND_RETURN_RET_LOG(result == 0, ErrorCode::ERR_MEMCPY_FAIL, "memory copy failed: %{public}d", result);
        }
        return ErrorCode::SUCCESS;
    }
    float scale = contrast / SCALE_FACTOR;

    unsigned char lut[UNSIGHED_CHAR_DATA_RECORDS] = {0};
    for (uint32_t idx = 0; idx < UNSIGHED_CHAR_DATA_RECORDS; idx++) {
        float current = (float)(idx) / UNSIGHED_CHAR_MAX;
        current = current - scale * 0.1f * sin(ALGORITHM_PARAMTER_FACTOR * PI * current);
        current = CommonUtils::Clip(current, 0, 1);
        lut[idx] = (unsigned char)(current * UNSIGHED_CHAR_MAX);
    }

    uint8_t *srcNV12UV = srcNV12 + width * height;
    uint8_t *dstNV12UV = dstNV12 + width * height;

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

float CpuContrastAlgo::ParseContrast(std::map<std::string, Plugin::Any> &value)
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