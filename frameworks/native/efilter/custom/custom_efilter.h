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

#ifndef IMAGE_EFFECT_CUSTOM_EFILTER_H
#define IMAGE_EFFECT_CUSTOM_EFILTER_H

#include <string>
#include <mutex>

#include "any.h"
#include "effect_log.h"
#include "efilter_factory.h"
#include "error_code.h"

namespace OHOS {
namespace Media {
namespace Effect {
class CustomEFilter : public EFilter {
public:
    explicit CustomEFilter(const std::string &name) : EFilter(name)
    {
        EFFECT_LOGI("CustomEFilter Constructor name = %{public}s", name.c_str());
        delegate_ = EFilterFactory::Instance()->GetDelegate(name);
    }

    ~CustomEFilter() override = default;

    ErrorCode Render(EffectBuffer *buffer, std::shared_ptr<EffectContext> &context) override;

    ErrorCode Render(EffectBuffer *src, EffectBuffer *dst, std::shared_ptr<EffectContext> &context) override;

    ErrorCode SetValue(const std::string &key, Any &value) override;

    ErrorCode Save(EffectJsonPtr &res) override;

    ErrorCode Restore(const EffectJsonPtr &value) override;

    static void SetEffectInfo(const std::string &name, const std::shared_ptr<EffectInfo> &effectInfo);

    static std::shared_ptr<EffectInfo> GetEffectInfo(const std::string &name);

    void SetHandler(void *handler)
    {
        handler_ = handler;
    }

private:
    std::shared_ptr<IFilterDelegate> delegate_;
    void *handler_ = nullptr;
    static std::map<std::string, std::shared_ptr<EffectInfo>> effectInfos_;
    static std::mutex effectInfosMutex_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_CUSTOM_EFILTER_H