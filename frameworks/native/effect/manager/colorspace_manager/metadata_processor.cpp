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

#include "metadata_processor.h"

#include "effect_log.h"
#ifdef VPE_ENABLE
#include "metadata_generator.h"
#endif
#include "colorspace_helper.h"
#include "effect_trace.h"

namespace OHOS {
namespace Media {
namespace Effect {
MetadataProcessor::MetadataProcessor()
{
#ifdef VPE_ENABLE
    metaDataGenerator = MetadataGenerator::Create();
#endif
}

MetadataProcessor::~MetadataProcessor()
{
}

ErrorCode MetadataProcessor::ProcessImage(SurfaceBuffer *inputImage)
{
#ifdef VPE_ENABLE
    EFFECT_TRACE_NAME("MetadataProcessor::ProcessImage");
    EFFECT_LOGI("MetadataProcessor::ProcessImage IN");
    CHECK_AND_RETURN_RET_LOG(metaDataGenerator != nullptr, ErrorCode::ERR_INVALID_VPE_INSTANCE,
        "ProcessImage: invalid vpe instance!");
    CHECK_AND_RETURN_RET_LOG(inputImage != nullptr, ErrorCode::ERR_INPUT_NULL, "ProcessImage: inputImage is null!");

    sptr<SurfaceBuffer> sb = inputImage;
    CHECK_AND_RETURN_RET_LOG(sb != nullptr, ErrorCode::ERR_INVALID_SURFACE_BUFFER,
        "ProcessImage: invalid surface buffer!");
    CM_HDR_Metadata_Type metadataType = CM_HDR_Metadata_Type::CM_METADATA_NONE;
    ColorSpaceHelper::GetSurfaceBufferMetadataType(inputImage, metadataType);
    MetadataGeneratorParameter parameterMT;
    if (metadataType == CM_HDR_Metadata_Type::CM_VIDEO_HLG
        || metadataType == CM_HDR_Metadata_Type::CM_VIDEO_HDR10
        || metadataType == CM_HDR_Metadata_Type::CM_VIDEO_HDR_VIVID) {
        parameterMT.algoType = MetadataGeneratorAlgoType::META_GEN_ALGO_TYPE_VIDEO;
    } else {
        parameterMT.algoType = MetadataGeneratorAlgoType::META_GEN_ALGO_TYPE_IMAGE;
    }

    auto ret = metaDataGenerator->SetParameter(parameterMT);
    CHECK_AND_RETURN_RET_LOG(ret == VPE_ALGO_ERR_OK,
        ErrorCode::ERR_VPE_METADATA_PROCESS_IMAGE_FAIL, "Decompose mdg SetParameter failed!\n");

    int32_t res = metaDataGenerator->Process(sb);
    CHECK_AND_RETURN_RET_LOG(res == 0, ErrorCode::ERR_VPE_METADATA_PROCESS_IMAGE_FAIL,
        "ProcessImage: MetadataProcessorProcessImage fail! res=%{public}d", res);

    EFFECT_LOGI("MetadataProcessor::ProcessImage OUT");
#endif
    return ErrorCode::SUCCESS;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS
