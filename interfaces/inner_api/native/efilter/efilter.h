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

#ifndef IMAGE_EFFECT_EFILTER_H
#define IMAGE_EFFECT_EFILTER_H

#include <map>
#include <string>

#include "any.h"
#include "effect_buffer.h"
#include "effect_type.h"
#include "efilter_base.h"
#include "error_code.h"
#include "effect_json_helper.h"
#include "image_effect_marco_define.h"
#include "efilter_cache_config.h"

namespace OHOS {
namespace Media {
namespace Effect {
class EFilter : public EFilterBase {
public:
    class Parameter {
    public:
        static const std::string KEY_DEFAULT_VALUE;
    };

    IMAGE_EFFECT_EXPORT explicit EFilter(const std::string &name);

    IMAGE_EFFECT_EXPORT ~EFilter() override;

    IMAGE_EFFECT_EXPORT virtual ErrorCode Render(EffectBuffer *buffer, std::shared_ptr<EffectContext> &context) = 0;

    IMAGE_EFFECT_EXPORT ErrorCode Render(std::shared_ptr<EffectBuffer> &src, std::shared_ptr<EffectBuffer> &dst);

    IMAGE_EFFECT_EXPORT virtual ErrorCode PreRender(IEffectFormat &format);

    IMAGE_EFFECT_EXPORT
    virtual ErrorCode Render(EffectBuffer *src, EffectBuffer *dst, std::shared_ptr<EffectContext> &context) = 0;

    IMAGE_EFFECT_EXPORT virtual ErrorCode SetValue(const std::string &key, Plugin::Any &value);

    IMAGE_EFFECT_EXPORT virtual ErrorCode GetValue(const std::string &key, Plugin::Any &value);

    IMAGE_EFFECT_EXPORT virtual ErrorCode Save(EffectJsonPtr &res);

    IMAGE_EFFECT_EXPORT virtual ErrorCode Restore(const EffectJsonPtr &values) = 0;

    static std::shared_ptr<EffectInfo> GetEffectInfo(const std::string &name)
    {
        return nullptr;
    }

    IMAGE_EFFECT_EXPORT void HandleCacheStart(const std::shared_ptr<EffectBuffer>& source,
        std::shared_ptr<EffectContext>& context);

    IMAGE_EFFECT_EXPORT ErrorCode PushData(const std::string &inPort, const std::shared_ptr<EffectBuffer> &buffer,
        std::shared_ptr<EffectContext> &context) override;

    IMAGE_EFFECT_EXPORT ErrorCode PushData(EffectBuffer *buffer, std::shared_ptr<EffectContext> &context);

    std::map<std::string, Plugin::Any> &GetValues()
    {
        return values_;
    }

    IMAGE_EFFECT_EXPORT
    ErrorCode ProcessConfig(const std::string &key);

    IMAGE_EFFECT_EXPORT
    ErrorCode StartCache();

    IMAGE_EFFECT_EXPORT
    virtual ErrorCode CacheBuffer(EffectBuffer *cache, std::shared_ptr<EffectContext> &context);

    IMAGE_EFFECT_EXPORT
    virtual ErrorCode GetFilterCache(std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context);

    IMAGE_EFFECT_EXPORT
    virtual ErrorCode ReleaseCache();

    IMAGE_EFFECT_EXPORT
    ErrorCode CancelCache();

    IMAGE_EFFECT_EXPORT
    virtual std::shared_ptr<MemNegotiatedCap> Negotiate(const std::shared_ptr<MemNegotiatedCap> &input,
        std::shared_ptr<EffectContext> &context);

    IMAGE_EFFECT_EXPORT
    virtual bool IsTextureInput();
protected:
    ErrorCode CalculateEFilterIPType(IEffectFormat &formatType, IPType &ipType);

    std::map<std::string, Plugin::Any> values_;

    std::shared_ptr<EFilterCacheConfig> cacheConfig_ = nullptr;

    LOG_STRATEGY logStrategy_ = LOG_STRATEGY::NORMAL;
private:
    IMAGE_EFFECT_EXPORT void Negotiate(const std::string &inPort, const std::shared_ptr<Capability> &capability,
        std::shared_ptr<EffectContext> &context) override;

    std::shared_ptr<EffectBuffer> IpTypeConvert(const std::shared_ptr<EffectBuffer> &buffer,
        std::shared_ptr<EffectContext> &context);

    ErrorCode RenderWithGPU(std::shared_ptr<EffectContext> &context, std::shared_ptr<EffectBuffer> &src,
        std::shared_ptr<EffectBuffer> &dst, bool needModifySource);

    ErrorCode RenderInner(std::shared_ptr<EffectBuffer> &src, std::shared_ptr<EffectBuffer> &dst);

    std::shared_ptr<Capability> outputCap_ = nullptr;

    static std::shared_ptr<EffectBuffer> CreateEffectBufferFromTexture(const std::shared_ptr<EffectBuffer> &buffer,
        const std::shared_ptr<EffectContext> &context);

    std::shared_ptr<EffectBuffer> ConvertFromGPU2CPU(const std::shared_ptr<EffectBuffer> &buffer,
        std::shared_ptr<EffectContext> &context, std::shared_ptr<EffectBuffer> &source);

    std::shared_ptr<EffectBuffer> ConvertFromCPU2GPU(const std::shared_ptr<EffectBuffer> &buffer,
        std::shared_ptr<EffectContext> &context, std::shared_ptr<EffectBuffer> &source);

    std::shared_ptr<EffectBuffer> cacheBuffer_ = nullptr;
    std::unique_ptr<AbsMemory> cacheMemory_ = nullptr;

    ErrorCode UseCache(std::shared_ptr<EffectContext> &context);

    ErrorCode AllocBuffer(std::shared_ptr<EffectContext> &context,
        const std::shared_ptr<MemNegotiatedCap> &memNegotiatedCap, std::shared_ptr<EffectBuffer> &source,
        std::shared_ptr<EffectBuffer> &effectBuffer) const;

    ErrorCode UseTextureInput();
    void InitContext(std::shared_ptr<EffectContext> &context, IPType &runningType, bool isCustomEnv);
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_EFILTER_H
