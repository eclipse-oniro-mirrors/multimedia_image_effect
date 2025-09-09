/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "image_sink_filter.h"

#include <sync_fence.h>
#include <v1_1/buffer_handle_meta_key_type.h>

#include "common_utils.h"
#include "effect_log.h"
#include "filter_factory.h"
#include "image_packer.h"
#include "memcpy_helper.h"
#include "format_helper.h"
#include "colorspace_helper.h"
#include "render_environment.h"
#include "effect_trace.h"

namespace OHOS {
namespace Media {
namespace Effect {
REGISTER_FILTER_FACTORY(ImageSinkFilter);
constexpr int SINGLE_BUFFER = 1;
constexpr int DOUBLE_BUFFER = 2;

ErrorCode ImageSinkFilter::SetSink(const std::shared_ptr<EffectBuffer> &sink)
{
    EFFECT_LOGD("SetSink entered.");
    sinkBuffer_ = sink;
    return ErrorCode::SUCCESS;
}

ErrorCode ImageSinkFilter::SetXComponentSurface(sptr<Surface> &surface)
{
    EFFECT_LOGD("ImageSinkFilter::SetRenderSurface entered.");
    toXComponentSurface_ = surface;
    return ErrorCode::SUCCESS;
}

void ImageSinkFilter::DestoryTexureCache()
{
    if (!texureCacheSeqs_.empty()) {
        for (auto [_, texureCacheSeq] : texureCacheSeqs_) {
            GLUtils::DestroyImage(texureCacheSeq.eglImage_);
            GLUtils::DeleteTexture(texureCacheSeq.texId_);
            GLUtils::DestroySyncKHR(texureCacheSeq.eglSync_);
        }
        texureCacheSeqs_.clear();
    }
}

ErrorCode ImageSinkFilter::Start()
{
    FilterBase::Start();
    return ErrorCode::SUCCESS;
}

void CopyDataToPixelMap(PixelMap *pixelMap, const std::shared_ptr<EffectBuffer> &buffer)
{
    CopyInfo dst = {
        .bufferInfo = {
            .width_ = static_cast<uint32_t>(pixelMap->GetWidth()),
            .height_ = static_cast<uint32_t>(pixelMap->GetHeight()),
            .len_ = FormatHelper::CalculateDataRowCount(static_cast<uint32_t>(pixelMap->GetHeight()),
                CommonUtils::SwitchToEffectFormat(pixelMap->GetPixelFormat())) *
                static_cast<uint32_t>(pixelMap->GetRowStride()),
            .formatType_ = CommonUtils::SwitchToEffectFormat(pixelMap->GetPixelFormat()),
            .rowStride_ = static_cast<uint32_t>(pixelMap->GetRowStride()),
        },
        .data = const_cast<uint8_t *>(pixelMap->GetPixels()),
    };

    MemcpyHelper::CopyData(buffer.get(), dst);
}

ErrorCode ModifyPixelMap(EffectBuffer *src, const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    PixelMap *pixelMap = src->bufferInfo_->pixelMap_;
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, ErrorCode::ERR_INPUT_NULL, "pixelMap is null!");

    uint8_t *pixels = const_cast<uint8_t *>(pixelMap->GetPixels());
    if (pixels == buffer->buffer_) {
        EFFECT_LOGD("ModifyPixelMap: not need modify pixelmap!");
        CommonUtils::UpdateImageExifDateTime(pixelMap);
        ColorSpaceHelper::UpdateMetadata(src, context);
        return ErrorCode::SUCCESS;
    }

    if (buffer->extraInfo_->dataType == DataType::TEX) {
        if (pixelMap->GetWidth() == static_cast<int32_t>(buffer->bufferInfo_->width_) &&
            pixelMap->GetHeight() == static_cast<int32_t>(buffer->bufferInfo_->height_) && pixels == src->buffer_) {
            context->renderEnvironment_->ConvertTextureToBuffer(buffer->bufferInfo_->tex_, src, true);
            CommonUtils::UpdateImageExifDateTime(pixelMap);
            ColorSpaceHelper::UpdateMetadata(src, context);
            return ErrorCode::SUCCESS;
        } else {
            return CommonUtils::ModifyPixelMapPropertyForTexture(pixelMap, buffer, context);
        }
    }

    if (static_cast<uint32_t>(pixelMap->GetRowStride()) == buffer->bufferInfo_->rowStride_ &&
        static_cast<uint32_t>(pixelMap->GetHeight()) == buffer->bufferInfo_->height_ &&
        CommonUtils::SwitchToEffectFormat(pixelMap->GetPixelFormat()) == buffer->bufferInfo_->formatType_) {
        EFFECT_LOGD("Copy data to pixel map.");
        CopyDataToPixelMap(pixelMap, buffer);
        ColorSpaceHelper::UpdateMetadata(buffer.get(), context);
        return ErrorCode::SUCCESS;
    }

    ErrorCode result = CommonUtils::ModifyPixelMapProperty(pixelMap, buffer, context);
    return result;
}

void CopyDataToSurfaceBuffer(SurfaceBuffer *surfaceBuffer, const std::shared_ptr<EffectBuffer> &buffer)
{
    CopyInfo dst = {
        .bufferInfo = {
            .width_ = static_cast<uint32_t>(surfaceBuffer->GetWidth()),
            .height_ = static_cast<uint32_t>(surfaceBuffer->GetHeight()),
            .len_ = surfaceBuffer->GetSize(),
            .formatType_ = CommonUtils::SwitchToEffectFormat((GraphicPixelFormat)surfaceBuffer->GetFormat()),
            .rowStride_ = static_cast<uint32_t>(surfaceBuffer->GetStride()),
        },
        .data = static_cast<uint8_t *>(surfaceBuffer->GetVirAddr()),
    };

    MemcpyHelper::CopyData(buffer.get(), dst);
}

ErrorCode ModifySurfaceBuffer(EffectBuffer *src, const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    SurfaceBuffer *surfaceBuffer = src->bufferInfo_->surfaceBuffer_;
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, ErrorCode::ERR_INPUT_NULL, "surfaceBuffer is null!");
    EFFECT_LOGD("ModifySurfaceBuffer");
    if (surfaceBuffer->GetVirAddr() == buffer->buffer_) {
        ColorSpaceHelper::UpdateMetadata(buffer.get(), context);
        return ErrorCode::SUCCESS;
    }

    if (buffer->extraInfo_->dataType == DataType::TEX) {
        if (surfaceBuffer->GetWidth() == static_cast<int32_t>(buffer->bufferInfo_->width_) &&
            surfaceBuffer->GetHeight() == static_cast<int32_t>(buffer->bufferInfo_->height_)) {
            context->renderEnvironment_->ConvertTextureToBuffer(buffer->bufferInfo_->tex_, src, true);
            ColorSpaceHelper::UpdateMetadata(src, context);
            return ErrorCode::SUCCESS;
        }
        return ErrorCode::ERR_BUFFER_NOT_ALLOW_CHANGE;
    }

    if (static_cast<uint32_t>(surfaceBuffer->GetStride()) == buffer->bufferInfo_->rowStride_ &&
        static_cast<uint32_t>(surfaceBuffer->GetHeight()) == buffer->bufferInfo_->height_ &&
        CommonUtils::SwitchToEffectFormat((GraphicPixelFormat)surfaceBuffer->GetFormat()) ==
        buffer->bufferInfo_->formatType_) {
        EFFECT_LOGD("Copy data to surface buffer.");
        CopyDataToSurfaceBuffer(surfaceBuffer, buffer);
        CHECK_AND_RETURN_RET(context->metaInfoNegotiate_->IsNeedUpdate(), ErrorCode::SUCCESS);
        ColorSpaceHelper::UpdateMetadata(buffer.get(), context);
        return ErrorCode::SUCCESS;
    }

