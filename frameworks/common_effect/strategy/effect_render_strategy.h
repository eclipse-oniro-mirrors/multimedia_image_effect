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

#ifndef IEFFECT_EFFECT_RENDER_STRATEGY_H
#define IEFFECT_EFFECT_RENDER_STRATEGY_H

#include "effect_buffer.h"
#include "capability.h"

namespace OHOS {
namespace Media {
namespace Effect {
class EffectRenderStrategy {
public:
    EffectRenderStrategy() = default;
    ~EffectRenderStrategy() = default;

    void Init(EffectBuffer *src, EffectBuffer *dst);

    EffectBuffer *ChooseBestOutput(EffectBuffer *buffer, std::shared_ptr<MemNegotiateCap> &memNegotiateCap);

    EffectBuffer *GetInput();

    EffectBuffer *GetOutput();

    void Deinit();
private:
    EffectBuffer *src_ = nullptr;
    EffectBuffer *dst_ = nullptr;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IEFFECT_EFFECT_RENDER_STRATEGY_H