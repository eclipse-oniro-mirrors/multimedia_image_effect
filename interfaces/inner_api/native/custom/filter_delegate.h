/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef IMAGE_EFFECT_FILTER_DELEGATE_H
#define IMAGE_EFFECT_FILTER_DELEGATE_H

#include <memory>

#include "delegate.h"
#include "any.h"
#include "effect_buffer.h"
#include "effect_info.h"
#include "effect_context.h"
#include "image_effect_marco_define.h"
#include "effect_json_helper.h"
#include "native_effect_base.h"

namespace OHOS {
namespace Media {
namespace Effect {
class FilterDelegate : public IFilterDelegate {
public:
    FilterDelegate(const OH_EffectFilterInfo *info, const ImageEffect_FilterDelegate *delegate)
        : ohInfo_(*info), ohDelegate_(delegate) {}

    ~FilterDelegate() override = default;

    bool Render(void *efilter, EffectBuffer *src, EffectBuffer *dst, std::shared_ptr<EffectContext> &context) override;

    bool Render(void *efilter, EffectBuffer *src, std::shared_ptr<EffectContext> &context) override;

    bool SetValue(void *efilter, const std::string &key, const Any &value) override;

    bool Save(void *efilter, EffectJsonPtr &res) override;

    void *Restore(const EffectJsonPtr &values) override;

    void *GetEffectInfo() override;
protected:
    static void PushData(OH_EffectFilter *filter, OH_EffectBufferInfo *dst);
private:
    OH_EffectFilterInfo ohInfo_;
    const ImageEffect_FilterDelegate *ohDelegate_;

    static std::shared_ptr<EffectBuffer> GenDstEffectBuffer(const OH_EffectBufferInfo *dst, const EffectBuffer *src);
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_FILTER_DELEGATE_H