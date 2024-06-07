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

#include "colorspace_converter.h"

#include "vpe_helper.h"
#include "effect_log.h"
#include "v1_0/cm_color_space.h"
#include "colorspace_helper.h"
#include "format_helper.h"
#include "common_utils.h"
#include "pixel_map.h"

#define VPE_INVALID_INSTANCE_ID (-1)

namespace OHOS {
namespace Media {
namespace Effect {

using namespace OHOS::ColorManager;
using namespace OHOS::HDI::Display::Graphic::Common::V1_0;

ColorSpaceConverter::~ColorSpaceConverter()
{
    memoryDataArray_.clear();
}

// The same instance cannot be used for ComposeImage and DecomposeImage.
int32_t ColorSpaceConverter::CreateVpeColorSpaceInstance()
{
    int instance = VPE_INVALID_INSTANCE_ID;
    int32_t res = VpeHelper::ColorSpaceConverterCreate(&instance);
    CHECK_AND_RETURN_RET_LOG(res == 0, VPE_INVALID_INSTANCE_ID,
        "GetVpeColorSpaceInstance: get instance fail! res=%{public}d", res);
    EFFECT_LOGD("GetVpeColorSpaceInstance: instance=%{public}d", instance);

    return instance;
}

void ColorSpaceConverter::DestroyVpeColorSpaceInstance(int32_t vpeColorSpaceInstance)
{
    // destroy vpe color space converter instance.
    if (vpeColorSpaceInstance != VPE_INVALID_INSTANCE_ID) {
        VpeHelper::ColorSpaceConverterDestroy(&vpeColorSpaceInstance);
    }
}

SurfaceBuffer *AllocSurfaceBuffer(std::vector<std::shared_ptr<MemoryData>> &memoryDataArray, MemoryInfo &allocMemInfo,
    CM_HDR_Metadata_Type type, CM_ColorSpaceType colorSpace)
{
    BufferInfo &bufferInfo = allocMemInfo.bufferInfo;
    EFFECT_LOGD("AllocSurfaceBuffer: Alloc surface buffer memory! w=%{public}d, h=%{public}d, format=%{public}d",
        bufferInfo.width_, bufferInfo.height_, bufferInfo.formatType_);

    std::unique_ptr<AbsMemory> absMemory = EffectMemory::CreateMemory(BufferType::DMA_BUFFER);
    CHECK_AND_RETURN_RET_LOG(absMemory != nullptr, nullptr, "AllocSurfaceBuffer: absMemory is null!");
    std::shared_ptr<MemoryData> memoryData = absMemory->Alloc(allocMemInfo);
    CHECK_AND_RETURN_RET_LOG(memoryData != nullptr, nullptr, "AllocSurfaceBuffer: memoryData is null!");

    void *surfaceBuffer = memoryData->memoryInfo.extra;
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, nullptr, "AllocSurfaceBuffer: extra info is null!");

