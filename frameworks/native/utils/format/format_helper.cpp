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

namespace {
    const float YUV_BYTES_PER_PIXEL = 1.5f;
    const int32_t RGBA_8888_BYTES_PER_PIXEL = 4;
}

namespace OHOS {
namespace Media {
namespace Effect {
uint32_t FormatHelper::CalculateDataRowCount(uint32_t height, IEffectFormat format)
{
    switch (format) {
        case IEffectFormat::RGBA8888:
            return height;
        case IEffectFormat::YUVNV12:
        case IEffectFormat::YUVNV21:
            return static_cast<uint32_t>(height * YUV_BYTES_PER_PIXEL);
        default:
            return height;
    }
}

uint32_t FormatHelper::CalculateRowStride(uint32_t width, IEffectFormat format)
{
    switch (format) {
        case IEffectFormat::RGBA8888:
            return width * RGBA_8888_BYTES_PER_PIXEL;
        case IEffectFormat::YUVNV12:
        case IEffectFormat::YUVNV21:
            return width;
        default:
            return width;
    }
}

uint32_t FormatHelper::CalculateSize(uint32_t width, uint32_t height, IEffectFormat format)
{
    return CalculateDataRowCount(height, format) * CalculateRowStride(width, format);
}
} // namespace Effect
} // namespace Media
} // namespace OHOS