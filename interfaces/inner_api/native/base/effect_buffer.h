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

#ifndef IMAGE_EFFECT_EFFECT_BUFFER_H
#define IMAGE_EFFECT_EFFECT_BUFFER_H

#include <memory>

#include "effect_info.h"
#include "effect_type.h"
#include "pixel_map.h"
#include "surface_buffer.h"

namespace OHOS {
namespace Media {
namespace Effect {
class BufferInfo {
public:
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t len_ = 0;
    IEffectFormat formatType_ = IEffectFormat::DEFAULT;
    uint32_t rowStride_ = 0;
};

enum class DataType {
    UNKNOWN = 0,
    PIXEL_MAP,
    SURFACE,
    SURFACE_BUFFER,
    URI,
    PATH,
};

struct ExtraInfo {
    DataType dataType = DataType::UNKNOWN;
    BufferType bufferType = BufferType::DEFAULT;
    PixelMap *pixelMap = nullptr;
    std::shared_ptr<PixelMap> innerPixelMap = nullptr; // decoded pixel map for url or path
    OHOS::SurfaceBuffer *surfaceBuffer = nullptr;
    std::string uri;
    std::string path;
};

class EffectBuffer {
public:
    EffectBuffer(std::shared_ptr<BufferInfo> &info, void *buffer, std::shared_ptr<ExtraInfo> &extraInfo)
        : bufferInfo_(info), buffer_(buffer), extraInfo_(extraInfo) {};

    std::shared_ptr<BufferInfo> bufferInfo_ = nullptr;
    void *buffer_ = nullptr;
    std::shared_ptr<ExtraInfo> extraInfo_ = nullptr;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_EFFECT_BUFFER_H