    auto *sb = static_cast<SurfaceBuffer *>(surfaceBuffer);
    ColorSpaceHelper::SetSurfaceBufferMetadataType(sb, type);
    ColorSpaceHelper::SetSurfaceBufferColorSpaceType(sb, colorSpace);
    memoryDataArray.emplace_back(memoryData);
    return sb;
}

sptr<SurfaceBuffer> AllocSdrSurfaceBuffer(std::vector<std::shared_ptr<MemoryData>> &memoryDataArray,
    const EffectBuffer *inputHdr, CM_HDR_Metadata_Type type, CM_ColorSpaceType colorSpaceType,
    EffectColorSpace effectColorSpace)
{
    MemoryInfo allocMemInfo;
    BufferInfo &bufferInfo = allocMemInfo.bufferInfo;
    bufferInfo = *inputHdr->bufferInfo_;
    bufferInfo.formatType_ = IEffectFormat::RGBA8888;
    bufferInfo.colorSpace_ = effectColorSpace;
    SurfaceBuffer *sb = AllocSurfaceBuffer(memoryDataArray, allocMemInfo, type, colorSpaceType);
    CHECK_AND_RETURN_RET_LOG(sb != nullptr, nullptr,
        "AllocSdrSurfaceBuffer: alloc fail! w=%{public}d, h=%{public}d, format=%{public}d",
        bufferInfo.width_, bufferInfo.height_, bufferInfo.formatType_);

    return sb;
}

sptr<SurfaceBuffer> AllocGainmapSurfaceBuffer(std::vector<std::shared_ptr<MemoryData>> &memoryDataArray,
    const EffectBuffer *inputHdr, CM_HDR_Metadata_Type type, CM_ColorSpaceType colorSpaceType,
    EffectColorSpace effectColorSpace)
{
    MemoryInfo allocMemInfo;
    BufferInfo &bufferInfo = allocMemInfo.bufferInfo;
    bufferInfo = *inputHdr->bufferInfo_;
    uint32_t half = 2;
    bufferInfo.width_ = bufferInfo.width_ / half;
    bufferInfo.height_ = bufferInfo.height_ / half;
    bufferInfo.formatType_ = IEffectFormat::RGBA8888;
    bufferInfo.colorSpace_ = effectColorSpace;
    SurfaceBuffer *sb = AllocSurfaceBuffer(memoryDataArray, allocMemInfo, type, colorSpaceType);
    CHECK_AND_RETURN_RET_LOG(sb != nullptr, nullptr,
        "AllocGainmapSurfaceBuffer: alloc fail! w=%{public}d, h=%{public}d, format=%{public}d",
        bufferInfo.width_, bufferInfo.height_, bufferInfo.formatType_);

    return sb;
}

void PrintColorSpaceInfo(const sptr<SurfaceBuffer> &surfaceBuffer, const std::string &tag)
{
    if (surfaceBuffer == nullptr) {
        return;
    }

    CM_HDR_Metadata_Type metadataType = CM_HDR_Metadata_Type::CM_METADATA_NONE;
    ColorSpaceHelper::GetSurfaceBufferMetadataType(surfaceBuffer, metadataType);
    CM_ColorSpaceType colorSpaceType = CM_ColorSpaceType::CM_COLORSPACE_NONE;
    ColorSpaceHelper::GetSurfaceBufferColorSpaceType(surfaceBuffer, colorSpaceType);

    EFFECT_LOGD("%{public}s: metadataType=%{public}d, colorSpaceType=%{public}d, format=%{public}d",
        tag.c_str(), metadataType, colorSpaceType, surfaceBuffer->GetFormat());
}

ErrorCode ColorSpaceConverter::ComposeHdrImageInner(int32_t vpeColorSpaceInstance, const EffectBuffer *inputSdr,
    const SurfaceBuffer *inputGainmap, EffectBuffer *outputHdr)
{
    CHECK_AND_RETURN_RET_LOG(vpeColorSpaceInstance != VPE_INVALID_INSTANCE_ID,
        ErrorCode::ERR_INVALID_VPE_INSTANCE, "ComposeHdrImageInner: invalid vpe instance!");
    CHECK_AND_RETURN_RET_LOG(inputSdr != nullptr && inputGainmap != nullptr && outputHdr != nullptr,
        ErrorCode::ERR_INPUT_NULL, "ComposeHdrImageInner: inputSdr or inputGainmap or outputHdr is null!");

    sptr<SurfaceBuffer> sdrSb = inputSdr->extraInfo_->surfaceBuffer;
    sptr<SurfaceBuffer> gainmapSb = const_cast<SurfaceBuffer *>(inputGainmap);
    sptr<SurfaceBuffer> hdrSb = outputHdr->extraInfo_->surfaceBuffer;
    CHECK_AND_RETURN_RET_LOG(sdrSb != nullptr && gainmapSb != nullptr && hdrSb != nullptr,
        ErrorCode::ERR_INVALID_SURFACE_BUFFER, "ComposeHdrImageInner: invalid surface buffer! sdrSb=%{public}d,"
        "gainmapSb=%{public}d, hdrSb=%{public}d", sdrSb == nullptr, gainmapSb == nullptr, hdrSb == nullptr);

    PrintColorSpaceInfo(sdrSb, "ComposeHdrImageInner:SdrSurfaceBuffer");
    PrintColorSpaceInfo(gainmapSb, "ComposeHdrImageInner:GainmapSurfaceBuffer");
    PrintColorSpaceInfo(hdrSb, "ComposeHdrImageInner:HdrSurfaceBuffer");

    int32_t res = VpeHelper::ColorSpaceConverterComposeImage(vpeColorSpaceInstance, sdrSb, gainmapSb, hdrSb, false);
    CHECK_AND_RETURN_RET_LOG(res == 0, ErrorCode::ERR_VPE_COMPOSE_IMAGE_FAIL,
        "ComposeHdrImageInner: ColorSpaceConverterComposeImage fail! res=%{public}d", res);

    return ErrorCode::SUCCESS;
}

ErrorCode ColorSpaceConverter::ComposeHdrImage(const EffectBuffer *inputSdr, const SurfaceBuffer *inputGainmap,
    EffectBuffer *outputHdr)
{
    EFFECT_LOGD("ComposeHdrImage IN");
    int32_t vpeColorSpaceInstance = CreateVpeColorSpaceInstance();
    ErrorCode res = ComposeHdrImageInner(vpeColorSpaceInstance, inputSdr, inputGainmap, outputHdr);
    DestroyVpeColorSpaceInstance(vpeColorSpaceInstance);
    EFFECT_LOGD("ComposeHdrImage OUT");
    return res;
}

ErrorCode ColorSpaceConverter::DecomposeHdrImageInner(int32_t vpeColorSpaceInstance, const EffectBuffer *inputHdr,
    std::shared_ptr<EffectBuffer> &outputSdr, SurfaceBuffer **outputGainmap)
{
    CHECK_AND_RETURN_RET_LOG(vpeColorSpaceInstance != VPE_INVALID_INSTANCE_ID,
        ErrorCode::ERR_INVALID_VPE_INSTANCE, "DecomposeHdrImageInner: invalid vpe instance!");
    CHECK_AND_RETURN_RET_LOG(inputHdr != nullptr && outputGainmap != nullptr,
        ErrorCode::ERR_INPUT_NULL, "DecomposeHdrImageInner: inputHdr or outputGainmap is null!");

    sptr<SurfaceBuffer> hdrSb = inputHdr->extraInfo_->surfaceBuffer;
    sptr<SurfaceBuffer> sdrSb = AllocSdrSurfaceBuffer(memoryDataArray_, inputHdr, CM_IMAGE_HDR_VIVID_DUAL, CM_P3_FULL,
        EffectColorSpace::DISPLAY_P3);
    sptr<SurfaceBuffer> gainmapSb = AllocGainmapSurfaceBuffer(memoryDataArray_, inputHdr, CM_METADATA_NONE, CM_P3_FULL,
        EffectColorSpace::DISPLAY_P3);
    CHECK_AND_RETURN_RET_LOG(hdrSb != nullptr && sdrSb != nullptr && gainmapSb != nullptr,
        ErrorCode::ERR_INVALID_SURFACE_BUFFER, "DecomposeHdrImageInner: invalid surface buffer! hdrSb=%{public}d,"
        "sdrSb=%{public}d, gainmapSb=%{public}d", hdrSb == nullptr, sdrSb == nullptr, gainmapSb == nullptr);

    ColorSpaceHelper::SetHDRDynamicMetadata(hdrSb, std::vector<uint8_t>(0));
    ColorSpaceHelper::SetHDRStaticMetadata(hdrSb, std::vector<uint8_t>(0));

    PrintColorSpaceInfo(hdrSb, "DecomposeHdrImageInner:HdrSurfaceBuffer");
    PrintColorSpaceInfo(sdrSb, "DecomposeHdrImageInner:SdrSurfaceBuffer");
    PrintColorSpaceInfo(gainmapSb, "DecomposeHdrImageInner:GainmapSurfaceBuffer");

    int32_t res = VpeHelper::ColorSpaceConverterDecomposeImage(vpeColorSpaceInstance, hdrSb, sdrSb, gainmapSb);
    CHECK_AND_RETURN_RET_LOG(res == 0, ErrorCode::ERR_VPE_DECOMPOSE_IMAGE_FAIL,
        "DecomposeHdrImageInner: ColorSpaceConverterDecomposeImage fail! res=%{public}d", res);

    // update effectBuffer
    std::shared_ptr<EffectBuffer> buffer = nullptr;
    ErrorCode errorCode = CommonUtils::ParseSurfaceData(sdrSb, buffer, inputHdr->extraInfo_->dataType);
    CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, errorCode,
        "DecomposeHdrImageInner: ParseSurfaceData fail! errorCode=%{public}d", errorCode);
    *buffer->extraInfo_ = *inputHdr->extraInfo_;
    buffer->extraInfo_->surfaceBuffer = sdrSb;
    buffer->extraInfo_->bufferType = BufferType::DMA_BUFFER;

