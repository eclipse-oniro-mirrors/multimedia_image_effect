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
const int32_t STRUCT_EFFECT_SURFACE_CONSTANT = 1;
const int32_t DESTRUCTOR_EFFECT_SURFACE_CONSTANT = 2;

EffectSurfaceAdapter::EffectSurfaceAdapter()
{
    effectSurfaceFlag_ = STRUCT_EFFECT_SURFACE_CONSTANT;
}

EffectSurfaceAdapter::~EffectSurfaceAdapter()
{
    Destroy();
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
    CHECK_AND_RETURN_RET_LOG(!fromProducerSurface_, fromProducerSurface_,
        "EffectSurfaceAdapter::GetProducerSurface producerSurface exists.");

    auto ret = Initialize();
    CHECK_AND_RETURN_RET_LOG(ret == ErrorCode::SUCCESS, nullptr,
        "EffectSurfaceAdapter::GetProducerSurface Initialize failed.");

    return fromProducerSurface_;
}

sptr<IConsumerSurface> EffectSurfaceAdapter::GetConsumerSurface()
{
    CHECK_AND_RETURN_RET_LOG(!receiverConsumerSurface_, receiverConsumerSurface_,
        "EffectSurfaceAdapter::GetConsumerSurface consumerSurface exists.");

    auto ret = Initialize();
    CHECK_AND_RETURN_RET_LOG(ret == ErrorCode::SUCCESS, nullptr,
        "EffectSurfaceAdapter::GetConsumerSurface Initialize failed.");

    return receiverConsumerSurface_;
}

bool EffectSurfaceAdapter::CheckEffectSurface() const
{
    return effectSurfaceFlag_ == STRUCT_EFFECT_SURFACE_CONSTANT;
}

GSError EffectSurfaceAdapter::AcquireConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& syncFence,
    int64_t& timestamp, OHOS::Rect& damages) const
{
    CHECK_AND_RETURN_RET_LOG(receiverConsumerSurface_!= nullptr, GSERROR_NOT_INIT,
        "EffectSurfaceAdapter::AcquireEffectSurfaceBuffer receiverConsumerSurface_ is nullptr");

    return receiverConsumerSurface_->AcquireBuffer(buffer, syncFence, timestamp, damages);
}

GSError EffectSurfaceAdapter::ReleaseConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer,
    const sptr<SyncFence>& syncFence) const
{
    CHECK_AND_RETURN_RET_LOG(receiverConsumerSurface_!= nullptr, GSERROR_NOT_INIT,
        "EffectSurfaceAdapter::ReleaseEffectSurfaceBuffer receiverConsumerSurface_ is nullptr");

    return receiverConsumerSurface_->ReleaseBuffer(buffer, syncFence);
}

GSError EffectSurfaceAdapter::DetachConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer) const
{
    CHECK_AND_RETURN_RET_LOG(receiverConsumerSurface_!= nullptr, GSERROR_NOT_INIT,
        "EffectSurfaceAdapter::DetachEffectSurfaceBuffer receiverConsumerSurface_ is nullptr");

    return receiverConsumerSurface_->DetachBufferFromQueue(buffer, true);
}

GSError EffectSurfaceAdapter::AttachConsumerSurfaceBuffer(sptr<OHOS::SurfaceBuffer> &buffer) const
{
    CHECK_AND_RETURN_RET_LOG(receiverConsumerSurface_ != nullptr, GSERROR_NOT_INIT,
        "EffectSurfaceAdapter::AttachEffectSurfaceBuffer receiverConsumerSurface_ is nullptr");

    return receiverConsumerSurface_->AttachBufferToQueue(buffer);
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
    if (consumerBufferAvailable_) {
        consumerBufferAvailable_();
    } else {
        EFFECT_LOGE("not register handle buffer.");
    }
}

void EffectSurfaceAdapter::OnTunnelHandleChange() {}
void EffectSurfaceAdapter::OnGoBackground() {}
void EffectSurfaceAdapter::OnCleanCache(uint32_t* bufSeqNum)
{
    (void)bufSeqNum;
}

void EffectSurfaceAdapter::Destroy()
{
    if (receiverConsumerSurface_) {
        GSError result = receiverConsumerSurface_->UnregisterConsumerListener();
        EFFECT_LOGI("EffectSurfaceAdapter::Destroy UnregisterConsumerListener. result=%{public}d",
            result);
        effectSurfaceFlag_ = DESTRUCTOR_EFFECT_SURFACE_CONSTANT;
        receiverConsumerSurface_ = nullptr;
    }
}
}
}
}