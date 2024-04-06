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
#include "memory_manager.h"
#include "memcpy_helper.h"

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
    if (context->ipType_ != IPType::GPU) {
        return ErrorCode::SUCCESS;
    }
    if (srcBuffer->extraInfo_->bufferType == BufferType::DMA_BUFFER) {
        return ErrorCode::SUCCESS;
    }

    MemoryInfo memoryInfo = {
        .bufferInfo = *srcBuffer->bufferInfo_,
        .extra = static_cast<void *>(srcBuffer->extraInfo_->surfaceBuffer),
        .bufferType = BufferType::DMA_BUFFER, // alloc dma buffer
    };
    MemoryData *allocMemoryData = context->memoryManager_->AllocMemory(srcBuffer->buffer_, memoryInfo);
    CHECK_AND_RETURN_RET_LOG(allocMemoryData != nullptr, ErrorCode::ERR_ALLOC_MEMORY_FAIL, "Alloc new memory fail!");

    MemcpyHelper::CopyData(srcBuffer.get(), allocMemoryData);
    EFFECT_LOGD("Update input buffer. srcBufferType=%{public}d, allocBufferType=%{public}d",
        srcBuffer->extraInfo_->bufferType, allocMemoryData->memoryInfo.bufferType);
    MemoryInfo &allocMemInfo = allocMemoryData->memoryInfo;
    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    *bufferInfo = allocMemInfo.bufferInfo;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_shared<ExtraInfo>();
    *extraInfo = *srcBuffer->extraInfo_;
    extraInfo->bufferType = allocMemInfo.bufferType;
    extraInfo->surfaceBuffer = (allocMemInfo.bufferType == BufferType::DMA_BUFFER) ?
        static_cast<SurfaceBuffer *>(allocMemInfo.extra) : nullptr;

    buffer = std::make_shared<EffectBuffer>(bufferInfo, allocMemoryData->data, extraInfo);
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

    std::shared_ptr<EffectBuffer> &buffer = srcBuffer_;
    ErrorCode res = UpdateInputBufferIfNeed(srcBuffer_, buffer, context_);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "Update input buffer fail! res=%{public}d", res);

    outPorts_[0]->PushData(buffer, context_);
    return ErrorCode::SUCCESS;
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

    if (outPorts_[0] == nullptr) {
        EFFECT_LOGE("Negotiate: outPort is null. filterName=%{public}s", name_.c_str());
        return ErrorCode::ERR_PIPELINE_INVALID_FILTER_PORT;
    }
    outPorts_[0]->Negotiate(capability, context_);
    return ErrorCode::SUCCESS;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS
