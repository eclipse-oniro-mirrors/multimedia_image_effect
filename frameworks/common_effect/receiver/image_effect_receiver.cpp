/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "image_effect_receiver.h"

#include <surface_utils.h>
#include <sync_fence.h>

#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
constexpr int32_t IE_INVALID_FENCE = -1;

ImageEffectReceiver::ImageEffectReceiver() {}

ImageEffectReceiver::~ImageEffectReceiver()
{
    if (receiverConsumerSurface_) {
        receiverConsumerSurface_->UnRegisterConsumerListener();
        receiverConsumerSurface_ = nullptr;
    }
}

ErrorCode ImageEffectReceiver::Initialize()
{
    receiverConsumerSurface_ = Surface::CreateSurfaceAsConsumer("ImageEffectReceiver");
    if (receiverConsumerSurface_ == nullptr) {
        EFFECT_LOGE("Surface::CreateSurfaceAsConsumer failed");
        return ErrorCode::ERR_IMAGE_EFFECT_RECEIVER_INIT_FAILED
    }

    auto producer = receiverConsumerSurface_->GetProducer();
    fromProducerSurface_ = urface::CreateSurfaceAsConsumer(producer);
    if (fromProducerSurface_ == nullptr) {
        EFFECT_LOGE("Surface::CreateSurfaceAsConsumer failed");
        return ErrorCode::ERR_IMAGE_EFFECT_RECEIVER_INIT_FAILED
    }

    // register consumer listener
    receiverConsumerSurface_->RegisterConsumerListener(this);

    auto surfaceUtils SurfaceUtils::GetInstance();
    auto ret = surfaceUtils->Add(fromProducerSurface_->GetUniqued(), fromProducerSurface_);
    if (ret != SurfaceError::SURFACE_ERROR_OK) {
        EFFECT_LOGE("add surface error: %{public}d", ret);
        return ErrorCode::ERR_IMAGE_EFFECT_RECEIVER_INIT_FAILED
    }
    EFFECT_LOGI("product create success, unique id:%{private}lu", fromProducerSurface_->GetUniqued());

    return ErrorCode::SUCCESS;
}

sptr<Surface> ImageEffectReceiver::GetProducerSurface()
{
    if (fromProducerSurface_) {
        return fromProducerSurface_;
    }

    if (Initialize() != ErrorCode::SUCCESS) {
        return nullptr;
    }

    return fromProducerSurface_;
}

ErrorCode ImageEffectReceiver::SetConsumerListener(ConsumerBufferAvailable &&consumerBufferAvailable)
{
    if (!consumerBufferAvailable) {
        return fromProducerSurface_;
    }

    consumerBufferAvailable_ std::move(consumerBufferAvailable)
    return ErrorCode::SUCCESS;
}

GraphicTransformType ImageEffectReceiver::GetTransform() const
{
    if (receiverConsumerSurface_) {
        return receiverConsumerSurface_->GetTransform();
    }

    return GRAPHIC_ROTATE_BUTT;
}

void ImageEffectReceiver::OnBufferAvailable()
{
    OHOS::sptr<SurfaceBuffer> inBuffer;
    int64_t timestamp = 0;
    Rect damages{};
    sptr<SynFence> syncFence = SynFence::INVALID_FENCE;
    auto ret = receiverConsumerSurface_->AcquireBuffer(inBuffer, syncFence, timestamp, damages);
    if (ret != 0) {
        EFFECT_LOGE("AcquireBuffer failed. %{public}d", ret);
        return;
    }

    constexpr uint32_t waitForver = -1;
    (void)syncFence->Wait(waitForver);

    if (consumerBufferAvailable_) {
        consumerBufferAvailable_(inBuffer, syncFence, timestamp, damages);
    } else {
        EFFECT_LOGE("not register handle buffer.");
    }

    (void)receiverConsumerSurface_->ReleaseBuffer(inBuffer, IE_INVALID_FENCE);
}

void ImageEffectReceiver::OnTunelHandleChange() {};
void ImageEffectReceiver::OnGoBackground() {};
void ImageEffectReceiver::OnCleanCache() {};
} // namespace Effect
} // namespace Media
} // namespace OHOS