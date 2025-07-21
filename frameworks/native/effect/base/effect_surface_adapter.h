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
#include "image_effect_marco_define.h"

namespace OHOS {
namespace Media {
namespace Effect {
using ConsumerBufferAvailable = std::function<void()>;

/**
 * Adapter class for image effect surface operation.
 */
class EffectSurfaceAdapter : public IBufferConsumerListenerClazz {
public:
    IMAGE_EFFECT_EXPORT EffectSurfaceAdapter();
    IMAGE_EFFECT_EXPORT ~EffectSurfaceAdapter();

    /**
     * Retrieves the producer surface.
     * @return Smart pointer to the producer surface.
     */
    IMAGE_EFFECT_EXPORT sptr<Surface> GetProducerSurface();

    /**
     * Checks if the effect surface is valid.
     * @return True if valid, false otherwise.
     */
    bool CheckEffectSurface() const;

    /**
     * Acquires a consumer surface buffer.
     * @param buffer Smart pointer to the surface buffer.
     * @param syncFence Smart pointer to the sync fence.
     * @param timestamp Timestamp of the buffer.
     * @param damages Damages rectangle.
     * @return Error code indicating the result.
     */
    GSError AcquireConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& syncFence,
        int64_t& timestamp, OHOS::Rect& damages) const;

    /**
     * Releases a consumer surface buffer.
     * @param buffer Smart pointer to the surface buffer.
     * @param syncFence Smart pointer to the sync fence.
     * @return Error code indicating the result.
     */
    IMAGE_EFFECT_EXPORT GSError ReleaseConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer,
        const sptr<SyncFence>& syncFence) const;

    /**
     * Detaches a consumer surface buffer.
     * @param buffer Smart pointer to the surface buffer.
     * @return Error code indicating the result.
     */
    IMAGE_EFFECT_EXPORT GSError DetachConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer) const;

    /**
     * Attaches a consumer surface buffer.
     * @param buffer Smart pointer to the surface buffer.
     * @return Error code indicating the result.
     */
    IMAGE_EFFECT_EXPORT GSError AttachConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer) const;

    /**
     * Sets the consumer surface buffer.
     * @param consumerBufferAvailable Function to be called when buffer is available.
     * @return Error code indicating the result.
     */
    IMAGE_EFFECT_EXPORT ErrorCode SetConsumerListener(ConsumerBufferAvailable &&consumerBufferAvailable);

    /**
     * Retrieves the transformation type.
     * @return Transformation type.
     */
    IMAGE_EFFECT_EXPORT GraphicTransformType GetTransform() const;

    /**
     * Sets the default usage for the output surface.
     * @param usage Usage value.
     */
    void SetOutputSurfaceDefaultUsage(uint64_t usage);

    /**
     * Requests CPU access for the consumer.
     * @param isCpuAccess True if CPU access is requested, false otherwise.
     */
    void ConsumerRequestCpuAccess(bool isCpuAccess);

    // Implementation of IBufferConsumerListener interface begin
    /**
     * Buffer available callback.
     */
    void OnBufferAvailable() override;

    /**
     * Tunnel handle change callback.
     */
    void OnTunnelHandleChange() override;

    /**
     * Go background callback.
     */
    void OnGoBackground() override;

    /**
     * Clean cache callback.
     * @param bufSeqNum Buffer sequence number.
     */
    void OnCleanCache(uint32_t* bufSeqNum) override;
    // Implementation of IBufferConsumerListener interface end

private:
    /**
     * Initializes the EffectSurfaceAdapter.
     * @return Error code indicating the result.
     */
    ErrorCode Initialize();

    OHOS::sptr<IConsumerSurface> receiverConsumerSurface_ = nullptr; // Consumer surface interface pointer
    OHOS::sptr<Surface> fromProducerSurface_ = nullptr; // Producer surface pointer
    ConsumerBufferAvailable consumerBufferAvailable_; // Consumer buffer available callback
    uint64_t outputSurfaceDefaultUsage_ = 0; // Default usage for output surface
    volatile int32_t effectSurfaceFlag_ = 0; // Flag for effect surface validity
};
}
}
}

#endif // IMAGE_EFFECT_SURFACE_ADAPTER_H