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

#include "test_pixel_map_utils.h"
#include "image_source.h"
#include "test_effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
namespace Test {
    
std::unique_ptr<PixelMap> TestPixelMapUtils::ParsePixelMapByPath(const std::string &pathName)
{
    SourceOptions opts;
    uint32_t errorCode = 0;
    std::unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(pathName, opts, errorCode);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, nullptr,
        "ImageSource::CreateImageSource fail! pathName=%{public}s, errorCode=%{public}d", pathName.c_str(), errorCode);
    
    DecodeOptions options;
    options.desiredDynamicRange = DecodeDynamicRange::AUTO;
    std::unique_ptr<PixelMap> pixelMap = imageSource->CreatePixelMap(options, errorCode);
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, nullptr,
        "CreatePixelMap fail! pathName=%{public}s, errorCode=%{public}d", pathName.c_str(), errorCode);
    
    return pixelMap;
}
} // Test
} // namespace Effect
} // namespace Media
} // namespace OHOS