    outputSdr = buffer;
    *outputGainmap = gainmapSb;

    return ErrorCode::SUCCESS;
}

ErrorCode ColorSpaceConverter::DecomposeHdrImage(const EffectBuffer *inputHdr, std::shared_ptr<EffectBuffer> &outputSdr,
    SurfaceBuffer **outputGainmap)
{
    EFFECT_LOGD("DecomposeHdrImage IN");
    int32_t vpeColorSpaceInstance = CreateVpeColorSpaceInstance();
    ErrorCode res = DecomposeHdrImageInner(vpeColorSpaceInstance, inputHdr, outputSdr, outputGainmap);
    DestroyVpeColorSpaceInstance(vpeColorSpaceInstance);
    EFFECT_LOGD("DecomposeHdrImage OUT");
    return res;
}

ErrorCode ColorSpaceConverter::ProcessHdrImageInner(int32_t vpeColorSpaceInstance, const EffectBuffer *inputHdr,
    std::shared_ptr<EffectBuffer> &outputSdr)
{
    CHECK_AND_RETURN_RET_LOG(vpeColorSpaceInstance != VPE_INVALID_INSTANCE_ID,
        ErrorCode::ERR_INVALID_VPE_INSTANCE, "ProcessHdrImageInner: invalid vpe instance!");
    CHECK_AND_RETURN_RET_LOG(inputHdr != nullptr, ErrorCode::ERR_INPUT_NULL,
        "ProcessHdrImageInner: inputHdr is null!");

    sptr<SurfaceBuffer> sdrSb = AllocSdrSurfaceBuffer(memoryDataArray_, inputHdr, CM_IMAGE_HDR_VIVID_DUAL, CM_P3_FULL,
        EffectColorSpace::DISPLAY_P3);
    sptr<SurfaceBuffer> hdrSb = inputHdr->extraInfo_->surfaceBuffer;
    CHECK_AND_RETURN_RET_LOG(sdrSb != nullptr && hdrSb != nullptr, ErrorCode::ERR_INVALID_SURFACE_BUFFER,
        "ProcessHdrImageInner: invalid surface buffer! sdrSb=%{public}d, hdrSb=%{public}d",
        sdrSb == nullptr, hdrSb == nullptr);

    PrintColorSpaceInfo(hdrSb, "ProcessHdrImageInner:HdrSurfaceBuffer");
    PrintColorSpaceInfo(sdrSb, "ProcessHdrImageInner:SdrSurfaceBuffer");

    int32_t res = VpeHelper::ColorSpaceConverterProcessImage(vpeColorSpaceInstance, hdrSb, sdrSb);
    CHECK_AND_RETURN_RET_LOG(res == 0, ErrorCode::ERR_VPE_PROCESS_IMAGE_FAIL,
        "ProcessHdrImageInner: ColorSpaceConverterProcessImage fail! res=%{public}d", res);

    // update effectBuffer
    std::shared_ptr<EffectBuffer> buffer;
    ErrorCode errorCode = CommonUtils::ParseSurfaceData(sdrSb, buffer, inputHdr->extraInfo_->dataType);
    CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, errorCode,
        "ProcessHdrImageInner: ParseSurfaceData fail! errorCode=%{public}d", errorCode);
    *buffer->extraInfo_ = *inputHdr->extraInfo_;
    buffer->extraInfo_->surfaceBuffer = sdrSb;
    buffer->extraInfo_->bufferType = BufferType::DMA_BUFFER;

    outputSdr = buffer;
    return ErrorCode::SUCCESS;
}

ErrorCode ColorSpaceConverter::ProcessHdrImage(const EffectBuffer *inputHdr, std::shared_ptr<EffectBuffer> &outputSdr)
{
    EFFECT_LOGD("ProcessHdrImage IN");
    int32_t vpeColorSpaceInstance = CreateVpeColorSpaceInstance();
    ErrorCode res = ProcessHdrImageInner(vpeColorSpaceInstance, inputHdr, outputSdr);
    DestroyVpeColorSpaceInstance(vpeColorSpaceInstance);
    EFFECT_LOGD("ProcessHdrImage OUT");
    return res;
}

std::shared_ptr<MemoryData> ColorSpaceConverter::GetMemoryData(SurfaceBuffer *sb)
{
    CHECK_AND_RETURN_RET_LOG(sb != nullptr, nullptr, "GetMemoryData: sb is null!");

    void *pSurfaceBuffer = static_cast<void *>(sb);
    for (const auto &memoryData : memoryDataArray_) {
        if (memoryData->memoryInfo.extra == pSurfaceBuffer) {
            return memoryData;
        }
    }

    return nullptr;
}

PixelMap *CreatePixelMap(EffectBuffer *effectBuffer)
{
    std::shared_ptr<BufferInfo> &bufferInfo = effectBuffer->bufferInfo_;
    InitializationOptions options = {
        .size = {
            .width = static_cast<int32_t>(bufferInfo->width_),
            .height = static_cast<int32_t>(bufferInfo->height_),
        },
        .srcPixelFormat = CommonUtils::SwitchToPixelFormat(bufferInfo->formatType_),
        .pixelFormat = CommonUtils::SwitchToPixelFormat(bufferInfo->formatType_),
    };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(static_cast<uint32_t *>(effectBuffer->buffer_),
        bufferInfo->len_, 0, static_cast<int32_t>(bufferInfo->rowStride_), options, true);
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, nullptr, "CreatePixelMap: create fail!");
    CHECK_AND_RETURN_RET_LOG(pixelMap->GetPixelFormat() == options.pixelFormat, nullptr,
        "CreatePixelMap: pixelFormat error! pixelFormat=%{public}d, optionPixelFormat=%{public}d",
        pixelMap->GetPixelFormat(), options.pixelFormat);

    const std::shared_ptr<ExtraInfo> &extraInfo = effectBuffer->extraInfo_;
    extraInfo->innerPixelMap = std::move(pixelMap);
    return extraInfo->innerPixelMap.get();
}

PixelMap *GetPixelMap(EffectBuffer *effectBuffer)
{
    std::shared_ptr<ExtraInfo> &extraInfo = effectBuffer->extraInfo_;
    switch (extraInfo->dataType) {
        case DataType::PIXEL_MAP:
            return extraInfo->pixelMap;
        case DataType::URI:
        case DataType::PATH:
            return extraInfo->innerPixelMap.get();
        case DataType::SURFACE:
        case DataType::SURFACE_BUFFER:
            return CreatePixelMap(effectBuffer);
        default:
            EFFECT_LOGE("Data type not support! dataType=%{public}d", extraInfo->dataType);
            return nullptr;
    }
}

ErrorCode ColorSpaceConverter::ApplyColorSpace(EffectBuffer *effectBuffer, EffectColorSpace targetColorSpace)
{
    CHECK_AND_RETURN_RET_LOG(effectBuffer != nullptr && effectBuffer->bufferInfo_ != nullptr &&
        effectBuffer->buffer_ != nullptr && effectBuffer->extraInfo_ != nullptr,
        ErrorCode::ERR_INPUT_NULL, "ApplyColorSpace: effectBuffer is null!");

    ColorSpaceName colorSpaceName = ColorSpaceHelper::ConvertToColorSpaceName(targetColorSpace);
    CHECK_AND_RETURN_RET_LOG(colorSpaceName != ColorSpaceName::NONE, ErrorCode::ERR_INVALID_COLORSPACE,
        "ApplyColorSpace: invalid color space! targetColorSpace=%{public}d", targetColorSpace);

    PixelMap *pixelMap = GetPixelMap(effectBuffer);
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, ErrorCode::ERR_CREATE_PIXELMAP_FAIL,
        "ApplyColorSpace create pixelmap fail!");

    OHOS::ColorManager::ColorSpace grColorSpace(colorSpaceName);
    uint32_t res = pixelMap->ApplyColorSpace(grColorSpace);
    CHECK_AND_RETURN_RET_LOG(res == 0, ErrorCode::ERR_APPLYCOLORSPACE_FAIL, "ApplyColorSpace: pixelMap "
        "ApplyColorSpace fail! res=%{public}d, colorSpaceName=%{public}d", res, colorSpaceName);

    // update effectBuffer
    std::shared_ptr<EffectBuffer> buffer;
    ErrorCode errorCode = CommonUtils::LockPixelMap(pixelMap, buffer);
    CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, errorCode, "ApplyColorSpace: pixelMap parse fail!"
        "res=%{public}d, colorSpaceName=%{public}d", res, colorSpaceName);

    effectBuffer->bufferInfo_ = buffer->bufferInfo_;
    effectBuffer->buffer_ = buffer->buffer_;
    effectBuffer->extraInfo_->surfaceBuffer = buffer->extraInfo_->surfaceBuffer;

    return ErrorCode::SUCCESS;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS