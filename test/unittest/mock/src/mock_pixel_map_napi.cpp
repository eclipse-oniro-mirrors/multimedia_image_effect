/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "mock_pixel_map_napi.h"

namespace {
    constexpr int32_t WIDTH = 960;
    constexpr int32_t HEIGHT = 1280;
    constexpr OHOS::Media::PixelFormat PIXEL_FORMAT = OHOS::Media::PixelFormat::RGBA_8888;
    constexpr int32_t ROW_SIZE = WIDTH * 4;
    constexpr int32_t BYTE_COUNT = ROW_SIZE * HEIGHT;
    constexpr int32_t PIXEL_BYTES = 4;
}

namespace OHOS {
namespace Media {
namespace Effect {
MockPixelMapNapi::MockPixelMapNapi()
{
    addr = malloc(BYTE_COUNT);
    nativePixelMap_ = std::make_unique<PixelMap>();
    nativePixelMap_->imageInfo_.size.width = WIDTH;
    nativePixelMap_->imageInfo_.size.height = HEIGHT;
    nativePixelMap_->imageInfo_.pixelFormat = PIXEL_FORMAT;
    nativePixelMap_->rowStride_ = ROW_SIZE;
    nativePixelMap_->data_ = static_cast<uint8_t *>(addr);
    nativePixelMap_->pixelBytes_ = PIXEL_BYTES;
}

MockPixelMapNapi::~MockPixelMapNapi()
{
    free(addr);
    addr = nullptr;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS