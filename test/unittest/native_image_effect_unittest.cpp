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

#include <vector>
#include "gtest/gtest.h"

#include "image_effect_errors.h"
#include "image_effect.h"
#include "image_effect_filter.h"
#include "image_type.h"
#include "efilter_factory.h"
#include "brightness_efilter.h"
#include "contrast_efilter.h"
#include "test_common.h"
#include "external_loader.h"
#include "native_effect_base.h"
#include "native_window.h"
#include "mock_pixel_map.h"
#include "pixelmap_native_impl.h"
#include "test_native_buffer_utils.h"
#include "test_pixel_map_utils.h"
#include "crop_efilter.h"
#include "mock_producer_surface.h"
#include "surface_utils.h"
#include "mock_picture.h"
#include "picture_native_impl.h"

using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Effect::Test;
using ::testing::InSequence;
using ::testing::Mock;

static std::string g_jpgPath;
static std::string g_notJpgPath;
static std::string g_jpgUri;
static std::string g_notJpgUri;
static std::string g_jpgHdrPath;
static std::string g_adobeHdrPath;
static OHNativeWindow *g_nativeWindow = nullptr;

namespace {
    constexpr uint32_t CROP_FACTOR = 2;
}

namespace OHOS {
namespace Media {
namespace Effect {
class CustomTestEFilter : public EFilter {
public:
    explicit CustomTestEFilter(const std::string &name) : EFilter(name) {}
    ~CustomTestEFilter() {}

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

    static std::shared_ptr<EffectInfo> GetEffectInfo(const std::string &name)
    {
        std::shared_ptr<EffectInfo> effectInfo = std::make_shared<EffectInfo>();
        effectInfo->formats_.emplace(IEffectFormat::RGBA8888, std::vector<IPType>{ IPType::GPU });
        return effectInfo;
    }

    ErrorCode Save(EffectJsonPtr &res) override
    {
        res->Put("name", name_);
        EffectJsonPtr jsonValues = EffectJsonHelper::CreateObject();
        Plugin::Any any;
        auto it = values_.find(Test::KEY_FILTER_INTENSITY);
        if (it == values_.end()) {
            return ErrorCode::ERR_UNKNOWN;
        }

        auto value = Plugin::AnyCast<void *>(&it->second);
        if (value == nullptr) {
            return ErrorCode::ERR_UNKNOWN;
        }

        std::string jsonValue = *reinterpret_cast<char **>(value);
        jsonValues->Put(it->first, jsonValue);
        res->Put("values", jsonValues);
        return ErrorCode::SUCCESS;
    }
};

class CustomTestEFilter2 : public CustomTestEFilter {
public:
    explicit CustomTestEFilter2(const std::string &name) : CustomTestEFilter(name) {}
    ~CustomTestEFilter2() {}
    static std::shared_ptr<EffectInfo> GetEffectInfo(const std::string &name)
    {
        std::shared_ptr<EffectInfo> effectInfo = std::make_shared<EffectInfo>();
        effectInfo->formats_.emplace(IEffectFormat::RGBA8888, std::vector<IPType>{ IPType::CPU });
        return effectInfo;
    }
};

namespace Test {
class NativeImageEffectUnittest : public testing::Test {
public:
    NativeImageEffectUnittest() = default;

    ~NativeImageEffectUnittest() override = default;

    static void SetUpTestCase()
    {
        g_jpgPath = std::string("/data/test/resource/image_effect_1k_test1.jpg");
        g_notJpgPath = std::string("/data/test/resource/image_effect_1k_test1.png");
        g_jpgUri = std::string("file:///data/test/resource/image_effect_1k_test1.jpg");
        g_notJpgUri = std::string("file:///data/test/resource/image_effect_1k_test1.png");
        g_jpgHdrPath = std::string("/data/test/resource/image_effect_hdr_test1.jpg");
        g_adobeHdrPath = std::string("/data/test/resource/image_effect_adobe_test1.jpg");
        consumerSurface_ = Surface::CreateSurfaceAsConsumer("UnitTest");
        sptr<IBufferProducer> producer = consumerSurface_->GetProducer();
        sptr<ProducerSurface> surf = new(std::nothrow) MockProducerSurface(producer);
        surf->Init();
        auto utils = SurfaceUtils::GetInstance();
        utils->Add(surf->GetUniqueId(), surf);
        ohSurface_ = surf;
        g_nativeWindow = CreateNativeWindowFromSurface(&ohSurface_);

        nativeBufferRgba_ = TestNativeBufferUtils::CreateNativeBuffer(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888);
        nativeBufferNV12_ =
            TestNativeBufferUtils::CreateNativeBuffer(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
        nativeBufferNV21_ =
            TestNativeBufferUtils::CreateNativeBuffer(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP);
    }

    static void TearDownTestCase()
    {
        if (g_nativeWindow != nullptr) {
            DestoryNativeWindow(g_nativeWindow);
            g_nativeWindow = nullptr;
        }
        consumerSurface_ = nullptr;
        ohSurface_ = nullptr;
        nativeBufferRgba_ = nullptr;
        nativeBufferNV12_ = nullptr;
        nativeBufferNV21_ = nullptr;
    }

    void SetUp() override
    {
        mockPixelMap_ = std::make_shared<MockPixelMap>();
        pixelmapNative_ = new OH_PixelmapNative(mockPixelMap_);
        ExternLoader::Instance()->InitExt();
        EFilterFactory::Instance()->functions_.clear();
        EFilterFactory::Instance()->RegisterEFilter<BrightnessEFilter>(BRIGHTNESS_EFILTER);
        EFilterFactory::Instance()->RegisterEFilter<ContrastEFilter>(CONTRAST_EFILTER);
        EFilterFactory::Instance()->RegisterEFilter<CustomTestEFilter>(CUSTOM_TEST_EFILTER);
        EFilterFactory::Instance()->RegisterEFilter<CustomTestEFilter2>(CUSTOM_TEST_EFILTER2);
        EFilterFactory::Instance()->RegisterEFilter<CropEFilter>(CROP_EFILTER);
        EFilterFactory::Instance()->delegates_.clear();
        filterInfo_ = OH_EffectFilterInfo_Create();
        OH_EffectFilterInfo_SetFilterName(filterInfo_, BRIGHTNESS_EFILTER);
        ImageEffect_BufferType bufferTypes[] = { ImageEffect_BufferType::EFFECT_BUFFER_TYPE_PIXEL };
        OH_EffectFilterInfo_SetSupportedBufferTypes(filterInfo_, sizeof(bufferTypes) / sizeof(ImageEffect_BufferType),
            bufferTypes);
        ImageEffect_Format formats[] = { ImageEffect_Format::EFFECT_PIXEL_FORMAT_RGBA8888,
            ImageEffect_Format::EFFECT_PIXEL_FORMAT_NV12, ImageEffect_Format::EFFECT_PIXEL_FORMAT_NV21};
        OH_EffectFilterInfo_SetSupportedFormats(filterInfo_, sizeof(formats) / sizeof(ImageEffect_Format), formats);
    }

    void TearDown() override
    {
        delete pixelmapNative_;
        mockPixelMap_ = nullptr;

        pixelmapNative_ = nullptr;
        if (filterInfo_ != nullptr) {
            OH_EffectFilterInfo_Release(filterInfo_);
            filterInfo_ = nullptr;
        }
    }

    std::shared_ptr<PixelMap> mockPixelMap_;
    OH_PixelmapNative *pixelmapNative_ = nullptr;

    ImageEffect_FilterDelegate delegate_ = {
        .setValue = [](OH_EffectFilter *filter, const char *key, const ImageEffect_Any *value) { return true; },
        .render = [](OH_EffectFilter *filter, OH_EffectBufferInfo *src, OH_EffectFilterDelegate_PushData pushData) {
            void *addr = nullptr;
            (void)OH_EffectBufferInfo_GetAddr(src, &addr);
            int32_t width = 0;
            (void)OH_EffectBufferInfo_GetWidth(src, &width);
            int32_t height = 0;
            (void)OH_EffectBufferInfo_GetHeight(src, &height);
            int32_t rowSize = 0;
            (void)OH_EffectBufferInfo_GetRowSize(src, &rowSize);
            ImageEffect_Format format = ImageEffect_Format::EFFECT_PIXEL_FORMAT_UNKNOWN;
            (void)OH_EffectBufferInfo_GetEffectFormat(src, &format);
            int64_t timestamp = 0;
            (void)OH_EffectBufferInfo_GetTimestamp(src, &timestamp);

            (void)OH_EffectBufferInfo_SetAddr(src, addr);
            (void)OH_EffectBufferInfo_SetWidth(src, width);
            (void)OH_EffectBufferInfo_SetHeight(src, height);
            (void)OH_EffectBufferInfo_SetRowSize(src, rowSize);
            (void)OH_EffectBufferInfo_SetEffectFormat(src, format);
            (void)OH_EffectBufferInfo_SetTimestamp(src, timestamp);

            pushData(filter, src);
            return true;
        },
        .save = [](OH_EffectFilter *filter, char **info) {
            EffectJsonPtr root = EffectJsonHelper::CreateObject();
            root->Put("name", std::string(CUSTOM_BRIGHTNESS_EFILTER));
            std::string infoStr = root->ToString();
            char *infoChar = static_cast<char *>(malloc(infoStr.length() + 1));
            infoChar[infoStr.length()] = '\0';
            auto res = strcpy_s(infoChar, infoStr.length() + 1, infoStr.c_str());
            if (res != 0) {
                return false;
            }
            *info = infoChar;
            return true;
        },
        .restore = [](const char *info) {
            OH_EffectFilter *filter = OH_EffectFilter_Create(CUSTOM_BRIGHTNESS_EFILTER);
            ImageEffect_Any value;
            value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
            value.dataValue.floatValue = 50.f;
            OH_EffectFilter_SetValue(filter, BRIGHTNESS_EFILTER, &value);
            return filter;
        }
    };

    OH_EffectFilterInfo *filterInfo_ = nullptr;
    static inline sptr<Surface> consumerSurface_;
    static inline sptr<Surface> ohSurface_;
    static inline std::shared_ptr<OH_NativeBuffer> nativeBufferRgba_;
    static inline std::shared_ptr<OH_NativeBuffer> nativeBufferNV12_;
    static inline std::shared_ptr<OH_NativeBuffer> nativeBufferNV21_;
};

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputUri with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputUri with normal parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetInputUri001, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetInputUri(imageEffect, g_jpgUri.c_str());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputUri with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputUri with all empty parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetInputUri002, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputUri(nullptr, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputUri with empty OH_ImageEffect
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputUri with empty OH_ImageEffect
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetInputUri003, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputUri(nullptr, g_jpgUri.c_str());
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputUri with empty path
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputUri with empty path
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetInputUri004, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetInputUri(imageEffect, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputUri with unsupport path
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputUri with unsupport path
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetInputUri005, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetInputUri(imageEffect, g_notJpgUri.c_str());
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputUri with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputUri with normal parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetOutputUri001, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetInputUri(imageEffect, g_jpgUri.c_str());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetOutputUri(imageEffect, g_jpgUri.c_str());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputUri with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputUri with all empty parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetOutputUri002, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputUri(imageEffect, g_jpgUri.c_str());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetOutputUri(nullptr, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputUri with empty OH_ImageEffect
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputUri with empty OH_ImageEffect
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetOutputUri003, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputUri(imageEffect, g_jpgUri.c_str());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetOutputUri(nullptr, g_jpgUri.c_str());
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputUri with empty path
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputUri with empty path
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetOutputUri004, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetInputUri(imageEffect, g_jpgUri.c_str());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetOutputUri(imageEffect, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputUri with unsupport path
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputUri with unsupport path
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetOutputUri005, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetInputUri(imageEffect, g_jpgUri.c_str());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetOutputUri(imageEffect, g_notJpgUri.c_str());
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputUri with no OH_ImageEffect_SetInputUri
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputUri with no OH_ImageEffect_SetInputUri
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetOutputUri006, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetOutputUri(imageEffect, g_jpgUri.c_str());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS); // not set input data

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputNativeBuffer with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputNativeBuffer with normal parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetInputNativeBufferUnittest001, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetInputNativeBuffer(imageEffect, nativeBufferRgba_.get());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputNativeBuffer with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputNativeBuffer with all empty parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetInputNativeBufferUnittest002, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputNativeBuffer(nullptr, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputNativeBuffer with empty OH_ImageEffect
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputNativeBuffer with empty OH_ImageEffect
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetInputNativeBufferUnittest003, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_NativeBuffer *nativeBuffer = nativeBufferRgba_.get();
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputNativeBuffer(nullptr, nativeBuffer);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputNativeBuffer with empty OH_NativeBuffer
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputNativeBuffer with empty OH_NativeBuffer
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetInputNativeBufferUnittest004, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetInputNativeBuffer(imageEffect, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputNativeBuffer with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputNativeBuffer with normal parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetOutputNativeBuffer001, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_NativeBuffer *inNativeBuffer = nativeBufferRgba_.get();
    errorCode = OH_ImageEffect_SetInputNativeBuffer(imageEffect, inNativeBuffer);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_NativeBuffer *outNativeBuffer = nativeBufferRgba_.get();
    errorCode = OH_ImageEffect_SetOutputNativeBuffer(imageEffect, outNativeBuffer);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputNativeBuffer with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputNativeBuffer with all empty parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetOutputNativeBuffer002, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_NativeBuffer *inNativeBuffer = nativeBufferRgba_.get();
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputNativeBuffer(imageEffect, inNativeBuffer);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetOutputNativeBuffer(nullptr, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputNativeBuffer with empty OH_ImageEffect
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputNativeBuffer with empty OH_ImageEffect
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetOutputNativeBuffer003, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_NativeBuffer *inNativeBuffer = nativeBufferRgba_.get();
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputNativeBuffer(imageEffect, inNativeBuffer);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_NativeBuffer *outNativeBuffer = nativeBufferRgba_.get();
    errorCode = OH_ImageEffect_SetOutputNativeBuffer(nullptr, outNativeBuffer);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputNativeBuffer with empty OH_NativeBuffer
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputNativeBuffer with empty OH_NativeBuffer
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetOutputNativeBuffer004, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_NativeBuffer *inNativeBuffer = nativeBufferRgba_.get();
    errorCode = OH_ImageEffect_SetInputNativeBuffer(imageEffect, inNativeBuffer);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetOutputNativeBuffer(imageEffect, nullptr);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputNativeBuffer with no OH_ImageEffect_SetInputNativeBuffer
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputNativeBuffer with no OH_ImageEffect_SetInputNativeBuffer
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetOutputNativeBuffer005, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_NativeBuffer *outNativeBuffer = nativeBufferRgba_.get();
    errorCode = OH_ImageEffect_SetOutputNativeBuffer(imageEffect, outNativeBuffer);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS); // not set input data

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilters with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilters with normal parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHEFilterLookupFilterInfo001, TestSize.Level1)
{
    ImageEffect_Format formats[] = { ImageEffect_Format::EFFECT_PIXEL_FORMAT_NV12,
        ImageEffect_Format::EFFECT_PIXEL_FORMAT_NV21};
    OH_EffectFilterInfo_SetSupportedFormats(filterInfo_, sizeof(formats) / sizeof(ImageEffect_Format), formats);
    ImageEffect_FilterDelegate delegate = {
        .setValue = [](OH_EffectFilter *filter, const char *key, const ImageEffect_Any *value) { return true; },
        .render = [](OH_EffectFilter *filter, OH_EffectBufferInfo *src, OH_EffectFilterDelegate_PushData pushData) {
            pushData(filter, src);
            return true;
        },
        .save = [](OH_EffectFilter *filter, char **info) { return true; },
        .restore = [](const char *info) { return OH_EffectFilter_Create(BRIGHTNESS_EFILTER); }
    };

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Register(filterInfo_, &delegate);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    const char *name = BRIGHTNESS_EFILTER;
    OH_EffectFilterInfo *info = OH_EffectFilterInfo_Create();
    errorCode = OH_EffectFilter_LookupFilterInfo(name, info);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    char *filterName = nullptr;
    OH_EffectFilterInfo_GetFilterName(info, &filterName);
    ASSERT_STREQ(filterName, BRIGHTNESS_EFILTER);
    uint32_t bufferTypeSize = 0;
    ImageEffect_BufferType *bufferTypeArray = nullptr;
    OH_EffectFilterInfo_GetSupportedBufferTypes(info, &bufferTypeSize, &bufferTypeArray);
    ASSERT_EQ(bufferTypeSize, 1);
    uint32_t formatSize = 0;
    ImageEffect_Format *formatArray = nullptr;
    OH_EffectFilterInfo_GetSupportedFormats(info, &formatSize, &formatArray);
    ASSERT_EQ(formatSize, 2);
    OH_EffectFilterInfo_Release(info);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilters with not support name
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilters with not support name
 */
HWTEST_F(NativeImageEffectUnittest, OHEFilterLookupFilterInfo002, TestSize.Level1)
{
    ImageEffect_FilterDelegate delegate = {
        .setValue = [](OH_EffectFilter *filter, const char *key, const ImageEffect_Any *value) { return true; },
        .render = [](OH_EffectFilter *filter, OH_EffectBufferInfo *src, OH_EffectFilterDelegate_PushData pushData) {
            pushData(filter, src);
            return true;
        },
        .save = [](OH_EffectFilter *filter, char **info) { return true; },
        .restore = [](const char *info) { return OH_EffectFilter_Create(BRIGHTNESS_EFILTER); }
    };

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Register(filterInfo_, &delegate);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    const char *name = "TestEFilter";
    OH_EffectFilterInfo *info = OH_EffectFilterInfo_Create();
    errorCode = OH_EffectFilter_LookupFilterInfo(name, info);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    OH_EffectFilterInfo_Release(info);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilters with not OH_EFilter_Register
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilters with not OH_EFilter_Register
 */
HWTEST_F(NativeImageEffectUnittest, OHEFilterLookupFilterInfo003, TestSize.Level1)
{
    const char *name = "TestEFilter";
    OH_EffectFilterInfo *info = OH_EffectFilterInfo_Create();
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_LookupFilterInfo(name, info);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    OH_EffectFilterInfo_Release(info);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilters with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilters with all empty parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHEFilterLookupFilterInfo004, TestSize.Level1)
{
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_LookupFilterInfo(nullptr, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilters with empty name
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilters with empty name
 */
HWTEST_F(NativeImageEffectUnittest, OHEFilterLookupFilterInfo005, TestSize.Level1)
{
    OH_EffectFilterInfo *info = OH_EffectFilterInfo_Create();
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_LookupFilterInfo(nullptr, info);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    OH_EffectFilterInfo_Release(info);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilters with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilters with normal parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHEFilterLookupFilters001, TestSize.Level1)
{
    ImageEffect_FilterNames *filterNames = OH_EffectFilter_LookupFilters("Format:default");
    const char **nameList = filterNames->nameList;
    uint32_t size = filterNames->size;

    ASSERT_NE(filterNames, nullptr);
    ASSERT_EQ(size, static_cast<uint32_t>(5));
    std::vector<string> filterNamesVector;
    for (uint32_t i = 0; i < size; i++) {
        filterNamesVector.emplace_back(nameList[i]);
    }

    auto brightnessIndex = std::find(filterNamesVector.begin(), filterNamesVector.end(), BRIGHTNESS_EFILTER);
    ASSERT_NE(brightnessIndex, filterNamesVector.end());

    auto contrastIndex = std::find(filterNamesVector.begin(), filterNamesVector.end(), CONTRAST_EFILTER);
    ASSERT_NE(contrastIndex, filterNamesVector.end());
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilters with empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilters with empty parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHEFilterLookupFilters002, TestSize.Level1)
{
    ImageEffect_FilterNames *filterNames= OH_EffectFilter_LookupFilters(nullptr);
    ASSERT_EQ(filterNames, nullptr);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilters with not support parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilters with not support parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHEFilterLookupFilters003, TestSize.Level1)
{
    bool result = OH_EffectFilter_LookupFilters("test");
    ASSERT_EQ(result, true);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilters with not support key parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilters with not support key parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHEFilterLookupFilters004, TestSize.Level1)
{
    bool result = OH_EffectFilter_LookupFilters("test:default");
    ASSERT_EQ(result, true);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilters with not support value parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilters with not support value parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHEFilterLookupFilters005, TestSize.Level1)
{
    bool result = OH_EffectFilter_LookupFilters("Category:test");
    ASSERT_EQ(result, true);
}

/**
 * Feature: ImageEffect
 * Function: Test CustomFilterAdjustmentSaveAndRestore with normal process
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test CustomFilterAdjustmentSaveAndRestore with normal process
 */
HWTEST_F(NativeImageEffectUnittest, CustomFilterAdjustmentSaveAndRestore001, TestSize.Level1)
{
    OH_EffectFilterInfo_SetFilterName(filterInfo_, CUSTOM_BRIGHTNESS_EFILTER);

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Register(filterInfo_, &delegate_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, CUSTOM_BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 50.f;
    errorCode = OH_EffectFilter_SetValue(filter, BRIGHTNESS_EFILTER, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    char *imageEffectInfo = nullptr;
    errorCode = OH_ImageEffect_Save(imageEffect, &imageEffectInfo);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    ASSERT_NE(imageEffectInfo, nullptr);
    std::string saveInfo = imageEffectInfo;

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    imageEffect = OH_ImageEffect_Restore(saveInfo.c_str());
    ASSERT_NE(imageEffect, nullptr);

    errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test CustomTestFilterSave001 with non-utf-8 abnormal json object
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test CustomTestFilterSave001 with non-utf-8 abnormal json object
 */
HWTEST_F(NativeImageEffectUnittest, CustomTestFilterSave001, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, CUSTOM_TEST_EFILTER);
    ASSERT_NE(filter, nullptr);

    char value[] = { static_cast<char>(0xb2), static_cast<char>(0xe2), static_cast<char>(0xca),
        static_cast<char>(0xd4), '\0' }; // ANSI encode data

    ImageEffect_Any any = {
        .dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_PTR,
        .dataValue.ptrValue = static_cast<void *>(value),
    };
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &any);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    char *info = nullptr;
    errorCode = OH_ImageEffect_Save(imageEffect, &info);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    ASSERT_NE(info, nullptr);

    std::string data = info;
    EffectJsonPtr root = EffectJsonHelper::ParseJsonData(data);
    ASSERT_NE(root, nullptr);

    EffectJsonPtr imageInfo = root->GetElement("imageEffect");
    ASSERT_NE(imageInfo, nullptr);

    std::vector<EffectJsonPtr> filters = imageInfo->GetArray("filters");
    ASSERT_EQ(filters.size(), 1);

    EffectJsonPtr values = filters[0]->GetElement("values");
    ASSERT_NE(values, nullptr);

    std::string parsedValue = values->GetString(KEY_FILTER_INTENSITY);
    ASSERT_STREQ(parsedValue.c_str(), value);
}

HWTEST_F(NativeImageEffectUnittest, OHImageEffectDataTypeSurface001, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetOutputSurface(imageEffect, g_nativeWindow);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    NativeWindow *nativeWindow = nullptr;
    errorCode = OH_ImageEffect_GetInputSurface(imageEffect, &nativeWindow);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    ASSERT_NE(nativeWindow, nullptr);
    
    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_ImageEffect_Release(imageEffect);
    OH_NativeWindow_DestroyNativeWindow(nativeWindow);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputNativeBuffer with brightness yuv nv21
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputNativeBuffer with brightness yuv nv21
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectYuvUnittest001, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 50.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_NativeBuffer *nativeBuffer = nativeBufferNV21_.get();
    errorCode = OH_ImageEffect_SetInputNativeBuffer(imageEffect, nativeBuffer);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    int32_t ipType = 2;
    ImageEffect_Any runningType;
    runningType.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_INT32;
    runningType.dataValue.int32Value = ipType;
    errorCode = OH_ImageEffect_Configure(imageEffect, "runningType", &runningType);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputNativeBuffer with yuv brightness yuv nv12
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputNativeBuffer with yuv brightness yuv nv12
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectYuvUnittest002, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 0.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_EffectFilter *contrastFilter = OH_ImageEffect_AddFilter(imageEffect, CONTRAST_EFILTER);
    ASSERT_NE(contrastFilter, nullptr);

    ImageEffect_Any contrastValue;
    contrastValue.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    contrastValue.dataValue.floatValue = 0.f;
    errorCode = OH_EffectFilter_SetValue(contrastFilter, KEY_FILTER_INTENSITY, &contrastValue);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_NativeBuffer *nativeBuffer = nativeBufferNV12_.get();
    errorCode = OH_ImageEffect_SetInputNativeBuffer(imageEffect, nativeBuffer);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    int32_t ipType = 2;
    ImageEffect_Any runningType;
    runningType.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_INT32;
    runningType.dataValue.int32Value = ipType;
    errorCode = OH_ImageEffect_Configure(imageEffect, "runningType", &runningType);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputNativeBuffer with brightness gpu yuv nv21
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputNativeBuffer with brightness gpu yuv nv21
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectYuvUnittest003, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 0.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_EffectFilter *contrastFilter = OH_ImageEffect_AddFilter(imageEffect, CONTRAST_EFILTER);
    ASSERT_NE(contrastFilter, nullptr);

    ImageEffect_Any contrastValue;
    contrastValue.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    contrastValue.dataValue.floatValue = 0.f;
    errorCode = OH_EffectFilter_SetValue(contrastFilter, KEY_FILTER_INTENSITY, &contrastValue);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_NativeBuffer *nativeBuffer = nativeBufferNV21_.get();
    errorCode = OH_ImageEffect_SetInputNativeBuffer(imageEffect, nativeBuffer);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    int32_t ipType = 1;
    ImageEffect_Any runningType;
    runningType.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_INT32;
    runningType.dataValue.int32Value = ipType;
    errorCode = OH_ImageEffect_Configure(imageEffect, "runningType", &runningType);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputNativeBuffer with contrast gpu yuv nv21
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputNativeBuffer with contrast gpu yuv nv21
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectYuvUnittest004, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, CONTRAST_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 50.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_NativeBuffer *nativeBuffer = nativeBufferNV21_.get();
    errorCode = OH_ImageEffect_SetInputNativeBuffer(imageEffect, nativeBuffer);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    int32_t ipType = 1;
    ImageEffect_Any runningType;
    runningType.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_INT32;
    runningType.dataValue.int32Value = ipType;
    errorCode = OH_ImageEffect_Configure(imageEffect, "runningType", &runningType);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Hdr with BRIGHTNESS_EFILTER
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Hdr with BRIGHTNESS_EFILTER
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectHdr001, TestSize.Level1)
{
    std::shared_ptr<OH_PixelmapNative> pixelmapNative = std::make_shared<OH_PixelmapNative>(nullptr);
    std::unique_ptr<PixelMap> pixelMap = TestPixelMapUtils::ParsePixelMapByPath(g_jpgHdrPath);
    pixelmapNative->pixelmap_ = std::move(pixelMap);
    
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    int32_t ipType = 2;
    ImageEffect_Any runningType;
    runningType.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_INT32;
    runningType.dataValue.int32Value = ipType;
    errorCode = OH_ImageEffect_Configure(imageEffect, "runningType", &runningType);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Hdr with CROP_EFILTER
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Hdr with CROP_EFILTER
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectHdr002, TestSize.Level1)
{
    std::shared_ptr<OH_PixelmapNative> pixelmapNative = std::make_shared<OH_PixelmapNative>(nullptr);
    std::unique_ptr<PixelMap> pixelMap = TestPixelMapUtils::ParsePixelMapByPath(g_jpgHdrPath);
    pixelmapNative->pixelmap_ = std::move(pixelMap);
    
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, CROP_EFILTER);
    ASSERT_NE(filter, nullptr);

    uint32_t x1 = static_cast<uint32_t>(pixelmapNative->pixelmap_->GetWidth() / CROP_FACTOR);
    uint32_t y1 = static_cast<uint32_t>(pixelmapNative->pixelmap_->GetHeight() / CROP_FACTOR);
    uint32_t areaInfo[] = { 0, 0, x1, y1};
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_PTR;
    value.dataValue.ptrValue = static_cast<void *>(areaInfo);
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_REGION, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, pixelmapNative.get());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Hdr with SetOutputPixelmap
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Hdr with SetOutputPixelmap
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectHdr003, TestSize.Level1)
{
    std::shared_ptr<OH_PixelmapNative> pixelmapNative = std::make_shared<OH_PixelmapNative>(nullptr);
    std::unique_ptr<PixelMap> pixelMap = TestPixelMapUtils::ParsePixelMapByPath(g_jpgHdrPath);
    ASSERT_NE(pixelMap, nullptr);
    pixelmapNative->pixelmap_ = std::move(pixelMap);
    
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, pixelmapNative.get());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetOutputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    int32_t ipType = 2;
    ImageEffect_Any runningType;
    runningType.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_INT32;
    runningType.dataValue.int32Value = ipType;
    errorCode = OH_ImageEffect_Configure(imageEffect, "runningType", &runningType);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    if (pixelmapNative->pixelmap_->IsHdr()) {
        ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    } else {
        ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    }

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Hdr with ConverCPU2GPU and ConverGPU2CPU
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Hdr with ConverCPU2GPU and ConverGPU2CPU
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectHdr004, TestSize.Level1)
{
    std::shared_ptr<OH_PixelmapNative> pixelmapNative = std::make_shared<OH_PixelmapNative>(nullptr);
    std::unique_ptr<PixelMap> pixelMap = TestPixelMapUtils::ParsePixelMapByPath(g_jpgHdrPath);
    ASSERT_NE(pixelMap, nullptr);
    pixelmapNative->pixelmap_ = std::move(pixelMap);
    
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *customfilter = OH_ImageEffect_AddFilter(imageEffect, CUSTOM_TEST_EFILTER);
    ASSERT_NE(customfilter, nullptr);

    customfilter = OH_ImageEffect_AddFilter(imageEffect, CUSTOM_TEST_EFILTER2);
    ASSERT_NE(customfilter, nullptr);

    customfilter = OH_ImageEffect_AddFilter(imageEffect, CUSTOM_TEST_EFILTER);
    ASSERT_NE(customfilter, nullptr);

    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, pixelmapNative.get());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    int32_t ipType = 1;
    ImageEffect_Any runningType;
    runningType.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_INT32;
    runningType.dataValue.int32Value = ipType;
    errorCode = OH_ImageEffect_Configure(imageEffect, "runningType", &runningType);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    if (pixelmapNative->pixelmap_->IsHdr()) {
        ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    } else {
        ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    }

    errorCode = OH_ImageEffect_Stop(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Hdr with Adobe
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Hdr with Adobe
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectAdobe001, TestSize.Level1)
{
    std::shared_ptr<OH_PixelmapNative> pixelmapNative = std::make_shared<OH_PixelmapNative>(nullptr);
    std::unique_ptr<PixelMap> pixelMap = TestPixelMapUtils::ParsePixelMapByPath(g_adobeHdrPath);
    pixelmapNative->pixelmap_ = std::move(pixelMap);
    
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, pixelmapNative.get());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    int32_t ipType = 2;
    ImageEffect_Any runningType;
    runningType.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_INT32;
    runningType.dataValue.int32Value = ipType;
    errorCode = OH_ImageEffect_Configure(imageEffect, "runningType", &runningType);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect with picture
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect with picture
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectPicture001, TestSize.Level1)
{
    std::shared_ptr<Picture> picture = std::make_shared<MockPicture>();
    std::shared_ptr<OH_PictureNative> pictureNative = std::make_shared<OH_PictureNative>(picture);
 
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);
 
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, OH_EFFECT_CONTRAST_FILTER);
    ASSERT_NE(filter, nullptr);
 
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
 
    errorCode = OH_ImageEffect_SetInputPicture(imageEffect, pictureNative.get());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
 
    // start with input
    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
 
    // start with in and out(input equal to output)
    errorCode = OH_ImageEffect_SetOutputPicture(imageEffect, pictureNative.get());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
 
    // start with in and out
    std::shared_ptr<Picture> outputPicture = std::make_shared<MockPicture>();
    std::shared_ptr<OH_PictureNative> outputPictureNative = std::make_shared<OH_PictureNative>(outputPicture);
    errorCode = OH_ImageEffect_SetOutputPicture(imageEffect, outputPictureNative.get());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
 
    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}
 
/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect with picture for abnormal input parameters.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect with picture for abnormal input parameters.
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectPicture002, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    std::shared_ptr<OH_ImageEffect> imageEffectPtr(imageEffect, [](OH_ImageEffect *imageEffect) {
        if (imageEffect != nullptr) {
            OH_ImageEffect_Release(imageEffect);
        }
    });
    std::shared_ptr<Picture> picture = std::make_shared<MockPicture>();
    std::shared_ptr<OH_PictureNative> pictureNative = std::make_shared<OH_PictureNative>(picture);
    std::shared_ptr<Picture> defaultPicture;
    std::shared_ptr<OH_PictureNative> abnormalPictureNative = std::make_shared<OH_PictureNative>(defaultPicture);
 
    ASSERT_NE(OH_ImageEffect_SetInputPicture(nullptr, pictureNative.get()), ImageEffect_ErrorCode::EFFECT_SUCCESS);
    ASSERT_NE(OH_ImageEffect_SetInputPicture(imageEffectPtr.get(), nullptr), ImageEffect_ErrorCode::EFFECT_SUCCESS);
    ASSERT_NE(OH_ImageEffect_SetInputPicture(imageEffectPtr.get(), abnormalPictureNative.get()),
        ImageEffect_ErrorCode::EFFECT_SUCCESS);
    ASSERT_EQ(OH_ImageEffect_SetInputPicture(imageEffectPtr.get(), pictureNative.get()),
        ImageEffect_ErrorCode::EFFECT_SUCCESS);
 
    ASSERT_NE(OH_ImageEffect_SetOutputPicture(nullptr, pictureNative.get()), ImageEffect_ErrorCode::EFFECT_SUCCESS);
    ASSERT_EQ(OH_ImageEffect_SetOutputPicture(imageEffectPtr.get(), nullptr), ImageEffect_ErrorCode::EFFECT_SUCCESS);
    ASSERT_EQ(OH_ImageEffect_SetOutputPicture(imageEffectPtr.get(), abnormalPictureNative.get()),
        ImageEffect_ErrorCode::EFFECT_SUCCESS);
    ASSERT_EQ(OH_ImageEffect_SetOutputPicture(imageEffectPtr.get(), pictureNative.get()),
        ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect with texture
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect with texture
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectTexture001, TestSize.Level1)
{
    std::shared_ptr<RenderEnvironment> renderEnv = std::make_shared<RenderEnvironment>();
    renderEnv->Init();
    renderEnv->BeginFrame();
 
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);
    std::shared_ptr<OH_ImageEffect> imageEffectPtr(imageEffect, [](OH_ImageEffect *imageEffect) {
        if (imageEffect != nullptr) {
            OH_ImageEffect_Release(imageEffect);
        }
    });

    std::shared_ptr<RenderTexture> input = renderEnv->RequestBuffer(1920, 1080, GL_RGBA8);
    ASSERT_NE(OH_ImageEffect_SetInputTextureId(nullptr, input->GetName(), ColorManager::ColorSpaceName::SRGB),
        ImageEffect_ErrorCode::EFFECT_SUCCESS);
    ASSERT_NE(OH_ImageEffect_SetInputTextureId(imageEffect, 0, ColorManager::ColorSpaceName::SRGB),
        ImageEffect_ErrorCode::EFFECT_SUCCESS);
 
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, OH_EFFECT_CONTRAST_FILTER);
    ASSERT_NE(filter, nullptr);
 
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
 
    ASSERT_EQ(OH_ImageEffect_SetInputTextureId(imageEffectPtr.get(), input->GetName(),
        ColorManager::ColorSpaceName::SRGB), ImageEffect_ErrorCode::EFFECT_SUCCESS);

    // start with input
    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
 
    // start with in and out(input equal to output)
    errorCode = OH_ImageEffect_SetOutputTextureId(imageEffect, input->GetName());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
 
    // start with in and out
    std::shared_ptr<RenderTexture> output = renderEnv->RequestBuffer(1920, 1080, GL_RGBA8);
    errorCode = OH_ImageEffect_SetOutputTextureId(imageEffect, output->GetName());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    renderEnv->Release();
    renderEnv->ReleaseParam();
    renderEnv = nullptr;
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect with texture for abnormal input paramters.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect with texture for abnormal input paramters
 */

HWTEST_F(NativeImageEffectUnittest, OHImageEffectTexture002, TestSize.Level1)
{
    std::shared_ptr<RenderEnvironment> renderEnv = std::make_shared<RenderEnvironment>();
    renderEnv->Init();
    renderEnv->BeginFrame();
 
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);
    std::shared_ptr<OH_ImageEffect> imageEffectPtr(imageEffect, [](OH_ImageEffect *imageEffect) {
        if (imageEffect != nullptr) {
            OH_ImageEffect_Release(imageEffect);
        }
    });

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, OH_EFFECT_CONTRAST_FILTER);
    ASSERT_NE(filter, nullptr);
 
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    std::shared_ptr<RenderTexture> output = renderEnv->RequestBuffer(1920, 1080, GL_RGBA8);
    errorCode = OH_ImageEffect_SetOutputTextureId(imageEffect, output->GetName());
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    std::shared_ptr<RenderTexture> input = renderEnv->RequestBuffer(1920, 1080, GL_RGBA8);
    ASSERT_EQ(OH_ImageEffect_SetInputTextureId(imageEffectPtr.get(), input->GetName(),
        ColorManager::ColorSpaceName::ADOBE_RGB), ImageEffect_ErrorCode::EFFECT_SUCCESS);
    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    std::shared_ptr<RenderTexture> input_err = renderEnv->RequestBuffer(1920, 1080, GL_RGB16F);
    ASSERT_EQ(OH_ImageEffect_SetInputTextureId(imageEffectPtr.get(), input_err->GetName(),
        ColorManager::ColorSpaceName::SRGB), ImageEffect_ErrorCode::EFFECT_SUCCESS);
    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    renderEnv->Release();
    renderEnv->ReleaseParam();
    renderEnv = nullptr;
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EFilter with texture.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EFilter with texture
 */

HWTEST_F(NativeImageEffectUnittest, OHEffectFilterRenderWithTexture001, TestSize.Level1)
{
    std::shared_ptr<RenderEnvironment> renderEnv = std::make_shared<RenderEnvironment>();
    renderEnv->Init();
    renderEnv->BeginFrame();
    OH_EffectFilter *filter = OH_EffectFilter_Create(OH_EFFECT_CONTRAST_FILTER);
    ASSERT_NE(filter, nullptr);
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_EffectFilter_RenderWithTextureId(filter, 0, 0, ColorManager::ColorSpaceName::SRGB);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    std::shared_ptr<RenderTexture> input = renderEnv->RequestBuffer(1920, 1080, GL_RGBA8);
    std::shared_ptr<RenderTexture> output = renderEnv->RequestBuffer(1920, 1080, GL_RGBA8);
    errorCode = OH_EffectFilter_RenderWithTextureId(filter, input->GetName(), input->GetName(),
        ColorManager::ColorSpaceName::SRGB);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_EffectFilter_RenderWithTextureId(filter, input->GetName(), output->GetName(),
        ColorManager::ColorSpaceName::ADOBE_RGB);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    std::shared_ptr<RenderTexture> input_error = renderEnv->RequestBuffer(1920, 1080, GL_RGB16F);
    errorCode = OH_EffectFilter_RenderWithTextureId(filter, input_error->GetName(), output->GetName(),
        ColorManager::ColorSpaceName::SRGB);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_EffectFilter_RenderWithTextureId(filter, input->GetName(), output->GetName(),
        ColorManager::ColorSpaceName::SRGB);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    errorCode = OH_EffectFilter_Release(filter);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
    renderEnv->Release();
    renderEnv->ReleaseParam();
    renderEnv = nullptr;
}

} // namespace Test
} // namespace Effect
} // namespace Media
} // namespace OHOS
