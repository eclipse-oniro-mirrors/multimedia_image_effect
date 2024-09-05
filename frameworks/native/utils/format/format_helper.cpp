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

#include "format_helper.h"

#include "effect_log.h"

namespace {
    const float YUV_BYTES_PER_PIXEL = 1.5f;
    const int32_t RGBA_BYTES_PER_PIXEL = 4;
    const int32_t P10_BYTES_PER_LUMA = 2;
    const int32_t R = 0;
    const int32_t G = 1;
    const int32_t B = 2;
    const int32_t A = 3;
    const int32_t UV_SPLIT_FACTOR = 2;
}

namespace OHOS {
namespace Media {
namespace Effect {

void ConvertRGBAToNV12(FormatConverterInfo &src, FormatConverterInfo &dst);
void ConvertRGBAToNV21(FormatConverterInfo &src, FormatConverterInfo &dst);
void ConvertNV12ToRGBA(FormatConverterInfo &src, FormatConverterInfo &dst);
void ConvertNV21ToRGBA(FormatConverterInfo &src, FormatConverterInfo &dst);

using FormatConverterFunc = std::function<void(FormatConverterInfo &src, FormatConverterInfo &dst)>;

struct FormatConverter {
    IEffectFormat srcFormat;
    IEffectFormat dstFormat;
    FormatConverterFunc converterFunc;
};

static const std::vector<FormatConverter> FORMAT_CONVERTER = {
    FormatConverter{ IEffectFormat::RGBA8888, IEffectFormat::YUVNV12, ConvertRGBAToNV12 },
    FormatConverter{ IEffectFormat::RGBA8888, IEffectFormat::YUVNV21, ConvertRGBAToNV21 },
    FormatConverter{ IEffectFormat::YUVNV12, IEffectFormat::RGBA8888, ConvertNV12ToRGBA },
    FormatConverter{ IEffectFormat::YUVNV21, IEffectFormat::RGBA8888, ConvertNV21ToRGBA },
};

static const std::unordered_set<IEffectFormat> SUPPORTED_FORMATS = {
    IEffectFormat::RGBA8888,
    IEffectFormat::YUVNV12,
    IEffectFormat::YUVNV21,
    IEffectFormat::RGBA_1010102,
    IEffectFormat::YCRCB_P010,
    IEffectFormat::YCBCR_P010,
};

uint32_t FormatHelper::CalculateDataRowCount(uint32_t height, IEffectFormat format)
{
    switch (format) {
        case IEffectFormat::RGBA8888:
        case IEffectFormat::RGBA_1010102:
            return height;
        case IEffectFormat::YUVNV12:
        case IEffectFormat::YUVNV21:
        case IEffectFormat::YCBCR_P010:
        case IEffectFormat::YCRCB_P010:
            return static_cast<uint32_t>(height * YUV_BYTES_PER_PIXEL);
        default:
            return height;
    }
}

uint32_t FormatHelper::CalculateRowStride(uint32_t width, IEffectFormat format)
{
    switch (format) {
        case IEffectFormat::RGBA8888:
        case IEffectFormat::RGBA_1010102:
            return width * RGBA_BYTES_PER_PIXEL;
        case IEffectFormat::YUVNV12:
        case IEffectFormat::YUVNV21:
            return width;
        case IEffectFormat::YCRCB_P010:
        case IEffectFormat::YCBCR_P010:
            return width * P10_BYTES_PER_LUMA;
        default:
            return width;
    }
}

uint32_t FormatHelper::CalculateSize(uint32_t width, uint32_t height, IEffectFormat format)
{
    return CalculateDataRowCount(height, format) * CalculateRowStride(width, format);
}

std::unordered_set<IEffectFormat> FormatHelper::GetAllSupportedFormats()
{
    return SUPPORTED_FORMATS;
}

bool FormatHelper::IsSupportConvert(IEffectFormat srcFormat, IEffectFormat dstFormat)
{
    return std::any_of(FORMAT_CONVERTER.begin(), FORMAT_CONVERTER.end(), [=](const FormatConverter &converter) {
        return converter.srcFormat == srcFormat && converter.dstFormat == dstFormat;
    });
}

FormatConverterFunc GetFormatConverterFunc(IEffectFormat srcFormat, IEffectFormat dstFormat)
{
    for (const auto &converter : FORMAT_CONVERTER) {
        if (converter.srcFormat == srcFormat && converter.dstFormat == dstFormat) {
            return converter.converterFunc;
        }
    }

    return nullptr;
}

ErrorCode CheckConverterInfo(FormatConverterInfo &src, FormatConverterInfo &dst)
{
    CHECK_AND_RETURN_RET_LOG(src.buffer != nullptr && dst.buffer != nullptr, ErrorCode::ERR_PARAM_INVALID,
        "CheckConverterInfo: invalid buffer! srcBuf=%{public}p, dstBuf=%{public}p", src.buffer, dst.buffer);

    BufferInfo &srcBuffInfo = src.bufferInfo;
    BufferInfo &dstBuffInfo = dst.bufferInfo;
    if (srcBuffInfo.width_ != dstBuffInfo.width_ || srcBuffInfo.height_ != dstBuffInfo.height_) {
        EFFECT_LOGW("CheckConverterInfo: diff size! srcW=%{public}d, srcH=%{public}d, dstW=%{public}d, dstH=%{public}d",
            srcBuffInfo.width_, srcBuffInfo.height_, dstBuffInfo.width_, dstBuffInfo.height_);
    }

    EFFECT_LOGD("CheckConverterInfo: src{w=%{public}d h=%{public}d format=%{public}d len=%{public}d stride=%{public}d},"
        " dts={w=%{public}d h=%{public}d format=%{public}d len=%{public}d stride=%{public}d}",
        srcBuffInfo.width_, srcBuffInfo.height_, srcBuffInfo.formatType_, srcBuffInfo.len_, srcBuffInfo.rowStride_,
        dstBuffInfo.width_, dstBuffInfo.height_, dstBuffInfo.formatType_, dstBuffInfo.len_, dstBuffInfo.rowStride_);

    uint32_t minSrcRowStride = FormatHelper::CalculateRowStride(srcBuffInfo.width_, srcBuffInfo.formatType_);
    uint32_t minDstRowStride = FormatHelper::CalculateRowStride(dstBuffInfo.width_, dstBuffInfo.formatType_);
    uint32_t minSrcLen = FormatHelper::CalculateSize(srcBuffInfo.width_, srcBuffInfo.height_, srcBuffInfo.formatType_);
    uint32_t minDstLen = FormatHelper::CalculateSize(dstBuffInfo.width_, dstBuffInfo.height_, dstBuffInfo.formatType_);
    CHECK_AND_RETURN_RET_LOG(minSrcRowStride <= srcBuffInfo.rowStride_ && minDstRowStride <= dstBuffInfo.rowStride_ &&
        minSrcLen <= srcBuffInfo.len_ && minDstLen <= dstBuffInfo.len_, ErrorCode::ERR_PARAM_INVALID,
        "CheckConverterInfo: invalid size! src{%{public}d %{public}d %{public}d %{public}d %{public}d}, "
        "dst{%{public}d %{public}d %{public}d %{public}d %{public}d}",
        srcBuffInfo.width_, srcBuffInfo.height_, srcBuffInfo.formatType_, srcBuffInfo.len_, srcBuffInfo.rowStride_,
        dstBuffInfo.width_, dstBuffInfo.height_, dstBuffInfo.formatType_, dstBuffInfo.len_, dstBuffInfo.rowStride_);

    return ErrorCode::SUCCESS;
}

ErrorCode FormatHelper::ConvertFormat(FormatConverterInfo &src, FormatConverterInfo &dst)
{
    IEffectFormat srcFormat = src.bufferInfo.formatType_;
    IEffectFormat dstFormat = dst.bufferInfo.formatType_;

    auto func = GetFormatConverterFunc(srcFormat, dstFormat);
    CHECK_AND_RETURN_RET_LOG(func != nullptr, ErrorCode::ERR_NOT_SUPPORT_CONVERT_FORMAT,
        "ConvertFormat: format not support convert! srcFormat=%{public}d, dstFormat=%{public}d", srcFormat, dstFormat);

    ErrorCode res = CheckConverterInfo(src, dst);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "ConvertFormat: invalid para! res=%{public}d", res);

