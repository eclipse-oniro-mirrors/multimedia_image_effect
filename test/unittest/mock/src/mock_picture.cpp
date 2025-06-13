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

#include "mock_picture.h"
#include "exif_metadata.h"
#include "surface_buffer.h"

namespace OHOS {
namespace Media {
namespace Effect {
constexpr uint32_t HALF = 2;
const std::string IMAGE_LENGTH = "ImageLength";
const std::string IMAGE_WIDTH = "ImageWidth";

MockPicture::MockPicture()
{
    mainPixelMap_ = std::make_shared<MockPixelMap>();

    mockGainmapPixelMap_ =
        std::make_shared<MockPixelMap>(mainPixelMap_->GetWidth() / HALF, mainPixelMap_->GetHeight() / HALF);
    mockGainmapAuxiliaryPicture_ = std::make_shared<AuxiliaryPicture>();
    mockGainmapAuxiliaryPicture_->content_ = mockGainmapPixelMap_;
    auxiliaryPictures_.emplace(AuxiliaryPictureType::GAINMAP, mockGainmapAuxiliaryPicture_);

    auto exifMetadata = std::make_shared<ExifMetadata>();
    exifMetadata->SetValue(IMAGE_LENGTH, std::to_string(mainPixelMap_->GetHeight()));
    exifMetadata->SetValue(IMAGE_WIDTH, std::to_string(mainPixelMap_->GetWidth()));
    SetExifMetadata(exifMetadata);
}
} // namespace Effect
} // namespace Media
} // namespace OHOS