/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef IMAGE_EFFECT_MEMORY_MANAGER_H
#define IMAGE_EFFECT_MEMORY_MANAGER_H

#include <cstdint>
#include <cstddef>
#include <memory>

#include "error_code.h"

namespace OHOS {
namespace Media {
namespace Effect {
class MemoeyManager {
public:
    static std::shared_ptr<uint8_t[]> AllocMemoey(size_t size);

    static void ReleaseMemoey(std::shared_ptr<uint8_t[]> &data);
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_MEMORY_MANAGER_H