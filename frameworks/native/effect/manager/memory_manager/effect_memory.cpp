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
#include "unique_fd.h"

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
    EFFECT_LOGI("HeapMemoryData destructor!");
    ReleaseHeapMemory(heapData);
}

std::shared_ptr<MemoryData> HeapMemory::Alloc(MemoryInfo &memoryInfo)
{
    uint32_t size = memoryInfo.bufferInfo.len_;
    EFFECT_LOGI("HeapMemory::Alloc size=%{public}d", size);
    CHECK_AND_RETURN_RET_LOG(size <= MAX_RAM_SIZE && size > 0, nullptr, "size out of range! size=%{public}d", size);

    auto *buffer = malloc(size);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "malloc fail!");
    EFFECT_LOGI("HeapMemory::Alloc alloc buffer success!");

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

    EFFECT_LOGI("DmaMemoryData destructor!");
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
    CHECK_AND_RETURN_RET_LOG(bufferInfo.width_ > 0 && bufferInfo.height_ > 0,
        nullptr, "para calculated over alloc size! h=%{public}d, w=%{public}d, format=%{public}d, size=%{public}d",
        bufferInfo.height_, bufferInfo.width_, bufferInfo.formatType_, bufferInfo.len_);
    auto *src = reinterpret_cast<SurfaceBuffer *>(memoryInfo.extra);

    BufferRequestConfig requestConfig = {
        .width = static_cast<int32_t>(bufferInfo.width_),
        .height = static_cast<int32_t>(bufferInfo.height_),
        .strideAlignment = 0x8, // set 0x8 as default value to alloc SurfaceBufferImpl
        .format = CommonUtils::SwitchToGraphicPixelFormat(bufferInfo.formatType_), // PixelFormat
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
        "DmaMemory::Alloc seqNum=%{public}d, stride=%{public}d, size=%{public}d, format=%{public}d",
        sb->GetSeqNum(), sb->GetStride(), sb->GetSize(), sb->GetFormat());
    std::shared_ptr<DmaMemoryData> memoryData = std::make_unique<DmaMemoryData>();
    memoryData->data = sb->GetVirAddr();
    memoryData->memoryInfo.isAutoRelease = memoryInfo.isAutoRelease;
    memoryData->memoryInfo.bufferInfo.width_ = static_cast<uint32_t>(sb->GetWidth());
    memoryData->memoryInfo.bufferInfo.height_ = static_cast<uint32_t>(sb->GetHeight());
    memoryData->memoryInfo.bufferInfo.formatType_ = bufferInfo.formatType_;
    memoryData->memoryInfo.bufferInfo.len_ = sb->GetSize();
    memoryData->memoryInfo.bufferInfo.rowStride_ = static_cast<uint32_t>(sb->GetStride());
    memoryData->memoryInfo.bufferInfo.colorSpace_ = memoryInfo.bufferInfo.colorSpace_;
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

void ReleaseSharedMemory(SharedMemoryData *memoryData)
{
    if (memoryData == nullptr) {
        return;
    }
    // sptr 引用清零，触发 ~Ashmem() 依次执行 UnmapAshmem(munmap) 和 CloseAshmem(close(fd))
    // fd 与 映射的释放交由 Ashmem 类统一管理， 避免手写 ::munmap 和 ::close(fd) 引发的 FDSAN 告警
    memoryData->ashmem = nullptr;
    memoryData->data = nullptr;
}

SharedMemoryData::~SharedMemoryData()
{
    if (!memoryInfo.isAutoRelease) {
        return;
    }
    EFFECT_LOGI("SharedMemoryData destructor! len=%{public}zu", len);
    if (fdPtr != nullptr && !fdTransferred) {
        // fd 未被消费方接管，通过 RAII 关闭
        UniqueFd exportFd(*fdPtr);
        delete(fdPtr);
        fdPtr = nullptr;
    }
    ReleaseSharedMemory(this);
}

std::shared_ptr<MemoryData> SharedMemory::Alloc(MemoryInfo &memoryInfo)
{
    uint32_t size = memoryInfo.bufferInfo.len_;
    EFFECT_LOGI("SharedMemory::Alloc size=%{public}u", size);
    CHECK_AND_RETURN_RET_LOG(size <= MAX_RAM_SIZE && size > 0, nullptr, "size out of range! size=%{public}u", size);

    // 使用 Ashmem 类替代裸 AshmemCreate/AshmemSetProt/mmap 接口，fd 与映射由 sptr RAII 管理，
    // 失败路径无需手写 ::close(fd)，规避 FDSAN 告警
    sptr<Ashmem> ashmem = Ashmem::CreateAshmem("ImageEffectAlloc Data", static_cast<int32_t>(size));
    CHECK_AND_RETURN_RET_LOG(ashmem != nullptr, nullptr, "SharedMemory::Alloc AshmemCreate failed.");

    if (!ashmem->SetProtection(PROT_READ | PROT_WRITE)) {
        EFFECT_LOGE("SharedMemory::Alloc AshmemSetProt errno %{public}d.", errno);
        // ashmem 出作用域析构时自动 close fd
        return nullptr;
    }
    if (!ashmem->MapReadAndWriteAshmem()) {
        EFFECT_LOGE("SharedMemory::Alloc AshmemMap errno %{public}d.", errno);
        // ashmem 出作用域析构时自动 close fd
        return nullptr;
    }

    // 映射成功后获取用户态地址，等价于 mmap 返回值
    void *data = const_cast<void *>(ashmem->ReadFromAshmem(0, 0));
    CHECK_AND_RETURN_RET_LOG(data != nullptr, nullptr, "SharedMemory::Alloc get mapped addr failed.");
    std::shared_ptr<SharedMemoryData> memoryData = std::make_unique<SharedMemoryData>();
    memoryData->data = data;
    memoryData->memoryInfo = memoryInfo;
    memoryData->memoryInfo.bufferInfo.rowStride_ =
        FormatHelper::CalculateRowStride(memoryInfo.bufferInfo.width_, memoryInfo.bufferInfo.formatType_);
    // 为外部消费方(如 PixelMap/跨进程)导出独立的 fd 副本，避免与内部 ashmem 的 fd 冲突导致重复 close
    // 消费方接管后自行 close，本类仅负责 delete 该内存
    UniqueFd exportFd(::dup(ashmem->GetAshmemFd()));
    CHECK_AND_RETURN_RET_LOG(exportFd.Get() >= 0, nullptr, "SharedMemory::Alloc dup fd failed.");
    memoryData->fdPtr = new int(exportFd.Release()); // 交由 SharedMemoryData 析构释放
    memoryData->memoryInfo.extra = memoryData->fdPtr;
    memoryData->memoryInfo.bufferType = BufferType::SHARED_MEMORY;
    memoryData->len = size;
    memoryData->ashmem = ashmem; // 保留 sptr 引用，确保映射和 fd 生命周期
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
    if (memoryData_->fdPtr != nullptr && !memoryData_->fdTransferred) {
        // fd 本体由 Ashmem 类管理，此处仅释放外部导出的独立句柄内存，不再执行close
        // fd 未被消费方接管，通过 RAII 关闭
        UniqueFd exportFd(*memoryData_->fdPtr);
        delete(memoryData_->fdPtr);
        memoryData_->fdPtr = nullptr;
    }
    // 置空 Ashmem，触发 unmap + close
    ReleaseSharedMemory(memoryData_.get());
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