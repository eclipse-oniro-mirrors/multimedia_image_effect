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

#include "image_effect_inner.h"
#include "efilter_factory.h"
#include "mock_pixel_map.h"
#include "brightness_efilter.h"
#include "contrast_efilter.h"
#include "mock_surface_buffer.h"
#include "test_common.h"
#include "external_loader.h"
#include "crop_efilter.h"

using namespace testing::ext;
using namespace OHOS::Media::Effect;

static std::string g_jpgPath;
static std::string g_notJpgPath;
static std::string g_jpgUri;
static std::string g_notJpgUri;

namespace OHOS {
namespace Media {
namespace Effect {
namespace Test {
class CustomTestEFilter : public IFilterDelegate {
public:
    bool Render(void *efilter, EffectBuffer *src, EffectBuffer *dst, std::shared_ptr<EffectContext> &context) override
    {
        return true;
    }

    bool Render(void *efilter, EffectBuffer *src, std::shared_ptr<EffectContext> &context) override
    {
        return true;
    }

    bool SetValue(void *efilter, const std::string &key, const Plugin::Any &value) override
    {
        return true;
    }

    bool Save(void *efilter, nlohmann::json &res) override
    {
        auto *filter = static_cast<EFilter *>(efilter);
        return filter != nullptr && filter->Save(res) == ErrorCode::SUCCESS;
    }

    void *Restore(const nlohmann::json &values) override
    {
        filter_ = EFilterFactory::Instance()->Create(CUSTOM_TEST_EFILTER);
        Plugin::Any brightness = values.at("brightness").get<float>();
        filter_->SetValue("brightness", brightness);
        return filter_.get();
    }

    void *GetEffectInfo() override
    {
        if (info_ != nullptr) {
            return &info_;
        }
        info_ = std::make_unique<EffectInfo>();
        info_->formats_.emplace(IEffectFormat::RGBA8888, std::vector<IPType>{ IPType::CPU });
        info_->category_ = Category::SHAPE_ADJUST;
        return &info_;
    }
private:
    static std::shared_ptr<EffectInfo> info_;
    std::shared_ptr<EFilter> filter_;
};
std::shared_ptr<EffectInfo> CustomTestEFilter::info_ = nullptr;

class TestImageEffect : public testing::Test {
public:
    TestImageEffect() = default;

    ~TestImageEffect() override = default;

    static void SetUpTestCase()
    {
        g_jpgPath = std::string("/data/test/resource/image_effect_test1.jpg");
        g_notJpgPath = std::string("/data/test/resource/image_effect_test2.png");
        g_jpgUri = std::string("file:///data/test/resource/image_effect_test1.jpg");
        g_notJpgUri = std::string("file:///data/test/resource/image_effect_test2.png");
    }

    static void TearDownTestCase() {}

    void SetUp() override
    {
        mockPixelMap_ = new MockPixelMap();
        imageEffect_ = new ImageEffect();
        ExternLoader::Instance()->InitExt();
        EFilterFactory::Instance()->functions_.clear();
        EFilterFactory::Instance()->RegisterEFilter<BrightnessEFilter>(BRIGHTNESS_EFILTER);
        EFilterFactory::Instance()->RegisterEFilter<ContrastEFilter>(CONTRAST_EFILTER);
        EFilterFactory::Instance()->RegisterEFilter<CropEFilter>(CROP_EFILTER);
    }

