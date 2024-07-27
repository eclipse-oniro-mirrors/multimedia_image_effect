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

#include "metadata_generator.h"

#include "vpe_helper.h"
#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {

MetadataGenerator::~MetadataGenerator()
{
    // destroy vpe color space converter instance.
    if (vpeMetadataGeneratorInstance_ != VPE_INVALID_INSTANCE_ID) {
        VpeHelper::MetadataGeneratorDestroy(&vpeMetadataGeneratorInstance_);
    }
}

ErrorCode MetadataGenerator::ProcessImage(SurfaceBuffer *inputImage)
{
    EFFECT_LOGI("MetadataGenerator::ProcessImage IN");
    int32_t vpeMetadataGeneratorInstance = GetVpeMetadataGeneratorInstance();
    CHECK_AND_RETURN_RET_LOG(vpeMetadataGeneratorInstance != VPE_INVALID_INSTANCE_ID,
        ErrorCode::ERR_INVALID_VPE_INSTANCE, "ProcessImage: invalid vpe instance!");
    CHECK_AND_RETURN_RET_LOG(inputImage != nullptr, ErrorCode::ERR_INPUT_NULL, "ProcessImage: inputImage is null!");

    sptr<SurfaceBuffer> sb = inputImage;
    CHECK_AND_RETURN_RET_LOG(sb != nullptr, ErrorCode::ERR_INVALID_SURFACE_BUFFER,
        "ProcessImage: invalid surface buffer!");

    int32_t res = VpeHelper::MetadataGeneratorProcessImage(vpeMetadataGeneratorInstance, sb);
    CHECK_AND_RETURN_RET_LOG(res == 0, ErrorCode::ERR_VPE_METADATA_PROCESS_IMAGE_FAIL,
        "ProcessImage: MetadataGeneratorProcessImage fail! res=%{public}d", res);

    EFFECT_LOGI("MetadataGenerator::ProcessImage OUT");
    return ErrorCode::SUCCESS;
}

int32_t MetadataGenerator::GetVpeMetadataGeneratorInstance()
{
    if (vpeMetadataGeneratorInstance_ != VPE_INVALID_INSTANCE_ID) {
        return vpeMetadataGeneratorInstance_;
    }

    int32_t res = VpeHelper::MetadataGeneratorCreate(&vpeMetadataGeneratorInstance_);
    CHECK_AND_RETURN_RET_LOG(res == 0, VPE_INVALID_INSTANCE_ID,
        "GetVpeMetadataGeneratorInstance: get instance fail! res=%{public}d", res);
    EFFECT_LOGD("GetVpeColorSpaceInstance: instance=%{public}d", vpeMetadataGeneratorInstance_);

    return vpeMetadataGeneratorInstance_;
}

} // namespace Effect
} // namespace Media
} // namespace OHOS