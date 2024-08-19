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
#include "image_effect_marco_define.h"
#include "picture.h"

namespace OHOS {
namespace Media {
namespace Effect {
enum class EffectPixelmapType {
    UNKNOWN = 0,
    PRIMARY,
    GAINMAP,
    DEPTHMAP,
    UNREFOCUS,
    WATERMARK_CUT,
};

class BufferInfo {
public:
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t len_ = 0;
    IEffectFormat formatType_ = IEffectFormat::DEFAULT;
    EffectColorSpace colorSpace_ = EffectColorSpace::DEFAULT;
    uint32_t rowStride_ = 0;
    BufferType bufferType_ = BufferType::DEFAULT;
    EffectPixelmapType pixelmapType_ = EffectPixelmapType::UNKNOWN;
    void *addr_ = nullptr;
};

enum class DataType {
    UNKNOWN = 0,
    PIXEL_MAP,
    SURFACE,
    SURFACE_BUFFER,
    URI,
    PATH,
    TEX,
    NATIVE_WINDOW,
    PICTURE,
};

struct ExtraInfo {
    DataType dataType = DataType::UNKNOWN;
    BufferType bufferType = BufferType::DEFAULT;
    PixelMap *pixelMap = nullptr;
    std::shared_ptr<PixelMap> innerPixelMap = nullptr; // converter pixel map for color space, such as adobe rgb
    OHOS::SurfaceBuffer *surfaceBuffer = nullptr;
    Picture *picture = nullptr;
    std::shared_ptr<Picture> innerPicture = nullptr; // decoded pixel map for url or path
    int *fd = nullptr;
    std::string uri;
    std::string path;
    int64_t timestamp = 0;
};

class RenderTexture;
using RenderTexturePtr = std::shared_ptr<RenderTexture>;
class EffectBuffer {
public:
    IMAGE_EFFECT_EXPORT
    EffectBuffer(std::shared_ptr<BufferInfo> &info, void *buffer, std::shared_ptr<ExtraInfo> &extraInfo)
        : bufferInfo_(info), buffer_(buffer), extraInfo_(extraInfo) {};

    std::shared_ptr<BufferInfo> bufferInfo_ = nullptr;
    void *buffer_ = nullptr;
    RenderTexturePtr tex;
    std::shared_ptr<ExtraInfo> extraInfo_ = nullptr;
    std::shared_ptr<std::unordered_map<EffectPixelmapType, std::shared_ptr<BufferInfo>>> auxiliaryBufferInfos = nullptr;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_EFFECT_BUFFER_H
