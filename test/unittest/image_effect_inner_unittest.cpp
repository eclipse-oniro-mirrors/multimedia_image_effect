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
#include "mock_picture.h"
#include "mock_producer_surface.h"
#include "external_loader.h"
#include "color_space.h"

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

    ErrorCode Restore(const EffectJsonPtr &values) override
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
    info->pixelMap_ = mockPixelMap_;
    info->surfaceBuffer_ = nullptr;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_unique<ExtraInfo>();
    extraInfo->dataType = DataType::PIXEL_MAP;
    extraInfo->bufferType = BufferType::HEAP_MEMORY;
    effectBuffer_ = new EffectBuffer(info, (void *)mockPixelMap_->GetPixels(), extraInfo);
}

void ImageEffectInnerUnittest::TearDown()
{
    delete(mockPixelMap_);
    mockPixelMap_ = nullptr;
    Mock::AllowLeak(imageEffect_);
    Mock::AllowLeak(efilter_);
    delete(effectBuffer_);
    effectBuffer_ = nullptr;
}

HWTEST_F(ImageEffectInnerUnittest, Image_effect_unittest_001, TestSize.Level1)
{
    InSequence s;
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    imageEffect_->AddEFilter(efilter);
    Any value = 200.f;
    efilter->SetValue(KEY_FILTER_INTENSITY, value);
    ErrorCode result = imageEffect_->SetInputPixelMap(mockPixelMap_);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    result = imageEffect_->Start();
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, Image_effect_unittest_002, TestSize.Level1)
{
    InSequence s;
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    Any value = 200.f;
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
    Any value = 100.f;
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
    Any value = static_cast<void *>(areaInfo);
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
    Any value = static_cast<void *>(areaInfo);
    efilter->SetValue(KEY_FILTER_REGION, value);
    ErrorCode result = imageEffect_->SetInputPixelMap(mockPixelMap_);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    result = imageEffect_->Start();
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, Image_effect_unittest_006, TestSize.Level1)
{
    DataInfo dataInfo;
    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    std::shared_ptr<ExtraInfo> extraInfo = std::make_unique<ExtraInfo>();
    std::shared_ptr<EffectBuffer> effectBuffer = std::make_unique<EffectBuffer>(bufferInfo, nullptr, extraInfo);
    std::shared_ptr<ImageEffect> imageEffect = std::make_unique<ImageEffect>();

    dataInfo.dataType_ = DataType::PATH;
    dataInfo.path_ = "path";
    IEffectFormat format = IEffectFormat::RGBA8888;
    EXPECT_EQ(imageEffect->ParseDataInfo(dataInfo, effectBuffer, true, format), ErrorCode::SUCCESS);

    dataInfo.dataType_ = DataType::NATIVE_WINDOW;
    EXPECT_EQ(imageEffect->ParseDataInfo(dataInfo, effectBuffer, false, format), ErrorCode::SUCCESS);

    dataInfo.dataType_ = DataType::UNKNOWN;
    EXPECT_EQ(imageEffect->ParseDataInfo(dataInfo, effectBuffer, false, format), ErrorCode::ERR_NO_DATA);

    dataInfo.dataType_ = static_cast<DataType>(100);
    EXPECT_EQ(imageEffect->ParseDataInfo(dataInfo, effectBuffer, false, format), ErrorCode::ERR_UNSUPPORTED_DATA_TYPE);
}

HWTEST_F(ImageEffectInnerUnittest, SetOutputPicture_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    ErrorCode result = imageEffect_->SetOutputPicture(nullptr);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetOutputPicture_002, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    std::shared_ptr<Picture> picture = std::make_shared<MockPicture>();
    ErrorCode result = imageEffect_->SetOutputPicture(picture.get());
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetInputPicture_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    ErrorCode result = imageEffect_->SetInputPicture(nullptr);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetInputPicture_002, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    std::shared_ptr<Picture> picture = std::make_shared<MockPicture>();
    ErrorCode result = imageEffect_->SetInputPicture(picture.get());
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetExtraInfo_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    ErrorCode result = imageEffect_->SetExtraInfo(nullptr);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetOutputSurfaceBuffer_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    ErrorCode result = imageEffect_->SetOutputSurfaceBuffer(nullptr);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetOutputSurfaceBuffer_002, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    sptr<SurfaceBuffer> surfaceBuffer;
    MockProducerSurface::AllocDmaMemory(surfaceBuffer);
    ErrorCode result = imageEffect_->SetOutputSurfaceBuffer(surfaceBuffer);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    MockProducerSurface::ReleaseDmaBuffer(surfaceBuffer);
}