    EFFECT_LOGE("surface buffer not allow changed!");
    return ErrorCode::ERR_BUFFER_NOT_ALLOW_CHANGE;
}

ErrorCode ModifyInnerPicture(EffectBuffer *src, const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    std::shared_ptr<Picture> picture = src->extraInfo_->innerPicture;
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, ErrorCode::ERR_INPUT_NULL, "inner picture is null!");
    std::shared_ptr<PixelMap> mainPixel = picture->GetMainPixel();
    CHECK_AND_RETURN_RET_LOG(mainPixel != nullptr, ErrorCode::ERR_INPUT_NULL, "picture main pixel is null!");
    uint8_t *pixels = const_cast<uint8_t *>(mainPixel->GetPixels());
    if (pixels == buffer->buffer_) {
        // update output exif info
        CommonUtils::UpdateImageExifDateTime(picture.get());

        // update metadata
        ColorSpaceHelper::UpdateMetadata(buffer.get(), context);
        return ErrorCode::SUCCESS;
    }

    // update picture exif
    CommonUtils::UpdateImageExifInfo(picture.get());

    if (buffer->extraInfo_->dataType == DataType::TEX) {
        return CommonUtils::ModifyPixelMapPropertyForTexture(mainPixel.get(), buffer, context, false);
    }

    ErrorCode res = CommonUtils::ModifyPixelMapProperty(mainPixel.get(), buffer, context, false);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "ModifyInnerPicture: modify main pixelmap property fail!");

    if (buffer->auxiliaryBufferInfos == nullptr) {
        return ErrorCode::SUCCESS;
    }

    auto bufferIt = buffer->auxiliaryBufferInfos->find(EffectPixelmapType::GAINMAP);
    if (bufferIt == buffer->auxiliaryBufferInfos->end()) {
        return ErrorCode::SUCCESS;
    }

    auto gainMapBufferInfo = bufferIt->second;
    std::shared_ptr<PixelMap> gainMap = picture->GetGainmapPixelMap();
    if (gainMap != nullptr && gainMapBufferInfo != nullptr && gainMapBufferInfo->addr_ != nullptr &&
        static_cast<uint8_t *>(gainMapBufferInfo->addr_) != gainMap->GetPixels()) {
        std::shared_ptr<ExtraInfo> defaultExtraInfo = std::make_shared<ExtraInfo>();
        std::shared_ptr<EffectBuffer> gainMapEffectBuffer =
            std::make_shared<EffectBuffer>(gainMapBufferInfo, gainMapBufferInfo->addr_, defaultExtraInfo);
        res = CommonUtils::ModifyPixelMapProperty(gainMap.get(), gainMapEffectBuffer, context, false);
        auto auxilaryPicture = picture->GetAuxiliaryPicture(AuxiliaryPictureType::GAINMAP);
        CHECK_AND_RETURN_RET_LOG(auxilaryPicture, res, "ModifyInnerPicture: auxilaryPicture not exist!");
        auxilaryPicture->SetContentPixel(gainMap);
        return res;
    }

    return ErrorCode::SUCCESS;
}

ErrorCode ModifyPictureForGainMap(PixelMap *gainMapPixelMap, EffectBuffer *src,
    const std::shared_ptr<EffectBuffer> &gainMapBuffer, std::shared_ptr<EffectContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(gainMapPixelMap != nullptr, ErrorCode::ERR_INPUT_NULL,
        "ModifyPictureForInnerPixelMap: pixelMap is null!");

    uint8_t *pixels = const_cast<uint8_t *>(gainMapPixelMap->GetPixels());
    if (pixels == gainMapBuffer->buffer_) {
        EFFECT_LOGD("ModifyPicture: not need modify picture!");
        return ErrorCode::SUCCESS;
    }
    auto srcGainMapBufferInfo = src->auxiliaryBufferInfos->at(EffectPixelmapType::GAINMAP);
    auto defaultExtraInfo = std::make_shared<ExtraInfo>();
    auto srcGainMapBuffer = std::make_shared<EffectBuffer>(srcGainMapBufferInfo, nullptr, defaultExtraInfo);
    if (gainMapBuffer->extraInfo_->dataType == DataType::TEX) {
        if (gainMapPixelMap->GetWidth() == static_cast<int32_t>(gainMapBuffer->bufferInfo_->width_) &&
            gainMapPixelMap->GetHeight() == static_cast<int32_t>(gainMapBuffer->bufferInfo_->height_)) {
                context->renderEnvironment_->ConvertTextureToBuffer(gainMapBuffer->bufferInfo_->tex_,
                    srcGainMapBuffer.get(), true);
                ColorSpaceHelper::UpdateMetadata(src, context);
                return ErrorCode::SUCCESS;
            } else {
                EFFECT_LOGD("ModifyPicture: ModifyPixelMapPropertyForTexture");
                return CommonUtils::ModifyPixelMapPropertyForTexture(gainMapPixelMap, gainMapBuffer, context);
            }
    }

    auto gainMapEffectType = CommonUtils::SwitchToEffectFormat(gainMapPixelMap->GetPixelFormat());
    if (static_cast<uint32_t>(gainMapPixelMap->GetRowStride()) == gainMapBuffer->bufferInfo_->rowStride_ &&
        static_cast<uint32_t>(gainMapPixelMap->GetHeight()) == gainMapBuffer->bufferInfo_->height_ &&
        gainMapEffectType == gainMapBuffer->bufferInfo_->formatType_) {
            EFFECT_LOGD("ModifyPicture: Copy data to pixel map.");
            CopyDataToPixelMap(gainMapPixelMap, gainMapBuffer);
            return ErrorCode::SUCCESS;
        }

    return CommonUtils::ModifyPixelMapProperty(gainMapPixelMap, gainMapBuffer, context, false);
}


ErrorCode ModifyPictureForInnerPixelMap(PixelMap *pixelMap, EffectBuffer *src,
    const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, ErrorCode::ERR_INPUT_NULL,
        "ModifyPictureForInnerPixelMap: pixelMap is null!");

    uint8_t *pixels = const_cast<uint8_t *>(pixelMap->GetPixels());
    if (pixels == buffer->bufferInfo_->addr_) {
        EFFECT_LOGD("ModifyPicture: not need modify picture!");
        return ErrorCode::SUCCESS;
    }

    if (buffer->extraInfo_->dataType == DataType::TEX) {
        if (pixelMap->GetWidth() == static_cast<int32_t>(buffer->bufferInfo_->width_) &&
            pixelMap->GetHeight() == static_cast<int32_t>(buffer->bufferInfo_->height_) && pixels == src->buffer_) {
            context->renderEnvironment_->ConvertTextureToBuffer(buffer->bufferInfo_->tex_, src, true);
            ColorSpaceHelper::UpdateMetadata(src, context);
            return ErrorCode::SUCCESS;
        } else {
            return CommonUtils::ModifyPixelMapPropertyForTexture(pixelMap, buffer, context);
        }
    }

    if (static_cast<uint32_t>(pixelMap->GetRowStride()) == buffer->bufferInfo_->rowStride_ &&
        static_cast<uint32_t>(pixelMap->GetHeight()) == buffer->bufferInfo_->height_ &&
        CommonUtils::SwitchToEffectFormat(pixelMap->GetPixelFormat()) == buffer->bufferInfo_->formatType_) {
        EFFECT_LOGD("ModifyPicture: Copy data to pixel map.");
        CopyDataToPixelMap(pixelMap, buffer);
        return ErrorCode::SUCCESS;
    }

    return CommonUtils::ModifyPixelMapProperty(pixelMap, buffer, context, false);
}

ErrorCode ModifyTex(EffectBuffer *src, const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(src->bufferInfo_->tex_ != nullptr, ErrorCode::ERR_INPUT_NULL,
        "ModifyTex: src tex is null!");
    CHECK_AND_RETURN_RET_LOG(buffer->bufferInfo_->tex_ != nullptr, ErrorCode::ERR_INPUT_NULL,
        "ModifyTex: buffer tex is null!");
    context->renderEnvironment_->DrawTex(buffer->bufferInfo_->tex_, src->bufferInfo_->tex_);
    glFinish();
    return ErrorCode::SUCCESS;
}

ErrorCode ModifyPicture(EffectBuffer *src, const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    Picture *picture = src->extraInfo_->picture;
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, ErrorCode::ERR_INPUT_NULL, "ModifyPicture: picture is null!");
    auto primary = picture->GetMainPixel();
    CHECK_AND_RETURN_RET_LOG(primary != nullptr, ErrorCode::ERR_INPUT_NULL, "ModifyPicture: main pixelmap is null!");

    auto primaryBuffer = std::make_shared<EffectBuffer>(buffer->bufferInfo_, buffer->buffer_, buffer->extraInfo_);
    primaryBuffer->bufferInfo_->tex_ = buffer->bufferInfo_->tex_;
    ErrorCode res = ModifyPictureForInnerPixelMap(primary.get(), src, primaryBuffer, context);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "ModifyPicture: modify main pixelMap fail!");

    CommonUtils::UpdateImageExifInfo(picture);

    if (!buffer->auxiliaryBufferInfos || (buffer->auxiliaryBufferInfos && buffer->auxiliaryBufferInfos->empty())) {
        return ErrorCode::SUCCESS;
    }

    EFFECT_LOGD("ModifyPicture: save gainmap");
    auto bufferIt = buffer->auxiliaryBufferInfos->find(EffectPixelmapType::GAINMAP);
    if (bufferIt == buffer->auxiliaryBufferInfos->end()) {
        return ErrorCode::SUCCESS;
    }

    auto srcGainMap = picture->GetGainmapPixelMap();
    CHECK_AND_RETURN_RET_LOG(srcGainMap != nullptr, ErrorCode::ERR_INPUT_NULL, "ModifyPicture: srcGainMap is null!");

    auto gainMapInfo = bufferIt->second;
    CHECK_AND_RETURN_RET_LOG(gainMapInfo != nullptr, ErrorCode::ERR_INPUT_NULL,
        "ModifyPicture: gainMap after render is null!");

    auto defaultExtraInfo = std::make_shared<ExtraInfo>();
    std::shared_ptr<EffectBuffer> gainMapEffectBuffer = std::make_shared<EffectBuffer>(gainMapInfo, gainMapInfo->addr_,
                                                                                       defaultExtraInfo);
    gainMapEffectBuffer->extraInfo_->dataType = buffer->extraInfo_->dataType;
    res = ModifyPictureForGainMap(srcGainMap.get(), src, gainMapEffectBuffer, context);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "ModifyPicture: modify gainmap pixelMap fail!");

    auto auxilaryPicture = picture->GetAuxiliaryPicture(AuxiliaryPictureType::GAINMAP);
    CHECK_AND_RETURN_RET_LOG(auxilaryPicture, res, "ModifyPicture: auxilaryPicture not exist!");
    auxilaryPicture->SetContentPixel(srcGainMap);
    return ErrorCode::SUCCESS;
}

GraphicTransformType GetSurfaceTransform(EffectBuffer *input)
{
    CHECK_AND_RETURN_RET_LOG(input != nullptr, GRAPHIC_ROTATE_NONE, "GetSurfaceTransform: input is null!");
    CHECK_AND_RETURN_RET_LOG(input->bufferInfo_ != nullptr, GRAPHIC_ROTATE_NONE,
        "GetSurfaceTransform: bufferInfo_ is null!");
    if (input->bufferInfo_->surfaceBuffer_ != nullptr) {
        return input->bufferInfo_->surfaceBuffer_->GetSurfaceBufferTransform();
    }
    return GRAPHIC_ROTATE_NONE;
}

std::pair<MetaDataMap, MetaDataMap> PrepareMetaDatas(const EffectBuffer *input)
{
    auto inputPicture = input->extraInfo_->picture;
    CHECK_AND_RETURN_RET_LOG(inputPicture, {}, "PrepareMetaDatas: picture is nullptr!");
    auto primaryMetaData = CommonUtils::GetMetaData(reinterpret_cast<SurfaceBuffer*>(
        inputPicture->GetMainPixel()->GetFd()));
    auto getGainmapPixelMap = inputPicture->GetGainmapPixelMap();
    CHECK_AND_RETURN_RET_LOG(getGainmapPixelMap, {}, "PrepareMetaDatas: GetGainmapPixelMap is nullptr!");
    auto gainMapMetaData = CommonUtils::GetMetaData(reinterpret_cast<SurfaceBuffer*>(
        getGainmapPixelMap->GetFd()));
    return { primaryMetaData, gainMapMetaData };
}

void SetPictureMetaData(EffectBuffer *output, MetaDataMap& primaryMetaData, MetaDataMap& gainMapMetaData)
{
    auto outputPicture = output->extraInfo_->picture;
    CHECK_AND_RETURN_LOG(outputPicture, "SetPictureMetaData: picture is nullptr!");
    CommonUtils::SetMetaData(primaryMetaData, reinterpret_cast<SurfaceBuffer*>(
        outputPicture->GetMainPixel()->GetFd()));
    auto getGainmapPixelMap = outputPicture->GetGainmapPixelMap();
    CHECK_AND_RETURN_LOG(getGainmapPixelMap, "SetPictureMetaData: GetGainmapPixelMap is nullptr!");
    CommonUtils::SetMetaData(gainMapMetaData, reinterpret_cast<SurfaceBuffer*>(
        getGainmapPixelMap->GetFd()));
}

std::pair<std::shared_ptr<EffectBuffer>, std::shared_ptr<EffectBuffer>> PrepareBuffers(const EffectBuffer *input)
{
    auto primaryBufferInfo = std::make_shared<BufferInfo>();
    CommonUtils::CopyBufferInfo(*input->bufferInfo_, *primaryBufferInfo);
    auto primaryExtraInfo = std::make_shared<ExtraInfo>();
    primaryExtraInfo->dataType = DataType::TEX;
    auto primaryBuffer = std::make_shared<EffectBuffer>(primaryBufferInfo, nullptr, primaryExtraInfo);

    if (input->bufferInfo_->hdrFormat_ != HdrFormat::HDR8_GAINMAP ||
        input->auxiliaryBufferInfos->find(EffectPixelmapType::GAINMAP) == input->auxiliaryBufferInfos->end()) {
        return {primaryBuffer, nullptr};
    }

    auto inputGainMapBufferInfo = input->auxiliaryBufferInfos->at(EffectPixelmapType::GAINMAP);
    auto auxiliaryBufferInfo = std::make_shared<BufferInfo>();
    CommonUtils::CopyBufferInfo(*inputGainMapBufferInfo, *auxiliaryBufferInfo);
    auto auxiliaryExtraInfo = std::make_shared<ExtraInfo>();
    auxiliaryExtraInfo->dataType = DataType::TEX;
    auto auxiliaryBuffer = std::make_shared<EffectBuffer>(auxiliaryBufferInfo, nullptr, auxiliaryExtraInfo);

    return {primaryBuffer, auxiliaryBuffer};
}

ErrorCode ModifyDataInfo(EffectBuffer *src, const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    switch (src->extraInfo_->dataType) {
        case DataType::PIXEL_MAP:
            return ModifyPixelMap(src, buffer, context);
        case DataType::SURFACE_BUFFER:
        case DataType::SURFACE:
            return ModifySurfaceBuffer(src, buffer, context);
        case DataType::PATH:
        case DataType::URI:
            return ModifyInnerPicture(src, buffer, context);
        case DataType::PICTURE:
            return ModifyPicture(src, buffer, context);
        case DataType::TEX:
            return ModifyTex(src, buffer, context);
        default:
            return ErrorCode::ERR_UNSUPPORTED_DATA_TYPE;
    }
}

