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

#include "common_utils.h"
#include "effect_log.h"
#include "filter_factory.h"
#include "image_packer.h"
#include "memcpy_helper.h"
#include "format_helper.h"
#include "colorspace_helper.h"
#include "render_environment.h"

namespace OHOS {
namespace Media {
namespace Effect {
REGISTER_FILTER_FACTORY(ImageSinkFilter);

ErrorCode ImageSinkFilter::SetSink(const std::shared_ptr<EffectBuffer> &sink)
{
    EFFECT_LOGI("SetSink entered.");
    sinkBuffer_ = sink;
    return ErrorCode::SUCCESS;
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
    PixelMap *pixelMap = src->extraInfo_->pixelMap;
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, ErrorCode::ERR_INPUT_NULL, "pixelMap is null!");
    uint8_t *pixels = const_cast<uint8_t *>(pixelMap->GetPixels());
    if (pixels == buffer->buffer_) {
        EFFECT_LOGD("ModifyPixelMap: not need modify pixelmap!");
        CommonUtils::UpdateImageExifDateTime(pixelMap);
        CHECK_AND_RETURN_RET(context->metaInfoNegotiate_->IsNeedUpdate() != true, ErrorCode::SUCCESS);
        return ColorSpaceHelper::UpdateMetadata(src);
    }

    if (buffer->extraInfo_->dataType == DataType::TEX) {
        if (pixelMap->GetWidth() == static_cast<int32_t>(buffer->bufferInfo_->width_) &&
            pixelMap->GetHeight() == static_cast<int32_t>(buffer->bufferInfo_->height_) && pixels == src->buffer_) {
            context->renderEnvironment_->ConvertTextureToBuffer(buffer->tex, src);
            CommonUtils::UpdateImageExifDateTime(pixelMap);
            CHECK_AND_RETURN_RET(context->metaInfoNegotiate_->IsNeedUpdate() != true, ErrorCode::SUCCESS);
            return ColorSpaceHelper::UpdateMetadata(src);
        } else {
            ErrorCode result = CommonUtils::ModifyPixelMapPropertyForTexture(pixelMap, buffer, context);
            return result;
        }
    }

    if (static_cast<uint32_t>(pixelMap->GetRowStride()) == buffer->bufferInfo_->rowStride_ &&
        static_cast<uint32_t>(pixelMap->GetHeight()) == buffer->bufferInfo_->height_ &&
        CommonUtils::SwitchToEffectFormat(pixelMap->GetPixelFormat()) == buffer->bufferInfo_->formatType_) {
        EFFECT_LOGD("Copy data to pixel map.");
        CopyDataToPixelMap(pixelMap, buffer);
        CHECK_AND_RETURN_RET(context->metaInfoNegotiate_->IsNeedUpdate() != true, ErrorCode::SUCCESS);
        return ColorSpaceHelper::UpdateMetadata(buffer.get());
    }

    ErrorCode result = CommonUtils::ModifyPixelMapProperty(pixelMap, buffer, context->memoryManager_);
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
    SurfaceBuffer *surfaceBuffer = src->extraInfo_->surfaceBuffer;
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, ErrorCode::ERR_INPUT_NULL, "surfaceBuffer is null!");
    EFFECT_LOGD("ModifySurfaceBuffer: virAddr=%{public}p, inputBufAddr=%{public}p",
        surfaceBuffer->GetVirAddr(), buffer->buffer_);
    if (surfaceBuffer->GetVirAddr() == buffer->buffer_) {
        CHECK_AND_RETURN_RET(context->metaInfoNegotiate_->IsNeedUpdate() != true, ErrorCode::SUCCESS);
        return ColorSpaceHelper::UpdateMetadata(buffer.get());
    }

    if (buffer->extraInfo_->dataType == DataType::TEX) {
        if (surfaceBuffer->GetWidth() == static_cast<int32_t>(buffer->bufferInfo_->width_) &&
            surfaceBuffer->GetHeight() == static_cast<int32_t>(buffer->bufferInfo_->height_)) {
            context->renderEnvironment_->ConvertTextureToBuffer(buffer->tex, src);
            CHECK_AND_RETURN_RET(context->metaInfoNegotiate_->IsNeedUpdate() != true, ErrorCode::SUCCESS);
            return ColorSpaceHelper::UpdateMetadata(src);
        }
        return ErrorCode::ERR_BUFFER_NOT_ALLOW_CHANGE;
    }

