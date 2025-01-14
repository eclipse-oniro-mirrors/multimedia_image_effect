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

#include "colorspace_helper.h"

#include <unordered_map>

#include "metadata_helper.h"
#include "colorspace_converter.h"
#include "metadata_generator.h"
#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
using namespace OHOS::ColorManager;
using namespace OHOS::HDI::Display::Graphic::Common::V1_0;

static const std::unordered_map<EffectColorSpace, ColorSpaceName> EFFECT_TO_COLORMANAGER_COLORSPACE_MAP = {
    { EffectColorSpace::SRGB, ColorSpaceName::SRGB },
    { EffectColorSpace::SRGB_LIMIT, ColorSpaceName::SRGB_LIMIT },
    { EffectColorSpace::DISPLAY_P3, ColorSpaceName::DISPLAY_P3 },
    { EffectColorSpace::DISPLAY_P3_LIMIT, ColorSpaceName::DISPLAY_P3_LIMIT },
    { EffectColorSpace::BT2020_HLG, ColorSpaceName::BT2020_HLG },
    { EffectColorSpace::BT2020_HLG_LIMIT, ColorSpaceName::BT2020_HLG_LIMIT },
    { EffectColorSpace::BT2020_PQ, ColorSpaceName::BT2020_PQ },
    { EffectColorSpace::BT2020_PQ_LIMIT, ColorSpaceName::BT2020_PQ_LIMIT },
    { EffectColorSpace::ADOBE_RGB, ColorSpaceName::ADOBE_RGB },
};

static const std::unordered_map<EffectColorSpace, CM_ColorSpaceType> EFFECT_TO_GRAPHIC_COLORSPACE_MAP = {
    { EffectColorSpace::SRGB, CM_ColorSpaceType::CM_SRGB_FULL },
    { EffectColorSpace::SRGB_LIMIT, CM_ColorSpaceType::CM_SRGB_LIMIT },
    { EffectColorSpace::DISPLAY_P3, CM_ColorSpaceType::CM_P3_FULL },
    { EffectColorSpace::DISPLAY_P3_LIMIT, CM_ColorSpaceType::CM_P3_LIMIT },
    { EffectColorSpace::BT2020_HLG, CM_ColorSpaceType::CM_BT2020_HLG_FULL },
    { EffectColorSpace::BT2020_HLG_LIMIT, CM_ColorSpaceType::CM_BT2020_HLG_LIMIT },
    { EffectColorSpace::BT2020_PQ, CM_ColorSpaceType::CM_BT2020_PQ_FULL },
    { EffectColorSpace::BT2020_PQ_LIMIT, CM_ColorSpaceType::CM_BT2020_PQ_LIMIT },
    { EffectColorSpace::ADOBE_RGB, CM_ColorSpaceType::CM_ADOBERGB_FULL },
};

static const std::unordered_map<EffectColorSpace, OH_NativeBuffer_ColorSpace> EFFECT_TO_NATIVE_BUFFER_COLORSPACE_MAP = {
    { EffectColorSpace::SRGB, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_SRGB_FULL },
    { EffectColorSpace::SRGB_LIMIT, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_SRGB_LIMIT },
    { EffectColorSpace::DISPLAY_P3,  OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL },
    { EffectColorSpace::DISPLAY_P3_LIMIT, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_LIMIT },
    { EffectColorSpace::BT2020_HLG, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_FULL },
    { EffectColorSpace::BT2020_HLG_LIMIT, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT },
    { EffectColorSpace::BT2020_PQ, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_PQ_FULL },
    { EffectColorSpace::BT2020_PQ_LIMIT, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_PQ_LIMIT },
    { EffectColorSpace::ADOBE_RGB, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_ADOBERGB_FULL },
};

bool ColorSpaceHelper::IsHdrColorSpace(EffectColorSpace colorSpace)
{
    return colorSpace == EffectColorSpace::BT2020_HLG || colorSpace == EffectColorSpace::BT2020_HLG_LIMIT ||
        colorSpace == EffectColorSpace::BT2020_PQ || colorSpace == EffectColorSpace::BT2020_PQ_LIMIT;
}

EffectColorSpace ColorSpaceHelper::ConvertToEffectColorSpace(ColorSpaceName colorSpaceName)
{
    EffectColorSpace colorSpaceType = EffectColorSpace::DEFAULT;

    for (const auto &it : EFFECT_TO_COLORMANAGER_COLORSPACE_MAP) {
        if (it.second == colorSpaceName) {
            colorSpaceType = it.first;
            break;
        }
    }

    EFFECT_LOGD("ConvertToEffectColorSpace: colorSpaceName=%{public}d, effectColorSpaceType=%{public}d",
        colorSpaceName, colorSpaceType);
    return colorSpaceType;
}

OH_NativeBuffer_ColorSpace ColorSpaceHelper::ConvertToNativeBufferColorSpace(EffectColorSpace effectColorSpace)
{
    OH_NativeBuffer_ColorSpace nativeBufferColorSpace = OH_NativeBuffer_ColorSpace::OH_COLORSPACE_NONE;
    auto it = EFFECT_TO_NATIVE_BUFFER_COLORSPACE_MAP.find(effectColorSpace);
    if (it != EFFECT_TO_NATIVE_BUFFER_COLORSPACE_MAP.end()) {
        nativeBufferColorSpace = it->second;
    }

    EFFECT_LOGD("ConvertToEffectColorSpace: colorSpaceType=%{public}d, effectColorSpaceType=%{public}d",
        effectColorSpace, nativeBufferColorSpace);
    return nativeBufferColorSpace;
}

ColorSpaceName ColorSpaceHelper::ConvertToColorSpaceName(EffectColorSpace colorSpace)
{
    ColorSpaceName colorSpaceName = ColorSpaceName::NONE;
    auto it = EFFECT_TO_COLORMANAGER_COLORSPACE_MAP.find(colorSpace);
    if (it != EFFECT_TO_COLORMANAGER_COLORSPACE_MAP.end()) {
        colorSpaceName =  it->second;
    }

    return colorSpaceName;
}

EffectColorSpace ColorSpaceHelper::ConvertToEffectColorSpace(CM_ColorSpaceType type)
{
    EffectColorSpace effectColorSpaceType = EffectColorSpace::DEFAULT;

    for (const auto &it : EFFECT_TO_GRAPHIC_COLORSPACE_MAP) {
        if (it.second == type) {
            effectColorSpaceType = it.first;
            break;
        }
    }

    EFFECT_LOGD("ConvertToEffectColorSpace: colorSpaceType=%{public}d, effectColorSpaceType=%{public}d",
        type, effectColorSpaceType);
    return effectColorSpaceType;
}

CM_ColorSpaceType ColorSpaceHelper::ConvertToCMColorSpace(EffectColorSpace colorSpace)
{
    CM_ColorSpaceType cmColorSpaceType = CM_ColorSpaceType::CM_COLORSPACE_NONE;
    auto it = EFFECT_TO_GRAPHIC_COLORSPACE_MAP.find(colorSpace);
    if (it != EFFECT_TO_GRAPHIC_COLORSPACE_MAP.end()) {
        cmColorSpaceType =  it->second;
    }

    return cmColorSpaceType;
}

ErrorCode ColorSpaceHelper::SetSurfaceBufferMetadataType(SurfaceBuffer *sb, const CM_HDR_Metadata_Type &type)
{
    sptr<SurfaceBuffer> buffer = sb;
    auto res = MetadataHelper::SetHDRMetadataType(buffer, type);
    CHECK_AND_RETURN_RET_LOG(res == GSError::GSERROR_OK, ErrorCode::ERR_SET_METADATA_FAIL,
        "SetSurfaceBufferMetadataType: SetHDRMetadataType fail! res=%{public}d", res);
    return ErrorCode::SUCCESS;
}

ErrorCode ColorSpaceHelper::GetSurfaceBufferMetadataType(SurfaceBuffer *sb, CM_HDR_Metadata_Type &type)
{
    sptr<SurfaceBuffer> buffer = sb;
    auto res = MetadataHelper::GetHDRMetadataType(buffer, type);
    CHECK_AND_RETURN_RET_LOG(res == GSError::GSERROR_OK, ErrorCode::ERR_GET_METADATA_FAIL,
        "GetSurfaceBufferMetadataType: GetHDRMetadataType fail! res=%{public}d", res);
    return ErrorCode::SUCCESS;
}

ErrorCode ColorSpaceHelper::SetSurfaceBufferColorSpaceType(SurfaceBuffer *sb, const CM_ColorSpaceType &type)
{
    sptr<SurfaceBuffer> buffer = sb;
    auto res = MetadataHelper::SetColorSpaceType(buffer, type);
    CHECK_AND_RETURN_RET_LOG(res == GSError::GSERROR_OK, ErrorCode::ERR_SET_COLORSPACETYPE_FAIL,
        "SetSurfaceBufferColorSpaceType: SetColorSpaceType fail! res=%{public}d", res);
    return ErrorCode::SUCCESS;
}

ErrorCode ColorSpaceHelper::GetSurfaceBufferColorSpaceType(SurfaceBuffer *sb, CM_ColorSpaceType &type)
{
    sptr<SurfaceBuffer> buffer = sb;
    auto res = MetadataHelper::GetColorSpaceType(buffer, type);
    CHECK_AND_RETURN_RET_LOG(res == GSError::GSERROR_OK, ErrorCode::ERR_GET_COLORSPACETYPE_FAIL,
        "GetSurfaceBufferColorSpaceType: GetColorSpaceType fail! res=%{public}d", res);
    return ErrorCode::SUCCESS;
}

