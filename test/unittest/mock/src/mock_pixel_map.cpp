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

#include "mock_pixel_map.h"

using ::testing::_;
using ::testing::Return;

namespace OHOS {
namespace Media {
namespace Effect {
constexpr int32_t WIDTH = 960;
constexpr int32_t HEIGHT = 1280;
constexpr PixelFormat PIXEL_FORMAT = PixelFormat::RGBA_8888;
constexpr int32_t ROW_SIZE = WIDTH * 4;
constexpr int32_t BYTE_COUNT = ROW_SIZE * HEIGHT;
constexpr int32_t PIXEL_BYTES = 4;

MockPixelMap::MockPixelMap()
{
    buffer = malloc(BYTE_COUNT);
    ON_CALL(*this, GetWidth()).WillByDefault(Return(WIDTH));
    ON_CALL(*this, GetHeight()).WillByDefault(Return(HEIGHT));
    ON_CALL(*this, GetPixelFormat()).WillByDefault(Return(PIXEL_FORMAT));
    ON_CALL(*this, GetRowBytes()).WillByDefault(Return(ROW_SIZE));
    ON_CALL(*this, GetRowStride()).WillByDefault(Return(ROW_SIZE));
    ON_CALL(*this, GetByteCount()).WillByDefault(Return(BYTE_COUNT));
    ON_CALL(*this, GetCapacity()).WillByDefault(Return(BYTE_COUNT));
    ON_CALL(*this, GetPixels()).WillByDefault(Return(static_cast<const uint8_t *>(buffer)));
    ON_CALL(*this, GetPixelBytes()).WillByDefault(Return(PIXEL_BYTES));
    ON_CALL(*this, GetImageInfo(_)).WillByDefault([](ImageInfo &imageInfo) {
        imageInfo.size.width = WIDTH;
        imageInfo.size.height = HEIGHT;
        imageInfo.pixelFormat = PIXEL_FORMAT;
    });
    ON_CALL(*this, GetAllocatorType()).WillByDefault(Return(AllocatorType::HEAP_ALLOC));
    ON_CALL(*this, SetPixelsAddr(_, _, _, _, _)).WillByDefault(
        [this](void *addr, void *context, uint32_t size, AllocatorType type, CustomFreePixelMap func) {
            if (this->buffer != nullptr) {
                free(this->buffer);
            }
            buffer = addr;
        });
}

MockPixelMap::~MockPixelMap()
{
    if (buffer != nullptr) {
        free(buffer);
        buffer = nullptr;
    }
}
} // namespace Effect
} // namespace Media
} // namespace OHOS