    func(src, dst);
    return ErrorCode::SUCCESS;
}

void ConvertRGBAToNV12(FormatConverterInfo &src, FormatConverterInfo &dst)
{
    EFFECT_LOGW("ConvertRGBAToNV12: ConvertRGBAToNV12 will loss alpha information!");
    BufferInfo &srcBuffInfo = src.bufferInfo;
    BufferInfo &dstBuffInfo = dst.bufferInfo;
    uint32_t width = std::min(srcBuffInfo.width_, dstBuffInfo.width_);
    uint32_t height = std::min(srcBuffInfo.height_, dstBuffInfo.height_);
    uint32_t srcRowStride = srcBuffInfo.rowStride_;
    uint32_t dstRowStride = dstBuffInfo.rowStride_;

    uint8_t *srcRGBA = static_cast<uint8_t *>(src.buffer);
    uint8_t *dstNV12 = static_cast<uint8_t *>(dst.buffer);
    uint8_t *dstNV12UV = dstNV12 + dstBuffInfo.height_ * dstRowStride;

#pragma omp parallel for default(none) shared(height, width, srcRGBA, dstNV12, dstNV12UV, srcRowStride, dstRowStride)
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            uint32_t y_index = i * dstRowStride + j;
            uint32_t nv_index = i / UV_SPLIT_FACTOR * dstRowStride + j - j % UV_SPLIT_FACTOR;
            uint32_t srcIndex = i * srcRowStride + j * RGBA_BYTES_PER_PIXEL;
            uint8_t r = srcRGBA[srcIndex + R];
            uint8_t g = srcRGBA[srcIndex + G];
            uint8_t b = srcRGBA[srcIndex + B];

            dstNV12[y_index] = FormatHelper::RGBToY(r, g, b);
            dstNV12UV[nv_index] = FormatHelper::RGBToU(r, g, b);
            dstNV12UV[nv_index + 1] = FormatHelper::RGBToV(r, g, b);
        }
    }
}

void ConvertRGBAToNV21(FormatConverterInfo &src, FormatConverterInfo &dst)
{
    EFFECT_LOGW("ConvertRGBAToNV21: ConvertRGBAToNV21 will loss alpha information!");
    BufferInfo &srcBuffInfo = src.bufferInfo;
    BufferInfo &dstBuffInfo = dst.bufferInfo;
    uint32_t width = std::min(srcBuffInfo.width_, dstBuffInfo.width_);
    uint32_t height = std::min(srcBuffInfo.height_, dstBuffInfo.height_);
    uint32_t srcRowStride = srcBuffInfo.rowStride_;
    uint32_t dstRowStride = dstBuffInfo.rowStride_;

    uint8_t *srcRGBA = static_cast<uint8_t *>(src.buffer);
    uint8_t *dstNV21 = static_cast<uint8_t *>(dst.buffer);
    uint8_t *dstNV21UV = dstNV21 + dstBuffInfo.height_ * dstRowStride;

#pragma omp parallel for default(none) shared(height, width, srcRGBA, dstNV12, dstNV12UV, srcRowStride, dstRowStride)
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            uint32_t y_index = i * dstRowStride + j;
            uint32_t nv_index = i / UV_SPLIT_FACTOR * dstRowStride + j - j % UV_SPLIT_FACTOR;
            uint32_t srcIndex = i * srcRowStride + j * RGBA_BYTES_PER_PIXEL;
            uint8_t r = srcRGBA[srcIndex + R];
            uint8_t g = srcRGBA[srcIndex + G];
            uint8_t b = srcRGBA[srcIndex + B];

            dstNV21[y_index] = FormatHelper::RGBToY(r, g, b);
            dstNV21UV[nv_index] = FormatHelper::RGBToV(r, g, b);
            dstNV21UV[nv_index + 1] = FormatHelper::RGBToU(r, g, b);
        }
    }
}

