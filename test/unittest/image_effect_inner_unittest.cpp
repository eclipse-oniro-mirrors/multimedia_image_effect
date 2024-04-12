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

#include "image_effect_inner_unittest.h"

#include "efilter_factory.h"
#include "brightness_efilter.h"
#include "contrast_efilter.h"
#include "test_common.h"
#include "external_loader.h"
#include "crop_efilter.h"

using namespace testing::ext;
using ::testing::_;
using ::testing::A;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::Return;
using namespace OHOS::Media::Effect::Test;

namespace {
    constexpr uint32_t CROP_FACTOR = 2;
}

namespace OHOS {
namespace Media {
namespace Effect {
class FakeEFilter : public EFilter {
public:
    explicit FakeEFilter(const std::string &name) : EFilter(name) {}
    ~FakeEFilter() {}

    ErrorCode Render(EffectBuffer *buffer, std::shared_ptr<EffectContext> &context) override
    {
        return PushData(buffer, context);
    }

    ErrorCode Render(EffectBuffer *src, EffectBuffer *dst, std::shared_ptr<EffectContext> &context) override
    {
        return ErrorCode::SUCCESS;
    }

    ErrorCode Restore(const nlohmann::json &values) override
    {
        return ErrorCode::SUCCESS;
    }
};

class FakeImageEffect : public ImageEffect {
public:
    explicit FakeImageEffect() : ImageEffect("ImageEffect") {}
    ~FakeImageEffect() {}
};

void ImageEffectInnerUnittest::SetUpTestCase() {}

void ImageEffectInnerUnittest::TearDownTestCase() {}

void ImageEffectInnerUnittest::SetUp()
{
    ExternLoader::Instance()->InitExt();
    EFilterFactory::Instance()->functions_.clear();
    EFilterFactory::Instance()->RegisterEFilter<BrightnessEFilter>(BRIGHTNESS_EFILTER);
    EFilterFactory::Instance()->RegisterEFilter<ContrastEFilter>(CONTRAST_EFILTER);
    EFilterFactory::Instance()->RegisterEFilter<CropEFilter>(CROP_EFILTER);
    EFilterFactory::Instance()->delegates_.clear();
    mockPixelMap_ = new MockPixelMap();
    imageEffect_ = new FakeImageEffect();
    efilter_ = new FakeEFilter(BRIGHTNESS_EFILTER);

    std::shared_ptr<BufferInfo> info = std::make_unique<BufferInfo>();
    info->width_ = mockPixelMap_->GetWidth();
    info->height_ = mockPixelMap_->GetHeight();
    info->rowStride_ = mockPixelMap_->GetRowStride();
    info->len_ = mockPixelMap_->GetHeight() * mockPixelMap_->GetRowStride();
    info->formatType_ = IEffectFormat::RGBA8888;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_unique<ExtraInfo>();
    extraInfo->dataType = DataType::PIXEL_MAP;
    extraInfo->bufferType = BufferType::HEAP_MEMORY;
    extraInfo->pixelMap = mockPixelMap_;
    extraInfo->surfaceBuffer = nullptr;
    effectBuffer_ = new EffectBuffer(info, (void *)mockPixelMap_->GetPixels(), extraInfo);
}

void ImageEffectInnerUnittest::TearDown()
{
    delete(mockPixelMap_);
    Mock::AllowLeak(imageEffect_);
    Mock::AllowLeak(efilter_);
    delete(effectBuffer_);
}

HWTEST_F(ImageEffectInnerUnittest, Image_effect_unittest_001, TestSize.Level0)
{
    InSequence s;
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    imageEffect_->AddEFilter(efilter);
    Plugin::Any value = 200.f;
    efilter->SetValue(KEY_FILTER_INTENSITY, value);
    ErrorCode result = imageEffect_->SetInputPixelMap(mockPixelMap_);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    result = imageEffect_->Start();
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, Image_effect_unittest_002, TestSize.Level0)
{
    InSequence s;
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    Plugin::Any value = 200.f;
    efilter->SetValue(KEY_FILTER_INTENSITY, value);
    std::shared_ptr<EffectBuffer> src = std::make_shared<EffectBuffer>(
        effectBuffer_->bufferInfo_, effectBuffer_->buffer_, effectBuffer_->extraInfo_);

    ErrorCode result = efilter->Render(src, src);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, Image_effect_unittest_003, TestSize.Level1)
{
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    imageEffect_->AddEFilter(efilter);
    Plugin::Any value = 100.f;
    efilter->SetValue(KEY_FILTER_INTENSITY, value);
    ErrorCode result = imageEffect_->SetInputPixelMap(mockPixelMap_);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    result = imageEffect_->SetOutputPixelMap(mockPixelMap_);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    result = imageEffect_->Start();
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, Image_effect_unittest_004, TestSize.Level1)
{
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(CROP_EFILTER);
    imageEffect_->AddEFilter(efilter);
    uint32_t x1 = static_cast<uint32_t>(mockPixelMap_->GetWidth() / CROP_FACTOR);
    uint32_t y1 = static_cast<uint32_t>(mockPixelMap_->GetHeight() / CROP_FACTOR);
    uint32_t areaInfo[] = { 0, 0, x1, y1};
    Plugin::Any value = static_cast<void *>(areaInfo);
    efilter->SetValue(KEY_FILTER_REGION, value);
    ErrorCode result = imageEffect_->SetInputPixelMap(mockPixelMap_);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    result = imageEffect_->SetOutputPixelMap(mockPixelMap_);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    result = imageEffect_->Start();
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, Image_effect_unittest_005, TestSize.Level1)
{
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(CROP_EFILTER);
    imageEffect_->AddEFilter(efilter);
    uint32_t x1 = static_cast<uint32_t>(mockPixelMap_->GetWidth() / CROP_FACTOR);
    uint32_t y1 = static_cast<uint32_t>(mockPixelMap_->GetHeight() / CROP_FACTOR);
    uint32_t areaInfo[] = { 0, 0, x1, y1};
    Plugin::Any value = static_cast<void *>(areaInfo);
    efilter->SetValue(KEY_FILTER_REGION, value);
    ErrorCode result = imageEffect_->SetInputPixelMap(mockPixelMap_);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    result = imageEffect_->Start();
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}
} // namespace Effect
} // namespace Media
} // namespace OHOS