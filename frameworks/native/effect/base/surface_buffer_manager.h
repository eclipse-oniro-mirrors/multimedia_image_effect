//
// Created by 11520 on 2024/10/26.
//

#ifndef IMAGE_EFFECT_SURFACE_BUFFER_MANAGER_H
#define IMAGE_EFFECT_SURFACE_BUFFER_MANAGER_H

#include <refbase.h>

#include "iconsumer_surface.h"

namespace OHOS {
namespace Media {
namespace Effect {
    class SurfaceBufferManager : public RefBase {
    public:
        static OHOS::sptr<SurfaceBufferManager> Create() {
            OHOS::sptr<SurfaceBufferManager> instance = new SurfaceBufferManager();
            return instance;
        }

        SurfaceBufferManager(const SurfaceBufferManager&) = delete;
        SurfaceBufferManager& operator=(const SurfaceBufferManager&) = delete;

        void SetConsumerSurface(OHOS::sptr<IConsumerSurface> &consumer);

        void SetProducerSurface(OHOS::sptr<Surface> &producer);

        void SwapBuffers();

        void ReleaseBuffer();

        void FlushBuffer();

        void SetProducerBuffer(OHOS::sptr<SurfaceBuffer> &buffer);

        void SetConsumerBuffer(OHOS::sptr<SurfaceBuffer> &buffer);

        void SetNeedSwitch(bool needSwitch);

        bool IsNeedSwap() const;

        SurfaceBufferManager() = default;
        virtual ~SurfaceBufferManager() = default;
        OHOS::sptr<Surface> producerSurface_ = nullptr;
        OHOS::sptr<IConsumerSurface> consumerSurface_ = nullptr;
        OHOS::sptr<SurfaceBuffer> producerBuffer_ = nullptr;
        OHOS::sptr<SurfaceBuffer> consumerBuffer_ = nullptr;
        bool isNeedSwap_ = true;

    private:

    };
}
}
}

#endif //IMAGE_EFFECT_SURFACE_BUFFER_MANAGER_H
