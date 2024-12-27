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

#include "colorspace_strategy.h"

#include <unordered_map>

#include "effect_log.h"
#include "colorspace_helper.h"

namespace OHOS {
namespace Media {
namespace Effect {

static const std::unordered_map<EffectColorSpace, EffectColorSpace> COLORSPACE_CONVERTER_MAP = {
    { EffectColorSpace::SRGB, EffectColorSpace::SRGB },
    { EffectColorSpace::SRGB_LIMIT, EffectColorSpace::SRGB_LIMIT },
    { EffectColorSpace::DISPLAY_P3, EffectColorSpace::DISPLAY_P3 },
    { EffectColorSpace::DISPLAY_P3_LIMIT, EffectColorSpace::DISPLAY_P3_LIMIT },
    { EffectColorSpace::BT2020_HLG, EffectColorSpace::BT2020_HLG },
    { EffectColorSpace::BT2020_HLG_LIMIT, EffectColorSpace::BT2020_HLG_LIMIT },
    { EffectColorSpace::BT2020_PQ, EffectColorSpace::BT2020_PQ },
    { EffectColorSpace::BT2020_PQ_LIMIT, EffectColorSpace::BT2020_PQ_LIMIT },
    { EffectColorSpace::ADOBE_RGB, EffectColorSpace::DISPLAY_P3 },
};

static const std::unordered_map<EffectColorSpace, EffectColorSpace> COLORSPACE_HDR_CONVERTER_MAP = {
    { EffectColorSpace::BT2020_HLG, EffectColorSpace::DISPLAY_P3 },
    { EffectColorSpace::BT2020_HLG_LIMIT, EffectColorSpace::DISPLAY_P3_LIMIT },
    { EffectColorSpace::BT2020_PQ, EffectColorSpace::DISPLAY_P3 },
    { EffectColorSpace::BT2020_PQ_LIMIT, EffectColorSpace::DISPLAY_P3_LIMIT },
};

static const std::vector<EffectColorSpace> DEFAULT_SUPPORTED_COLORSPACE = {
    EffectColorSpace::SRGB,
    EffectColorSpace::SRGB_LIMIT,
    EffectColorSpace::DISPLAY_P3,
    EffectColorSpace::DISPLAY_P3_LIMIT
};

bool ColorSpaceStrategy::IsSupportedColorSpace(EffectColorSpace colorSpace)
{
    for (const auto &it : COLORSPACE_CONVERTER_MAP) {
        if (it.first == colorSpace) {
            return true;
        }
    }
    return false;
}

bool ColorSpaceStrategy::IsNeedConvertColorSpace(EffectColorSpace colorSpace)
{
    EffectColorSpace converterColorSpace = EffectColorSpace::DEFAULT;
    for (const auto &it : COLORSPACE_CONVERTER_MAP) {
        if (it.first == colorSpace) {
            converterColorSpace = it.second;
            break;
        }
    }
    return colorSpace != converterColorSpace;
}

EffectColorSpace ColorSpaceStrategy::GetTargetColorSpace(EffectColorSpace src)
{
    auto it = COLORSPACE_CONVERTER_MAP.find(src);
    return it == COLORSPACE_CONVERTER_MAP.end() ? EffectColorSpace::DEFAULT : it->second;
}

std::unordered_set<EffectColorSpace> ColorSpaceStrategy::GetAllSupportedColorSpaces()
{
    std::unordered_set<EffectColorSpace> supportedColorSpace;
    for (const auto &it : COLORSPACE_CONVERTER_MAP) {
        supportedColorSpace.emplace(it.second);
    }

    return supportedColorSpace;
}

void ColorSpaceStrategy::Init(const std::shared_ptr<EffectBuffer> &src, const std::shared_ptr<EffectBuffer> &dst)
{
    src_ = src;
    dst_ = dst;
}

ErrorCode ChooseColorSpaceInner(const EffectColorSpace &srcRealColorSpace,
    const std::unordered_set<EffectColorSpace> &filtersSupportedColorSpace, EffectColorSpace &outputColorSpace)
{
    if (filtersSupportedColorSpace.find(srcRealColorSpace) != filtersSupportedColorSpace.end()) {
        outputColorSpace = srcRealColorSpace;
        return ErrorCode::SUCCESS;
    }

    if (std::find(DEFAULT_SUPPORTED_COLORSPACE.begin(), DEFAULT_SUPPORTED_COLORSPACE.end(), srcRealColorSpace) !=
        DEFAULT_SUPPORTED_COLORSPACE.end()) {
        outputColorSpace = srcRealColorSpace;
        return ErrorCode::SUCCESS;
    }

    auto it = COLORSPACE_HDR_CONVERTER_MAP.find(srcRealColorSpace);
    if (it == COLORSPACE_HDR_CONVERTER_MAP.end()) {
        EFFECT_LOGE("ChooseColorSpaceInner: colorSpace[%{public}d] not support convert.", srcRealColorSpace);
        return ErrorCode::ERR_COLORSPACE_NOT_SUPPORT_CONVERT;
    }

    outputColorSpace = it->second;
    return ErrorCode::SUCCESS;
}

ErrorCode ChooseColorSpaceWithOutput(const EffectColorSpace &srcRealColorSpace, EffectBuffer *src, EffectBuffer *dst,
    const std::unordered_set<EffectColorSpace> &filtersSupportedColorSpace, EffectColorSpace &outputColorSpace)
{
    const std::shared_ptr<BufferInfo> &dstBufferInfo = dst->bufferInfo_;
    CHECK_AND_RETURN_RET_LOG(dstBufferInfo != nullptr, ErrorCode::ERR_INPUT_NULL,
        "ChooseColorSpaceWithOutput: dstBufferInfo is null!");

    EffectColorSpace chooseColorSpace = EffectColorSpace::DEFAULT;
    ErrorCode res = ChooseColorSpaceInner(srcRealColorSpace, filtersSupportedColorSpace, chooseColorSpace);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "ChooseColorSpaceWithOutput: ChooseColorSpaceInner fail!");