    void TearDown() override
    {
        delete mockPixelMap_;
        mockPixelMap_ = nullptr;
        delete imageEffect_;
        imageEffect_ = nullptr;
    }
    PixelMap *mockPixelMap_ = nullptr;
    ImageEffect *imageEffect_ = nullptr;
};

HWTEST_F(TestImageEffect, AddEfilter001, TestSize.Level0)
{
    std::shared_ptr<ImageEffect> imageEffect = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    std::shared_ptr<EFilter> brightnessEFilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    imageEffect->AddEFilter(brightnessEFilter);
    ASSERT_EQ(imageEffect->efilters_.size(), 1);
    ASSERT_EQ(imageEffect->efilters_.at(0), brightnessEFilter);

    std::shared_ptr<EFilter> cropEFilter1 = EFilterFactory::Instance()->Create(CROP_EFILTER);
    imageEffect->AddEFilter(cropEFilter1);
    ASSERT_EQ(imageEffect->efilters_.size(), 2);
    ASSERT_EQ(imageEffect->efilters_.at(0), cropEFilter1);
    ASSERT_EQ(imageEffect->efilters_.at(1), brightnessEFilter);

    std::shared_ptr<EFilter> cropEFilter2 = EFilterFactory::Instance()->Create(CROP_EFILTER);
    imageEffect->AddEFilter(cropEFilter2);
    ASSERT_EQ(imageEffect->efilters_.size(), 3);
    ASSERT_EQ(imageEffect->efilters_.at(0), cropEFilter1);
    ASSERT_EQ(imageEffect->efilters_.at(1), cropEFilter2);
    ASSERT_EQ(imageEffect->efilters_.at(2), brightnessEFilter);
}

HWTEST_F(TestImageEffect, Start001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    Plugin::Any value = 100.f;
    ErrorCode result = efilter->SetValue(KEY_FILTER_INTENSITY, value);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    imageEffect->AddEFilter(efilter);
    result = imageEffect->SetInputPixelMap(mockPixelMap_);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    result = imageEffect->SetOutputPixelMap(mockPixelMap_);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    result = imageEffect->Start();
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(TestImageEffect, InsertEfilter001, TestSize.Level0)
{
    std::shared_ptr<ImageEffect> imageEffect = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    std::shared_ptr<EFilter> brightnessEFilter1 = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    imageEffect->InsertEFilter(brightnessEFilter1, 0);
    ASSERT_EQ(imageEffect->efilters_.size(), 1);
    ASSERT_EQ(imageEffect->efilters_.at(0), brightnessEFilter1);

    std::shared_ptr<EFilter> contrastEFilter = EFilterFactory::Instance()->Create(CONTRAST_EFILTER);
    imageEffect->AddEFilter(contrastEFilter);
    ASSERT_EQ(imageEffect->efilters_.size(), 2);
    ASSERT_EQ(imageEffect->efilters_.at(0), brightnessEFilter1);
    ASSERT_EQ(imageEffect->efilters_.at(1), contrastEFilter);

    std::shared_ptr<EFilter> brightnessEFilter2 = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    imageEffect->InsertEFilter(brightnessEFilter2, 1);
    ASSERT_EQ(imageEffect->efilters_.size(), 3);
    ASSERT_EQ(imageEffect->efilters_.at(0), brightnessEFilter1);
    ASSERT_EQ(imageEffect->efilters_.at(1), brightnessEFilter2);
    ASSERT_EQ(imageEffect->efilters_.at(2), contrastEFilter);
}

HWTEST_F(TestImageEffect, InsertEfilter002, TestSize.Level0)
{
    std::shared_ptr<ImageEffect> imageEffect = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    std::shared_ptr<EFilter> brightnessEFilter1 = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    ErrorCode result = imageEffect->InsertEFilter(brightnessEFilter1, 1);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffect::SetInputPath with normal param
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffect::SetInputPath with normal param
 */
HWTEST_F(TestImageEffect, SetInputPath001, TestSize.Level0)
{
    ErrorCode result = imageEffect_->SetInputPath(g_jpgPath);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffect::SetInputPath with nullptr param
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffect::SetInputPath with nullptr param
 */
HWTEST_F(TestImageEffect, SetInputPath002, TestSize.Level0)
{
    ErrorCode result = imageEffect_->SetInputPath(nullptr);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffect::SetInputPath with not support type param
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffect::SetInputPath with not support type param
 */
HWTEST_F(TestImageEffect, SetInputPath003, TestSize.Level0)
{
    ErrorCode result = imageEffect_->SetInputPath(g_notJpgPath);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffect::SetOutputPath with normal param
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffect::SetOutputPath with normal param
 */
HWTEST_F(TestImageEffect, SetOutputPath001, TestSize.Level0)
{
    ErrorCode result = imageEffect_->SetOutputPath(g_jpgPath);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffect::SetOutputPath with nullptr param
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffect::SetOutputPath with nullptr param
 */
HWTEST_F(TestImageEffect, SetOutputPath002, TestSize.Level0)
{
    ErrorCode result = imageEffect_->SetOutputPath(nullptr);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffect::SetOutputPath with not support type param
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffect::SetOutputPath with not support type param
 */
HWTEST_F(TestImageEffect, SetOutputPath003, TestSize.Level0)
{
    ErrorCode result = imageEffect_->SetOutputPath(g_notJpgPath);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffect::SetInputUri with normal param
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffect::SetInputUri with normal param
 */
HWTEST_F(TestImageEffect, SetInputUri001, TestSize.Level0)
{
    ErrorCode result = imageEffect_->SetInputUri(g_jpgUri);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffect::SetInputUri with nullptr param
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffect::SetInputUri with nullptr param
 */
HWTEST_F(TestImageEffect, SetInputUri002, TestSize.Level0)
{
    ErrorCode result = imageEffect_->SetInputUri(nullptr);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffect::SetInputUri with not support type param
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffect::SetInputUri with not support type param
 */
HWTEST_F(TestImageEffect, SetInputUri003, TestSize.Level0)
{
    ErrorCode result = imageEffect_->SetInputUri(g_notJpgUri);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffect::SetOutputUri with normal param
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffect::SetOutputUri with normal param
 */
HWTEST_F(TestImageEffect, SetOutputUri001, TestSize.Level0)
{
    ErrorCode result = imageEffect_->SetOutputUri(g_jpgUri);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffect::SetOutputUri with nullptr param
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffect::SetOutputUri with nullptr param
 */
HWTEST_F(TestImageEffect, SetOutputUri002, TestSize.Level0)
{
    ErrorCode result = imageEffect_->SetOutputUri(nullptr);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffect::SetOutputUri with not support type param
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffect::SetOutputUri with not support type param
 */
HWTEST_F(TestImageEffect, SetOutputUri003, TestSize.Level0)
{
    ErrorCode result = imageEffect_->SetOutputUri(g_notJpgUri);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

HWTEST_F(TestImageEffect, Restore001, TestSize.Level1)
{
    std::string info = "{\"imageEffect\":{\"filters\":[{\"name\":\"Brightness\",\"values\":{\"FilterIntensity\":"
        "100.0}},{\"name\":\"Contrast\",\"values\":{\"FilterIntensity\":50.0}}],\"name\":\"imageEdit\"}}";
    std::shared_ptr<ImageEffect> imageEffect = ImageEffect::Restore(info);
    ASSERT_NE(imageEffect, nullptr);
    std::vector<std::shared_ptr<EFilter>> efilters = imageEffect->GetEFilters();
    ASSERT_EQ(efilters.size(), 2);
    ASSERT_STREQ(efilters.at(0)->GetName().c_str(), BRIGHTNESS_EFILTER);
    Plugin::Any value;
    ASSERT_EQ(efilters.at(0)->GetValue(KEY_FILTER_INTENSITY, value), ErrorCode::SUCCESS);
    auto brightnessRatio = Plugin::AnyCast<float>(&value);
    ASSERT_NE(brightnessRatio, nullptr);
    ASSERT_FLOAT_EQ(*brightnessRatio, 100.f);

    Plugin::Any any;
    ASSERT_STREQ(efilters.at(1)->GetName().c_str(), CONTRAST_EFILTER);
    ASSERT_EQ(efilters.at(1)->GetValue(KEY_FILTER_INTENSITY, any), ErrorCode::SUCCESS);
    auto contrastRatio = Plugin::AnyCast<float>(&any);
    ASSERT_NE(contrastRatio, nullptr);
    ASSERT_FLOAT_EQ(*contrastRatio, 50.f);
}

HWTEST_F(TestImageEffect, Restore002, TestSize.Level1)
{
    std::shared_ptr<IFilterDelegate> customTestEFilter = std::make_unique<CustomTestEFilter>();
    auto *effectInfo = static_cast<std::shared_ptr<EffectInfo> *>(customTestEFilter->GetEffectInfo());
    ASSERT_NE(effectInfo, nullptr);
    EFilterFactory::Instance()->RegisterDelegate(CUSTOM_TEST_EFILTER, customTestEFilter, *effectInfo);

    std::string info = "{\"imageEffect\":{\"filters\":[{\"name\":\"CustomTestEFilter\",\"values\":{\"brightness\":"
        "60.0}},{\"name\":\"Contrast\",\"values\":{\"FilterIntensity\":50.0}}],\"name\":\"imageEdit\"}}";

    std::shared_ptr<ImageEffect> imageEffect = ImageEffect::Restore(info);
    ASSERT_NE(imageEffect, nullptr);
    std::vector<std::shared_ptr<EFilter>> efilters = imageEffect->GetEFilters();
    ASSERT_EQ(efilters.size(), 2);
    ASSERT_STREQ(efilters.at(0)->GetName().c_str(), "CustomTestEFilter");
    Plugin::Any value;
    ASSERT_EQ(efilters.at(0)->GetValue("brightness", value), ErrorCode::SUCCESS);
    auto brightness = Plugin::AnyCast<float>(&value);
    ASSERT_NE(brightness, nullptr);
    ASSERT_FLOAT_EQ(*brightness, 60.f);

    ErrorCode errorCode = imageEffect->SetInputPixelMap(mockPixelMap_);
    ASSERT_EQ(errorCode, ErrorCode::SUCCESS);
    errorCode = imageEffect->Start();
    ASSERT_EQ(errorCode, ErrorCode::SUCCESS);
}

HWTEST_F(TestImageEffect, Restore003, TestSize.Level1)
{
    std::string info = "{\"imageEffect\":{\"filters\":[{\"name\":\"Crop\"}],\"name\":\"imageEdit\"}}";
    std::shared_ptr<ImageEffect> imageEffect = ImageEffect::Restore(info);
    ASSERT_NE(imageEffect, nullptr);
    std::vector<std::shared_ptr<EFilter>> efilters = imageEffect->GetEFilters();
    ASSERT_EQ(efilters.size(), 1);
    ASSERT_STREQ(efilters.at(0)->GetName().c_str(), CROP_EFILTER);
}

HWTEST_F(TestImageEffect, Surface001, TestSize.Level1)
{
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer("UnitTest");
    ErrorCode result = imageEffect_->SetOutputSurface(surface);
    ASSERT_EQ(result, ErrorCode::SUCCESS);

    sptr<Surface> inputSurface = imageEffect_->GetInputSurface();
    ASSERT_NE(inputSurface, nullptr);

    std::shared_ptr<EFilter> contrastEFilter = EFilterFactory::Instance()->Create(CONTRAST_EFILTER);
    Plugin::Any value = 50.f;
    result = contrastEFilter->SetValue(KEY_FILTER_INTENSITY, value);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    imageEffect_->AddEFilter(contrastEFilter);

    result = imageEffect_->Start();
    ASSERT_EQ(result, ErrorCode::SUCCESS);

    sptr<SurfaceBuffer> surfaceBuffer = new MockSurfaceBuffer;
    OHOS::Rect damages;
    int64_t timeStamp = 0;
    imageEffect_->ConsumerBufferAvailable(surfaceBuffer, damages, timeStamp);
}
} // namespace Test
} // namespace Effect
} // namespace Media
} // namespace OHOS