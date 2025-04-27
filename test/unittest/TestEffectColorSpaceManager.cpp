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

#include "gtest/gtest.h"
#include "effect_log.h"
#include "image_source.h"
#include "pixel_map.h"
#include "colorspace_processor.h"
#include "colorspace_helper.h"
#include "colorspace_manager.h"
#include "colorspace_strategy.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Effect {
namespace Test {

static const std::string TAG = "ColorSpaceTest";
constexpr char TEST_IMAGE_PATH[] = "/data/test/resource/image_effect_1k_test1.jpg";

static std::unique_ptr<Picture> CreatePictureByPath(std::string imagePath)
{
    SourceOptions opts;
    uint32_t errorCode = 0;
    EFFECT_LOGW("%{public}s create pixelmap by path %{public}s", TAG.c_str(), imagePath.c_str());
    std::unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(imagePath, opts, errorCode);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, nullptr,
        "CreateImageSource fail! path=%{public}s, errorCode=%{public}d", imagePath.c_str(), errorCode);

    DecodingOptionsForPicture decodingOptions;
    decodingOptions.desireAuxiliaryPictures.insert(AuxiliaryPictureType::GAINMAP);
    std::unique_ptr<Picture> picture = imageSource->CreatePicture(decodingOptions, errorCode);
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, nullptr,
        "CreatePicture fail! path=%{public}s, errorCode=%{public}d", imagePath.c_str(), errorCode);
    
    return picture;
}

static std::shared_ptr<EffectBuffer> CreateEffectBufferByPicture(Picture* picture)
{
    if (picture == nullptr) {
        return nullptr;
    }

    std::shared_ptr<PixelMap> pixelMap = picture->GetMainPixel();
    if (pixelMap == nullptr) {
        return nullptr;
    }

    std::shared_ptr<BufferInfo> info = std::make_shared<BufferInfo>();
    info->width_ = pixelMap->GetWidth();
    info->height_ = pixelMap->GetHeight();
    info->rowStride_ = pixelMap->GetRowStride();
    info->len_ = info->rowStride_ * info->height_;
    info->formatType_ = IEffectFormat::RGBA8888;
    info->surfaceBuffer_ = nullptr;
    uint8_t *pixels = const_cast<uint8_t *>(pixelMap->GetPixels());
    void *addr = static_cast<void *>(pixels);
    info->pixelMap_ = pixelMap.get();

    std::shared_ptr<ExtraInfo> extraInfo = std::make_shared<ExtraInfo>();
    extraInfo->dataType = DataType::PIXEL_MAP;
    extraInfo->bufferType = BufferType::HEAP_MEMORY;
    extraInfo->picture = picture;

    std::shared_ptr<EffectBuffer> effectBuf = std::make_shared<EffectBuffer>(info, addr, extraInfo);
    return effectBuf != nullptr && effectBuf->buffer_ != nullptr ? effectBuf : nullptr;
}

static std::unique_ptr<Picture> g_picture = nullptr;

class TestEffectColorSpaceManager : public testing::Test {
public:
    TestEffectColorSpaceManager() = default;

    ~TestEffectColorSpaceManager() override = default;

    static void SetUpTestCase()
    {
        g_picture = CreatePictureByPath(TEST_IMAGE_PATH);
        EXPECT_NE(g_picture, nullptr);
    }

    static void TearDownTestCase()
    {
        g_picture = nullptr;
    }

    void SetUp() override
    {
        effectContext_->memoryManager_ = std::make_shared<EffectMemoryManager>();
        effectContext_->colorSpaceManager_ = std::make_shared<ColorSpaceManager>();
    }

    void TearDown() override
    {
        effectContext_ = nullptr;
    }

    std::shared_ptr<EffectContext> effectContext_ = std::make_shared<EffectContext>();
};

