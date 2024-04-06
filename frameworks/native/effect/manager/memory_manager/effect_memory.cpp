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

#include "effect_memory.h"

#include <sys/mman.h>
#include <unistd.h>

#include "ashmem.h"
#include "common_utils.h"
#include "effect_log.h"
#include "format_helper.h"

namespace OHOS {
namespace Media {
namespace Effect {
constexpr int32_t MAX_RAM_SIZE = 600 * 1024 * 1024;

void ReleaseHeapMemory(void* &data)
{
    if (data != nullptr) {
        free(data);
        data = nullptr;
    }
}

HeapMemoryData::~HeapMemoryData()
{
    if (!memoryInfo.isAutoRelease) {
        return;
    }
    EFFECT_LOGI("HeapMemoryData destructor! bufferAddr=%{public}p", heapData);
    ReleaseHeapMemory(heapData);
}

std::shared_ptr<MemoryData> HeapMemory::Alloc(MemoryInfo &memoryInfo)
{
    uint32_t size = memoryInfo.bufferInfo.len_;
    EFFECT_LOGI("HeapMemory::Alloc size=%{public}d", size);
    CHECK_AND_RETURN_RET_LOG(size <= MAX_RAM_SIZE && size > 0, nullptr, "size out of range! size=%{public}d", size);

    auto *buffer = malloc(size);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "malloc fail!");
    EFFECT_LOGI("HeapMemory::Alloc alloc addr=%{public}p", buffer);

    std::shared_ptr<HeapMemoryData> memoryData = std::make_unique<HeapMemoryData>();
    memoryData->data = buffer;
    memoryData->memoryInfo = memoryInfo;
    memoryData->memoryInfo.bufferInfo.rowStride_ =
        FormatHelper::CalculateRowStride(memoryInfo.bufferInfo.width_, memoryInfo.bufferInfo.formatType_);
    memoryData->memoryInfo.bufferType = BufferType::HEAP_MEMORY;
    memoryData->heapData = buffer;
    memoryData_ = memoryData;