void ConvertNV12ToRGBA(FormatConverterInfo &src, FormatConverterInfo &dst)
{
    BufferInfo &srcBuffInfo = src.bufferInfo;
    BufferInfo &dstBuffInfo = dst.bufferInfo;
    uint32_t width = std::min(srcBuffInfo.width_, dstBuffInfo.width_);
    uint32_t height = std::min(srcBuffInfo.height_, dstBuffInfo.height_);
    uint32_t srcRowStride = srcBuffInfo.rowStride_;
    uint32_t dstRowStride = dstBuffInfo.rowStride_;

    uint8_t *srcNV12 = static_cast<uint8_t *>(src.buffer);
    uint8_t *srcNV12UV = srcNV12 + srcBuffInfo.height_ * srcRowStride;
    uint8_t *dstRGBA = static_cast<uint8_t *>(dst.buffer);

#pragma omp parallel for default(none) shared(height, width, srcNV12, srcNV12UV, dstRGBA, srcRowStride, dstRowStride)
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            uint32_t y_index = i * srcRowStride + j;
            uint32_t nv_index = i / UV_SPLIT_FACTOR * srcRowStride + j - j % UV_SPLIT_FACTOR;
            uint32_t dstIndex = i * dstRowStride + j *RGBA_BYTES_PER_PIXEL;
            uint8_t y = srcNV12[y_index];
            uint8_t u = srcNV12UV[nv_index];
            uint8_t v = srcNV12UV[nv_index + 1];

            dstRGBA[dstIndex + R] = FormatHelper::YuvToR(y, u, v);
            dstRGBA[dstIndex + G] = FormatHelper::YuvToG(y, u, v);
            dstRGBA[dstIndex + B] = FormatHelper::YuvToB(y, u, v);
            dstRGBA[dstIndex + A] = UNSIGHED_CHAR_MAX;
        }
    }
}

void ConvertNV21ToRGBA(FormatConverterInfo &src, FormatConverterInfo &dst)
{
    BufferInfo &srcBuffInfo = src.bufferInfo;
    BufferInfo &dstBuffInfo = dst.bufferInfo;
    uint32_t width = std::min(srcBuffInfo.width_, dstBuffInfo.width_);
    uint32_t height = std::min(srcBuffInfo.height_, dstBuffInfo.height_);
    uint32_t srcRowStride = srcBuffInfo.rowStride_;
    uint32_t dstRowStride = dstBuffInfo.rowStride_;

    uint8_t *srcNV21 = static_cast<uint8_t *>(src.buffer);
    uint8_t *srcNV21UV = srcNV21 + srcBuffInfo.height_ * srcRowStride;
    uint8_t *dstRGBA = static_cast<uint8_t *>(dst.buffer);

#pragma omp parallel for default(none) shared(height, width, srcNV21, srcNV21UV, dstRGBA, srcRowStride, dstRowStride)
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            uint32_t y_index = i * srcRowStride + j;
            uint32_t nv_index = i / UV_SPLIT_FACTOR * srcRowStride + j - j % UV_SPLIT_FACTOR;
            uint32_t dstIndex = i * dstRowStride + j *RGBA_BYTES_PER_PIXEL;
            uint8_t y = srcNV21[y_index];
            uint8_t v = srcNV21UV[nv_index];
            uint8_t u = srcNV21UV[nv_index + 1];

            dstRGBA[dstIndex + R] = FormatHelper::YuvToR(y, u, v);
            dstRGBA[dstIndex + G] = FormatHelper::YuvToG(y, u, v);
            dstRGBA[dstIndex + B] = FormatHelper::YuvToB(y, u, v);
            dstRGBA[dstIndex + A] = UNSIGHED_CHAR_MAX;
        }
    }
}
} // namespace Effect
} // namespace Media
} // namespace OHOS