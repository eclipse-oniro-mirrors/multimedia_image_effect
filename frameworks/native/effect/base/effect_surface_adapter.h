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

#ifndef IMAGE_EFFECT_SURFACE_ADAPTER_H
#define IMAGE_EFFECT_SURFACE_ADAPTER_H

#include <cstdint>
#include <functional>
#include <iconsumer_surface.h>
#include <refbase.h>

#include "error_code.h"

namespace OHOS {
namespace Media {
namespace Effect {
using ConsumerBufferAvailable = std::function<void()>;

class EffectSurfaceAdapter : public IBufferConsumerListenerClazz {
public:
    EffectSurfaceAdapter();
    ~EffectSurfaceAdapter();

    sptr<Surface> GetProducerSurface();
    bool CheckEffectSurface() const;
    GSError AcquireConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& syncFence,
        int64_t& timestamp, OHOS::Rect& damages) const;
    GSError ReleaseConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& syncFence) const;
    GSError DetachConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer) const;
    GSError AttachConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer) const;
    ErrorCode SetConsumerListener(ConsumerBufferAvailable &&consumerBufferAvailable);
    GraphicTransformType GetTransform() const;
    void SetOutputSurfaceDefaultUsage(uint64_t usage);

    void ConsumerRequestCpuAccess(bool isCpuAccess);

    // IBufferConsumerListener 接口的实现 begin
    void OnBufferAvailable() override;
    void OnTunnelHandleChange() override;
    void OnGoBackground() override;
    void OnCleanCache() override;
    // IBufferConsumerListener 接口的实现 end

private:
    ErrorCode Initialize();

    OHOS::sptr<IConsumerSurface> receiverConsumerSurface_ = nullptr;
    OHOS::sptr<Surface> fromProducerSurface_ = nullptr;
    ConsumerBufferAvailable consumerBufferAvailable_;
    uint64_t outputSurfaceDefaultUsage_ = 0;
    volatile int32_t effectSurfaceFlag_ = 0;
};
}
}
}

#endif // IMAGE_EFFECT_SURFACE_ADAPTER_H