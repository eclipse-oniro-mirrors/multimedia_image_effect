/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "memory_manager.h"

#include "effect_log.h"
#include "securec.h"

namespace OHOS {
namespace Media {
namespace Effect {
constexpr int32_t MAX_RAM_SIZE = 600 * 1024 * 1024;

void FreeMallocBuffer(void *buffer)
{
    EFFECT_LOGI("Free Alloc buffer addr=%{public}p", buffer);
    if (buffer != nullptr) {
        free(buffer);
    }
}

std::shared_ptr<uint8_t[]> MemoryManager::AllocMemory(size_t size)
{
    EFFECT_LOGI("AllocMemory size=%{public}zu", size);
    CHECK_AND_RETURN_RET_LOG(size <= MAX_RAM_SIZE && size > 0, nullptr,
        "data size out of range! data.size=%{public}zu", size);

    auto *buffer = static_cast<uint8_t *>(malloc(size));
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "malloc fail!");
    std::shared_ptr<uint8_t[]> data(buffer, FreeMallocBuffer);
    EFFECT_LOGI("AllocMemory addr=%{public}p", data.get());
    return data;
}

void MemoryManager::ReleaseMemory(std::shared_ptr<uint8_t[]> &data)
{
    EFFECT_LOGI("ReleaseMemory");
    data = nullptr;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS