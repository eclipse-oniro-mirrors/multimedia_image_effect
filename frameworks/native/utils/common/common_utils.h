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

#ifndef IMAGE_EFFECT_COMMON_UTILS_H
#define IMAGE_EFFECT_COMMON_UTILS_H

#include <memory>

#include "any.h"
#include "display_type.h"
#include "effect_buffer.h"
#include "effect_log.h"
#include "error_code.h"
#include "image_type.h"
#include "nlohmann/json.hpp"
#include "pixel_map.h"
#include "surface.h"
#include "effect_memory_manager.h"

namespace OHOS {
namespace Media {
namespace Effect {
class CommonUtils {
public:
    static ErrorCode LockPixelMap(PixelMap *pixelMap, std::shared_ptr<EffectBuffer> &effectBuffer);
    static ErrorCode ParseSurfaceData(OHOS::SurfaceBuffer *surfaceBuffer,
        std::shared_ptr<EffectBuffer> &effectBuffer, const DataType &dataType);
    static std::string UrlToPath(const std::string &url);
    static ErrorCode ParseUri(std::string &uri, std::shared_ptr<EffectBuffer> &effectBuffer, bool isOutputData);
    static ErrorCode ParsePath(std::string &path, std::shared_ptr<EffectBuffer> &effectBuffer, bool isOutputData);
    static void UnlockPixelMap(const PixelMap *pixelMap);
    static ErrorCode ParseAnyToJson(Plugin::Any &any, nlohmann::json &result);
    static bool EndsWithJPG(const std::string &input);
    static ErrorCode ModifyPixelMapProperty(PixelMap *pixelMap, const std::shared_ptr<EffectBuffer> &buffer,
        const std::shared_ptr<EffectMemoryManager> &memoryManager);

    template <class ValueType> static ErrorCode ParseAny(Plugin::Any any, ValueType &value)
    {
        auto result = Plugin::AnyCast<ValueType>(&any);
        if (result == nullptr) {
            EFFECT_LOGE("value type is not match!");
            return ErrorCode::ERR_ANY_CAST_TYPE_NOT_MATCH;
        }
        EFFECT_LOGI("value get success!");

        value = *result;
        return ErrorCode::SUCCESS;
    }

    template <class ValueType>
    static ErrorCode GetValue(const std::string &key, std::map<std::string, Plugin::Any> &valueMap, ValueType &value)
    {
        auto it = valueMap.find(key);
        if (it == valueMap.end()) {
            EFFECT_LOGE("key is not set! key=%{public}s", key.c_str());
            return ErrorCode::ERR_NO_VALUE_KEY;
        }

        auto result = Plugin::AnyCast<ValueType>(&it->second);
        if (result == nullptr) {
            EFFECT_LOGE("value type is not match! key=%{public}s", key.c_str());
            return ErrorCode::ERR_ANY_CAST_TYPE_NOT_MATCH;
        }
        EFFECT_LOGI("value get success! key=%{public}s", key.c_str());

        value = *result;
        return ErrorCode::SUCCESS;
    }

    static inline float Clip(float a, float aMin, float aMax)
    {
        return a > aMax ? aMax : (a < aMin ? aMin : a);
    }

    static IEffectFormat SwitchToEffectFormat(::PixelFormat pixelFormat);
    static ::PixelFormat SwitchToPixelFormat(IEffectFormat formatType);
    static IEffectFormat SwitchToEffectFormat(PixelFormat pixelFormat);
    static BufferType SwitchToEffectBuffType(AllocatorType allocatorType);

private:
    static const std::unordered_map<PixelFormat, IEffectFormat> pixelFmtToEffectFmt_;
    static const std::unordered_map<::PixelFormat, IEffectFormat> surfaceBufferFmtToEffectFmt_;
    static const std::unordered_map<AllocatorType, BufferType> allocatorTypeToEffectBuffType_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_COMMON_UTILS_H