/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef IE_PIPELINE_CORE_CAPABILITY_H
#define IE_PIPELINE_CORE_CAPABILITY_H

#include <memory>
#include <utility>

#include "effect_info.h"
#include "effect_buffer.h"

namespace OHOS {
namespace Media {
namespace Effect {
struct MemNegotiatedCap {
    IEffectFormat format = IEffectFormat::DEFAULT;
    uint32_t width = 0;
    uint32_t height = 0;
    bool isSupportEditSrc = true;
};

struct PixelFormatCap {
    std::map<IEffectFormat, std::vector<IPType>> formats;
};

struct Capability {
    Capability(std::string &name):name_(name) {}

    std::string &name_;
    std::shared_ptr<PixelFormatCap> pixelFormatCap_ = nullptr;
    std::shared_ptr<MemNegotiatedCap> memNegotiatedCap_ = nullptr;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IE_PIPELINE_CORE_CAPABILITY_H