HWTEST_F(TestEffectColorSpaceManager, ColorSpaceConverter_ApplyColorSpace001, TestSize.Level1)
{
    EFFECT_LOGW("%{public}s ColorSpaceConverter_ApplyColorSpace001 enter", TAG.c_str());
    std::shared_ptr<EffectBuffer> inputEffectBuffer = CreateEffectBufferByPicture(g_picture.get());
    EXPECT_NE(inputEffectBuffer, nullptr);

    EffectColorSpace targetColorSpace = EffectColorSpace::SRGB;
    ErrorCode result = ColorSpaceProcessor::ApplyColorSpace(inputEffectBuffer.get(), targetColorSpace);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    inputEffectBuffer->extraInfo_->dataType = DataType::URI;
    result = ColorSpaceProcessor::ApplyColorSpace(inputEffectBuffer.get(), targetColorSpace);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    inputEffectBuffer->extraInfo_->dataType = DataType::UNKNOWN;
    result = ColorSpaceProcessor::ApplyColorSpace(inputEffectBuffer.get(), targetColorSpace);
    EXPECT_NE(result, ErrorCode::SUCCESS);
    EFFECT_LOGW("%{public}s ColorSpaceConverter_ApplyColorSpace001 end", TAG.c_str());
}

HWTEST_F(TestEffectColorSpaceManager, ColorSpaceConverter_GetMemoryData001, TestSize.Level1)
{
    EFFECT_LOGW("%{public}s ColorSpaceConverter_GetMemoryData001 enter", TAG.c_str());
    SurfaceBuffer *sb = OHOS::SurfaceBuffer::Create();
    std::shared_ptr<ColorSpaceProcessor> colorSpaceConverter = std::make_shared<ColorSpaceProcessor>();
    auto result = colorSpaceConverter->GetMemoryData(sb);
    EXPECT_EQ(result, nullptr);

    std::shared_ptr<MemoryData> memoryData = std::make_shared<MemoryData>();
    memoryData->memoryInfo.extra = static_cast<void *>(sb);

    colorSpaceConverter->memoryDataArray_.emplace_back(memoryData);
    result = colorSpaceConverter->GetMemoryData(sb);
    EXPECT_NE(result, nullptr);
    EFFECT_LOGW("%{public}s ColorSpaceConverter_GetMemoryData001 end", TAG.c_str());
}

HWTEST_F(TestEffectColorSpaceManager, ColorSpaceHelper_ConvertToColorSpaceName001, TestSize.Level1)
{
    EFFECT_LOGW("%{public}s ColorSpaceHelper_ConvertToColorSpaceName001 enter", TAG.c_str());
    EffectColorSpace colorSpace = EffectColorSpace::DEFAULT;
    OHOS::ColorManager::ColorSpaceName result = ColorSpaceHelper::ConvertToColorSpaceName(colorSpace);
    EXPECT_EQ(result, OHOS::ColorManager::ColorSpaceName::NONE);
    EFFECT_LOGW("%{public}s ColorSpaceHelper_ConvertToColorSpaceName001 end", TAG.c_str());
}

HWTEST_F(TestEffectColorSpaceManager, ColorSpaceHelper_ConvertToCMColorSpace001, TestSize.Level1)
{
    EFFECT_LOGW("%{public}s ColorSpaceHelper_ConvertToCMColorSpace001 enter", TAG.c_str());
    EffectColorSpace colorSpace = EffectColorSpace::DEFAULT;
    OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType type =
        ColorSpaceHelper::ConvertToCMColorSpace(colorSpace);
    EXPECT_EQ(type, OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType::CM_COLORSPACE_NONE);

    colorSpace = EffectColorSpace::SRGB;
    type = ColorSpaceHelper::ConvertToCMColorSpace(colorSpace);
    EXPECT_NE(type, OHOS::HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType::CM_COLORSPACE_NONE);
    EFFECT_LOGW("%{public}s ColorSpaceHelper_ConvertToCMColorSpace001 end", TAG.c_str());
}

HWTEST_F(TestEffectColorSpaceManager, ColorSpaceHelper_ConvertColorSpace001, TestSize.Level1)
{
    EFFECT_LOGW("%{public}s ColorSpaceHelper_ConvertColorSpace001 enter", TAG.c_str());
    std::shared_ptr<EffectBuffer> inputEffectBuffer = CreateEffectBufferByPicture(g_picture.get());
    EXPECT_NE(inputEffectBuffer, nullptr);

    inputEffectBuffer->bufferInfo_->colorSpace_ = EffectColorSpace::ADOBE_RGB;
    effectContext_->colorSpaceManager_->strategy_ = std::make_shared<ColorSpaceStrategy>();
    effectContext_->colorSpaceManager_->strategy_->src_ = CreateEffectBufferByPicture(g_picture.get());

    ErrorCode result = ColorSpaceHelper::ConvertColorSpace(inputEffectBuffer, effectContext_);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
    EFFECT_LOGW("%{public}s ColorSpaceHelper_ConvertColorSpace001 end", TAG.c_str());
}

HWTEST_F(TestEffectColorSpaceManager, ColorSpaceHelper_UpdateMetadata001, TestSize.Level1)
{
    EFFECT_LOGW("%{public}s ColorSpaceHelper_UpdateMetadata001 enter", TAG.c_str());
    std::shared_ptr<EffectBuffer> inputEffectBuffer = CreateEffectBufferByPicture(g_picture.get());
    EXPECT_NE(inputEffectBuffer, nullptr);

    inputEffectBuffer->bufferInfo_->surfaceBuffer_ = nullptr;
    std::shared_ptr<EffectContext> context = std::make_unique<EffectContext>();
    ErrorCode result = ColorSpaceHelper::UpdateMetadata(inputEffectBuffer.get(), context);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    inputEffectBuffer->bufferInfo_->surfaceBuffer_ = OHOS::SurfaceBuffer::Create();
    inputEffectBuffer->bufferInfo_->colorSpace_ = EffectColorSpace::DEFAULT;
    result = ColorSpaceHelper::UpdateMetadata(inputEffectBuffer.get(), context);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    inputEffectBuffer->bufferInfo_->colorSpace_ = EffectColorSpace::SRGB;
    result = ColorSpaceHelper::UpdateMetadata(inputEffectBuffer.get(), context);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
    EFFECT_LOGW("%{public}s ColorSpaceHelper_UpdateMetadata001 end", TAG.c_str());
}

HWTEST_F(TestEffectColorSpaceManager, ColorSpaceManager_ApplyColorSpace001, TestSize.Level1)
{
    EFFECT_LOGW("%{public}s ColorSpaceManager_ApplyColorSpace001 enter", TAG.c_str());
    std::shared_ptr<EffectBuffer> inputEffectBuffer = CreateEffectBufferByPicture(g_picture.get());
    EXPECT_NE(inputEffectBuffer, nullptr);

    EffectColorSpace colorSpace = EffectColorSpace::DEFAULT;
    EffectColorSpace outputColorSpace = EffectColorSpace::DEFAULT;
    std::shared_ptr<ColorSpaceManager> colorSpaceManager = std::make_shared<ColorSpaceManager>();
    ErrorCode result = colorSpaceManager->ApplyColorSpace(inputEffectBuffer.get(), colorSpace, outputColorSpace);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    colorSpace = EffectColorSpace::ADOBE_RGB;
    result = colorSpaceManager->ApplyColorSpace(inputEffectBuffer.get(), colorSpace, outputColorSpace);
    EXPECT_NE(result, ErrorCode::SUCCESS);
    EFFECT_LOGW("%{public}s ColorSpaceManager_ApplyColorSpace001 end", TAG.c_str());
}

HWTEST_F(TestEffectColorSpaceManager, ColorSpaceStrategy_IsSupportedColorSpace001, TestSize.Level1)
{
    EFFECT_LOGW("%{public}s ColorSpaceStrategy_IsSupportedColorSpace001 enter", TAG.c_str());
    EffectColorSpace colorSpace = EffectColorSpace::SRGB;
    bool result = ColorSpaceStrategy::IsSupportedColorSpace(colorSpace);
    EXPECT_EQ(result, true);

    colorSpace = EffectColorSpace::DEFAULT;
    result = ColorSpaceStrategy::IsSupportedColorSpace(colorSpace);
    EXPECT_EQ(result, false);
    EFFECT_LOGW("%{public}s ColorSpaceStrategy_IsSupportedColorSpace001 end", TAG.c_str());
}

HWTEST_F(TestEffectColorSpaceManager, ColorSpaceStrategy_ChooseColorSpace001, TestSize.Level1)
{
    EFFECT_LOGW("%{public}s ColorSpaceStrategy_ChooseColorSpace001 enter", TAG.c_str());
    std::unordered_set<EffectColorSpace> filtersSupportedColorSpace = { EffectColorSpace::SRGB };
    EffectColorSpace srcRealColorSpace = EffectColorSpace::SRGB;
    EffectColorSpace outputColorSpace = EffectColorSpace::DEFAULT;

    std::shared_ptr<ColorSpaceStrategy> colorSpaceStrategy = std::make_shared<ColorSpaceStrategy>();
    colorSpaceStrategy->src_ = CreateEffectBufferByPicture(g_picture.get());
    colorSpaceStrategy->dst_ = nullptr;
    ErrorCode result = colorSpaceStrategy->ChooseColorSpace(filtersSupportedColorSpace,
        srcRealColorSpace, outputColorSpace);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    colorSpaceStrategy->dst_ = CreateEffectBufferByPicture(g_picture.get());
    result = colorSpaceStrategy->ChooseColorSpace(filtersSupportedColorSpace, srcRealColorSpace, outputColorSpace);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    colorSpaceStrategy->dst_->buffer_ = nullptr;
    result = colorSpaceStrategy->ChooseColorSpace(filtersSupportedColorSpace, srcRealColorSpace, outputColorSpace);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    srcRealColorSpace = EffectColorSpace::ADOBE_RGB;
    result = colorSpaceStrategy->ChooseColorSpace(filtersSupportedColorSpace, srcRealColorSpace, outputColorSpace);
    EXPECT_NE(result, ErrorCode::SUCCESS);
    EFFECT_LOGW("%{public}s ColorSpaceStrategy_ChooseColorSpace001 end", TAG.c_str());
}

HWTEST_F(TestEffectColorSpaceManager, ColorSpaceStrategy_CheckConverterColorSpace001, TestSize.Level1)
{
    EFFECT_LOGW("%{public}s ColorSpaceStrategy_CheckConverterColorSpace001 enter", TAG.c_str());
    EffectColorSpace targetColorSpace = EffectColorSpace::SRGB;
    std::shared_ptr<ColorSpaceStrategy> colorSpaceStrategy = std::make_shared<ColorSpaceStrategy>();
    colorSpaceStrategy->src_ = CreateEffectBufferByPicture(g_picture.get());
    colorSpaceStrategy->dst_ = nullptr;
    ErrorCode result = colorSpaceStrategy->CheckConverterColorSpace(targetColorSpace);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    colorSpaceStrategy->dst_ = CreateEffectBufferByPicture(g_picture.get());
    result = colorSpaceStrategy->CheckConverterColorSpace(targetColorSpace);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    colorSpaceStrategy->dst_->buffer_ = nullptr;
    result = colorSpaceStrategy->CheckConverterColorSpace(targetColorSpace);
    EXPECT_NE(result, ErrorCode::SUCCESS);
    EFFECT_LOGW("%{public}s ColorSpaceStrategy_CheckConverterColorSpace001 end", TAG.c_str());
}
}
}
}
}
