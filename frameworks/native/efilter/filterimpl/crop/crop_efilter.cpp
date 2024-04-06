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

#include "crop_efilter.h"

#include "common_utils.h"
#include "efilter_factory.h"

namespace OHOS {
namespace Media {
namespace Effect {
REGISTER_EFILTER_FACTORY(CropEFilter, "Crop");
const std::string CropEFilter::Parameter::KEY_REGION = "FilterRegion";
std::shared_ptr<EffectInfo> CropEFilter::info_ = nullptr;
namespace {
    constexpr int32_t PIXEL_BYTES = 4;
}

struct AreaInfo {
    int32_t x0;
    int32_t y0;
    int32_t x1;
    int32_t y1;
};

struct Region {
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
};

void CalculateCropRegion(int32_t srcWidth, int32_t srcHeight, std::map<std::string, Plugin::Any> &values,
    Region *region)
{
    AreaInfo areaInfo = { 0, 0, srcWidth, srcHeight };
    void *area = nullptr;
    ErrorCode res = CommonUtils::GetValue(CropEFilter::Parameter::KEY_REGION, values, area);
    if (res != ErrorCode::SUCCESS || area == nullptr) {
        // allow developer not set para, not execute crop. execute copy.
        EFFECT_LOGW("CropEFilter::CalculateCropRegion get value fail! res=%{public}d. "
            "use default value, not execute crop!", res);
    } else {
        areaInfo = *(static_cast<AreaInfo *>(area));
    }

    EFFECT_LOGI("CropEFilter x0=%{public}d, y0=%{public}d, x1=%{public}d, y1=%{public}d",
        areaInfo.x0, areaInfo.y0, areaInfo.x1, areaInfo.y1);

    int32_t leftTopX = areaInfo.x0 > areaInfo.x1 ? areaInfo.x1 : areaInfo.x0;
    int32_t leftTopY = areaInfo.y0 > areaInfo.y1 ? areaInfo.y1 : areaInfo.y0;
    leftTopX = leftTopX < 0 ? 0 : leftTopX;
    leftTopY = leftTopY < 0 ? 0 : leftTopY;

    int32_t rightBottomX = areaInfo.x0 > areaInfo.x1 ? areaInfo.x0 : areaInfo.x1;
    int32_t rightBottomY = areaInfo.y0 > areaInfo.y1 ? areaInfo.y0 : areaInfo.y1;
    rightBottomX = rightBottomX > srcWidth ? srcWidth : rightBottomX;
    rightBottomY = rightBottomY > srcHeight ? srcHeight : rightBottomY;

    int32_t cropWidth = rightBottomX - leftTopX;
    int32_t cropHeight = rightBottomY - leftTopY;

    region->left = leftTopX;
    region->top = leftTopY;
    region->width = cropWidth;
    region->height = cropHeight;
}

void Crop(EffectBuffer *src, EffectBuffer *dst, Region *region)
{
    int32_t cropLeft = region->left;
    int32_t cropTop = region->top;
    int32_t cropWidth = region->width;
    int32_t cropHeight = region->height;
    int32_t dstWidth = static_cast<int32_t>(dst->bufferInfo_->width_);
    int32_t dstHeight = static_cast<int32_t>(dst->bufferInfo_->height_);

    int32_t rowCount = cropHeight > dstHeight ? dstHeight : cropHeight;
    int32_t pixelCount = cropWidth > dstWidth ? dstWidth : cropWidth;
    int32_t count = pixelCount * PIXEL_BYTES;

    auto *srcBuffer = static_cast<char *>(src->buffer_);
    auto *dstBuffer = static_cast<char *>(dst->buffer_);
    uint32_t srcRowStride = src->bufferInfo_->rowStride_;
    uint32_t dstRowStride = dst->bufferInfo_->rowStride_;
    char *srcStart = srcBuffer + cropTop * static_cast<int32_t>(srcRowStride) + cropLeft * PIXEL_BYTES;
    for (int32_t i = 0; i < rowCount; ++i) {
        errno_t ret = memcpy_s(dstBuffer + i * dstRowStride, dstRowStride, srcStart + i * srcRowStride, count);
        if (ret != 0) {
            EFFECT_LOGE("CropEFilter::Render memcpy_s failed. ret=%{public}d", ret);
            continue;
        }
    }
}

ErrorCode CropEFilter::Render(EffectBuffer *src, EffectBuffer *dst, std::shared_ptr<EffectContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(src != nullptr && dst != nullptr, ErrorCode::ERR_INPUT_NULL,
        "input error! src=%{public}p, dst=%{public}p", src, dst);
    CHECK_AND_RETURN_RET_LOG(src->bufferInfo_ != nullptr && dst->bufferInfo_ != nullptr, ErrorCode::ERR_INPUT_NULL,
        "input error! src->bufferInfo_=%{public}d, dst->bufferInfo_=%{public}d",
        src->bufferInfo_ == nullptr, dst->bufferInfo_ == nullptr);

    // only support RGBA8888 for now.
    CHECK_AND_RETURN_RET_LOG(src->bufferInfo_->formatType_ == IEffectFormat::RGBA8888,
        ErrorCode::ERR_UNSUPPORTED_FORMAT_TYPE,
        "crop not support format! format=%{public}d", src->bufferInfo_->formatType_);

    Region region = { 0, 0, 0, 0 };
    CalculateCropRegion(static_cast<int32_t>(src->bufferInfo_->width_), static_cast<int32_t>(src->bufferInfo_->height_),
        values_, &region);
    Crop(src, dst, &region);
    return ErrorCode::SUCCESS;
}

ErrorCode CropEFilter::CropToOutputBuffer(EffectBuffer *src, std::shared_ptr<EffectContext> &context,
    std::shared_ptr<EffectBuffer> &output)
{
    CHECK_AND_RETURN_RET_LOG(src != nullptr, ErrorCode::ERR_INPUT_NULL, "input src is null!");
    CHECK_AND_RETURN_RET_LOG(src->bufferInfo_ != nullptr, ErrorCode::ERR_INPUT_NULL,
        "input error! src->bufferInfo_=%{public}d", src->bufferInfo_ == nullptr);

    Region region = { 0, 0, 0, 0 };
    CalculateCropRegion(static_cast<int32_t>(src->bufferInfo_->width_), static_cast<int32_t>(src->bufferInfo_->height_),
        values_, &region);
    int32_t cropLeft = region.left;
    int32_t cropTop = region.top;
    int32_t cropWidth = region.width;
    int32_t cropHeight = region.height;

    EFFECT_LOGI("CropEFilter cropLeft=%{public}d, cropTop=%{public}d, cropWidth=%{public}d, cropHeight=%{public}d",
        cropLeft, cropTop, cropWidth, cropHeight);

    MemoryInfo allocMemInfo = {
        .bufferInfo = {
            .width_ = static_cast<uint32_t>(cropWidth),
            .height_ = static_cast<uint32_t>(cropHeight),
            .len_ = static_cast<uint32_t>(cropWidth * cropHeight * PIXEL_BYTES),
            .formatType_ = src->bufferInfo_->formatType_,
        },
        .extra = src->extraInfo_->surfaceBuffer,
    };
    MemoryData *memData = context->memoryManager_->AllocMemory(src->buffer_, allocMemInfo);
    CHECK_AND_RETURN_RET_LOG(memData != nullptr, ErrorCode::ERR_ALLOC_MEMORY_FAIL, "alloc memory fail!");
    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    *bufferInfo = memData->memoryInfo.bufferInfo;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_unique<ExtraInfo>();
    *extraInfo = *src->extraInfo_;
    extraInfo->bufferType = memData->memoryInfo.bufferType;
    extraInfo->surfaceBuffer = (memData->memoryInfo.bufferType == BufferType::DMA_BUFFER) ?
        static_cast<OHOS::SurfaceBuffer *>(memData->memoryInfo.extra) : nullptr;
    output = std::make_shared<EffectBuffer>(bufferInfo, memData->data, extraInfo);
    return Render(src, output.get(), context);
}

ErrorCode CropEFilter::Render(EffectBuffer *buffer, std::shared_ptr<EffectContext> &context)
{
    DataType dataType = buffer->extraInfo_->dataType;
    CHECK_AND_RETURN_RET_LOG(dataType == DataType::PIXEL_MAP || dataType == DataType::URI || dataType == DataType::PATH,
        ErrorCode::ERR_UNSUPPORTED_DATA_TYPE, "crop only support pixelMap uri path! dataType=%{public}d", dataType);

    // only support RGBA8888 for now.
    CHECK_AND_RETURN_RET_LOG(buffer->bufferInfo_->formatType_ == IEffectFormat::RGBA8888,
        ErrorCode::ERR_UNSUPPORTED_FORMAT_TYPE,
        "crop not support format! format=%{public}d", buffer->bufferInfo_->formatType_);

    std::shared_ptr<EffectBuffer> output;
    ErrorCode res = CropToOutputBuffer(buffer, context, output);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "filter(%{public}s) render fail", name_.c_str());

    return PushData(output.get(), context);
}

std::shared_ptr<MemNegotiatedCap> CropEFilter::Negotiate(const std::shared_ptr<MemNegotiatedCap> &input)
{
    Region region = { 0, 0, 0, 0 };
    CalculateCropRegion(static_cast<int32_t>(input->width), static_cast<int32_t>(input->height), values_, &region);

    std::shared_ptr<MemNegotiatedCap> current = std::make_shared<MemNegotiatedCap>();
    current->width = static_cast<uint32_t>(region.width);
    current->height = static_cast<uint32_t>(region.height);
    current->format = input->format;
    return current;
}

std::shared_ptr<EffectInfo> CropEFilter::GetEffectInfo(const std::string &name)
{
    if (info_ != nullptr) {
        return info_;
    }
    info_ = std::make_unique<EffectInfo>();
    info_->formats_.emplace(IEffectFormat::RGBA8888, std::vector<IPType>{ IPType::CPU });
    info_->category_ = Category::SHAPE_ADJUST;
    return info_;
}

ErrorCode CropEFilter::Restore(const nlohmann::json &values)
{
    return ErrorCode::SUCCESS;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS