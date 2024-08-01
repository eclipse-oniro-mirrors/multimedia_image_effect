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

#ifndef IMAGE_EFFECT_MOCK_PRODUCER_SURFACE_H
#define IMAGE_EFFECT_MOCK_PRODUCER_SURFACE_H

#include "gmock/gmock.h"
#include "native_window.h"
#include "producer_surface.h"

namespace OHOS {
namespace Media {
namespace Effect {
class MockProducerSurface : public ProducerSurface {
public:
    MockProducerSurface(sptr<IBufferProducer>& producer);
    ~MockProducerSurface();
    MOCK_METHOD3(RequestBuffer, GSError(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
        BufferRequestConfig &config));
    MOCK_METHOD3(FlushBuffer, GSError(sptr<SurfaceBuffer>& buffer, int32_t fence, BufferFlushConfig &config));

    static void AllocDmaMemory(sptr<SurfaceBuffer> &buffer);
    static void ReleaseDmaBuffer(sptr<SurfaceBuffer> &buffer);
private:
    sptr<SurfaceBuffer> buffer_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_MOCK_PRODUCER_SURFACE_H
