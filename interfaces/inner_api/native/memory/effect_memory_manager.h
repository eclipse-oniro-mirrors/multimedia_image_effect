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

#ifndef IMAGE_EFFECT_EFFECT_MEMORY_MANAGER_H
#define IMAGE_EFFECT_EFFECT_MEMORY_MANAGER_H

#include "effect_memory.h"
#include "error_code.h"
#include "effect_buffer.h"

namespace OHOS {
namespace Media {
namespace Effect {

enum class MemDataType {
    INPUT = 0,
    OUTPUT,
    OTHER,
};

struct Memory {
    std::shared_ptr<MemoryData> memoryData_ = nullptr;
    MemDataType memDataType_ = MemDataType::OTHER;
    bool isAllowModify_ = true;
};

class EffectMemoryManager {
public:
    EffectMemoryManager() = default;
    ~EffectMemoryManager() = default;

    ErrorCode Init(const std::shared_ptr<EffectBuffer> &srcEffectBuffer,
        const std::shared_ptr<EffectBuffer> &dstEffectBuffer);
    void SetIPType(IPType ipType);

    MemoryData *AllocMemory(void *srcAddr, MemoryInfo &allocMemInfo);
    std::shared_ptr<Memory> GetAllocMemoryByAddr(void *addr);

    void ClearMemory();

    void Deinit();
private:
    void AddFilterMemory(const std::shared_ptr<EffectBuffer> &effectBuffer, MemDataType memDataType,
        bool isAllowModify);
    void AddMemory(std::shared_ptr<Memory> &memory);

    std::vector<std::shared_ptr<Memory>> memorys_;
    IPType runningIPType_ = IPType::DEFAULT;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_EFFECT_MEMORY_MANAGER_H