ErrorCode FillOutputData(const std::shared_ptr<EffectBuffer> &inputBuffer, std::shared_ptr<EffectBuffer> &outputBuffer,
    const std::shared_ptr<EffectContext> &context)
{
    if (inputBuffer->buffer_ == outputBuffer->buffer_) {
        EFFECT_LOGD("ImageSinkFilter: not need copy!");

        // update output exif info
        if (outputBuffer->bufferInfo_->pixelMap_ != nullptr) {
            CommonUtils::UpdateImageExifDateTime(outputBuffer->bufferInfo_->pixelMap_);
        } else {
            EFFECT_LOGD("Before UpdateImageExifDateTime: pixelMap is null!");
        }

        // update metadata
        ColorSpaceHelper::UpdateMetadata(outputBuffer.get(), context);
        return ErrorCode::SUCCESS;
    }

    size_t outputBufferSize = outputBuffer->bufferInfo_->len_;
    size_t inputBufferSize = inputBuffer->bufferInfo_->len_;
    uint32_t outputRowStride = outputBuffer->bufferInfo_->rowStride_;
    uint32_t inputRowStride = inputBuffer->bufferInfo_->rowStride_;
    EFFECT_LOGI("outputBufferSize=%{public}zu, inputBufferSize=%{public}zu, outputRowStride=%{public}d, "
        "inputRowStride=%{public}d", outputBufferSize, inputBufferSize, outputRowStride, inputRowStride);
    // update nativePixelMap
    if (inputBuffer->extraInfo_->dataType == DataType::TEX) {
        context->renderEnvironment_->ConvertTextureToBuffer(inputBuffer->bufferInfo_->tex_, outputBuffer.get(), true);
    } else {
        MemcpyHelper::CopyData(inputBuffer.get(), outputBuffer.get());
    }

    // update output exif info
    CommonUtils::UpdateImageExifDateTime(outputBuffer->bufferInfo_->pixelMap_);

    // update metadata
    ColorSpaceHelper::UpdateMetadata(outputBuffer.get(), context);
    return ErrorCode::SUCCESS;
}

ErrorCode FillPictureMainPixel(const std::shared_ptr<EffectBuffer> &inputBuffer,
    std::shared_ptr<EffectBuffer> &outputBuffer, const std::shared_ptr<EffectContext> &context)
{
    // update nativePixelMap
    auto dstPicture = outputBuffer->extraInfo_->picture;
    CHECK_AND_RETURN_RET_LOG(dstPicture != nullptr, ErrorCode::ERR_INPUT_NULL,
        "FillPictureMainPixel: dstPicture is null!");

    auto dstPixelMap = dstPicture->GetMainPixel();
    CHECK_AND_RETURN_RET_LOG(dstPixelMap != nullptr, ErrorCode::ERR_INPUT_NULL,
        "FillPictureMainPixel: dstPixelMap is null!");

    ErrorCode res = ErrorCode::SUCCESS;
    if (inputBuffer->extraInfo_->dataType == DataType::TEX) {
        if (outputBuffer->bufferInfo_->width_ == inputBuffer->bufferInfo_->width_ &&
            outputBuffer->bufferInfo_->height_ == inputBuffer->bufferInfo_->height_) {
            context->renderEnvironment_->ConvertTextureToBuffer(inputBuffer->bufferInfo_->tex_,
                outputBuffer.get(), true);
            ColorSpaceHelper::UpdateMetadata(outputBuffer.get(), context);
        } else {
            res = CommonUtils::ModifyPixelMapPropertyForTexture(dstPixelMap.get(), inputBuffer, context);
        }
        return res;
    }

    if (inputBuffer->buffer_ == outputBuffer->buffer_) {
        EFFECT_LOGI("ImageSinkFilter: not need copy!");
    } else {
        // memcpy primary
        MemcpyHelper::CopyData(inputBuffer.get(), outputBuffer.get());
    }

    return res;
}

void CreateGainMapIfNeed(int width, int height, std::shared_ptr<EffectBuffer> &buffer)
{
    if (buffer->auxiliaryBufferInfos->find(EffectPixelmapType::GAINMAP) != buffer->auxiliaryBufferInfos->end()) {
        EFFECT_LOGE("CreateGainMapIfNeed no need to CreateGainMap");
        return;
    }
    std::shared_ptr<EffectBuffer> gainMapEffectBuffer;
    InitializationOptions initOptions;
    initOptions.size = { width, height };
    initOptions.pixelFormat = PixelFormat::RGBA_8888;
    initOptions.editable = true;
    std::unique_ptr<PixelMap> gainMap = PixelMap::Create(initOptions);
    auto errorCode = CommonUtils::LockPixelMap(gainMap.get(), gainMapEffectBuffer);
    CHECK_AND_RETURN_LOG(errorCode == ErrorCode::SUCCESS,
        "CreateGainMapIfNeed: LockPixelMap fail! errorCode=%{public}d", errorCode);
    gainMapEffectBuffer->bufferInfo_->pixelmapType_ = EffectPixelmapType::GAINMAP;
    gainMapEffectBuffer->bufferInfo_->bufferType_ = gainMapEffectBuffer->extraInfo_->bufferType;
    gainMapEffectBuffer->bufferInfo_->addr_ = gainMapEffectBuffer->buffer_;
    buffer->auxiliaryBufferInfos->emplace(EffectPixelmapType::GAINMAP, gainMapEffectBuffer->bufferInfo_);
}

struct AuxiliaryProcessContext {
    std::shared_ptr<EffectContext> context;
    Picture* dstPicture;
};

void ProcessGainMap(const std::shared_ptr<EffectBuffer>& srcEffectBuffer,
    const std::shared_ptr<EffectBuffer>& dstEffectBuffer, const AuxiliaryProcessContext& procCtx)
{
    if (srcEffectBuffer->extraInfo_->dataType != DataType::TEX) {
        MemcpyHelper::CopyData(srcEffectBuffer.get(), dstEffectBuffer.get());
        return;
    }

    std::shared_ptr<PixelMap> dstPixelMap = nullptr;
    MetaDataMap metaData;

    auto srcGainMapBufferInfo = srcEffectBuffer->bufferInfo_;
    if (!srcGainMapBufferInfo) {
        EFFECT_LOGE("FillPictureOutputData: src gainmap not found in auxiliary buffer");
        return;
    }
    auto defaultExtraInfo = std::make_shared<ExtraInfo>();
    auto srcGainMapBuffer = std::make_shared<EffectBuffer>(srcGainMapBufferInfo, nullptr, defaultExtraInfo);
    dstPixelMap = procCtx.dstPicture->GetGainmapPixelMap();
    CHECK_AND_RETURN_LOG(dstPixelMap, "ProcessGainMap: dstPixelMap is nullptr!");
    uint8_t *pixels = const_cast<uint8_t *>(dstPixelMap->GetPixels());
    CHECK_AND_RETURN_LOG(pixels, "ProcessGainMap: pixels is nullptr!");
    dstEffectBuffer->buffer_ = static_cast<void *>(pixels);
    if (srcGainMapBuffer && CommonUtils::IsEnableCopyMetaData(DOUBLE_BUFFER, srcGainMapBuffer.get(),
        dstEffectBuffer.get())) {
        metaData = CommonUtils::GetMetaData(
            static_cast<SurfaceBuffer*>(srcEffectBuffer->bufferInfo_->pixelMap_->GetFd()));
    }

    if (dstEffectBuffer->bufferInfo_->width_ == srcEffectBuffer->bufferInfo_->width_ &&
        dstEffectBuffer->bufferInfo_->height_ == srcEffectBuffer->bufferInfo_->height_) {
        procCtx.context->renderEnvironment_->ConvertTextureToBuffer(srcEffectBuffer->bufferInfo_->tex_,
            dstEffectBuffer.get(), true);
    } else {
        CommonUtils::ModifyPixelMapPropertyForTexture(dstPixelMap.get(), srcEffectBuffer, procCtx.context);
    }

    if (!metaData.empty()) {
        CommonUtils::SetMetaData(metaData,
            reinterpret_cast<SurfaceBuffer*>(dstEffectBuffer->bufferInfo_->pixelMap_->GetFd()));
    }
    auto auxilaryPicture = procCtx.dstPicture->GetAuxiliaryPicture(AuxiliaryPictureType::GAINMAP);
    CHECK_AND_RETURN_LOG(auxilaryPicture, "ModifyPicture: auxilaryPicture not exist!");
    auxilaryPicture->SetContentPixel(dstPixelMap);
}

void ProcessAuxiliaryEntry(EffectPixelmapType pixelmapType, const std::shared_ptr<EffectBuffer>& srcEffectBuffer,
    const std::shared_ptr<EffectBuffer>& dstEffectBuffer, const AuxiliaryProcessContext& procCtx)
{
    switch (pixelmapType) {
        case EffectPixelmapType::GAINMAP:
            ProcessGainMap(srcEffectBuffer, dstEffectBuffer, procCtx);
            break;
        default:
            break;
    }
}

ErrorCode FillPictureAuxilaryMap(EffectBuffer *src, const std::shared_ptr<EffectBuffer>& inputBuffer,
    std::shared_ptr<EffectBuffer>& outputBuffer, const std::shared_ptr<EffectContext>& context)
{
    auto dstPicture = outputBuffer->extraInfo_->picture;
    CHECK_AND_RETURN_RET_LOG(dstPicture != nullptr, ErrorCode::ERR_INPUT_NULL,
        "FillPictureOutputData: srcPicture or dstPicture is null!");

    auto inputAuxiliaryInfos = inputBuffer->auxiliaryBufferInfos;
    auto outputAuxiliaryInfos = outputBuffer->auxiliaryBufferInfos;
    CHECK_AND_RETURN_RET_LOG(inputAuxiliaryInfos != nullptr && outputAuxiliaryInfos != nullptr, ErrorCode::SUCCESS,
        "FillPictureOutputData: inputAuxiliary or outputAuxiliary is null!");

    CreateGainMapIfNeed(static_cast<int>(src->bufferInfo_->width_), static_cast<int>(src->bufferInfo_->height_),
        outputBuffer);

    EFFECT_LOGD("FillPictureOutputData: Process auxilaryBuffer");
    ErrorCode res = ErrorCode::SUCCESS;

    AuxiliaryProcessContext procCtx = { context, dstPicture };
    for (auto& [pixelmapType, srcEffectBufferInfo] : *inputAuxiliaryInfos) {
        auto outputIt = outputAuxiliaryInfos->find(pixelmapType);
        if (outputIt == outputAuxiliaryInfos->end()) {
            EFFECT_LOGD("FillPictureOutputData: No matching PixelmapType: %{public}d found in outputAuxiliary",
                pixelmapType);
            continue;
        }
        auto defaultExtraInfo = std::make_shared<ExtraInfo>();
        auto srcEffectBuffer = std::make_shared<EffectBuffer>(srcEffectBufferInfo, nullptr, defaultExtraInfo);
        defaultExtraInfo = std::make_shared<ExtraInfo>();
        auto dstEffectBuffer = std::make_shared<EffectBuffer>(outputIt->second, nullptr, defaultExtraInfo);
        CommonUtils::CopyExtraInfo(*inputBuffer->extraInfo_, *srcEffectBuffer->extraInfo_);
        CommonUtils::CopyExtraInfo(*outputBuffer->extraInfo_, *dstEffectBuffer->extraInfo_);
        ProcessAuxiliaryEntry(pixelmapType, srcEffectBuffer, dstEffectBuffer, procCtx);
    }

    return res;
}

ErrorCode FillPictureOutputData(EffectBuffer *src, const std::shared_ptr<EffectBuffer> &inputBuffer,
    std::shared_ptr<EffectBuffer> &outputBuffer, const std::shared_ptr<EffectContext> &context)
{
    // update output exif info
    CommonUtils::UpdateImageExifDateTime(outputBuffer->extraInfo_->picture);

    auto res = FillPictureMainPixel(inputBuffer, outputBuffer, context);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "FillPictureOutputData: FillPictureMainPixel failed!");

    res = FillPictureAuxilaryMap(src, inputBuffer, outputBuffer, context);
    return res;
}

ErrorCode StartImagePacking(const std::shared_ptr<ImagePacker> &imagePacker, const std::string &path,
    const PackOption option)
{
    uint32_t ret = imagePacker->StartPacking(path, option);
    CHECK_AND_RETURN_RET_LOG(ret == 0, ErrorCode::ERR_IMAGE_PACKER_EXEC_FAIL,
        "StartPacking fail! result=%{public}d, format=%{public}s", ret, option.format.c_str());
    return ErrorCode::SUCCESS;
}

ErrorCode ImageSinkFilter::PackToFile(const std::string &path, const std::shared_ptr<Picture> &picture)
{
    ErrorCode result = ErrorCode::SUCCESS;
    SourceOptions opts;
    uint32_t ret = 0;
    std::unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(inPath_, opts, ret);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, ErrorCode::ERR_CREATE_IMAGESOURCE_FAIL,
        "ImageSource::CreateImageSource fail! path=%{public}s, ret=%{public}d", inPath_.c_str(), ret);

    ImageInfo info;
    ret = imageSource->GetImageInfo(info);
    CHECK_AND_RETURN_RET_LOG(ret == 0, ErrorCode::ERR_FILE_TYPE_NOT_SUPPORT, "imageSource get image info fail!");

    std::string encodedFormat = info.encodedFormat;
    std::shared_ptr<ImagePacker> imagePacker = std::make_shared<ImagePacker>();
    PackOption option = {
        .format = encodedFormat,
        .desiredDynamicRange = EncodeDynamicRange::AUTO,
        .needsPackProperties = true,
    };
    if (encodedFormat == "image/heic" || encodedFormat == "image/heif") {
        result = StartImagePacking(imagePacker, path, option);
        if (result != ErrorCode::SUCCESS) {
            option.format = "image/jpeg";
            result = StartImagePacking(imagePacker, path, option);
            CHECK_AND_RETURN_RET_LOG(result == ErrorCode::SUCCESS, ErrorCode::ERR_IMAGE_PACKER_EXEC_FAIL,
                "StartPacking fail! result=%{public}d, format=%{public}s", result, option.format.c_str());
        }
    } else {
        result = StartImagePacking(imagePacker, path, option);
        CHECK_AND_RETURN_RET_LOG(result == ErrorCode::SUCCESS, ErrorCode::ERR_IMAGE_PACKER_EXEC_FAIL,
            "StartPacking fail! result=%{public}d, format=%{public}s", result, option.format.c_str());
    }

    ret = imagePacker->AddPicture(*picture);
    CHECK_AND_RETURN_RET_LOG(ret == 0, ErrorCode::ERR_IMAGE_PACKER_EXEC_FAIL,
        "AddImage fail! result=%{public}d", result);

    int64_t packedSize = 0;
    ret = imagePacker->FinalizePacking(packedSize);
    CHECK_AND_RETURN_RET_LOG(ret == 0, ErrorCode::ERR_IMAGE_PACKER_EXEC_FAIL,
        "FinalizePacking fail! result=%{public}d", result);

    EFFECT_LOGI("PackToFile success! path=%{public}s, packedSize=%{public}lld, encodedFormat=%{public}s", path.c_str(),
        static_cast<long long>(packedSize), encodedFormat.c_str());
    return result;
}

ErrorCode ImageSinkFilter::SaveUrlData(const std::string &url, const std::shared_ptr<EffectBuffer> &buffer)
{
    std::shared_ptr<Picture> picture = buffer->extraInfo_->innerPicture;
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, ErrorCode::ERR_INPUT_NULL, "SaveUrlData: picture is null!");

    std::string path = CommonUtils::UrlToPath(url);
    return PackToFile(path, picture);
}

ErrorCode ImageSinkFilter::SaveUrlData(const std::string &url, const std::shared_ptr<Picture> &picture)
{
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, ErrorCode::ERR_INPUT_NULL, "SaveUrlData: picture is null!");

    std::string path = CommonUtils::UrlToPath(url);
    return PackToFile(path, picture);
}

ErrorCode ImageSinkFilter::SavePathData(const std::string &path, const std::shared_ptr<EffectBuffer> &buffer)
{
    std::shared_ptr<Picture> picture = buffer->extraInfo_->innerPicture;
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, ErrorCode::ERR_INPUT_NULL, "SavePathData: picture is null!");

    return PackToFile(path, picture);
}

ErrorCode ImageSinkFilter::SavePathData(const std::string &path, const std::shared_ptr<Picture> &picture)
{
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, ErrorCode::ERR_INPUT_NULL, "SavePathData: picture is null!");
    return PackToFile(path, picture);
}

