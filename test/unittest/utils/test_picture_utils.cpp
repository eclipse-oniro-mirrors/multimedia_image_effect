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

#include "test_picture_utils.h"
#include "image_source.h"
#include "test_effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
namespace Test {
    
std::unique_ptr<Picture> TestPictureUtils::CreatePictureByPath(std::string imagePath)
{
    SourceOptions opts;
    uint32_t errorCode = 0;
    EFFECT_LOGW("TestPictureUtils create pixelmap by path %{public}s", imagePath.c_str());
    std::unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(imagePath, opts, errorCode);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, nullptr,
        "CreateImageSource fail! path=%{public}s, errorCode=%{public}d", imagePath.c_str(), errorCode);

    DecodingOptionsForPicture decodingOptions;
    decodingOptions.desireAuxiliaryPictures.insert(AuxiliaryPictureType::GAINMAP);
    decodingOptions.desireAuxiliaryPictures.insert(AuxiliaryPictureType::DEPTH_MAP);
    decodingOptions.desireAuxiliaryPictures.insert(AuxiliaryPictureType::UNREFOCUS_MAP);
    decodingOptions.desireAuxiliaryPictures.insert(AuxiliaryPictureType::LINEAR_MAP);
    std::unique_ptr<Picture> picture = imageSource->CreatePicture(decodingOptions, errorCode);
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, nullptr,
        "CreatePicture fail! path=%{public}s, errorCode=%{public}d", imagePath.c_str(), errorCode);
    
    return picture;
}
} // Test
} // namespace Effect
} // namespace Media
} // namespace OHOS