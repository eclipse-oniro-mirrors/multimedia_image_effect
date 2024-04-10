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

#ifndef IMAGE_EFFECT_MOCK_PIXEL_MAP_H
#define IMAGE_EFFECT_MOCK_PIXEL_MAP_H

#include "gmock/gmock.h"
#include "pixel_map.h"

namespace OHOS {
namespace Media {
namespace Effect {
class MockPixelMap : public PixelMap {
public:
    MockPixelMap();
    ~MockPixelMap();
    MOCK_METHOD0(GetWidth, int32_t());
    MOCK_METHOD0(GetHeight, int32_t());
    MOCK_METHOD0(GetPixelFormat, PixelFormat());
    MOCK_METHOD0(GetRowBytes, int32_t());
    MOCK_METHOD0(GetRowStride, int32_t());
    MOCK_METHOD0(GetByteCount, int32_t());
    MOCK_METHOD0(GetCapacity, uint32_t());
    MOCK_METHOD0(GetPixels, const uint8_t *());
    MOCK_METHOD0(GetPixelBytes, int32_t());
    MOCK_METHOD1(GetImageInfo, void(ImageInfo &imageInfo));
    MOCK_METHOD0(GetAllocatorType, AllocatorType());
    MOCK_METHOD5(SetPixelsAddr, void(void *addr, void *context, uint32_t size, AllocatorType type,
        CustomFreePixelMap func));

private:
    void *buffer = nullptr;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_MOCK_PIXEL_MAP_H