ErrorCode ImageSinkFilter::SaveInputData(EffectBuffer *src, const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    ErrorCode result = ErrorCode::SUCCESS;
    if (CommonUtils::IsEnableCopyMetaData(SINGLE_BUFFER, src)) {
        auto metaData = CommonUtils::GetMetaData(src->bufferInfo_->surfaceBuffer_);
        result = ModifyDataInfo(src, buffer, context);
        CommonUtils::SetMetaData(metaData, reinterpret_cast<SurfaceBuffer*>(src->bufferInfo_->pixelMap_->GetFd()));
    } else {
        result = ModifyDataInfo(src, buffer, context);
    }
    CHECK_AND_RETURN_RET_LOG(result == ErrorCode::SUCCESS, result, "ModifyDataInfo fail! result=%{public}d", result);

    EFFECT_LOGD("SaveInputData: dataType=%{public}d", buffer->extraInfo_->dataType);
    switch (src->extraInfo_->dataType) {
        case DataType::URI:
            return SaveUrlData(src->extraInfo_->uri, src->extraInfo_->innerPicture);
        case DataType::PATH:
            return SavePathData(src->extraInfo_->path, src->extraInfo_->innerPicture);
        default:
            return ErrorCode::SUCCESS;
    }
}

ErrorCode CheckAndProcessOutTex(RenderTexturePtr dstTex, RenderTexturePtr srcTex)
{
    CHECK_AND_RETURN_RET_LOG(srcTex != nullptr, ErrorCode::ERR_INPUT_NULL, "ModifyTex srcTex is null");
    CHECK_AND_RETURN_RET_LOG(dstTex != nullptr, ErrorCode::ERR_INPUT_NULL, "ModifyTex dstTex is null");
    if (dstTex->Width() == 0 || dstTex->Height() == 0) {
        GLUtils::CreateDefaultTexture(srcTex->Width(), srcTex->Height(), srcTex->Format(), dstTex->GetName());
    }
    return ErrorCode::SUCCESS;
}

ErrorCode ImageSinkFilter::SavaOutputData(EffectBuffer *src, const std::shared_ptr<EffectBuffer> &inputBuffer,
    std::shared_ptr<EffectBuffer> &outputBuffer, std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGD("SavaOutputData: outputBuffer dataType=%{public}d", outputBuffer->extraInfo_->dataType);
    switch (outputBuffer->extraInfo_->dataType) {
        case DataType::URI: {
            ErrorCode ret = ModifyInnerPicture(src, inputBuffer, context);
            CHECK_AND_RETURN_RET_LOG(ret == ErrorCode::SUCCESS, ret,
                "SavaOutputData: Uri ModifyInnerPicture fail! ret=%{public}d", ret);
            return SaveUrlData(outputBuffer->extraInfo_->uri, src->extraInfo_->innerPicture);
        }
        case DataType::PATH: {
            ErrorCode ret = ModifyInnerPicture(src, inputBuffer, context);
            CHECK_AND_RETURN_RET_LOG(ret == ErrorCode::SUCCESS, ret,
                "SavaOutputData: Path ModifyInnerPicture fail! ret=%{public}d", ret);
            return SavePathData(outputBuffer->extraInfo_->path, src->extraInfo_->innerPicture);
        }
        case DataType::PIXEL_MAP:
        case DataType::SURFACE:
        case DataType::SURFACE_BUFFER:
            return FillOutputData(inputBuffer, outputBuffer, context);
        case DataType::PICTURE:
            return FillPictureOutputData(src, inputBuffer, outputBuffer, context);
        case DataType::TEX: {
            ErrorCode ret = CheckAndProcessOutTex(outputBuffer->bufferInfo_->tex_, inputBuffer->bufferInfo_->tex_);
            CHECK_AND_RETURN_RET_LOG(ret == ErrorCode::SUCCESS, ret,
                "SavaOutputData: CheckAndProcessOutTex fail! ret=%{public}d", ret);
            return ModifyTex(outputBuffer.get(), inputBuffer, context);
        }
        default:
            return ErrorCode::ERR_UNSUPPORTED_DATA_TYPE;
    }
}

ErrorCode ImageSinkFilter::SaveData(const std::shared_ptr<EffectBuffer> &inputBuffer,
    std::shared_ptr<EffectBuffer> &outputBuffer, std::shared_ptr<EffectContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(inputBuffer != nullptr && inputBuffer->bufferInfo_ != nullptr &&
        (inputBuffer->buffer_ != nullptr || inputBuffer->bufferInfo_->tex_ != nullptr)
        && inputBuffer->extraInfo_ != nullptr, ErrorCode::ERR_INPUT_NULL, "inputBuffer para error!");
    EffectBuffer *src = context->renderStrategy_->GetInput();
    CHECK_AND_RETURN_RET_LOG(src != nullptr, ErrorCode::ERR_SRC_EFFECT_BUFFER_NULL, "src is null!");
    if (outputBuffer == nullptr) {
        return SaveInputData(src, inputBuffer, context);
    }
    CHECK_AND_RETURN_RET_LOG(outputBuffer != nullptr && outputBuffer->extraInfo_ != nullptr,
        ErrorCode::ERR_INPUT_NULL, "outputBuffer extra info error!");

    // part para can be null for url or path data
    if (outputBuffer->extraInfo_->dataType != DataType::URI && outputBuffer->extraInfo_->dataType != DataType::PATH) {
        CHECK_AND_RETURN_RET_LOG(outputBuffer->bufferInfo_ != nullptr &&
            (outputBuffer->buffer_ != nullptr || outputBuffer->bufferInfo_->tex_ != nullptr),
            ErrorCode::ERR_INPUT_NULL, "outputBuffer buffer info or buffer addr error!");
    }

    ErrorCode res;
    if (CommonUtils::IsEnableCopyMetaData(DOUBLE_BUFFER, src, outputBuffer.get())) {
        auto extraInfo = src->extraInfo_;
        auto bufferInfo = src->bufferInfo_;
        auto metaData = CommonUtils::GetMetaData(bufferInfo->surfaceBuffer_);
        res = SavaOutputData(src, inputBuffer, outputBuffer, context);
        CommonUtils::SetMetaData(metaData, reinterpret_cast<SurfaceBuffer*>(
            outputBuffer->bufferInfo_->pixelMap_->GetFd()));
    } else {
        res = SavaOutputData(src, inputBuffer, outputBuffer, context);
    }
    return res;
}

void ImageSinkFilter::Negotiate(const std::string& inPort, const std::shared_ptr<Capability> &capability,
    std::shared_ptr<EffectContext> &context)
{
    std::shared_ptr<MemNegotiatedCap> memNegotiatedCap;
    if (sinkBuffer_) {
        memNegotiatedCap = std::make_shared<MemNegotiatedCap>();
        memNegotiatedCap->width = sinkBuffer_->bufferInfo_->width_;
        memNegotiatedCap->height = sinkBuffer_->bufferInfo_->height_;
        memNegotiatedCap->format = sinkBuffer_->bufferInfo_->formatType_;
    } else {
        memNegotiatedCap = capability->memNegotiatedCap_;
    }
    std::shared_ptr<Capability> sinkCap = std::make_shared<Capability>(name_);
    sinkCap->memNegotiatedCap_ = memNegotiatedCap;
    context->capNegotiate_->AddCapability(sinkCap);
}

void ImageSinkFilter::FlushBufferToScreen(sptr<SurfaceBuffer> &outBuffer, sptr<SyncFence> &fence) const
{
    CHECK_AND_RETURN_LOG(outBuffer, "FlushBufferToScreen: outBuffer is nullptr");
    EFFECT_LOGI("FlushBufferToScreen::toProducerSurface_ width = %{public}d, height = %{public}d",
        toXComponentSurface_->GetDefaultWidth(), toXComponentSurface_->GetDefaultHeight());
    BufferFlushConfig flushConfig = {
        .damage = {
            .w = outBuffer->GetWidth(),
            .h = outBuffer->GetHeight(),
        },
    };
    toXComponentSurface_->FlushBuffer(outBuffer, fence, flushConfig);
}

void ImageSinkFilter::RequestBufferFromScreen(BufferRequestConfig &requestConfig, sptr<SurfaceBuffer> &outBuffer,
    sptr<SyncFence> &syncFence) const
{
    EFFECT_LOGD("RequestBufferFromScreen: requestConfig.height = %{public}d requestConfig.width = %{public}d",
        requestConfig.height, requestConfig.width);
    toXComponentSurface_->RequestBuffer(outBuffer, syncFence, requestConfig);
}

ErrorCode ImageSinkFilter::SurfaceRenderFlow(SurfaceBuffer* srcBuffer, BufferRequestConfig& requestConfig,
    MetaDataMap& hdrMetaDataMap, const int32_t &colorSpaceType, const std::shared_ptr<EffectContext>& context) const
{
    sptr<SurfaceBuffer> outBuffer = nullptr;
    sptr<SyncFence> fence = SyncFence::InvalidFence();
    RequestBufferFromScreen(requestConfig, outBuffer, fence);
    // GPU
    context->renderEnvironment_->BeginFrame();
    context->renderEnvironment_->DrawSurfaceBufferFromSurfaceBuffer(
        srcBuffer, outBuffer, IEffectFormat::RGBA_1010102);
    glFinish();
    ColorSpaceHelper::SetSurfaceBufferColorSpaceType(outBuffer, static_cast<CM_ColorSpaceType>(colorSpaceType));

    // flush
    CommonUtils::SetMetaData(hdrMetaDataMap, outBuffer);
    FlushBufferToScreen(outBuffer, fence);
    return ErrorCode::SUCCESS;
}

ErrorCode ImageSinkFilter::TextureRenderFlow(RenderTexturePtr texture, BufferRequestConfig &requestConfig,
    MetaDataMap& hdrMetaDataMap, const int32_t &colorSpaceType, const std::shared_ptr<EffectContext>& context)
{
    sptr<SurfaceBuffer> outBuffer = nullptr;
    sptr<SyncFence> requestFence = SyncFence::InvalidFence();
    RequestBufferFromScreen(requestConfig, outBuffer, requestFence);
    CHECK_AND_RETURN_RET_LOG(outBuffer, ErrorCode::ERR_NO_VALUE, "RequestBufferFromScreen fail");

    const auto seqNum = outBuffer->GetSeqNum();
    auto it = texureCacheSeqs_.find(seqNum);
    if (it == texureCacheSeqs_.end()) {
        EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        EGLImageKHR outputImg = GLUtils::CreateEGLImage(display, outBuffer);
        CHECK_AND_RETURN_RET_LOG(outputImg != EGL_NO_IMAGE_KHR, ErrorCode::ERR_GL_DRAW_FAILED,
            "TextureRenderFlow: CreateEGLImage fail");

        GLuint outputTex = GLUtils::CreateTextureFromImage(outputImg);
        if (outputTex == 0) {
            EFFECT_LOGE("TextureRenderFlow: CreateTextureFromImage fail");
            GLUtils::DestroyImage(outputImg);
            return ErrorCode::ERR_GL_DRAW_FAILED;
        }

        it = texureCacheSeqs_.emplace(seqNum, TextureCacheSeq{outputTex, outputImg, EGL_NO_SYNC_KHR}).first;
    }
    TextureCacheSeq& cacheSeq = it->second;
    GLuint outputTex = cacheSeq.texId_;

    context->renderEnvironment_->BeginFrame();
    context->renderEnvironment_->DrawOesTexture2DFromTexture(texture, outputTex, outBuffer->GetWidth(),
        outBuffer->GetHeight(), IEffectFormat::RGBA_1010102);
    GLUtils::CreateSyncKHR(cacheSeq.eglSync_);
    glFlush();

    ColorSpaceHelper::SetSurfaceBufferColorSpaceType(outBuffer, static_cast<CM_ColorSpaceType>(colorSpaceType));
    CommonUtils::SetMetaData(hdrMetaDataMap, outBuffer);
    // flush
    int fenceFd = GLUtils::GetEGLFenceFd(cacheSeq.eglSync_);
    sptr<SyncFence> flushFence = new SyncFence(fenceFd);
    FlushBufferToScreen(outBuffer, flushFence);
    return ErrorCode::SUCCESS;
}

sptr<SurfaceBuffer> ImageSinkFilter::GetOrCreateSurfaceBuffer(const BufferRequestConfig& requestConfig)
{
    if (hdrSurfaceBuffer_) {
        if (hdrSurfaceBuffer_->GetHeight() == requestConfig.height &&
            hdrSurfaceBuffer_->GetWidth() == requestConfig.width) {
            return hdrSurfaceBuffer_;
        } else {
            hdrSurfaceBuffer_->DecStrongRef(hdrSurfaceBuffer_);
            hdrSurfaceBuffer_ = nullptr;
        }
    }

    EFFECT_LOGI("ImageSinkFilter::GetOrCreateSurfaceBuffer: Create new hdrSurfaceBuffer");
    hdrSurfaceBuffer_ = SurfaceBuffer::Create();
    hdrSurfaceBuffer_->Alloc(requestConfig);
    hdrSurfaceBuffer_->Map();
    hdrSurfaceBuffer_->IncStrongRef(hdrSurfaceBuffer_);
    return hdrSurfaceBuffer_;
}

BufferRequestConfig ImageSinkFilter::CreateBaseBufferConfig(int32_t width, int32_t height, GraphicPixelFormat format,
    GraphicTransformType transform, GraphicColorGamut colorGamut) {
    return {
        .width = width,
        .height = height,
        .strideAlignment = 0x8, // set 0x8 as default value to alloc SurfaceBufferImpl
        .format = format,
        .usage = BUFFER_USAGE_CPU_READ
               | BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_MEM_MMZ_CACHE,
        .timeout = 0,
        .colorGamut = colorGamut,
        .transform = transform
    };
}

ErrorCode ImageSinkFilter::RenderHdr10(const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    EffectBuffer *input = context->renderStrategy_->GetInput();
    CHECK_AND_RETURN_RET_LOG(input, ErrorCode::ERR_INPUT_NULL, "Input buffer is nullptr");
    auto sb = buffer->bufferInfo_->surfaceBuffer_;
    CHECK_AND_RETURN_RET_LOG(input, ErrorCode::ERR_INPUT_NULL, "Input buffer is nullptr");
    CM_ColorSpaceType colorSpaceType;
    ColorSpaceHelper::GetSurfaceBufferColorSpaceType(sb, colorSpaceType);
    auto srcMetaData = CommonUtils::GetMetaData(sb);
    auto transformType = GetSurfaceTransform(input);
    auto requestConfig = CreateBaseBufferConfig(
        static_cast<int32_t>(buffer->bufferInfo_->tex_->Width()),
        static_cast<int32_t>(buffer->bufferInfo_->tex_->Height()),
        CommonUtils::SwitchToGraphicPixelFormat(buffer->bufferInfo_->formatType_),
        transformType, sb->GetSurfaceBufferColorGamut());

    return TextureRenderFlow(buffer->bufferInfo_->tex_, requestConfig, srcMetaData, colorSpaceType, context);
}

