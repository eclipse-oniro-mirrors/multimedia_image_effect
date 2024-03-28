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

#ifndef IMAGE_EFFECT_MEMCPY_HELPER_H
#define IMAGE_EFFECT_MEMCPY_HELPER_H

#include <cstdint>

#include "effect_info.h"
#include "effect_buffer.h"
#include "effect_memory.h"

namespace OHOS {
namespace Media {
namespace Effect {
struct CopyInfo {
    BufferInfo bufferInfo;
    uint8_t *data = nullptr;
};

class MemcpyHelper {
public:
    static void CopyData(CopyInfo &src, CopyInfo &dst);
    static void CopyData(EffectBuffer *src, EffectBuffer *dst);
    static void CopyData(EffectBuffer *src, CopyInfo &dst);
    static void CopyData(CopyInfo &src, EffectBuffer *dst);
    static void CopyData(EffectBuffer *buffer, MemoryData *memoryData);
    static void CopyData(MemoryData *src, MemoryData *dst);
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_MEMCPY_HELPER_H