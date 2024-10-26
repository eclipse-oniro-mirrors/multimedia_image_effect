//
// Created by 11520 on 2024/10/26.
//

#include "surface_buffer_manager.h"
#include "effect_log.h"
#include "effect_trace.h"

namespace OHOS {
namespace Media {
namespace Effect {


    void SurfaceBufferManager::SetConsumerSurface(sptr<IConsumerSurface> &consumer) {
        consumerSurface_ = consumer;
    }

    void SurfaceBufferManager::SetProducerSurface(sptr<Surface> &producer) {
        producerSurface_ = producer;
    }

    void SurfaceBufferManager::SwapBuffers() {
        CHECK_AND_RETURN_LOG(consumerSurface_!= nullptr && producerSurface_!= nullptr, "SwapBuffers: consumerSurface_ is nullptr");
        EFFECT_TRACE_NAME("SwapBuffers: switch buffer");
        auto detRet = consumerSurface_->DetachBufferFromQueue(consumerBuffer_);
        CHECK_AND_RETURN_LOG(detRet == GSError::GSERROR_OK, "SwapBuffers: detach buffer from consumerSurface_ failed");
        detRet = producerSurface_->DetachBufferFromQueue(producerBuffer_);
        CHECK_AND_RETURN_LOG(detRet == GSError::GSERROR_OK, "SwapBuffers: detach buffer from producerSurface_ failed");
        detRet = consumerSurface_->AttachBufferToQueue(producerBuffer_);
        CHECK_AND_RETURN_LOG(detRet == GSError::GSERROR_OK, "SwapBuffers: attach buffer from consumerSurface_ failed");
        detRet = producerSurface_->AttachBufferToQueue(consumerBuffer_);
        CHECK_AND_RETURN_LOG(detRet == GSError::GSERROR_OK, "SwapBuffers: attach buffer from producerSurface_ failed")
    }

    void SurfaceBufferManager::ReleaseBuffer() {

    }

    void SurfaceBufferManager::FlushBuffer() {

    }

    void SurfaceBufferManager::SetProducerBuffer(sptr<SurfaceBuffer> &buffer) {

    }

    void SurfaceBufferManager::SetConsumerBuffer(sptr<SurfaceBuffer> &buffer) {

    }

    void SurfaceBufferManager::SetNeedSwitch(bool needSwitch) {

    }

    bool SurfaceBufferManager::IsNeedSwap() const {
        return isNeedSwap_;
    }
}
}
}