ErrorCode ImageSinkFilter::Render8GainMap(const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    EffectBuffer *input = context->renderStrategy_->GetInput();
    CHECK_AND_RETURN_RET_LOG(input, ErrorCode::ERR_INPUT_NULL, "Input buffer is nullptr");

    auto [primaryBuffer, auxiliaryBuffer] = PrepareBuffers(input);

    // save data
    if (!CommonUtils::IsEnableCopyMetaData(SINGLE_BUFFER, input)) {
        EFFECT_LOGE("Render8GainMap: IsEnableCopyMetaData fail");
    }
    auto [primaryMetaData, gainMapMetaData] = PrepareMetaDatas(input);
    auto result = ModifyPicture(input, buffer, context);
    CHECK_AND_RETURN_RET_LOG(result == ErrorCode::SUCCESS, result, "SaveData fail");
    EffectBuffer *output = input;
    SetPictureMetaData(output, primaryMetaData, gainMapMetaData);

    std::shared_ptr<PixelMap> composedPixelMap = output->extraInfo_->picture->GetHdrComposedPixelMap();
    CHECK_AND_RETURN_RET_LOG(composedPixelMap->GetFd() != nullptr, result,
        "ImageSinkFilter::RenderToDisplay surfaceBuffer is nullptr!");

    auto fromConsumerSurfaceBuffer = reinterpret_cast<SurfaceBuffer*>(composedPixelMap->GetFd());
    CM_ColorSpaceType colorSpaceType;
    ColorSpaceHelper::GetSurfaceBufferColorSpaceType(fromConsumerSurfaceBuffer, colorSpaceType);
    auto composedMetaData = CommonUtils::GetMetaData(fromConsumerSurfaceBuffer);
    auto requestConfig = CreateBaseBufferConfig(
        fromConsumerSurfaceBuffer->GetWidth(), fromConsumerSurfaceBuffer->GetHeight(),
        CommonUtils::SwitchToGraphicPixelFormat(CommonUtils::SwitchToEffectFormat(composedPixelMap->GetPixelFormat())),
        GetSurfaceTransform(input), fromConsumerSurfaceBuffer->GetSurfaceBufferColorGamut());

    result = SurfaceRenderFlow(fromConsumerSurfaceBuffer, requestConfig, composedMetaData, colorSpaceType, context);
    CHECK_AND_RETURN_RET_LOG(result == ErrorCode::SUCCESS, result, "CommonRenderFlow failed!");

    // rollback
    primaryBuffer->auxiliaryBufferInfos =
        std::make_shared<std::unordered_map<EffectPixelmapType, std::shared_ptr<BufferInfo>>>();
    primaryBuffer->auxiliaryBufferInfos->emplace(EffectPixelmapType::GAINMAP, auxiliaryBuffer->bufferInfo_);
    context->renderEnvironment_->GetOrCreateTextureFromCache(primaryBuffer->bufferInfo_->tex_, "Primary", 0, 0, false);
    context->renderEnvironment_->GetOrCreateTextureFromCache(auxiliaryBuffer->bufferInfo_->tex_,
        "GainMap", 0, 0, false);

    result = ModifyPicture(input, primaryBuffer, context);
    SetPictureMetaData(output, primaryMetaData, gainMapMetaData);
    return result;
}

ErrorCode ImageSinkFilter::RenderToDisplay(const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGD("ImageSinkFilter::RenderToDisplay");
    if (buffer->bufferInfo_->hdrFormat_ == HdrFormat::HDR8_GAINMAP &&
        buffer->bufferInfo_->formatType_ == IEffectFormat::RGBA8888) {
        return Render8GainMap(buffer, context);
    } else if (buffer->bufferInfo_->hdrFormat_ == HdrFormat::HDR10 || (buffer->bufferInfo_->hdrFormat_ ==
        HdrFormat::HDR8_GAINMAP && buffer->bufferInfo_->formatType_ == IEffectFormat::RGBA_1010102)) {
        return RenderHdr10(buffer, context);
    }
    return ErrorCode::SUCCESS;
}

ErrorCode ImageSinkFilter::ProcessDisplayForNoTex(const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    if (context->renderEnvironment_->GetEGLStatus() != EGLStatus::READY) {
        context->renderEnvironment_->Init();
        context->renderEnvironment_->Prepare();
    }
    if (buffer->bufferInfo_->hdrFormat_ == HdrFormat::HDR10
        && buffer->bufferInfo_->formatType_ == IEffectFormat::RGBA_1010102) {
        EffectBuffer *input = context->renderStrategy_->GetInput();
        CHECK_AND_RETURN_RET_LOG(input, ErrorCode::ERR_INPUT_NULL, "Input buffer is nullptr");
        auto sb = buffer->bufferInfo_->surfaceBuffer_;
        CHECK_AND_RETURN_RET_LOG(input, ErrorCode::ERR_INPUT_NULL, "Input buffer is nullptr");
        auto transformType = GetSurfaceTransform(input);
        auto requestConfig = CreateBaseBufferConfig(
            static_cast<int32_t>(buffer->bufferInfo_->width_),
            static_cast<int32_t>(buffer->bufferInfo_->height_),
            CommonUtils::SwitchToGraphicPixelFormat(buffer->bufferInfo_->formatType_),
            transformType, sb->GetSurfaceBufferColorGamut());
        CM_ColorSpaceType colorSpaceType;
        ColorSpaceHelper::GetSurfaceBufferColorSpaceType(sb, colorSpaceType);
        ColorSpaceHelper::UpdateMetadata(sb, ColorSpaceHelper::ConvertToEffectColorSpace(colorSpaceType), context);
        auto srcMetaData = CommonUtils::GetMetaData(sb);
        return SurfaceRenderFlow(sb, requestConfig, srcMetaData, colorSpaceType, context);
    }
    EGLImageKHR img = GLUtils::CreateEGLImage(eglGetDisplay(EGL_DEFAULT_DISPLAY),
        buffer->bufferInfo_->surfaceBuffer_);
    int tex = static_cast<int>(GLUtils::CreateTextureFromImage(img));
    buffer->bufferInfo_->surfaceBuffer_->FlushCache();
    context->renderEnvironment_->UpdateCanvas();
    GraphicTransformType transformType = buffer->bufferInfo_->surfaceBuffer_->GetSurfaceBufferTransform();
    context->renderEnvironment_->DrawFrame(tex, transformType);
    GLUtils::DestroyImage(img);
    return ErrorCode::SUCCESS;
}

ErrorCode ImageSinkFilter::PushData(const std::string &inPort, const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGD("image sink effect push data started, state: %{public}d", state_.load());
    EffectBuffer *output = nullptr;
    if (sinkBuffer_ != nullptr) {
        output = sinkBuffer_.get();
    } else {
        output = context->renderStrategy_->GetInput();
    }

    if (buffer->extraInfo_->dataType == DataType::TEX) {
        if (output->extraInfo_->dataType == DataType::NATIVE_WINDOW) {
            if (buffer->bufferInfo_->hdrFormat_ == HdrFormat::HDR8_GAINMAP ||
                buffer->bufferInfo_->hdrFormat_ == HdrFormat::HDR10) {
                ErrorCode res = RenderToDisplay(buffer, context);
                return res;
            }

            EffectBuffer *input = context->renderStrategy_->GetInput();
            GraphicTransformType transformType = GRAPHIC_ROTATE_NONE;
            if (input->bufferInfo_->surfaceBuffer_ != nullptr) {
                transformType = input->bufferInfo_->surfaceBuffer_->GetSurfaceBufferTransform();
            }
            RenderTexturePtr renderTexture = buffer->bufferInfo_->tex_;
            context->renderEnvironment_->SetNativeWindowColorSpace(buffer->bufferInfo_->colorSpace_);
            RenderTexturePtr tempTex = context->renderEnvironment_->RequestBuffer(renderTexture->Width(),
                renderTexture->Height());
            context->renderEnvironment_->DrawFlipTex(renderTexture, tempTex);
            buffer->bufferInfo_->tex_ = tempTex;
            context->renderEnvironment_->DrawFrameWithTransform(const_cast<std::shared_ptr<EffectBuffer> &>(buffer),
                transformType);
            return ErrorCode::SUCCESS;
        }
    }

    if (output->extraInfo_->dataType == DataType::NATIVE_WINDOW && buffer->bufferInfo_->surfaceBuffer_ != nullptr) {
        ErrorCode result = ProcessDisplayForNoTex(buffer, context);
        CHECK_AND_RETURN_RET_LOG(result == ErrorCode::SUCCESS, result, "ProcessDisplayForNoTex fail!");

        return ErrorCode::SUCCESS;
    }

    EFFECT_LOGD("ImageSinkFilter::PushData SaveData");
    ErrorCode result = SaveData(buffer, sinkBuffer_, context);
    CHECK_AND_RETURN_RET_LOG(result == ErrorCode::SUCCESS, result, "SaveData fail! result=%{public}d", result);
    eventReceiver_->OnEvent(Event{ name_, EventType::EVENT_COMPLETE, { buffer } });
    return ErrorCode::SUCCESS;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS
