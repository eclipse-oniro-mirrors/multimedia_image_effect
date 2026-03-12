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
#include "error_code.h"
#include "test_pixel_map_utils.h"
#include "test_picture_utils.h"
#include "image_sink_filter.h"
#include "common_utils.h"
#include "effect_memory_manager.h"
#include "efilter_metainfo_negotiate.h"
#include "effect_buffer.h"
#include "render_context.h"
#include "render_texture.h"
#include "render_environment.h"

using namespace testing::ext;
namespace {
    const std::string FILTER_NAME = "TestImageSinkFilter";
    const std::string TEST_INCLUDE_AUX_PATH = "/data/test/resource/camera_efilter_test.jpg";
    const std::string TEST_IMAGE_PATH = "/data/test/resource/image_effect_1k_test1.jpg";
}

namespace OHOS {
namespace Media {
namespace Effect {
namespace Test {

class TestImageSinkFilter : public testing::Test {
public:
    TestImageSinkFilter() = default;

    ~TestImageSinkFilter() override = default;
    static void SetUpTestCase() {}

    static void TearDownTestCase() {}

    void SetUp() override
    {
        const ::testing::TestInfo* test_info = 
            ::testing::UnitTest::GetInstance()->current_test_info();
        EFFECT_LOGI("Running test: %{public}s start", test_info->name());
        imageSinkFilter_ = std::make_shared<ImageSinkFilter>(FILTER_NAME);
        InitEffectContext();
    }

    void TearDown() override
    {
        const ::testing::TestInfo* test_info = 
            ::testing::UnitTest::GetInstance()->current_test_info();
        EFFECT_LOGI("Running test: %{public}s end", test_info->name());
        imageSinkFilter_ = nullptr;
        context_ = nullptr;
    }

    void InitEffectContext()
    {
        context_ = std::make_shared<EffectContext>();
        context_->memoryManager_ = std::make_shared<EffectMemoryManager>();
        context_->renderStrategy_ = std::make_shared<RenderStrategy>();
        context_->capNegotiate_ = std::make_shared<CapabilityNegotiate>();
        context_->renderEnvironment_ = std::make_shared<RenderEnvironment>();
        context_->colorSpaceManager_ = std::make_shared<ColorSpaceManager>();
        context_->cacheNegotiate_ = std::make_shared<EFilterCacheNegotiate>();
        context_->metaInfoNegotiate_ = std::make_shared<EfilterMetaInfoNegotiate>();
    }

    std::shared_ptr<ImageSinkFilter> imageSinkFilter_ = nullptr;
    std::shared_ptr<EffectContext> context_ = nullptr;
};

HWTEST_F(TestImageSinkFilter, SetXComponentSurface_001, TestSize.Level1) {
    sptr<Surface> inputSurface = nullptr;
    ErrorCode ret = imageSinkFilter_->SetXComponentSurface(inputSurface);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);
    ASSERT_EQ(nullptr, imageSinkFilter_->toXComponentSurface_);
}

HWTEST_F(TestImageSinkFilter, SetXComponentSurface_002, TestSize.Level1) {
    sptr<Surface> inputSurface = Surface::CreateSurfaceAsConsumer(FILTER_NAME);
    imageSinkFilter_->toXComponentSurface_ = inputSurface;
    ErrorCode ret = imageSinkFilter_->SetXComponentSurface(inputSurface);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);
    ASSERT_EQ(inputSurface, imageSinkFilter_->toXComponentSurface_);
}

HWTEST_F(TestImageSinkFilter, SetXComponentSurface_003, TestSize.Level1) {
    sptr<Surface> inputSurface = Surface::CreateSurfaceAsConsumer(FILTER_NAME);
    ErrorCode ret = imageSinkFilter_->SetXComponentSurface(inputSurface);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);
    ASSERT_EQ(inputSurface, imageSinkFilter_->toXComponentSurface_);
}

HWTEST_F(TestImageSinkFilter, SaveInputData_001, TestSize.Level1) {
    std::unique_ptr<Picture> picture4Src_ = TestPictureUtils::CreatePictureByPath(TEST_INCLUDE_AUX_PATH);
    Picture* picture4Src = picture4Src_.get();
    std::shared_ptr<EffectBuffer> src_ = nullptr;
    ErrorCode ret = CommonUtils::ParsePicture(picture4Src, src_);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);
    EffectBuffer* src = src_.get();
    
    std::unique_ptr<Picture> picture4Buffer_ = TestPictureUtils::CreatePictureByPath(TEST_INCLUDE_AUX_PATH);
    Picture* picture4Buffer = picture4Buffer_.get();
    std::shared_ptr<EffectBuffer> buffer = nullptr;
    ret = CommonUtils::ParsePicture(picture4Buffer, buffer);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);
    
    buffer->extraInfo_ = std::make_shared<ExtraInfo>();
    buffer->extraInfo_->dataType = DataType::PICTURE;

    ret = imageSinkFilter_->SaveInputData(src, buffer, context_);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);
}

HWTEST_F(TestImageSinkFilter, SaveOutputData_001, TestSize.Level1) {
    std::unique_ptr<Picture> picture4Buffer_ = TestPictureUtils::CreatePictureByPath(TEST_INCLUDE_AUX_PATH);
    Picture* picture4Buffer = picture4Buffer_.get();
    std::shared_ptr<EffectBuffer> buffer_ = nullptr;
    ErrorCode ret = CommonUtils::ParsePicture(picture4Buffer, buffer_);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);
    EffectBuffer* buffer = buffer_.get();
    
    std::unique_ptr<Picture> picture4Input_ = TestPictureUtils::CreatePictureByPath(TEST_INCLUDE_AUX_PATH);
    Picture* picture4Input = picture4Input_.get();
    std::shared_ptr<EffectBuffer> input = nullptr;
    ret = CommonUtils::ParsePicture(picture4Input, input);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);

    std::unique_ptr<Picture> picture4Output_ = TestPictureUtils::CreatePictureByPath(TEST_INCLUDE_AUX_PATH);
    Picture* picture4Output = picture4Output_.get();
    std::shared_ptr<EffectBuffer> output = nullptr;
    ret = CommonUtils::ParsePicture(picture4Output, output);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);

    ret = imageSinkFilter_->SaveOutputData(buffer, input, output, context_);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);
}


HWTEST_F(TestImageSinkFilter, GetOrCreateSurfaceBuffer_001, TestSize.Level1) {
    BufferRequestConfig config;
    auto ret = imageSinkFilter_->GetOrCreateSurfaceBuffer(config);
    ASSERT_NE(ret, nullptr);
}

HWTEST_F(TestImageSinkFilter, GetOrCreateSurfaceBuffer_002, TestSize.Level1) {
    BufferRequestConfig config = {
        .width = 1080,
        .height = 1920,
    };
    BufferRequestConfig hdrConfig = {
        .width = 1920,
        .height = 1920,
    };

    auto hdrSurfaceBuffer = SurfaceBuffer::Create();
    hdrSurfaceBuffer->Alloc(hdrConfig);
    imageSinkFilter_->hdrSurfaceBuffer_ = hdrSurfaceBuffer;
    auto ret = imageSinkFilter_->GetOrCreateSurfaceBuffer(config);
    ASSERT_NE(static_cast<void *>(ret), static_cast<void *>(hdrSurfaceBuffer));
}

HWTEST_F(TestImageSinkFilter, RenderToDisplay_001, TestSize.Level1) {
    std::unique_ptr<Picture> picture4Buffer_ = TestPictureUtils::CreatePictureByPath(TEST_INCLUDE_AUX_PATH);
    Picture* picture4Buffer = picture4Buffer_.get();
    std::shared_ptr<EffectBuffer> buffer = nullptr;
    auto ret = CommonUtils::ParsePicture(picture4Buffer, buffer);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);

    buffer->bufferInfo_->hdrFormat_ = HdrFormat::DEFAULT;
    ret = imageSinkFilter_->RenderToDisplay(buffer, context_);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);
}

HWTEST_F(TestImageSinkFilter, RenderToDisplay_002, TestSize.Level1) {
    std::unique_ptr<Picture> picture4Buffer_ = TestPictureUtils::CreatePictureByPath(TEST_INCLUDE_AUX_PATH);
    Picture* picture4Buffer = picture4Buffer_.get();
    std::shared_ptr<EffectBuffer> buffer = nullptr;
    auto ret = CommonUtils::ParsePicture(picture4Buffer, buffer);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);

    buffer->bufferInfo_->hdrFormat_ = HdrFormat::HDR8_GAINMAP;
    buffer->bufferInfo_->formatType_ = IEffectFormat::YCBCR_P010;
    ret = imageSinkFilter_->RenderToDisplay(buffer, context_);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);
}

HWTEST_F(TestImageSinkFilter, RenderToDisplay_003, TestSize.Level1) {
    std::unique_ptr<Picture> picture4Buffer_ = TestPictureUtils::CreatePictureByPath(TEST_INCLUDE_AUX_PATH);
    Picture* picture4Buffer = picture4Buffer_.get();
    std::shared_ptr<EffectBuffer> buffer = nullptr;
    auto ret = CommonUtils::ParsePicture(picture4Buffer, buffer);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);

    buffer->bufferInfo_->hdrFormat_ = HdrFormat::HDR8_GAINMAP;
    buffer->bufferInfo_->formatType_ = IEffectFormat::RGBA8888;
    ret = imageSinkFilter_->RenderToDisplay(buffer, context_);
    // No input
    ASSERT_EQ(ret, ErrorCode::ERR_INPUT_NULL);
}

HWTEST_F(TestImageSinkFilter, RenderToDisplay_004, TestSize.Level1) {
    std::unique_ptr<Picture> picture4Buffer_ = TestPictureUtils::CreatePictureByPath(TEST_INCLUDE_AUX_PATH);
    Picture* picture4Buffer = picture4Buffer_.get();
    std::shared_ptr<EffectBuffer> buffer = nullptr;
    auto ret = CommonUtils::ParsePicture(picture4Buffer, buffer);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);

    buffer->bufferInfo_->hdrFormat_ = HdrFormat::HDR10;
    ret = imageSinkFilter_->RenderToDisplay(buffer, context_);
    // No input
    ASSERT_EQ(ret, ErrorCode::ERR_INPUT_NULL);
}

HWTEST_F(TestImageSinkFilter, RenderToDisplay_005, TestSize.Level1) {
    std::unique_ptr<Picture> picture4Buffer_ = TestPictureUtils::CreatePictureByPath(TEST_INCLUDE_AUX_PATH);
    Picture* picture4Buffer = picture4Buffer_.get();
    std::shared_ptr<EffectBuffer> buffer = nullptr;
    auto ret = CommonUtils::ParsePicture(picture4Buffer, buffer);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);

    buffer->bufferInfo_->hdrFormat_ = HdrFormat::HDR8_GAINMAP;
    buffer->bufferInfo_->formatType_ = IEffectFormat::RGBA_1010102;
    ret = imageSinkFilter_->RenderToDisplay(buffer, context_);
    // No input
    ASSERT_EQ(ret, ErrorCode::ERR_INPUT_NULL);
}

HWTEST_F(TestImageSinkFilter, ProcessDisplayForNoTex_001, TestSize.Level1) {
    std::unique_ptr<Picture> picture4Buffer_ = TestPictureUtils::CreatePictureByPath(TEST_INCLUDE_AUX_PATH);
    Picture* picture4Buffer = picture4Buffer_.get();
    std::shared_ptr<EffectBuffer> buffer = nullptr;
    auto ret = CommonUtils::ParsePicture(picture4Buffer, buffer);
    ASSERT_EQ(ret, ErrorCode::SUCCESS);

    buffer->bufferInfo_->hdrFormat_ = HdrFormat::HDR10;
    buffer->bufferInfo_->formatType_ = IEffectFormat::RGBA_1010102;
    ret = imageSinkFilter_->ProcessDisplayForNoTex(buffer, context_);
    // No input
    ASSERT_EQ(ret, ErrorCode::ERR_INPUT_NULL);
}

}
}
}
}