/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "image_source_filter.h"

#include "effect_log.h"
#include "filter_factory.h"
#include "memcpy_helper.h"
#include "render_environment.h"

namespace OHOS {
namespace Media {
namespace Effect {
REGISTER_FILTER_FACTORY(ImageSourceFilter);

ErrorCode ImageSourceFilter::SetSource(const std::shared_ptr<EffectBuffer> &source,
    std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGI("SetSource entered.");
    if (source == nullptr) {
        EFFECT_LOGE("Invalid source");
        return ErrorCode::ERR_INVALID_PARAMETER_VALUE;
    }

    srcBuffer_ = source;
    context_ = context;
    return ErrorCode::SUCCESS;
}

ErrorCode ImageSourceFilter::Prepare()
{
    state_ = FilterState::PREPARING;
    return DoNegotiate();
}

ErrorCode UpdateInputBufferIfNeed(std::shared_ptr<EffectBuffer> &srcBuffer, std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, ErrorCode::ERR_INPUT_NULL, "UpdateInputBufferIfNeed context is null!");
    if (context->ipType_ != IPType::GPU && context->renderEnvironment_->GetOutputType() == DataType::NATIVE_WINDOW) {
        MemoryInfo memInfo = {
            .bufferInfo = {
                .width_ = srcBuffer->bufferInfo_->width_,
                .height_ = srcBuffer->bufferInfo_->height_,
                .len_ = srcBuffer->bufferInfo_->len_,
                .formatType_ = srcBuffer->bufferInfo_->formatType_,
            },
            .bufferType = BufferType::DMA_BUFFER,
        };
        MemoryData *memoryData = context->memoryManager_->AllocMemory(srcBuffer->buffer_, memInfo);
        CHECK_AND_RETURN_RET_LOG(memoryData != nullptr, ErrorCode::ERR_ALLOC_MEMORY_FAIL, "Alloc new memory fail!");
        MemoryInfo &allocMemInfo = memoryData->memoryInfo;
        std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
        *bufferInfo = allocMemInfo.bufferInfo;
        std::shared_ptr<ExtraInfo> extraInfo = std::make_shared<ExtraInfo>();
        *extraInfo = *srcBuffer->extraInfo_;
        extraInfo->bufferType = allocMemInfo.bufferType;
        extraInfo->surfaceBuffer = (allocMemInfo.bufferType == BufferType::DMA_BUFFER) ?
            static_cast<SurfaceBuffer *>(allocMemInfo.extra) : nullptr;
        if (extraInfo->surfaceBuffer != nullptr) {
            GraphicTransformType transformType = srcBuffer->extraInfo_->surfaceBuffer->GetSurfaceBufferTransform();
            extraInfo->surfaceBuffer->SetSurfaceBufferTransform(transformType);
        }
        buffer = std::make_shared<EffectBuffer>(bufferInfo, memoryData->data, extraInfo);

        CopyInfo dst = {
            .bufferInfo = {
                .width_ = buffer->bufferInfo_->width_,
                .height_ = buffer->bufferInfo_->height_,
                .len_ = buffer->bufferInfo_->len_,
                .formatType_ = buffer->bufferInfo_->formatType_,
                .rowStride_ = buffer->bufferInfo_->rowStride_,
            },
            .data = static_cast<uint8_t *>(buffer->buffer_),
        };
        MemcpyHelper::CopyData(srcBuffer.get(), dst);
        return ErrorCode::SUCCESS;
    }

    if (context->ipType_ != IPType::GPU) {
        return ErrorCode::SUCCESS;
    }

    context->renderEnvironment_->BeginFrame();
    context->renderEnvironment_->GenMainTex(srcBuffer, buffer);
    return ErrorCode::SUCCESS;
}

ErrorCode ImageSourceFilter::Start()
{
    EFFECT_LOGI("Start entered.");
    state_ = FilterState::RUNNING;
    if (outPorts_[0] == nullptr) {
        EFFECT_LOGE("Start: outPort is null. filterName=%{public}s", name_.c_str());
        return ErrorCode::ERR_PIPELINE_INVALID_FILTER_PORT;
    }

    std::shared_ptr<EffectBuffer> buffer = srcBuffer_;
    ErrorCode res = UpdateInputBufferIfNeed(srcBuffer_, buffer, context_);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "Update input buffer fail! res=%{public}d", res);

    return outPorts_[0]->PushData(buffer, context_);
}

ErrorCode ImageSourceFilter::DoNegotiate()
{
    std::shared_ptr<MemNegotiatedCap> memNegotiatedCap = std::make_shared<MemNegotiatedCap>();
    memNegotiatedCap->width = srcBuffer_->bufferInfo_->width_;
    memNegotiatedCap->height = srcBuffer_->bufferInfo_->height_;
    memNegotiatedCap->format = srcBuffer_->bufferInfo_->formatType_;
    std::shared_ptr<Capability> capability = std::make_shared<Capability>(name_);
    capability->memNegotiatedCap_ = memNegotiatedCap;
    context_->capNegotiate_->AddCapability(capability);
    std::unordered_set<EffectColorSpace> allSupportedColorSpaces = ColorSpaceManager::GetAllSupportedColorSpaces();
    std::for_each(allSupportedColorSpaces.begin(), allSupportedColorSpaces.end(), [&](const auto &item) {
        context_->filtersSupportedColorSpace_.emplace(item);
    });

    if (outPorts_[0] == nullptr) {
        EFFECT_LOGE("Negotiate: outPort is null. filterName=%{public}s", name_.c_str());
        return ErrorCode::ERR_PIPELINE_INVALID_FILTER_PORT;
    }
    outPorts_[0]->Negotiate(capability, context_);
    if (context_->renderEnvironment_->GetEGLStatus() != EGLStatus::READY && context_->ipType_ == IPType::GPU) {
        context_->renderEnvironment_->Init();
        context_->renderEnvironment_->Prepare();
    }
    return ErrorCode::SUCCESS;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS
