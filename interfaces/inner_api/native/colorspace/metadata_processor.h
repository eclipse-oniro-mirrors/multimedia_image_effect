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

#ifndef IMAGE_EFFECT_METADATA_PROCESSOR_H
#define IMAGE_EFFECT_METADATA_PROCESSOR_H

#include "error_code.h"
#include "surface_buffer.h"
#include "image_effect_marco_define.h"

namespace OHOS {
namespace Media {
namespace VideoProcessingEngine {
class MetadataGenerator;
}
namespace Effect {
using namespace OHOS::Media::VideoProcessingEngine;
class MetadataProcessor {
public:
    MetadataProcessor();
    ~MetadataProcessor();

    IMAGE_EFFECT_EXPORT ErrorCode ProcessImage(SurfaceBuffer *inputImage);
private:
    std::shared_ptr<MetadataGenerator> metaDataGenerator;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_METADATA_PROCESSOR_H
