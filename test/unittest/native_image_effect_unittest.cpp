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
#include "mock_surface_buffer.h"
#include "mock_surface_buffer_yuv_nv21.h"
#include "mock_surface_buffer_yuv_nv12.h"
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

using namespace testing::ext;
using namespace OHOS::Media;
using ::testing::InSequence;
using ::testing::Mock;

static std::string g_jpgPath;
static std::string g_notJpgPath;
static std::string g_jpgUri;
static std::string g_notJpgUri;
static OHNativeWindow *g_nativeWindow = nullptr;

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

    ErrorCode Restore(const nlohmann::json &values) override
    {
        return ErrorCode::SUCCESS;
    }

    static std::shared_ptr<EffectInfo> GetEffectInfo(const std::string &name)
    {
        return std::make_shared<EffectInfo>();
    }

    ErrorCode Save(nlohmann::json &res) override
    {
        res["name"] = name_;
        nlohmann::json jsonValues;
        Plugin::Any any;
        auto it = values_.find(Test::KEY_FILTER_INTENSITY);
        if (it == values_.end()) {
            return ErrorCode::ERR_UNKNOWN;
        }

        auto value = Plugin::AnyCast<void *>(&it->second);
        if (value == nullptr) {
            return ErrorCode::ERR_UNKNOWN;
        }

        nlohmann::json jsonValue = *reinterpret_cast<char **>(value);
        jsonValues[it->first] = jsonValue;
        res["values"] = jsonValues;
        return ErrorCode::SUCCESS;
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
        consumerSurface_ = Surface::CreateSurfaceAsConsumer("UnitTest");
        sptr<IBufferProducer> producer = consumerSurface_->GetProducer();
        ohSurface_ = Surface::CreateSurfaceAsProducer(producer);
        g_nativeWindow = CreateNativeWindowFromSurface(&ohSurface_);
    }

    static void TearDownTestCase()
    {
        if (g_nativeWindow != nullptr) {
            DestoryNativeWindow(g_nativeWindow);
            g_nativeWindow = nullptr;
        }
        consumerSurface_ = nullptr;
        ohSurface_ = nullptr;
    }

    void SetUp() override
    {
        mockSurfaceBuffer_ = new MockSurfaceBuffer();
        mockSurfaceBufferYuvNv21_ = new MockSurfaceBufferYuvNv21();
        mockSurfaceBufferYuvNv12_ = new MockSurfaceBufferYuvNv12();
        mockPixelMap_ = std::make_shared<MockPixelMap>();
        pixelmapNative_ = new OH_PixelmapNative(mockPixelMap_);
        ExternLoader::Instance()->InitExt();
        EFilterFactory::Instance()->functions_.clear();
        EFilterFactory::Instance()->RegisterEFilter<BrightnessEFilter>(BRIGHTNESS_EFILTER);
        EFilterFactory::Instance()->RegisterEFilter<ContrastEFilter>(CONTRAST_EFILTER);
        EFilterFactory::Instance()->RegisterEFilter<CustomTestEFilter>(CUSTOM_TEST_EFILTER);
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

        mockSurfaceBuffer_ = nullptr;
        mockSurfaceBufferYuvNv21_ = nullptr;
        mockSurfaceBufferYuvNv12_ = nullptr;
        pixelmapNative_ = nullptr;
        if (filterInfo_ != nullptr) {
            OH_EffectFilterInfo_Release(filterInfo_);
            filterInfo_ = nullptr;
        }
    }

    sptr<SurfaceBuffer> mockSurfaceBuffer_;
    sptr<SurfaceBuffer> mockSurfaceBufferYuvNv21_;
    sptr<SurfaceBuffer> mockSurfaceBufferYuvNv12_;
    std::shared_ptr<PixelMap> mockPixelMap_;
    OH_PixelmapNative *pixelmapNative_ = nullptr;

    ImageEffect_FilterDelegate delegate_ = {
        .setValue = [](OH_EffectFilter *filter, const char *key, const ImageEffect_Any *value) { return true; },
        .render = [](OH_EffectFilter *filter, OH_EffectBufferInfo *src, OH_EffectFilterDelegate_PushData pushData) {
            pushData(filter, src);
            return true;
        },
        .save = [](OH_EffectFilter *filter, char **info) {
            nlohmann::json root;
            root["name"] = std::string(CUSTOM_BRIGHTNESS_EFILTER);
            std::string infoStr = root.dump();
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
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

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

    OH_NativeBuffer *nativeBuffer = mockSurfaceBuffer_->SurfaceBufferToNativeBuffer();
    errorCode = OH_ImageEffect_SetInputNativeBuffer(imageEffect, nativeBuffer);
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

    OH_NativeBuffer *nativeBuffer = mockSurfaceBuffer_->SurfaceBufferToNativeBuffer();
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

    OH_NativeBuffer *inNativeBuffer = mockSurfaceBuffer_->SurfaceBufferToNativeBuffer();
    errorCode = OH_ImageEffect_SetInputNativeBuffer(imageEffect, inNativeBuffer);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_NativeBuffer *outNativeBuffer = mockSurfaceBuffer_->SurfaceBufferToNativeBuffer();
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

    OH_NativeBuffer *inNativeBuffer = mockSurfaceBuffer_->SurfaceBufferToNativeBuffer();
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

    OH_NativeBuffer *inNativeBuffer = mockSurfaceBuffer_->SurfaceBufferToNativeBuffer();
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputNativeBuffer(imageEffect, inNativeBuffer);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_NativeBuffer *outNativeBuffer = mockSurfaceBuffer_->SurfaceBufferToNativeBuffer();
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

    OH_NativeBuffer *inNativeBuffer = mockSurfaceBuffer_->SurfaceBufferToNativeBuffer();
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

    OH_NativeBuffer *outNativeBuffer = mockSurfaceBuffer_->SurfaceBufferToNativeBuffer();
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
    ASSERT_EQ(size, static_cast<uint32_t>(3));

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
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
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

    OH_NativeBuffer *nativeBuffer = mockSurfaceBufferYuvNv21_->SurfaceBufferToNativeBuffer();
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
    value.dataValue.floatValue = 50.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_NativeBuffer *nativeBuffer = mockSurfaceBufferYuvNv12_->SurfaceBufferToNativeBuffer();
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
    value.dataValue.floatValue = 50.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    OH_NativeBuffer *nativeBuffer = mockSurfaceBufferYuvNv21_->SurfaceBufferToNativeBuffer();
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

    OH_NativeBuffer *nativeBuffer = mockSurfaceBufferYuvNv21_->SurfaceBufferToNativeBuffer();
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
} // namespace Test
} // namespace Effect
} // namespace Media
} // namespace OHOS