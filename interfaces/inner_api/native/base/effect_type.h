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

#ifndef IMAGE_EFFECT_EFFECT_TYPE_H
#define IMAGE_EFFECT_EFFECT_TYPE_H

#include <stdint.h>

namespace OHOS {
namespace Media {
namespace Effect {
enum class ConfigType : int32_t {
    DEFAULT = 0,
    IPTYPE = 1,
};

enum class BufferType {
    DEFAULT,
    HEAP_MEMORY,
    DMA_BUFFER, // SurfaceBuffer
    SHARED_MEMORY,
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_EFFECT_TYPE_H
