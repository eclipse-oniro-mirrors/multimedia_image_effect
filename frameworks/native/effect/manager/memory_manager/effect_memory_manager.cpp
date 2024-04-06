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

#include "effect_memory_manager.h"

#include "effect_log.h"
#include "effect_buffer.h"

namespace OHOS {
namespace Media {
namespace Effect {
ErrorCode EffectMemoryManager::Init(const std::shared_ptr<EffectBuffer> &srcEffectBuffer,
    const std::shared_ptr<EffectBuffer> &dstEffectBuffer)
{
    AddFilterMemory(srcEffectBuffer, MemDataType::INPUT,
        dstEffectBuffer == nullptr || srcEffectBuffer->buffer_ == dstEffectBuffer->buffer_);
    AddFilterMemory(dstEffectBuffer, MemDataType::OUTPUT,
        dstEffectBuffer != nullptr && srcEffectBuffer->buffer_ != dstEffectBuffer->buffer_);
    return ErrorCode::SUCCESS;
}

void EffectMemoryManager::SetIPType(IPType ipType)
{
    runningIPType_ = ipType;
}

void EffectMemoryManager::AddFilterMemory(const std::shared_ptr<EffectBuffer> &effectBuffer, MemDataType memDataType,
    bool isAllowModify)
{
    // dst effect buffer is null while not set output data.
    if (effectBuffer == nullptr || effectBuffer->buffer_ == nullptr) {
        return;
    }

    std::shared_ptr<Memory> memory = std::make_shared<Memory>();
    memory->memoryData_ = std::make_shared<MemoryData>();
    memory->memoryData_->data = effectBuffer->buffer_;
    memory->memoryData_->memoryInfo.bufferInfo = *effectBuffer->bufferInfo_;
    memory->memoryData_->memoryInfo.extra = effectBuffer->extraInfo_->surfaceBuffer;
    memory->memoryData_->memoryInfo.bufferType = effectBuffer->extraInfo_->bufferType;
    memory->memDataType_ = memDataType;
    memory->isAllowModify_ = isAllowModify;
    AddMemory(memory);
}

std::shared_ptr<Memory> AllocMemoryInner(MemoryInfo &allocMemInfo, BufferType allocBufferType)
{
    EFFECT_LOGI("Alloc Memory! bufferType=%{public}d", allocBufferType);
    std::unique_ptr<AbsMemory> absMemory = EffectMemory::CreateMemory(allocBufferType);
    CHECK_AND_RETURN_RET_LOG(absMemory != nullptr, nullptr,
        "absMemory is null! bufferType=%{public}d", allocBufferType);
    std::shared_ptr<MemoryData> memoryData = absMemory->Alloc(allocMemInfo);
    CHECK_AND_RETURN_RET_LOG(memoryData != nullptr, nullptr,
        "memoryData is null! bufferType=%{public}d", allocBufferType);

    std::shared_ptr<Memory> memory = std::make_shared<Memory>();
    memory->memoryData_ = memoryData;
    memory->isAllowModify_ = true;

    return memory;
}

MemoryData *EffectMemoryManager::AllocMemory(void *srcAddr, MemoryInfo &allocMemInfo)
{
    for (const auto &memory : memorys_) {
        if (!memory->isAllowModify_ || memory->memoryData_->data == srcAddr) {
            continue;
        }

        const MemoryInfo &memInfo = memory->memoryData_->memoryInfo;
        const BufferInfo &bufferInfo = memInfo.bufferInfo;
        const BufferInfo &allocBufInfo = allocMemInfo.bufferInfo;
        if (bufferInfo.width_ == allocBufInfo.width_ && bufferInfo.height_ == allocBufInfo.height_ &&
            (allocMemInfo.bufferType == BufferType::DEFAULT || allocMemInfo.bufferType == memInfo.bufferType)) {
            EFFECT_LOGD("reuse memory. width=%{public}d, height=%{public}d, addr=%{public}p, format=%{public}d, "
                "bufferType=%{public}d, allocBufType=%{public}d", bufferInfo.width_, bufferInfo.height_,
                memory->memoryData_->data, bufferInfo.formatType_, memInfo.bufferType, allocMemInfo.bufferType);
            return memory->memoryData_.get();
        }
    }

    BufferType allocBufferType = BufferType::DMA_BUFFER; // default alloc dma buffer
    if (runningIPType_ == IPType::CPU) {
        allocBufferType = BufferType::HEAP_MEMORY; // default alloc heap buffer on running with cpu filter
    }
    std::shared_ptr<Memory> memory = AllocMemoryInner(allocMemInfo, allocBufferType);
    CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr,
        "AllocMemory fail! bufferType=%{public}d", allocBufferType);
    AddMemory(memory);
    EFFECT_LOGD("alloc new memory. memorys size=%{public}zu", memorys_.size());
    return memory->memoryData_.get();
}

void EffectMemoryManager::AddMemory(std::shared_ptr<Memory> &memory)
{
    CHECK_AND_RETURN_LOG(memory != nullptr, "memory is null!");
    CHECK_AND_RETURN_LOG(memory->memoryData_ != nullptr && memory->memoryData_->data != nullptr,
        "memory data is null!");
    if (std::find(memorys_.begin(), memorys_.end(), memory) == memorys_.end()) {
        memorys_.emplace_back(memory);
    } else {
        EFFECT_LOGW("memory is already add! memory=%{public}p", memory.get());
    }
}

std::shared_ptr<Memory> EffectMemoryManager::GetAllocMemoryByAddr(void *addr)
{
    for (auto &memory : memorys_) {
        if (memory->memDataType_ != MemDataType::OTHER) {
            continue;
        }

        if (memory->memoryData_->data == addr) {
            EFFECT_LOGD("addr is find! addr=%{public}p, bufferType=%{public}d",
                addr, memory->memoryData_->memoryInfo.bufferType);
            return memory;
        }
    }
    EFFECT_LOGI("addr is not find! addr=%{public}p", addr);
    return nullptr;
}

void EffectMemoryManager::ClearMemory()
{
    EFFECT_LOGD("EffectMemoryManager::ClearMemory");
    memorys_.clear();
}

void EffectMemoryManager::Deinit()
{
    for (auto it = memorys_.begin(); it != memorys_.end();) {
        const MemDataType &memDataType_ = (*it)->memDataType_;
        if (memDataType_ == MemDataType::INPUT || memDataType_ == MemDataType::OUTPUT) {
            it = memorys_.erase(it);
        } else {
            ++it;
        }
    }
    EFFECT_LOGD("EffectMemoryManager: Deinit memorySize=%{public}zu", memorys_.size());
}
} // namespace Effect
} // namespace Media
} // namespace OHOS