ErrorCode ColorSpaceHelper::SetHDRDynamicMetadata(SurfaceBuffer *sb, const std::vector<uint8_t> &hdrDynamicMetadata)
{
    sptr<SurfaceBuffer> buffer = sb;
    auto res = MetadataHelper::SetHDRDynamicMetadata(buffer, hdrDynamicMetadata);
    CHECK_AND_RETURN_RET_LOG(res == GSError::GSERROR_OK, ErrorCode::ERR_SET_METADATA_FAIL,
        "SetHDRDynamicMetadata: SetHDRDynamicMetadata fail! res=%{public}d", res);
    return ErrorCode::SUCCESS;
}

ErrorCode ColorSpaceHelper::SetHDRStaticMetadata(SurfaceBuffer *sb, const std::vector<uint8_t> &hdrStaticMetadata)
{
    sptr<SurfaceBuffer> buffer = sb;
    auto res = MetadataHelper::SetHDRStaticMetadata(buffer, hdrStaticMetadata);
    CHECK_AND_RETURN_RET_LOG(res == GSError::GSERROR_OK, ErrorCode::ERR_SET_METADATA_FAIL,
        "SetHDRStaticMetadata: SetHDRStaticMetadata fail! res=%{public}d", res);
    return ErrorCode::SUCCESS;
}

ErrorCode ColorSpaceHelper::UpdateMetadata(EffectBuffer *input)
{
    CHECK_AND_RETURN_RET_LOG(input != nullptr && input->bufferInfo_ != nullptr && input->extraInfo_ != nullptr,
        ErrorCode::ERR_INPUT_NULL, "UpdateMetadata: inputBuffer is null");

    return UpdateMetadata(input->extraInfo_->surfaceBuffer, input->bufferInfo_->colorSpace_);
}

ErrorCode ColorSpaceHelper::UpdateMetadata(SurfaceBuffer *input, const EffectColorSpace &colorSpace)
{
    EFFECT_LOGD("UpdateMetadata: colorSpace=%{public}d}", colorSpace);
    if (input == nullptr || !ColorSpaceHelper::IsHdrColorSpace(colorSpace)) {
        return ErrorCode::SUCCESS;
    }

    std::shared_ptr<MetadataGenerator> metadataGenerator = std::make_shared<MetadataGenerator>();
    return metadataGenerator->ProcessImage(input);
}

ErrorCode ApplyColorSpaceIfNeed(std::shared_ptr<EffectBuffer> &srcBuffer, const std::shared_ptr<EffectContext> &context,
    EffectColorSpace &colorSpace)
{
    if (!ColorSpaceManager::IsNeedConvertColorSpace(colorSpace)) {
        return ErrorCode::SUCCESS;
    }

    EFFECT_LOGD("ColorSpaceHelper::ApplyColorSpace");
    void *buffer = srcBuffer->buffer_;
    std::shared_ptr<Memory> memory = context->memoryManager_->GetMemoryByAddr(buffer);
    EffectColorSpace outputColorSpace = EffectColorSpace::DEFAULT;
    ErrorCode res = context->colorSpaceManager_->ApplyColorSpace(srcBuffer.get(), colorSpace, outputColorSpace);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res,
        "ApplyColorSpaceIfNeed: ConvertColorSpace fail! res=%{public}d, colorSpace=%{public}d", res, colorSpace);

    // update memoryManager memory
    if (buffer != srcBuffer->buffer_) {
        EFFECT_LOGD("ApplyColorSpaceIfNeed: update memory.");
        context->memoryManager_->RemoveMemory(memory);

        std::shared_ptr<Memory> updateMemory = std::make_shared<Memory>();
        updateMemory->memoryData_ = std::make_shared<MemoryData>();
        updateMemory->memoryData_->data = srcBuffer->buffer_;
        updateMemory->memoryData_->memoryInfo.bufferInfo = *srcBuffer->bufferInfo_;
        updateMemory->memoryData_->memoryInfo.extra = srcBuffer->extraInfo_->surfaceBuffer;
        updateMemory->memoryData_->memoryInfo.bufferType = srcBuffer->extraInfo_->bufferType;
        updateMemory->memDataType_ = MemDataType::INPUT;
        updateMemory->isAllowModify_ = true;
        context->memoryManager_->AddMemory(updateMemory);
    }

    colorSpace = outputColorSpace;
    return ErrorCode::SUCCESS;
}

ErrorCode DecomposeHdrImageIfNeed(const EffectColorSpace &colorSpace, const EffectColorSpace &chosenColorSpace,
    std::shared_ptr<EffectBuffer> &buffer, const std::shared_ptr<EffectContext> &context)
{
    bool isNeedDecompose =
        ColorSpaceHelper::IsHdrColorSpace(colorSpace) && !ColorSpaceHelper::IsHdrColorSpace(chosenColorSpace);
    if (!isNeedDecompose) {
        return ErrorCode::SUCCESS;
    }

    EFFECT_LOGI("ColorSpaceHelper::DecomposeHdrImage");
    std::shared_ptr<Memory> oldMemory = context->memoryManager_->GetMemoryByAddr(buffer->buffer_);
    std::shared_ptr<ColorSpaceConverter> converter = std::make_shared<ColorSpaceConverter>();
    std::shared_ptr<EffectBuffer> sdrImage = nullptr;
    ErrorCode res = converter->ProcessHdrImage(buffer.get(), sdrImage);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "DecomposeHdrImageIfNeed: ProcessHdrImage fail! "
        "res=%{public}d, colorSpace=%{public}d, chosenColorSpace=%{public}d", res, colorSpace, chosenColorSpace);
    CHECK_AND_RETURN_RET_LOG(sdrImage != nullptr && sdrImage->extraInfo_ != nullptr, ErrorCode::ERR_INPUT_NULL,
        "DecomposeHdrImageIfNeed: sdrImage is null or extraInfo of sdrImage is null!"
        "sdrImage=%{public}d, sdrImage->extraInfo_=%{public}d", sdrImage == nullptr, sdrImage->extraInfo_ == nullptr);

    context->memoryManager_->RemoveMemory(oldMemory);
    std::shared_ptr<MemoryData> memoryData = converter->GetMemoryData(sdrImage->extraInfo_->surfaceBuffer);

    SurfaceBuffer *sb = sdrImage->extraInfo_->surfaceBuffer;
    ColorSpaceHelper::SetSurfaceBufferMetadataType(sb, CM_HDR_Metadata_Type::CM_METADATA_NONE);
    ColorSpaceHelper::SetSurfaceBufferColorSpaceType(sb, CM_ColorSpaceType::CM_COLORSPACE_NONE);

    std::shared_ptr<Memory> memory = std::make_shared<Memory>();
    memory->memoryData_ = memoryData;
    context->memoryManager_->AddMemory(memory);
    *buffer = *sdrImage;

    return ErrorCode::SUCCESS;
}

ErrorCode ColorSpaceHelper::ConvertColorSpace(std::shared_ptr<EffectBuffer> &srcBuffer,
    std::shared_ptr<EffectContext> &context)
{
    EffectColorSpace colorSpace = srcBuffer->bufferInfo_->colorSpace_;
    EFFECT_LOGD("ConvertColorSpace: colorSpace=%{public}d", colorSpace);

    // If color space is none, it means that color space is not supported. But it still should return success,
    // because the real color space maybe defined as ColorSpaceName::CUSTOM in ExtDecoder::getGrColorSpace or
    // the color space is not exist when invoking InnerGetGrColorSpacePtr of pixelmap returned null ptr.
    if (colorSpace == EffectColorSpace::DEFAULT) {
        EFFECT_LOGI("ConvertColorSpace: colorspace is none! Do nothing!");
        return ErrorCode::SUCCESS;
    }

    EFFECT_LOGD("ColorSpaceHelper::ConvertColorSpace colorSpace=%{public}d", colorSpace);
    ErrorCode res = ApplyColorSpaceIfNeed(srcBuffer, context, colorSpace);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "ConvertColorSpace: ConvertColorSpaceIfNeed fail! "
        "res=%{public}d, colorSpace=%{public}d", res, colorSpace);

    EffectColorSpace chosenColorSpace = EffectColorSpace::DEFAULT;
    res = context->colorSpaceManager_->ChooseColorSpace(
        context->filtersSupportedColorSpace_, colorSpace, chosenColorSpace);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "ConvertColorSpace: ChooseColorSpace fail! "
        "res=%{public}d, colorSpace=%{public}d", res, colorSpace);

    EFFECT_LOGD("ConvertColorSpace: colorSpace=%{public}d, chosenColorSpace=%{public}d", colorSpace, chosenColorSpace);
    res = DecomposeHdrImageIfNeed(colorSpace, chosenColorSpace, srcBuffer, context);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "ConvertColorSpace: DecomposeHdrImageIfNeed fail! "
        "res=%{public}d, colorSpace=%{public}d, chosenColorSpace=%{public}d", res, colorSpace, chosenColorSpace);

    return ErrorCode::SUCCESS;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS