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

#include "effect_context.h"
#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {

std::shared_ptr<ExifMetadata> GetExifMetadataFromPixelmap(PixelMap *pixelMap)
{
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, nullptr,
        "EffectContext::GetExifMetadataFromPixelmap: pixelmap is null");

    return pixelMap->GetExifMetadata();
}

std::shared_ptr<ExifMetadata> GetExifMetadataFromPicture(Picture *picture)
{
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, nullptr,
        "EffectContext::GetExifMetadataFromPicture: picture is null");

    return picture->GetExifMetadata();
}

std::shared_ptr<ExifMetadata> EffectContext::GetExifMetadata()
{
    CHECK_AND_RETURN_RET_LOG(renderStrategy_ != nullptr, nullptr,
        "EffectContext::GetExifMetadata: renderStrategy_ is null");

    EffectBuffer *src = renderStrategy_->GetInput();
    if (src == nullptr) {
        return exifMetadata_;
    }

    std::shared_ptr<ExtraInfo> &extraInfo = src->extraInfo_;
    std::shared_ptr<BufferInfo> &bufferInfo = src->bufferInfo_;
    CHECK_AND_RETURN_RET_LOG(extraInfo != nullptr, nullptr, "EffectContext::GetExifMetadata: extraInfo is null");
    CHECK_AND_RETURN_RET_LOG(bufferInfo != nullptr, nullptr, "EffectContext::GetExifMetadata: bufferInfo is null");

    switch (extraInfo->dataType) {
        case DataType::PIXEL_MAP:
            return GetExifMetadataFromPixelmap(bufferInfo->pixelMap_);
        case DataType::PATH:
        case DataType::URI:
            return GetExifMetadataFromPicture(extraInfo->innerPicture.get());
        case DataType::PICTURE:
            return GetExifMetadataFromPicture(extraInfo->picture);
        default:
            EFFECT_LOGW("EffectContext::GetExifMetadata: dataType=%{public}d not contain exif!", extraInfo->dataType);
            return nullptr;
    }
}

} // namespace Effect
} // namespace Media
} // namespace OHOS
