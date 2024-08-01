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

#include "mock_producer_surface.h"

using ::testing::_;
using ::testing::Return;

namespace OHOS {
namespace Media {
namespace Effect {

constexpr int32_t WIDTH = 960;
constexpr int32_t HEIGHT = 1280;
constexpr GraphicPixelFormat PIXEL_FORMAT = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888;

void MockProducerSurface::AllocDmaMemory(sptr<SurfaceBuffer> &buffer)
{
    BufferRequestConfig requestConfig = {
        .width = WIDTH,
        .height = HEIGHT,
        .strideAlignment = 0x8,
        .format = PIXEL_FORMAT,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_MEM_MMZ_CACHE,
        .timeout = 0,
        .colorGamut = GraphicColorGamut::GRAPHIC_COLOR_GAMUT_SRGB,
        .transform = GraphicTransformType::GRAPHIC_ROTATE_NONE,
    };

    buffer = SurfaceBuffer::Create();
    buffer->Alloc(requestConfig);
    buffer->Map();
    buffer->IncStrongRef(buffer);
}

void MockProducerSurface::ReleaseDmaBuffer(sptr<SurfaceBuffer> &buffer)
{
    if (buffer == nullptr) {
        return;
    }
    buffer->DecStrongRef(buffer);
    buffer = nullptr;
}

MockProducerSurface::MockProducerSurface(sptr<IBufferProducer>& producer) : ProducerSurface(producer)
{
    AllocDmaMemory(buffer_);
    ON_CALL(*this, RequestBuffer(_, _, _))
        .WillByDefault([this](sptr<SurfaceBuffer> &buffer, sptr<SyncFence> &fence, BufferRequestConfig &config) {
            buffer = buffer_;
            return GSError::GSERROR_OK;
        }
    );
    ON_CALL(*this, FlushBuffer(_, _, _)).WillByDefault(Return(GSError::GSERROR_OK));
}

MockProducerSurface::~MockProducerSurface()
{
    ReleaseDmaBuffer(buffer_);
}

} // namespace Effect
} // namespace Media
} // namespace OHOS