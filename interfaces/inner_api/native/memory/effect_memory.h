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

#ifndef IMAGE_EFFECT_EFFECT_MEMORY_H
#define IMAGE_EFFECT_EFFECT_MEMORY_H

#include <memory>
#include "effect_info.h"
#include "effect_type.h"
#include "error_code.h"
#include "surface_buffer.h"
#include "effect_buffer.h"

namespace OHOS {
namespace Media {
namespace Effect {
struct MemoryInfo {
    bool isAutoRelease = true; // alloc memory is auto release or not.
    BufferInfo bufferInfo;
    void *extra = nullptr;
    BufferType bufferType = BufferType::DEFAULT;
};

struct MemoryData {
    void *data = nullptr; // buffer addr
    MemoryInfo memoryInfo;
};

class AbsMemory {
public:
    virtual ~AbsMemory() = default;
    virtual std::shared_ptr<MemoryData> Alloc(MemoryInfo &memoryInfo) = 0;
    virtual ErrorCode Release() = 0;
    virtual BufferType GetBufferType()
    {
        return BufferType::DEFAULT;
    }
};

struct HeapMemoryData : public MemoryData {
    ~HeapMemoryData();
    void *heapData = nullptr;
};

class HeapMemory : public AbsMemory {
public:
    ~HeapMemory() override = default;
    std::shared_ptr<MemoryData> Alloc(MemoryInfo &memoryInfo) override;
    ErrorCode Release() override;
    BufferType GetBufferType() override
    {
        return BufferType::HEAP_MEMORY;
    }
private:
    std::shared_ptr<HeapMemoryData> memoryData_ = nullptr;
};

struct DmaMemoryData : public MemoryData {
    ~DmaMemoryData();
    SurfaceBuffer *surfaceBuffer = nullptr;
};

class DmaMemory : public AbsMemory {
public:
    ~DmaMemory() override = default;
    std::shared_ptr<MemoryData> Alloc(MemoryInfo &memoryInfo) override;
    ErrorCode Release() override;
    BufferType GetBufferType() override
    {
        return BufferType::DMA_BUFFER;
    }
private:
    std::shared_ptr<DmaMemoryData> memoryData_ = nullptr;
};

struct SharedMemoryData : public MemoryData {
    ~SharedMemoryData();
    int* fdPtr = nullptr;
    size_t len = 0;
};

class SharedMemory : public AbsMemory {
public:
    ~SharedMemory() override = default;
    std::shared_ptr<MemoryData> Alloc(MemoryInfo &memoryInfo) override;
    ErrorCode Release() override;
    BufferType GetBufferType() override
    {
        return BufferType::SHARED_MEMORY;
    }
private:
    std::shared_ptr<SharedMemoryData> memoryData_ = nullptr;
};

class EffectMemory {
public:
    static std::unique_ptr<AbsMemory> CreateMemory(BufferType type);
};

} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_EFFECT_MEMORY_H