    if (src->extraInfo_->dataType == DataType::PIXEL_MAP && dst->extraInfo_->dataType == DataType::PIXEL_MAP) {
        // color space is same.
        bool isSrcHdr = ColorSpaceHelper::IsHdrColorSpace(srcRealColorSpace);
        bool isChoseHdr = ColorSpaceHelper::IsHdrColorSpace(chooseColorSpace);
        CHECK_AND_RETURN_RET_LOG(isSrcHdr == isChoseHdr, ErrorCode::ERR_NOT_SUPPORT_INPUT_OUTPUT_COLORSPACE,
            "ChooseColorSpaceWithOutput: input and output color space not same! "
            "srcRealColorSpace=%{public}d, chooseColorSpace=%{public}d", srcRealColorSpace, chooseColorSpace);
    }

    outputColorSpace = chooseColorSpace;
    return ErrorCode::SUCCESS;
}

ErrorCode ChooseColorSpaceWithoutOutput(const EffectColorSpace &srcRealColorSpace,
    const std::unordered_set<EffectColorSpace> &filtersSupportedColorSpace, EffectColorSpace &outputColorSpace)
{
    return ChooseColorSpaceInner(srcRealColorSpace, filtersSupportedColorSpace, outputColorSpace);
}

ErrorCode ColorSpaceStrategy::ChooseColorSpace(const std::unordered_set<EffectColorSpace> &filtersSupportedColorSpace,
    const EffectColorSpace &srcRealColorSpace, EffectColorSpace &outputColorSpace)
{
    CHECK_AND_RETURN_RET_LOG(src_ != nullptr, ErrorCode::ERR_INPUT_NULL, "ChooseColorSpace: src_ is null!");

    if (dst_ == nullptr || dst_->buffer_ == src_->buffer_) {
        return ChooseColorSpaceWithoutOutput(srcRealColorSpace, filtersSupportedColorSpace, outputColorSpace);
    }
    return ChooseColorSpaceWithOutput(srcRealColorSpace, src_.get(), dst_.get(), filtersSupportedColorSpace,
        outputColorSpace);
}

ErrorCode ColorSpaceStrategy::CheckConverterColorSpace(const EffectColorSpace &targetColorSpace)
{
    CHECK_AND_RETURN_RET_LOG(src_ != nullptr, ErrorCode::ERR_PARAM_INVALID,
        "GetConverterEffectBuffer: src_ is null!");

    // only input data
    if (dst_ == nullptr || dst_->buffer_ == src_->buffer_) {
        return ErrorCode::SUCCESS;
    }

    DataType dataType = dst_->extraInfo_->dataType;
    if (dataType == DataType::NATIVE_WINDOW) {
        return ErrorCode::SUCCESS;
    }

    // check color space is match or not.
    const std::shared_ptr<BufferInfo> &srcBufferInfo = src_->bufferInfo_;
    const std::shared_ptr<BufferInfo> &dstBufferInfo = dst_->bufferInfo_;
    CHECK_AND_RETURN_RET_LOG(srcBufferInfo != nullptr && dstBufferInfo != nullptr, ErrorCode::ERR_PARAM_INVALID,
        "CheckConverterColorSpace: srcBufferInfo or dstBufferInfo is null!");
    EffectColorSpace srcColorSpace = srcBufferInfo->colorSpace_;
    EffectColorSpace dstColorSpace = dstBufferInfo->colorSpace_;
    CHECK_AND_RETURN_RET_LOG(targetColorSpace == dstColorSpace, ErrorCode::ERR_NOT_SUPPORT_INPUT_OUTPUT_COLORSPACE,
        "CheckConverterColorSpace: colorspace not match! target=%{public}d, src=%{public}d, dst=%{public}d",
        targetColorSpace, srcColorSpace, dstColorSpace);

    return ErrorCode::SUCCESS;
}

void ColorSpaceStrategy::Deinit()
{
    src_ = nullptr;
    dst_ = nullptr;
}

} // namespace Effect
} // namespace Media
} // namespace OHOS