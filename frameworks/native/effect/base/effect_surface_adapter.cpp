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

#include "effect_surface_adapter.h"

#include <surface_utils.h>
#include <sync_fence.h>

#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
constexpr int32_t IE_INVALID_FENCE = -1;
const int32_t STRUCT_EFFECT_SURFACE_CONSTANT = 1;
const int32_t DESTRUCTOR_EFFECT_SURFACE_CONSTANT = 2;

EffectSurfaceAdapter::EffectSurfaceAdapter()
{
    effectSurfaceFlag_ = STRUCT_EFFECT_SURFACE_CONSTANT;
}

EffectSurfaceAdapter::~EffectSurfaceAdapter()
{
    if (receiverConsumerSurface_) {
        GSError result = receiverConsumerSurface_->UnregisterConsumerListener();
        EFFECT_LOGE("EffectSurfaceAdapter::~EffectSurfaceAdapter UnregisterConsumerListener. result=%{public}d",
            result);
        effectSurfaceFlag_ = DESTRUCTOR_EFFECT_SURFACE_CONSTANT;
        receiverConsumerSurface_ = nullptr;
    }
}

ErrorCode EffectSurfaceAdapter::Initialize()
{
    receiverConsumerSurface_ = IConsumerSurface::Create("EffectSurfaceAdapter");
    if (receiverConsumerSurface_ == nullptr) {
        EFFECT_LOGE("Surface::CreateSurfaceAsConsumer::Create failed.");
        return ErrorCode::ERR_IMAGE_EFFECT_RECEIVER_INIT_FAILED;
    }

    uint64_t usage = BUFFER_USAGE_CPU_HW_BOTH | BUFFER_USAGE_MEM_MMZ_CACHE;
    if (outputSurfaceDefaultUsage_ & BUFFER_USAGE_HW_COMPOSER) {
        usage |= BUFFER_USAGE_HW_COMPOSER;
    }
    (void)receiverConsumerSurface_->SetDefaultUsage(usage);

    auto producer = receiverConsumerSurface_->GetProducer();
    fromProducerSurface_ = Surface::CreateSurfaceAsProducer(producer);
    if (fromProducerSurface_ == nullptr) {
        EFFECT_LOGE("Surface::CreateSurfaceAsProducer failed");
        return ErrorCode::ERR_IMAGE_EFFECT_RECEIVER_INIT_FAILED;
    }

    // register consumer listener
    receiverConsumerSurface_->RegisterConsumerListener(this);

    auto surfaceUtils = SurfaceUtils::GetInstance();
    auto ret = surfaceUtils->Add(fromProducerSurface_->GetUniqueId(), fromProducerSurface_);
    if (ret != SurfaceError::SURFACE_ERROR_OK) {
        EFFECT_LOGE("add surface error: %{public}d", ret);
        return ErrorCode::ERR_IMAGE_EFFECT_RECEIVER_INIT_FAILED;
    }
    EFFECT_LOGI("producer create success, unique id:%{private}llu",
        static_cast<unsigned long long>(fromProducerSurface_->GetUniqueId()));

    return ErrorCode::SUCCESS;
}

sptr<Surface> EffectSurfaceAdapter::GetProducerSurface()
{
    if (fromProducerSurface_) {
        return fromProducerSurface_;
    }

    if (Initialize() != ErrorCode::SUCCESS) {
        return nullptr;
    }

    return fromProducerSurface_;
}

ErrorCode EffectSurfaceAdapter::SetConsumerListener(ConsumerBufferAvailable &&consumerBufferAvailable)
{
    if (!consumerBufferAvailable) {
        return ErrorCode::ERR_INPUT_NULL;
    }

    consumerBufferAvailable_ = std::move(consumerBufferAvailable);
    return ErrorCode::SUCCESS;
}

GraphicTransformType EffectSurfaceAdapter::GetTransform() const
{
    if (receiverConsumerSurface_) {
        return receiverConsumerSurface_->GetTransform();
    }

    return GRAPHIC_ROTATE_BUTT;
}

void EffectSurfaceAdapter::SetOutputSurfaceDefaultUsage(uint64_t usage)
{
    EFFECT_LOGD("SetOutputSurfaceDefaultUsage: usage=%{public}llu", static_cast<unsigned long long>(usage));
    outputSurfaceDefaultUsage_ = usage;
}

void EffectSurfaceAdapter::ConsumerRequestCpuAccess(bool isCpuAccess)
{
    EFFECT_LOGD("ConsumerRequestCpuAccess: isCpuAccess=%{public}d", isCpuAccess);
    if (receiverConsumerSurface_) {
        receiverConsumerSurface_->ConsumerRequestCpuAccess(isCpuAccess);
    }
}

void EffectSurfaceAdapter::OnBufferAvailable()
{
    OHOS::sptr<SurfaceBuffer> inBuffer;
    OHOS::sptr<SurfaceBuffer> outBuffer;
    int64_t timestamp = 0;
    Rect damages{};
    sptr<SyncFence> syncFence = SyncFence::INVALID_FENCE;
    CHECK_AND_RETURN_LOG(effectSurfaceFlag_ == STRUCT_EFFECT_SURFACE_CONSTANT,
        "EffectSurfaceAdapter::OnBufferAvailable AcquireBuffer surface not exist.");
    CHECK_AND_RETURN_LOG(receiverConsumerSurface_,
        "EffectSurfaceAdapter::OnBufferAvailable receiverConsumerSurface_ is nullptr.");
    auto ret = receiverConsumerSurface_->AcquireBuffer(inBuffer, syncFence, timestamp, damages);
    CHECK_AND_RETURN_LOG(ret == 0, "AcquireBuffer failed. %{public}d", ret);

    constexpr uint32_t waitForEver = -1;
    (void)syncFence->Wait(waitForEver);

    bool isNeedSwap = true;
    if (consumerBufferAvailable_) {
        isNeedSwap = consumerBufferAvailable_(inBuffer, outBuffer, damages, timestamp);
    } else {
        EFFECT_LOGE("not register handle buffer.");
    }
    CHECK_AND_RETURN_LOG(effectSurfaceFlag_ == STRUCT_EFFECT_SURFACE_CONSTANT,
        "EffectSurfaceAdapter::OnBufferAvailable ReleaseBuffer surface not exist.");
    CHECK_AND_RETURN_LOG(receiverConsumerSurface_,
                         "EffectSurfaceAdapter::OnBufferAvailable receiverConsumerSurface_ is nullptr.");
    auto releaseBuffer = (isNeedSwap) ? outBuffer : inBuffer;
    if (isNeedSwap) {
        auto detRet = receiverConsumerSurface_->DetachBufferFromQueue(inBuffer);
        CHECK_AND_RETURN_LOG(detRet == GSError::GSERROR_OK,
                             "EffectSurfaceAdapter::OnBufferAvailable: detach buffer from consumerSurface_ failed");
        detRet = receiverConsumerSurface_->AttachBufferToQueue(outBuffer);
        CHECK_AND_RETURN_LOG(detRet == GSError::GSERROR_OK,
                             "EffectSurfaceAdapter::OnBufferAvailable: attach buffer from consumerSurface_ failed");
    }
    (void)receiverConsumerSurface_->ReleaseBuffer(releaseBuffer, IE_INVALID_FENCE);
}

void EffectSurfaceAdapter::OnTunnelHandleChange() {}
void EffectSurfaceAdapter::OnGoBackground() {}
void EffectSurfaceAdapter::OnCleanCache() {}
}
}
}