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
#include "test_common.h"
#include "external_loader.h"
#include "crop_efilter.h"
#include "mock_producer_surface.h"

#include "surface_buffer_impl.h"
#include <sync_fence.h>
#include "metadata_helper.h"
#include "effect_surface_adapter.h"
#include "v1_1/buffer_handle_meta_key_type.h"

using namespace testing::ext;
using namespace OHOS::Media::Effect;

static std::string g_jpgPath;
static std::string g_notJpgPath;
static std::string g_jpgUri;
static std::string g_notJpgUri;

namespace OHOS {
namespace Media {
namespace Effect {
using namespace OHOS::HDI::Display::Graphic::Common;
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

    bool Save(void *efilter, EffectJsonPtr &res) override
    {
        auto *filter = static_cast<EFilter *>(efilter);
        return filter != nullptr && filter->Save(res) == ErrorCode::SUCCESS;
    }

    void *Restore(const EffectJsonPtr &values) override
    {
        filter_ = EFilterFactory::Instance()->Create(CUSTOM_TEST_EFILTER);
        Plugin::Any brightness = values->GetFloat("brightness");
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
        effectSurfaceAdapter_ = new EffectSurfaceAdapter();
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
        delete effectSurfaceAdapter_;
        effectSurfaceAdapter_ = nullptr;
    }
    PixelMap *mockPixelMap_ = nullptr;
    ImageEffect *imageEffect_ = nullptr;
    EffectSurfaceAdapter *effectSurfaceAdapter_ = nullptr;
};

HWTEST_F(TestImageEffect, AddEfilter001, TestSize.Level1)
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

HWTEST_F(TestImageEffect, InsertEfilter001, TestSize.Level1)
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

HWTEST_F(TestImageEffect, InsertEfilter002, TestSize.Level1)
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
HWTEST_F(TestImageEffect, SetInputPath001, TestSize.Level1)
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
HWTEST_F(TestImageEffect, SetInputPath002, TestSize.Level1)
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
HWTEST_F(TestImageEffect, SetInputPath003, TestSize.Level1)
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
HWTEST_F(TestImageEffect, SetOutputPath001, TestSize.Level1)
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
HWTEST_F(TestImageEffect, SetOutputPath002, TestSize.Level1)
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
HWTEST_F(TestImageEffect, SetOutputPath003, TestSize.Level1)
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
HWTEST_F(TestImageEffect, SetInputUri001, TestSize.Level1)
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
HWTEST_F(TestImageEffect, SetInputUri002, TestSize.Level1)
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
HWTEST_F(TestImageEffect, SetInputUri003, TestSize.Level1)
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
HWTEST_F(TestImageEffect, SetOutputUri001, TestSize.Level1)
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
HWTEST_F(TestImageEffect, SetOutputUri002, TestSize.Level1)
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
HWTEST_F(TestImageEffect, SetOutputUri003, TestSize.Level1)
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
    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer("UnitTest");
    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    sptr<ProducerSurface> surf = new(std::nothrow) MockProducerSurface(producer);
    surf->Init();
    sptr<Surface> outputSurface = surf;

    ErrorCode result = imageEffect_->SetOutputSurface(outputSurface);
    ASSERT_EQ(result, ErrorCode::SUCCESS);

    sptr<Surface> inputSurface = imageEffect_->GetInputSurface();
    ASSERT_NE(inputSurface, nullptr);

    sptr<SurfaceBuffer> surfaceBuffer;
    MockProducerSurface::AllocDmaMemory(surfaceBuffer);

    // running without filter
    imageEffect_->ConsumerBufferAvailable();

    std::shared_ptr<EFilter> contrastEFilter = EFilterFactory::Instance()->Create(CONTRAST_EFILTER);
    Plugin::Any value = 50.f;
    result = contrastEFilter->SetValue(KEY_FILTER_INTENSITY, value);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    imageEffect_->AddEFilter(contrastEFilter);

    result = imageEffect_->Start();
    ASSERT_EQ(result, ErrorCode::SUCCESS);

    // contrast filter
    imageEffect_->ConsumerBufferAvailable();
    MockProducerSurface::ReleaseDmaBuffer(surfaceBuffer);
}

HWTEST_F(TestImageEffect, UpdateProducerSurfaceInfo001, TestSize.Level1)
{
    imageEffect_->UpdateProducerSurfaceInfo();
    ASSERT_EQ(imageEffect_->toProducerSurface_, nullptr);
}

HWTEST_F(TestImageEffect, MemoryCopyForSurfaceBuffer001, TestSize.Level1)
{
    sptr<SurfaceBuffer> buffer;
    MockProducerSurface::AllocDmaMemory(buffer);
    sptr<SurfaceBuffer> outBuffer;
    MockProducerSurface::AllocDmaMemory(outBuffer);
    buffer->SetSurfaceBufferWidth(100);
    buffer->SetSurfaceBufferHeight(100);

    buffer->SetSurfaceBufferWidth(200);
    buffer->SetSurfaceBufferHeight(200);

    MemoryCopyForSurfaceBuffer(buffer, outBuffer);
    ASSERT_EQ(buffer->GetWidth(), outBuffer->GetWidth());
}

HWTEST_F(TestImageEffect, IsSurfaceBufferHebc001, TestSize.Level1)
{
    sptr<SurfaceBuffer> buffer;
    MockProducerSurface::AllocDmaMemory(buffer);
    std::vector<uint8_t> values;
    buffer->SetMetadata(V1_1::BufferHandleAttrKey::ATTRKEY_ACCESS_TYPE, values);
    bool result = IsSurfaceBufferHebc(buffer);
    ASSERT_NE(result, true);
}

HWTEST_F(TestImageEffect, GetBufferRequestConfig001, TestSize.Level1)
{
    sptr<SurfaceBuffer> buffer;
    MockProducerSurface::AllocDmaMemory(buffer);
    BufferRequestConfig result = imageEffect_->GetBufferRequestConfig(buffer);
    ASSERT_EQ(result.strideAlignment, 0x8);

    sptr<SurfaceBuffer> outBuffer;
    MockProducerSurface::AllocDmaMemory(outBuffer);
    buffer->SetSurfaceBufferWidth(1920);
    buffer->SetSurfaceBufferHeight(1080);
    BufferRequestConfig result2 = imageEffect_->GetBufferRequestConfig(outBuffer);
    ASSERT_EQ(result2.width, 960);
    ASSERT_EQ(result2.height, 1280);
}

HWTEST_F(TestImageEffect, FlushBuffer001, TestSize.Level1)
{
    sptr<SurfaceBuffer> inBuffer;
    MockProducerSurface::AllocDmaMemory(inBuffer);
    sptr<SyncFence> inBufferSyncFence = SyncFence::INVALID_FENCE;
    int64_t timestamp = 0;
    GSError result = imageEffect_->FlushBuffer(inBuffer, inBufferSyncFence, true, true, timestamp);
    ASSERT_NE(result, GSERROR_OK);
}

HWTEST_F(TestImageEffect, FlushBuffer002, TestSize.Level1)
{
    sptr<SurfaceBuffer> inBuffer;
    MockProducerSurface::AllocDmaMemory(inBuffer);
    sptr<SyncFence> inBufferSyncFence = SyncFence::INVALID_FENCE;
    int64_t timestamp = 0;
    GSError result = imageEffect_->FlushBuffer(inBuffer, inBufferSyncFence, true, false, timestamp);
    ASSERT_NE(result, GSERROR_OK);
}

HWTEST_F(TestImageEffect, FlushBuffer003, TestSize.Level1)
{
    sptr<SurfaceBuffer> inBuffer;
    MockProducerSurface::AllocDmaMemory(inBuffer);
    sptr<SyncFence> inBufferSyncFence = SyncFence::INVALID_FENCE;
    int64_t timestamp = 0;
    GSError result = imageEffect_->FlushBuffer(inBuffer, inBufferSyncFence, false, true, timestamp);
    ASSERT_NE(result, GSERROR_OK);
}

HWTEST_F(TestImageEffect, FlushBuffer004, TestSize.Level1)
{
    sptr<SurfaceBuffer> inBuffer;
    MockProducerSurface::AllocDmaMemory(inBuffer);
    sptr<SyncFence> inBufferSyncFence = SyncFence::INVALID_FENCE;
    int64_t timestamp = 0;
    GSError result = imageEffect_->FlushBuffer(inBuffer, inBufferSyncFence, false, false, timestamp);
    ASSERT_NE(result, GSERROR_OK);
}

HWTEST_F(TestImageEffect, ReleaseBuffer001, TestSize.Level1)
{
    sptr<SurfaceBuffer> buffer;
    MockProducerSurface::AllocDmaMemory(buffer);
    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;
    GSError result = imageEffect_->ReleaseBuffer(buffer, fence);
    ASSERT_NE(result, GSERROR_OK);
}

HWTEST_F(TestImageEffect, ProcessRender001, TestSize.Level1)
{
    sptr<SurfaceBuffer> inBuffer;
    MockProducerSurface::AllocDmaMemory(inBuffer);
    sptr<SurfaceBuffer> outBuffer;
    MockProducerSurface::AllocDmaMemory(outBuffer);

    BufferProcessInfo bufferProcessInfo{
        .inBuffer_ = inBuffer,
        .outBuffer_ = outBuffer,
        .inBufferSyncFence_ = SyncFence::INVALID_FENCE,
        .outBufferSyncFence_ = SyncFence::INVALID_FENCE,
        .isSrcHebcData_ = true,
    };

    bool isNeedSwap = true;
    int64_t timestamp = 0;
    imageEffect_->ProcessRender(bufferProcessInfo, isNeedSwap, timestamp);
    ASSERT_NE(bufferProcessInfo.outBuffer_, nullptr);
}

HWTEST_F(TestImageEffect, ProcessRender002, TestSize.Level1)
{
    sptr<SurfaceBuffer> inBuffer;
    MockProducerSurface::AllocDmaMemory(inBuffer);
    sptr<SurfaceBuffer> outBuffer = nullptr;

    BufferProcessInfo bufferProcessInfo{
        .inBuffer_ = inBuffer,
        .outBuffer_ = outBuffer,
        .inBufferSyncFence_ = SyncFence::INVALID_FENCE,
        .outBufferSyncFence_ = SyncFence::INVALID_FENCE,
        .isSrcHebcData_ = true,
    };

    bool isNeedSwap = true;
    int64_t timestamp = 0;
    imageEffect_->ProcessRender(bufferProcessInfo, isNeedSwap, timestamp);
    ASSERT_EQ(bufferProcessInfo.outBuffer_, nullptr);
}

HWTEST_F(TestImageEffect, ProcessRender003, TestSize.Level1)
{
    sptr<SurfaceBuffer> inBuffer;
    MockProducerSurface::AllocDmaMemory(inBuffer);
    sptr<SurfaceBuffer> outBuffer;
    MockProducerSurface::AllocDmaMemory(outBuffer);

    BufferProcessInfo bufferProcessInfo{
        .inBuffer_ = inBuffer,
        .outBuffer_ = outBuffer,
        .inBufferSyncFence_ = SyncFence::INVALID_FENCE,
        .outBufferSyncFence_ = SyncFence::INVALID_FENCE,
        .isSrcHebcData_ = true,
    };

    bool isNeedSwap = false;
    int64_t timestamp = 0;
    imageEffect_->ProcessRender(bufferProcessInfo, isNeedSwap, timestamp);
    ASSERT_NE(bufferProcessInfo.outBuffer_, nullptr);
}

HWTEST_F(TestImageEffect, GetProducerSurface001, TestSize.Level1)
{
    auto receiverConsumerSurface_ = IConsumerSurface::Create("EffectSurfaceAdapter");
    auto producer = receiverConsumerSurface_->GetProducer();
    effectSurfaceAdapter_->fromProducerSurface_ = Surface::CreateSurfaceAsProducer(producer);
    effectSurfaceAdapter_->GetProducerSurface();
    ASSERT_NE(effectSurfaceAdapter_->fromProducerSurface_, nullptr);
}

HWTEST_F(TestImageEffect, ReleaseConsumerSurfaceBuffer001, TestSize.Level1)
{
    sptr<SurfaceBuffer> buffer;
    MockProducerSurface::AllocDmaMemory(buffer);
    sptr<SyncFence> fence = SyncFence::INVALID_FENCE;
    GSError result = effectSurfaceAdapter_->ReleaseConsumerSurfaceBuffer(buffer, fence);
    ASSERT_NE(result, GSERROR_OK);
}

HWTEST_F(TestImageEffect, DetachConsumerSurfaceBuffer001, TestSize.Level1)
{
    sptr<SurfaceBuffer> buffer;
    MockProducerSurface::AllocDmaMemory(buffer);
    GSError result = effectSurfaceAdapter_->DetachConsumerSurfaceBuffer(buffer);
    ASSERT_NE(result, GSERROR_OK);
}

HWTEST_F(TestImageEffect, AttachConsumerSurfaceBuffer001, TestSize.Level1)
{
    sptr<SurfaceBuffer> buffer;
    MockProducerSurface::AllocDmaMemory(buffer);
    GSError result = effectSurfaceAdapter_->AttachConsumerSurfaceBuffer(buffer);
    ASSERT_NE(result, GSERROR_OK);
}

HWTEST_F(TestImageEffect, SetConsumerListener001, TestSize.Level1)
{
    ErrorCode result = effectSurfaceAdapter_->SetConsumerListener(nullptr);
    ASSERT_EQ(result, ErrorCode::ERR_INPUT_NULL);
}

HWTEST_F(TestImageEffect, GetTransform001, TestSize.Level1)
{
    effectSurfaceAdapter_->receiverConsumerSurface_ = nullptr;
    auto result = effectSurfaceAdapter_->GetTransform();
    ASSERT_EQ(result, GRAPHIC_ROTATE_BUTT);
}

HWTEST_F(TestImageEffect, GetTransform002, TestSize.Level1)
{
    effectSurfaceAdapter_->receiverConsumerSurface_ = IConsumerSurface::Create("EffectSurfaceAdapter");
    auto result = effectSurfaceAdapter_->GetTransform();
    ASSERT_NE(result, GRAPHIC_ROTATE_BUTT);
}

HWTEST_F(TestImageEffect, OnBufferAvailable001, TestSize.Level1)
{
    effectSurfaceAdapter_->receiverConsumerSurface_ = nullptr;
    effectSurfaceAdapter_->OnBufferAvailable();
    ASSERT_EQ(effectSurfaceAdapter_->receiverConsumerSurface_, nullptr);
}

HWTEST_F(TestImageEffect, OnBufferAvailable002, TestSize.Level1)
{
    effectSurfaceAdapter_->receiverConsumerSurface_ = IConsumerSurface::Create("EffectSurfaceAdapter");
    effectSurfaceAdapter_->OnBufferAvailable();
    ASSERT_NE(effectSurfaceAdapter_->receiverConsumerSurface_, nullptr);
}
} // namespace Test
} // namespace Effect
} // namespace Media
} // namespace OHOS