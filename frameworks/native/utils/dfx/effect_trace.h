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

#ifndef CODE_EFFECT_TRACE_H
#define CODE_EFFECT_TRACE_H

#include "hitrace_meter.h"

#define PASTE(x, y) x##y
#define EFFECT_TRACE_NAME(name) ::OHOS::Media::Effect::EffectTrace \
    PASTE(__tracer, __LINE__)(HITRACE_TAG_ZIMAGE, name)

#define EFFECT_TRACE_CALL() EFFECT_TRACE_NAME(__FUNCTION__)

#define EFFECT_TRACE_BEGIN(name) StartTrace(HITRACE_TAG_ZIMAGE, name)
#define EFFECT_TRACE_END() FinishTrace(HITRACE_TAG_ZIMAGE)

namespace OHOS {
namespace Media {
namespace Effect {
class EffectTrace {
public:
    inline EffectTrace(std::uint64_t tag, const std::string &title) : mTag(tag)
    {
        StartTrace(mTag, title);
    }

    inline ~EffectTrace()
    {
        FinishTrace(mTag);
    }

private:
    std::uint64_t mTag;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS


#endif // CODE_EFFECT_TRACE_H