HWTEST_F(ImageEffectInnerUnittest, SetInputSurfaceBuffer_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    ErrorCode result = imageEffect_->SetInputSurfaceBuffer(nullptr);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetInputSurfaceBuffer_002, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    sptr<SurfaceBuffer> surfaceBuffer;
    MockProducerSurface::AllocDmaMemory(surfaceBuffer);
    ErrorCode result = imageEffect_->SetInputSurfaceBuffer(surfaceBuffer);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    imageEffect_->needPreFlush_ = true;
    result = imageEffect_->SetInputSurfaceBuffer(surfaceBuffer);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
    MockProducerSurface::ReleaseDmaBuffer(surfaceBuffer);
}

HWTEST_F(ImageEffectInnerUnittest, SetOutputSurface_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    sptr<Surface> surface = nullptr;
    ErrorCode result = imageEffect_->SetOutputSurface(surface);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetOutputPixelMap_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    ErrorCode result = imageEffect_->SetOutputPixelMap(nullptr);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetInputPixelMap_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    ErrorCode result = imageEffect_->SetInputPixelMap(nullptr);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetOutNativeWindow_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    ErrorCode result = imageEffect_->SetOutNativeWindow(nullptr);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetOutNativeWindow_002, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    OHNativeWindow *nativeWindow = new OHNativeWindow();
    nativeWindow->surface = nullptr;
    ErrorCode result = imageEffect_->SetOutNativeWindow(nativeWindow);
    ASSERT_NE(result, ErrorCode::SUCCESS);
    delete nativeWindow;
    nativeWindow = nullptr;
}

HWTEST_F(ImageEffectInnerUnittest, SetInputTexture_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    ErrorCode result = imageEffect_->SetInputTexture(0, 0);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetInputTexture_002, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    ErrorCode result = imageEffect_->SetInputTexture(2, 0);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetOutputTexture_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    ErrorCode result = imageEffect_->SetOutputTexture(0);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetOutputTexture_002, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    ErrorCode result = imageEffect_->SetOutputTexture(2);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, RenderTexture_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    std::shared_ptr<RenderEnvironment> env = std::make_shared<RenderEnvironment>();
    env->Init();
    RenderTexturePtr inTex = env->RequestBuffer(1080, 1920);
    RenderTexturePtr outTex = env->RequestBuffer(1080, 1920);
    ErrorCode result = imageEffect_->SetInputTexture(inTex->GetName(), ColorManager::ColorSpaceName::SRGB);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    result = imageEffect_->SetOutputTexture(outTex->GetName());
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    Any value = 70.f;
    efilter->SetValue(KEY_FILTER_INTENSITY, value);
    imageEffect_->AddEFilter(efilter);
    result = imageEffect_->Start();
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, ReplaceEFilter_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    imageEffect_->AddEFilter(efilter);
    std::shared_ptr<EFilter> efilter2 = EFilterFactory::Instance()->Create(CONTRAST_EFILTER);

    ErrorCode result = imageEffect_->ReplaceEFilter(efilter2, -1);
    ASSERT_NE(result, ErrorCode::SUCCESS);

    result = imageEffect_->ReplaceEFilter(efilter2, 0);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetInputUri_001, TestSize.Level1)
{
    std::string g_jpgUri = std::string("file:///data/test/resource/image_effect_1k_test1.jpg");
    std::string g_notJpgUri = std::string("file:///data/test/resource/image_effect_1k_test1.png");
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);

    ErrorCode result = imageEffect_->SetInputUri(g_notJpgUri);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    result = imageEffect_->SetInputUri(g_jpgUri);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetOutputUri_001, TestSize.Level1)
{
    std::string g_jpgUri = std::string("file:///data/test/resource/image_effect_1k_test1.jpg");
    std::string g_notJpgUri = std::string("file:///data/test/resource/image_effect_1k_test1.png");
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);

    ErrorCode result = imageEffect_->SetOutputUri("");
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    result = imageEffect_->SetOutputUri(g_notJpgUri);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    result = imageEffect_->SetOutputUri(g_jpgUri);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetInputPath_001, TestSize.Level1)
{
    std::string g_jpgPath = std::string("/data/test/resource/image_effect_1k_test1.jpg");
    std::string g_notJpgPath = std::string("/data/test/resource/image_effect_1k_test1.png");
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);

    ErrorCode result = imageEffect_->SetInputPath(g_notJpgPath);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    result = imageEffect_->SetInputPath(g_jpgPath);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, SetOutputPath_001, TestSize.Level1)
{
    std::string g_jpgPath = std::string("/data/test/resource/image_effect_1k_test1.jpg");
    std::string g_notJpgPath = std::string("/data/test/resource/image_effect_1k_test1.png");
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);

    ErrorCode result = imageEffect_->SetOutputPath("");
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    result = imageEffect_->SetOutputPath(g_notJpgPath);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    result = imageEffect_->SetOutputPath(g_jpgPath);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, Save_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    EffectJsonPtr jsonValues = EffectJsonHelper::CreateObject();
    ErrorCode result = imageEffect_->Save(jsonValues);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    EffectJsonPtr extra = EffectJsonHelper::CreateObject();
    imageEffect_->SetExtraInfo(extra);
    result = imageEffect_->Save(jsonValues);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, CacheBuffer_001, TestSize.Level1)
{
    std::shared_ptr<EffectContext> effectContext = std::make_shared<EffectContext>();
    std::shared_ptr<EffectBuffer> buffer = std::make_shared<EffectBuffer>(
        effectBuffer_->bufferInfo_, effectBuffer_->buffer_, effectBuffer_->extraInfo_);
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);

    ErrorCode result = efilter->StartCache();
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    result = efilter->CacheBuffer(buffer.get(), effectContext);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    result = efilter->GetFilterCache(buffer, effectContext);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    result = efilter->CancelCache();
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    result = efilter->ReleaseCache();
    EXPECT_EQ(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, GetImageInfo_001, TestSize.Level1)
{
    std::shared_ptr<ImageEffect> imageEffect_ = std::make_unique<ImageEffect>(IMAGE_EFFECT_NAME);
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    imageEffect_->AddEFilter(efilter);

    imageEffect_->inDateInfo_.dataType_ = DataType::UNKNOWN;
    ErrorCode result = imageEffect_->Render();

    imageEffect_->inDateInfo_.dataType_ = DataType::TEX;
    result = imageEffect_->Render();

    imageEffect_->inDateInfo_.dataType_ = DataType::NATIVE_WINDOW;
    result = imageEffect_->Render();
    EXPECT_NE(result, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, GetExifMetadata_001, TestSize.Level1)
{
    std::shared_ptr<EffectContext> context = std::make_shared<EffectContext>();
    context->renderStrategy_ = std::make_shared<RenderStrategy>();
    std::shared_ptr<EffectBuffer> src = std::make_shared<EffectBuffer>(
        effectBuffer_->bufferInfo_, effectBuffer_->buffer_, effectBuffer_->extraInfo_);
    std::shared_ptr<EffectBuffer> dst = std::make_shared<EffectBuffer>(
        effectBuffer_->bufferInfo_, effectBuffer_->buffer_, effectBuffer_->extraInfo_);
    context->renderStrategy_->src_ = src;
    context->renderStrategy_->dst_ = dst;
    std::shared_ptr<ExifMetadata> data = context->GetExifMetadata();

    context->renderStrategy_->src_->extraInfo_->dataType = DataType::PICTURE;
    data = context->GetExifMetadata();
    EXPECT_EQ(data, nullptr);
}

HWTEST_F(ImageEffectInnerUnittest, ExternLoader_001, TestSize.Level1)
{
    ExternLoader *instance = ExternLoader::Instance();
    instance->isExtLoad_ = true;
    instance->LoadExtSo();
    EXPECT_NE(instance, nullptr);
}

HWTEST_F(ImageEffectInnerUnittest, EFilter_001, TestSize.Level1)
{
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    ErrorCode res = efilter->ProcessConfig("START_CACHE");
    EXPECT_EQ(res, ErrorCode::SUCCESS);

    res = efilter->ProcessConfig("CANCEL_CACHE");
    EXPECT_EQ(res, ErrorCode::SUCCESS);

    std::shared_ptr<EffectContext> context = std::make_shared<EffectContext>();
    std::shared_ptr<EffectBuffer> src = std::make_shared<EffectBuffer>(
        effectBuffer_->bufferInfo_, effectBuffer_->buffer_, effectBuffer_->extraInfo_);
    efilter->cacheConfig_ = std::make_shared<EFilterCacheConfig>();
    efilter->cacheConfig_->status_ = CacheStatus::CACHE_START;
    context->cacheNegotiate_ = std::make_shared<EFilterCacheNegotiate>();
    context->cacheNegotiate_->config_ = nullptr;

    context->cacheNegotiate_->config_ = std::make_shared<EFilterCacheConfig>();

    efilter->cacheConfig_->status_ = CacheStatus::NO_CACHE;

    context->cacheNegotiate_->needCache_ = true;
    context->cacheNegotiate_->hasUsedCache_ = false;
    efilter->cacheConfig_->status_ = CacheStatus::CACHE_USED;
    res = efilter->PushData("test", src, context);
    EXPECT_NE(res, ErrorCode::SUCCESS);

    context->cacheNegotiate_->needCache_ = true;
    context->cacheNegotiate_->hasUsedCache_ = false;
    efilter->cacheConfig_->status_ = CacheStatus::NO_CACHE;
    src = nullptr;
    res = efilter->PushData("test", src, context);
    EXPECT_NE(res, ErrorCode::SUCCESS);

    context->cacheNegotiate_->needCache_ = true;
    context->cacheNegotiate_->hasUsedCache_ = false;
    efilter->cacheConfig_->status_ = CacheStatus::CACHE_ENABLED;
    res = efilter->PushData("test", src, context);
    EXPECT_NE(res, ErrorCode::SUCCESS);

    context->cacheNegotiate_->needCache_ = true;
    context->cacheNegotiate_->hasUsedCache_ = false;
    efilter->cacheConfig_->status_ = CacheStatus::CACHE_START;
    src = std::make_shared<EffectBuffer>(
        effectBuffer_->bufferInfo_, effectBuffer_->buffer_, effectBuffer_->extraInfo_);
    res = efilter->PushData("test", src, context);
    EXPECT_NE(res, ErrorCode::SUCCESS);
}

HWTEST_F(ImageEffectInnerUnittest, PushData_001, TestSize.Level1)
{
    std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    std::shared_ptr<EffectContext> context = std::make_shared<EffectContext>();
    std::shared_ptr<EffectBuffer> src = std::make_shared<EffectBuffer>(
        effectBuffer_->bufferInfo_, effectBuffer_->buffer_, effectBuffer_->extraInfo_);
    efilter->cacheConfig_ = std::make_shared<EFilterCacheConfig>();
    context->cacheNegotiate_ = std::make_shared<EFilterCacheNegotiate>();

    context->cacheNegotiate_->needCache_ = false;
    ErrorCode res = efilter->PushData("test", src, context);
    EXPECT_NE(res, ErrorCode::SUCCESS);

    context->cacheNegotiate_->needCache_ = true;
    context->cacheNegotiate_->config_ = nullptr;
    res = efilter->PushData("test", src, context);
    EXPECT_NE(res, ErrorCode::SUCCESS);

    context->cacheNegotiate_->needCache_ = true;
    context->cacheNegotiate_->config_ = std::make_shared<EFilterCacheConfig>();
    context->cacheNegotiate_->hasUsedCache_ = true;
    res = efilter->PushData("test", src, context);
    EXPECT_NE(res, ErrorCode::SUCCESS);
}
} // namespace Effect
} // namespace Media
} // namespace OHOS