    if (static_cast<uint32_t>(surfaceBuffer->GetStride()) == buffer->bufferInfo_->rowStride_ &&
        static_cast<uint32_t>(surfaceBuffer->GetHeight()) == buffer->bufferInfo_->height_ &&
        CommonUtils::SwitchToEffectFormat((GraphicPixelFormat)surfaceBuffer->GetFormat()) ==
        buffer->bufferInfo_->formatType_) {
        EFFECT_LOGD("Copy data to surface buffer.");
        CopyDataToSurfaceBuffer(surfaceBuffer, buffer);
        CHECK_AND_RETURN_RET(context->metaInfoNegotiate_->IsNeedUpdate() != true, ErrorCode::SUCCESS);
        return ColorSpaceHelper::UpdateMetadata(buffer.get());
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
        CHECK_AND_RETURN_RET(context->metaInfoNegotiate_->IsNeedUpdate() != true, ErrorCode::SUCCESS);
        return ColorSpaceHelper::UpdateMetadata(buffer.get());
    }

    // update picture exif
    CommonUtils::UpdateImageExifInfo(picture.get());

    if (buffer->extraInfo_->dataType == DataType::TEX) {
        return CommonUtils::ModifyPixelMapPropertyForTexture(mainPixel.get(), buffer, context, false);
    }

    ErrorCode res = CommonUtils::ModifyPixelMapProperty(mainPixel.get(), buffer, context->memoryManager_, false);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "ModifyInnerPicture: modify main pixelmap property fail!");

    if (buffer->auxiliaryBufferInfos == nullptr) {
        return ErrorCode::SUCCESS;
    }

    auto it = buffer->auxiliaryBufferInfos->find(EffectPixelmapType::GAINMAP);
    if (it == buffer->auxiliaryBufferInfos->end()) {
        return ErrorCode::SUCCESS;
    }

    auto gainMapBufferInfo = it->second;
    std::shared_ptr<PixelMap> gainMap = picture->GetGainmapPixelMap();
    if (gainMap != nullptr && gainMapBufferInfo != nullptr && gainMapBufferInfo->addr_ != nullptr &&
        static_cast<uint8_t *>(gainMapBufferInfo->addr_) != gainMap->GetPixels()) {
        std::shared_ptr<ExtraInfo> defaultExtraInfo = std::make_shared<ExtraInfo>();
        std::shared_ptr<EffectBuffer> gainMapEffectBuffer =
            std::make_shared<EffectBuffer>(gainMapBufferInfo, gainMapBufferInfo->addr_, defaultExtraInfo);
        return CommonUtils::ModifyPixelMapProperty(gainMap.get(), gainMapEffectBuffer, context->memoryManager_, false);
    }

    return ErrorCode::SUCCESS;
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
            context->renderEnvironment_->ConvertTextureToBuffer(buffer->tex, src);
            return ErrorCode::SUCCESS;
        } else {
            return CommonUtils::ModifyPixelMapPropertyForTexture(pixelMap, buffer, context, false);
        }
    }

    if (static_cast<uint32_t>(pixelMap->GetRowStride()) == buffer->bufferInfo_->rowStride_ &&
        static_cast<uint32_t>(pixelMap->GetHeight()) == buffer->bufferInfo_->height_ &&
        CommonUtils::SwitchToEffectFormat(pixelMap->GetPixelFormat()) == buffer->bufferInfo_->formatType_) {
        EFFECT_LOGD("ModifyPicture: Copy data to pixel map.");
        CopyDataToPixelMap(pixelMap, buffer);
        return ErrorCode::SUCCESS;
    }

    return CommonUtils::ModifyPixelMapProperty(pixelMap, buffer, context->memoryManager_, false);
}

ErrorCode ModifyPicture(EffectBuffer *src, const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    Picture *picture = src->extraInfo_->picture;
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, ErrorCode::ERR_INPUT_NULL, "ModifyPicture: picture is null!");
    auto primary = picture->GetMainPixel();
    CHECK_AND_RETURN_RET_LOG(primary != nullptr, ErrorCode::ERR_INPUT_NULL, "ModifyPicture: main pixelmap is null!");

    std::shared_ptr<EffectBuffer> primaryBuffer = std::make_shared<EffectBuffer>(buffer->bufferInfo_, buffer->buffer_,
        buffer->extraInfo_);
    primaryBuffer->tex = buffer->tex;
    ErrorCode res = ModifyPictureForInnerPixelMap(primary.get(), src, primaryBuffer, context);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "ModifyPicture: modify main pixelMap fail!");

    CommonUtils::UpdateImageExifInfo(picture);

    if (buffer->auxiliaryBufferInfos == nullptr) {
        return ErrorCode::SUCCESS;
    }

    auto it = buffer->auxiliaryBufferInfos->find(EffectPixelmapType::GAINMAP);
    if (it == buffer->auxiliaryBufferInfos->end()) {
        return ErrorCode::SUCCESS;
    }
    auto gainMap = picture->GetGainmapPixelMap();
    CHECK_AND_RETURN_RET_LOG(gainMap != nullptr, ErrorCode::ERR_INPUT_NULL, "ModifyPicture: gainMap is null!");

    auto gainMapBufferInfo = it->second;
    std::shared_ptr<ExtraInfo> defaultExtraInfo = std::make_shared<ExtraInfo>();
    std::shared_ptr<EffectBuffer> gainMapEffectBuffer = std::make_shared<EffectBuffer>(gainMapBufferInfo,
        gainMapBufferInfo->addr_, defaultExtraInfo);

    res = ModifyPictureForInnerPixelMap(gainMap.get(), src, gainMapEffectBuffer, context);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "ModifyPicture: modify gainmap pixelMap fail!");

    return ErrorCode::SUCCESS;
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
        default:
            return ErrorCode::ERR_UNSUPPORTED_DATA_TYPE;
    }
}

ErrorCode FillOutputData(const std::shared_ptr<EffectBuffer> &inputBuffer, std::shared_ptr<EffectBuffer> &outputBuffer,
    const std::shared_ptr<EffectContext> &context)
{
    if (inputBuffer->buffer_ == outputBuffer->buffer_) {
        EFFECT_LOGI("ImageSinkFilter: not need copy!");

        // update output exif info
        if (outputBuffer->extraInfo_->pixelMap != nullptr) {
            CommonUtils::UpdateImageExifDateTime(outputBuffer->extraInfo_->pixelMap);
        } else {
            EFFECT_LOGW("Before UpdateImageExifDateTime: pixelMap is null!");
        }

        // update metadata
        CHECK_AND_RETURN_RET(context->metaInfoNegotiate_->IsNeedUpdate() != true, ErrorCode::SUCCESS);
        return ColorSpaceHelper::UpdateMetadata(outputBuffer.get());
    }

    size_t outputBufferSize = outputBuffer->bufferInfo_->len_;
    size_t inputBufferSize = inputBuffer->bufferInfo_->len_;
    uint32_t outputRowStride = outputBuffer->bufferInfo_->rowStride_;
    uint32_t inputRowStride = inputBuffer->bufferInfo_->rowStride_;
    EFFECT_LOGI("outputBufferSize=%{public}zu, inputBufferSize=%{public}zu, outputRowStride=%{public}d, "
        "inputRowStride=%{public}d", outputBufferSize, inputBufferSize, outputRowStride, inputRowStride);

    // update nativePixelMap
    if (inputBuffer->extraInfo_->dataType == DataType::TEX) {
        context->renderEnvironment_->ConvertTextureToBuffer(inputBuffer->tex, outputBuffer.get());
    } else {
        MemcpyHelper::CopyData(inputBuffer.get(), outputBuffer.get());
    }

    // update output exif info
    CommonUtils::UpdateImageExifDateTime(outputBuffer->extraInfo_->pixelMap);

    // update metadata
    CHECK_AND_RETURN_RET(context->metaInfoNegotiate_->IsNeedUpdate() != true, ErrorCode::SUCCESS);
    return ColorSpaceHelper::UpdateMetadata(outputBuffer.get());
}

ErrorCode FillPictureOutputData(const std::shared_ptr<EffectBuffer> &inputBuffer,
    std::shared_ptr<EffectBuffer> &outputBuffer, const std::shared_ptr<EffectContext> &context)
{
    // update output exif info
    CommonUtils::UpdateImageExifDateTime(outputBuffer->extraInfo_->picture);

    // update nativePixelMap
    if (inputBuffer->extraInfo_->dataType == DataType::TEX) {
        context->renderEnvironment_->ConvertTextureToBuffer(inputBuffer->tex, outputBuffer.get());
        return ErrorCode::SUCCESS;
    }

    if (inputBuffer->buffer_ == outputBuffer->buffer_) {
        EFFECT_LOGI("ImageSinkFilter: not need copy!");
    } else {
        // memcpy primary
        MemcpyHelper::CopyData(inputBuffer.get(), outputBuffer.get());
    }

    auto inputAuxiliary = inputBuffer->auxiliaryBufferInfos;
    auto outputAuxiliary = outputBuffer->auxiliaryBufferInfos;
    if (inputAuxiliary == nullptr || outputAuxiliary == nullptr) {
        return ErrorCode::SUCCESS;
    }

    for (const auto &it : *inputAuxiliary) {
        const auto &outputBufferInfoIt = outputAuxiliary->find(it.first);
        if (outputBufferInfoIt == outputAuxiliary->end()) {
            continue;
        }

        auto srcBufferInfo = it.second;
        std::shared_ptr<ExtraInfo> defaultExtraInfo = std::make_shared<ExtraInfo>();
        std::shared_ptr<EffectBuffer> srcEffectBuffer = std::make_shared<EffectBuffer>(srcBufferInfo,
            srcBufferInfo->addr_, defaultExtraInfo);
        auto dstBufferInfo = outputBufferInfoIt->second;
        std::shared_ptr<EffectBuffer> dstEffectBuffer = std::make_shared<EffectBuffer>(dstBufferInfo,
            dstBufferInfo->addr_, defaultExtraInfo);

        MemcpyHelper::CopyData(srcEffectBuffer.get(), dstEffectBuffer.get());
    }
    return ErrorCode::SUCCESS;
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
    ErrorCode result = ModifyDataInfo(src, buffer, context);
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

ErrorCode ImageSinkFilter::SavaOutputData(EffectBuffer *src, const std::shared_ptr<EffectBuffer> &inputBuffer,
    std::shared_ptr<EffectBuffer> &outputBuffer, std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGD("SavaOutputData: dataType=%{public}d", outputBuffer->extraInfo_->dataType);
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
            return FillPictureOutputData(inputBuffer, outputBuffer, context);
        default:
            return ErrorCode::ERR_UNSUPPORTED_DATA_TYPE;
    }
}

ErrorCode ImageSinkFilter::SaveData(const std::shared_ptr<EffectBuffer> &inputBuffer,
    std::shared_ptr<EffectBuffer> &outputBuffer, std::shared_ptr<EffectContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(inputBuffer != nullptr && inputBuffer->bufferInfo_ != nullptr &&
        (inputBuffer->buffer_ != nullptr || inputBuffer->tex != nullptr) && inputBuffer->extraInfo_ != nullptr,
        ErrorCode::ERR_INPUT_NULL, "inputBuffer para error!");
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
            (outputBuffer->buffer_ != nullptr || outputBuffer->tex != nullptr),
            ErrorCode::ERR_INPUT_NULL, "outputBuffer buffer info or buffer addr error!");
    }

    return SavaOutputData(src, inputBuffer, outputBuffer, context);
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

ErrorCode ImageSinkFilter::PushData(const std::string &inPort, const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGI("image sink effect push data started, state: %{public}d", state_.load());
    EffectBuffer *output = nullptr;
    if (sinkBuffer_ != nullptr) {
        output = sinkBuffer_.get();
    } else {
        output = context->renderStrategy_->GetInput();
    }

    if (buffer->extraInfo_->dataType == DataType::TEX) {
        if (output->extraInfo_->dataType == DataType::NATIVE_WINDOW) {
            EffectBuffer *input = context->renderStrategy_->GetInput();
            GraphicTransformType transformType = GRAPHIC_ROTATE_NONE;
            if (input->extraInfo_->surfaceBuffer != nullptr) {
                transformType = input->extraInfo_->surfaceBuffer->GetSurfaceBufferTransform();
            }
            RenderTexturePtr renderTexture = buffer->tex;
            context->renderEnvironment_->SetNativeWindowColorSpace(buffer->bufferInfo_->colorSpace_);
            RenderTexturePtr tempTex = context->renderEnvironment_->RequestBuffer(renderTexture->Width(),
                renderTexture->Height());
            context->renderEnvironment_->DrawFlipTex(renderTexture, tempTex);
            buffer->tex = tempTex;
            context->renderEnvironment_->DrawFrameWithTransform(const_cast<std::shared_ptr<EffectBuffer> &>(buffer),
                transformType);
            return ErrorCode::SUCCESS;
        }
    }

    if (output->extraInfo_->dataType == DataType::NATIVE_WINDOW && buffer->extraInfo_->surfaceBuffer != nullptr) {
        EGLImageKHR img = GLUtils::CreateEGLImage(eglGetDisplay(EGL_DEFAULT_DISPLAY),
            buffer->extraInfo_->surfaceBuffer);
        int tex = static_cast<int>(GLUtils::CreateTextureFromImage(img));
        buffer->extraInfo_->surfaceBuffer->FlushCache();
        context->renderEnvironment_->UpdateCanvas();
        GraphicTransformType transformType = buffer->extraInfo_->surfaceBuffer->GetSurfaceBufferTransform();
        context->renderEnvironment_->DrawFrame(tex, transformType);
        GLUtils::DestroyImage(img);
        return ErrorCode::SUCCESS;
    }
    ErrorCode result = SaveData(buffer, sinkBuffer_, context);
    CHECK_AND_RETURN_RET_LOG(result == ErrorCode::SUCCESS, result, "SaveData fail! result=%{public}d", result);
    eventReceiver_->OnEvent(Event{ name_, EventType::EVENT_COMPLETE, { buffer } });
    return ErrorCode::SUCCESS;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS
