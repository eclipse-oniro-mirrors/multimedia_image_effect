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
#include <cstdarg>

#include "any.h"
#include "effect_buffer.h"
#include "effect_log.h"
#include "error_code.h"
#include "image_type.h"
#include "pixel_map.h"
#include "surface.h"
#include "effect_memory_manager.h"
#include "effect_context.h"
#include "image_effect_marco_define.h"
#include "effect_json_helper.h"
#include "picture.h"
#include "image_source.h"

namespace OHOS {
namespace Media {
namespace Effect {
class CommonUtils {
public:
    static const int32_t RGBA_BYTES_PER_PIXEL = 4;

    static void CopyBufferInfo(const BufferInfo& src, BufferInfo& dst);
    static void CopyExtraInfo(const ExtraInfo& src, ExtraInfo& dst);
    static void CopyAuxiliaryBufferInfos(const EffectBuffer *src, EffectBuffer *dst);

    static MetaDataMap GetMetaData(SurfaceBuffer *surfaceBuffer);
    static void SetMetaData(MetaDataMap &metaData, SurfaceBuffer *surfaceBuffer);
    
    static ErrorCode ParsePixelMapData(PixelMap *pixelMap, std::shared_ptr<EffectBuffer> &effectBuffer);
    IMAGE_EFFECT_EXPORT static ErrorCode LockPixelMap(PixelMap *pixelMap, std::shared_ptr<EffectBuffer> &effectBuffer);
    static ErrorCode ParseSurfaceData(OHOS::SurfaceBuffer *surfaceBuffer, std::shared_ptr<EffectBuffer> &effectBuffer,
        const DataType &dataType, LOG_STRATEGY strategy = LOG_STRATEGY::NORMAL, int64_t timestamp = 0);
    static std::string UrlToPath(const std::string &url);
    static ErrorCode ParseUri(std::string &uri, std::shared_ptr<EffectBuffer> &effectBuffer, bool isOutputData,
        IEffectFormat format);
    static ErrorCode ParsePath(std::string &path, std::shared_ptr<EffectBuffer> &effectBuffer, bool isOutputData,
        IEffectFormat format);
    IMAGE_EFFECT_EXPORT static ErrorCode ParseTex(unsigned int textureId, unsigned int colorSpace,
        std::shared_ptr<EffectBuffer> &effectBuffer);
    IMAGE_EFFECT_EXPORT static void UnlockPixelMap(const PixelMap *pixelMap);
    static ErrorCode ParseAnyAndAddToJson(const std::string &key, Any &any, EffectJsonPtr &result);
    static bool EndsWithJPG(const std::string &input);
    static bool EndsWithHEIF(const std::string &input);
    static ErrorCode ModifyPixelMapProperty(PixelMap *pixelMap, const std::shared_ptr<EffectBuffer> &buffer,
        const std::shared_ptr<EffectContext> &context, bool isUpdateExif = true);
    static ErrorCode ModifyPixelMapPropertyForTexture(PixelMap *pixelMap, const std::shared_ptr<EffectBuffer> &buffer,
        const std::shared_ptr<EffectContext> &context, bool isUpdateExif = true);
    static ErrorCode ParseNativeWindowData(std::shared_ptr<EffectBuffer> &effectBuffer, const DataType &dataType);
    static void UpdateImageExifDateTime(PixelMap *pixelMap);
    static void UpdateImageExifDateTime(Picture *picture);
    static void UpdateImageExifInfo(PixelMap *pixelMap);
    static void UpdateImageExifInfo(Picture *picture);
    static ErrorCode ParsePicture(Picture *picture, std::shared_ptr<EffectBuffer> &effectBuffer);
    static bool IsEnableCopyMetaData(int numBuffers, ...);
    IMAGE_EFFECT_EXPORT static void CopyTexture(const std::shared_ptr<EffectContext> &context, RenderTexturePtr input,
        RenderTexturePtr output);

    static std::shared_ptr<ImageSource> GetImageSourceFromPath(std::string path);

    template <class ValueType> static ErrorCode ParseAny(Any any, ValueType &value)
    {
        auto result = AnyCast<ValueType>(&any);
        if (result == nullptr) {
            EFFECT_LOGE("value type is not match!");
            return ErrorCode::ERR_ANY_CAST_TYPE_NOT_MATCH;
        }
        EFFECT_LOGD("value get success!");

        value = *result;
        return ErrorCode::SUCCESS;
    }

    template <class ValueType>
    static ErrorCode GetValue(const std::string &key, std::map<std::string, Any> &valueMap, ValueType &value)
    {
        auto it = valueMap.find(key);
        if (it == valueMap.end()) {
            EFFECT_LOGE("key is not set! key=%{public}s", key.c_str());
            return ErrorCode::ERR_NO_VALUE_KEY;
        }

        auto result = AnyCast<ValueType>(&it->second);
        if (result == nullptr) {
            EFFECT_LOGE("value type is not match! key=%{public}s", key.c_str());
            return ErrorCode::ERR_ANY_CAST_TYPE_NOT_MATCH;
        }
        EFFECT_LOGD("value get success! key=%{public}s", key.c_str());

        value = *result;
        return ErrorCode::SUCCESS;
    }

    static inline float Clip(float a, float aMin, float aMax)
    {
        return a > aMax ? aMax : (a < aMin ? aMin : a);
    }

    static IEffectFormat SwitchToEffectFormat(GraphicPixelFormat pixelFormat);
    static IEffectFormat SwitchToEffectFormat(PixelFormat pixelFormat);
    static GraphicPixelFormat SwitchToGraphicPixelFormat(IEffectFormat formatType);
    static PixelFormat SwitchToPixelFormat(IEffectFormat formatType);
    static BufferType SwitchToEffectBuffType(AllocatorType allocatorType);
    static PixelFormat SwitchGLFormatToPixelFormat(unsigned int formatType);
    static IEffectFormat SwitchGLFormatToEffectFormat(unsigned int formatType);

private:
    static const std::unordered_map<PixelFormat, IEffectFormat> pixelFmtToEffectFmt_;
    static const std::unordered_map<GraphicPixelFormat, IEffectFormat> surfaceBufferFmtToEffectFmt_;
    static const std::unordered_map<AllocatorType, BufferType> allocatorTypeToEffectBuffType_;
    static const std::unordered_map<unsigned int, PixelFormat> glFmtToPixelFmt_;
    static const std::unordered_map<unsigned int, IEffectFormat> glFmtToEffectFmt_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_COMMON_UTILS_H