    return memoryData;
}

ErrorCode HeapMemory::Release()
{
    EFFECT_LOGI("HeapMemory::Release");
    if (memoryData_ == nullptr) {
        EFFECT_LOGE("HeapMemory::Release memoryData is null!");
        return ErrorCode::ERR_MEMORY_DATA_ABNORMAL;
    }

    ReleaseHeapMemory(memoryData_->heapData);
    memoryData_ = nullptr;
    return ErrorCode::SUCCESS;
}

void ReleaseDmaMemory(SurfaceBuffer* &surfaceBuffer)
{
    if (surfaceBuffer != nullptr) {
        surfaceBuffer->DecStrongRef(surfaceBuffer);
        surfaceBuffer = nullptr;
    }
}

DmaMemoryData::~DmaMemoryData()
{
    if (!memoryInfo.isAutoRelease) {
        return;
    }

    EFFECT_LOGI("DmaMemoryData destructor! surfaceBuffer=%{public}p", surfaceBuffer);
    ReleaseDmaMemory(surfaceBuffer);
}

uint32_t CalculateTotalSize(BufferInfo &bufferInfo)
{
    return FormatHelper::CalculateSize(bufferInfo.width_, bufferInfo.height_, bufferInfo.formatType_);
}

SurfaceBuffer *CreateSurfaceBuffer(BufferRequestConfig &requestConfig)
{
    sptr<SurfaceBuffer> sb = SurfaceBuffer::Create();
    auto ret = sb->Alloc(requestConfig);
    CHECK_AND_RETURN_RET_LOG(ret == 0, nullptr, "surface buffer Alloc fail! res=%{public}d", ret);
    ret = sb->Map();
    CHECK_AND_RETURN_RET_LOG(ret == 0, nullptr, "surface buffer Map fail! res=%{public}d", ret);
    sb->IncStrongRef(sb);
    return sb;
}

std::shared_ptr<MemoryData> DmaMemory::Alloc(MemoryInfo &memoryInfo)
{
    BufferInfo &bufferInfo = memoryInfo.bufferInfo;
    uint32_t size = bufferInfo.len_;
    EFFECT_LOGI("DmaMemory::Alloc size=%{public}d", size);
    CHECK_AND_RETURN_RET_LOG(size <= MAX_RAM_SIZE && size > 0, nullptr, "size out of range! size=%{public}d", size);
    CHECK_AND_RETURN_RET_LOG(bufferInfo.width_ > 0 && bufferInfo.height_ > 0 && CalculateTotalSize(bufferInfo) <= size,
        nullptr, "para calculated over alloc size! h=%{public}d, w=%{public}d, format=%{public}d, size=%{public}d",
        bufferInfo.height_, bufferInfo.width_, bufferInfo.formatType_, bufferInfo.len_);
    auto *src = reinterpret_cast<SurfaceBuffer *>(memoryInfo.extra);

    BufferRequestConfig requestConfig = {
        .width = static_cast<int32_t>(bufferInfo.width_),
        .height = static_cast<int32_t>(bufferInfo.height_),
        .strideAlignment = 0x8, // set 0x8 as default value to alloc SurfaceBufferImpl
        .format = CommonUtils::SwitchToPixelFormat(bufferInfo.formatType_), // PixelFormat
        .usage = src == nullptr ?
            (BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_MEM_MMZ_CACHE) :
            src->GetUsage(),
        .timeout = 0,
        .colorGamut = src == nullptr ? GraphicColorGamut::GRAPHIC_COLOR_GAMUT_SRGB : src->GetSurfaceBufferColorGamut(),
        .transform = src == nullptr ? GraphicTransformType::GRAPHIC_ROTATE_NONE : src->GetSurfaceBufferTransform(),
    };
    SurfaceBuffer *sb = CreateSurfaceBuffer(requestConfig);
    CHECK_AND_RETURN_RET_LOG(sb != nullptr, nullptr, "create surface buffer fail!");

    EFFECT_LOGI(
        "DmaMemory::Alloc seqNum=%{public}d, stride=%{public}d, size=%{public}d, format=%{public}d, addr=%{public}p",
        sb->GetSeqNum(), sb->GetStride(), sb->GetSize(), sb->GetFormat(), sb->GetVirAddr());
    std::shared_ptr<DmaMemoryData> memoryData = std::make_unique<DmaMemoryData>();
    memoryData->data = sb->GetVirAddr();
    memoryData->memoryInfo.isAutoRelease = memoryInfo.isAutoRelease;
    memoryData->memoryInfo.bufferInfo.width_ = static_cast<uint32_t>(sb->GetWidth());
    memoryData->memoryInfo.bufferInfo.height_ = static_cast<uint32_t>(sb->GetHeight());
    memoryData->memoryInfo.bufferInfo.formatType_ = bufferInfo.formatType_;
    memoryData->memoryInfo.bufferInfo.len_ = sb->GetSize();
    memoryData->memoryInfo.bufferInfo.rowStride_ = static_cast<uint32_t>(sb->GetStride());
    memoryData->memoryInfo.extra = sb;
    memoryData->memoryInfo.bufferType = BufferType::DMA_BUFFER;
    memoryData->surfaceBuffer = sb;
    memoryData_ = memoryData;

    return memoryData;
}

ErrorCode DmaMemory::Release()
{
    EFFECT_LOGI("DmaMemory::Release");
    if (memoryData_ == nullptr) {
        EFFECT_LOGE("DmaMemory::Release memoryData is null!");
        return ErrorCode::ERR_MEMORY_DATA_ABNORMAL;
    }
    ReleaseDmaMemory(memoryData_->surfaceBuffer);
    memoryData_ = nullptr;
    return ErrorCode::SUCCESS;
}

void ReleaseSharedMemory(void* &data, int* &fdPtr, size_t len)
{
    if (data != nullptr && data != MAP_FAILED) {
        ::munmap(data, len);
        data = nullptr;
    }
    if (fdPtr != nullptr) {
        ::close(*fdPtr);
        delete(fdPtr);
        fdPtr = nullptr;
    }
}

SharedMemoryData::~SharedMemoryData()
{
    if (!memoryInfo.isAutoRelease) {
        return;
    }
    EFFECT_LOGI("SharedMemoryData destructor! data=%{public}p, fdPtr=%{public}p, len=%{public}zu", data, fdPtr, len);
    ReleaseSharedMemory(data, fdPtr, len);
}

std::shared_ptr<MemoryData> SharedMemory::Alloc(MemoryInfo &memoryInfo)
{
    size_t size = memoryInfo.bufferInfo.len_;
    EFFECT_LOGI("SharedMemory::Alloc size=%{public}zu", size);
    CHECK_AND_RETURN_RET_LOG(size <= MAX_RAM_SIZE && size > 0, nullptr, "size out of range! size=%{public}zu", size);

    int fd = AshmemCreate("ImageEffectAlloc Data", size);
    CHECK_AND_RETURN_RET_LOG(fd >= 0, nullptr, "SharedMemory::Alloc AshmemCreate fd:[%{public}d].", fd);

    if (AshmemSetProt(fd, PROT_READ | PROT_WRITE) < 0) {
        EFFECT_LOGE("SharedMemory::Alloc AshmemSetProt errno %{public}d.", errno);
        ::close(fd);
        return nullptr;
    }
    void *data = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        EFFECT_LOGE("SharedMemory::Alloc mmap failed, errno:%{public}d", errno);
        ::close(fd);
        return nullptr;
    }
    std::shared_ptr<SharedMemoryData> memoryData = std::make_unique<SharedMemoryData>();
    memoryData->data = data;
    memoryData->memoryInfo = memoryInfo;
    memoryData->memoryInfo.bufferInfo.rowStride_ =
        FormatHelper::CalculateRowStride(memoryInfo.bufferInfo.width_, memoryInfo.bufferInfo.formatType_);
    std::unique_ptr<int> fdPtr = std::make_unique<int>(fd);
    memoryData->fdPtr = fdPtr.release();
    memoryData->memoryInfo.extra = memoryData->fdPtr;
    memoryData->memoryInfo.bufferType = BufferType::SHARED_MEMORY;
    memoryData->len = size;
    memoryData_ = memoryData;

    return memoryData;
}

ErrorCode SharedMemory::Release()
{
    EFFECT_LOGI("SharedMemory::Release");
    if (memoryData_ == nullptr) {
        EFFECT_LOGE("SharedMemory::Release memoryData is null!");
        return ErrorCode::ERR_MEMORY_DATA_ABNORMAL;
    }

    ReleaseSharedMemory(memoryData_->data, memoryData_->fdPtr, memoryData_->len);
    memoryData_ = nullptr;
    return ErrorCode::SUCCESS;
}

std::unique_ptr<AbsMemory> EffectMemory::CreateMemory(BufferType bufferType)
{
    std::unique_ptr<AbsMemory> res = nullptr;
    switch (bufferType) {
        case BufferType::HEAP_MEMORY:
            res = std::make_unique<HeapMemory>();
            break;
        case BufferType::DMA_BUFFER:
            res = std::make_unique<DmaMemory>();
            break;
        case BufferType::SHARED_MEMORY:
            res = std::make_unique<SharedMemory>();
            break;
        default:
            EFFECT_LOGE("buffer type not support! bufferType=%{public}d", bufferType);
            return nullptr;
    }
    return res;
}

} // namespace Effect
} // namespace Media
} // namespace OHOS