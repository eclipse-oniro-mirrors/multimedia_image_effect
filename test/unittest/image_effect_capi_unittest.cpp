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

#include "image_effect_capi_unittest.h"
#include "image_effect.h"
#include "image_effect_filter.h"
#include "pixelmap_native_impl.h"
#include "efilter_factory.h"
#include "brightness_efilter.h"
#include "contrast_efilter.h"
#include "test_common.h"
#include "native_window.h"
#include "external_loader.h"

#define MAX_TEST_ADD_EFILTE_NUMS 120

using namespace testing::ext;
using ::testing::A;
using ::testing::InSequence;
using ::testing::Mock;
using namespace OHOS::Media::Effect::Test;

namespace OHOS {
namespace Media {
namespace Effect {
void ImageEffectCApiUnittest::SetUpTestCase()
{
    consumerSurface_ = Surface::CreateSurfaceAsConsumer("UnitTest");
    sptr<IBufferProducer> producer = consumerSurface_->GetProducer();
    ohSurface_ = Surface::CreateSurfaceAsProducer(producer);
    nativeWindow_ = CreateNativeWindowFromSurface(&ohSurface_);
}

void ImageEffectCApiUnittest::TearDownTestCase()
{
    if (nativeWindow_ != nullptr) {
        DestoryNativeWindow(nativeWindow_);
        nativeWindow_ = nullptr;
    }
    consumerSurface_ = nullptr;
    ohSurface_ = nullptr;
}

void ImageEffectCApiUnittest::SetUp()
{
    mockPixelMap_ = std::make_shared<MockPixelMap>();
    pixelmapNative_ = new OH_PixelmapNative(mockPixelMap_);
    ExternLoader::Instance()->InitExt();
    EFilterFactory::Instance()->functions_.clear();
    EFilterFactory::Instance()->RegisterEFilter<BrightnessEFilter>(BRIGHTNESS_EFILTER);
    EFilterFactory::Instance()->RegisterEFilter<ContrastEFilter>(CONTRAST_EFILTER);
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

void ImageEffectCApiUnittest::TearDown()
{
    delete pixelmapNative_;
    pixelmapNative_ = nullptr;
    mockPixelMap_ = nullptr;
    if (filterInfo_ != nullptr) {
        OH_EffectFilterInfo_Release(filterInfo_);
        filterInfo_ = nullptr;
    }
}

/**
 * Feature: ImageEffect
 * Function: Test image_effect capi unittest example
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test image_effect capi unittest example
 */
HWTEST_F(ImageEffectCApiUnittest, Image_effect_capi_unittest_001, TestSize.Level0)
{
    InSequence s;

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

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Create with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Create with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectCreate001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectCreate001 start";

    OH_ImageEffect *nativeImageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(nativeImageEffect, nullptr) << "OH_ImageEffect_Create failed";

    GTEST_LOG_(INFO) << "OHImageEffectCreate001 success! result: " << nativeImageEffect;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectCreate001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Create with empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Create with empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectCreate002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectCreate002 start";

    OH_ImageEffect *nativeImageEffect = OH_ImageEffect_Create(nullptr);
    ASSERT_NE(nativeImageEffect, nullptr) << "OH_ImageEffect_Create failed";

    GTEST_LOG_(INFO) << "OHImageEffectCreate002 success! result: " << nativeImageEffect;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectCreate002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Configure with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Configure with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectConfigure001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectConfigure001 start";

    OH_ImageEffect *nativeImageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    const char *key = "runningType";
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_INT32;
    value.dataValue.int32Value = 2;
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_Configure(nativeImageEffect, key, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_Configure failed";

    GTEST_LOG_(INFO) << "OHImageEffectConfigure001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectConfigure001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Configure with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Configure with all empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectConfigure002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectConfigure002 start";

    ImageEffect_ErrorCode errorCode = OH_ImageEffect_Configure(nullptr, nullptr, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_Configure failed";

    GTEST_LOG_(INFO) << "OHImageEffectConfigure002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectConfigure002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Configure with empty OH_ImageEffect
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Configure with empty OH_ImageEffect
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectConfigure003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectConfigure003 start";

    const char *key = KEY_FILTER_INTENSITY;
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_INT32;
    value.dataValue.int32Value = 1;
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_Configure(nullptr, key, &value);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_Configure failed";

    GTEST_LOG_(INFO) << "OHImageEffectConfigure003 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectConfigure003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Create with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Create with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterCreate001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterCreate001 start";

    OH_EffectFilter *nativeEFilter = OH_EffectFilter_Create(BRIGHTNESS_EFILTER);
    ASSERT_NE(nativeEFilter, nullptr) << "OH_EffectFilter_Create failed";

    GTEST_LOG_(INFO) << "OHEFilterCreate001 success! result: " << nativeEFilter;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterCreate001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Create with not exist parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Create with not exist parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterCreate002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterCreate002 start";

    OH_EffectFilter *nativeEFilter = OH_EffectFilter_Create("TestEFilter");
    ASSERT_EQ(nativeEFilter, nullptr) << "OH_EffectFilter_Create failed";

    GTEST_LOG_(INFO) << "OHEFilterCreate002 success! result: " << nativeEFilter;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterCreate002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Create with empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Create with empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterCreate003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterCreate003 start";

    OH_EffectFilter *nativeEFilter = OH_EffectFilter_Create(nullptr);
    ASSERT_EQ(nativeEFilter, nullptr) << "OH_EffectFilter_Create failed";

    GTEST_LOG_(INFO) << "OHEFilterCreate003 success! result: " << nativeEFilter;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterCreate003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_AddFilter with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_AddFilter with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectAddFilter001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectAddFilter001 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr) << "OH_ImageEffect_AddFilter failed";

    GTEST_LOG_(INFO) << "OHImageEffectAddFilter001 success! result: " << filter;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectAddFilter001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_AddFilter with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_AddFilter with all empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectAddFilter002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectAddFilter002 start";

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(nullptr, nullptr);
    ASSERT_EQ(filter, nullptr) << "OH_ImageEffect_AddFilter failed";

    GTEST_LOG_(INFO) << "OHImageEffectAddFilter002 success! result: " << filter;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectAddFilter002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_AddFilter with empty OH_ImageEffect parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_AddFilter with empty OH_ImageEffect parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectAddFilter003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectAddFilter003 start";

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(nullptr, BRIGHTNESS_EFILTER);
    ASSERT_EQ(filter, nullptr) << "OH_ImageEffect_AddFilter failed";

    GTEST_LOG_(INFO) << "OHImageEffectAddFilter003 success! result: " << filter;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectAddFilter003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_AddFilter with empty OH_EffectFilter parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_AddFilter with empty OH_EffectFilter parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectAddFilter004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectAddFilter004 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, nullptr);
    ASSERT_EQ(filter, nullptr) << "OH_ImageEffect_AddFilter failed";

    GTEST_LOG_(INFO) << "OHImageEffectAddFilter004 success! result: " << filter;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectAddFilter004 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_AddFilter with not exist OH_EffectFilter parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_AddFilter with not exist OH_EffectFilter parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectAddFilter005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectAddFilter005 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, "TestEFilter");
    ASSERT_EQ(filter, nullptr) << "OH_ImageEffect_AddFilter failed";

    GTEST_LOG_(INFO) << "OHImageEffectAddFilter005 success! result: " << filter;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectAddFilter005 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_AddFilter out of max nums
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_AddFilter out of max nums
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectAddFilter006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectAddFilter006 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = nullptr;
    for (int i = 0; i < MAX_TEST_ADD_EFILTE_NUMS; i++) {
        filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    }
    filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_EQ(filter, nullptr) << "OH_ImageEffect_AddFilter failed";

    GTEST_LOG_(INFO) << "OHImageEffectAddFilter006 success! result: " << filter;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectAddFilter006 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_RemoveFilter with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_RemoveFilter with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectRemoveFilter001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRemoveFilter001 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    GTEST_LOG_(INFO) << "OHImageEffectRemoveFilter001 OH_ImageEffect_AddFilter success! filter: " << filter;
    int32_t result = OH_ImageEffect_RemoveFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_EQ(result, 1) << "OH_ImageEffect_RemoveFilter failed";

    GTEST_LOG_(INFO) << "OHImageEffectRemoveFilter001 success! result: " << result;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRemoveFilter001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_RemoveFilter with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_RemoveFilter with all empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectRemoveFilter002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRemoveFilter002 start";

    int32_t result = OH_ImageEffect_RemoveFilter(nullptr, nullptr);
    ASSERT_EQ(result, 0) << "OH_ImageEffect_RemoveFilter failed";

    GTEST_LOG_(INFO) << "OHImageEffectRemoveFilter002 success! result: " << result;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRemoveFilter002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_RemoveFilter with empty OH_ImageEffect parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_RemoveFilter with empty OH_ImageEffect parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectRemoveFilter003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRemoveFilter003 start";

    int32_t result = OH_ImageEffect_RemoveFilter(nullptr, BRIGHTNESS_EFILTER);
    ASSERT_EQ(result, 0) << "OH_ImageEffect_RemoveFilter failed";

    GTEST_LOG_(INFO) << "OHImageEffectRemoveFilter003 success! result: " << result;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRemoveFilter003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_RemoveFilter with empty OH_EffectFilter parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_RemoveFilter with empty OH_EffectFilter parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectRemoveFilter004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRemoveFilter004 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    int32_t result = OH_ImageEffect_RemoveFilter(imageEffect, nullptr);
    ASSERT_EQ(result, 0) << "OH_ImageEffect_RemoveFilter failed";

    GTEST_LOG_(INFO) << "OHImageEffectRemoveFilter004 success! result: " << result;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRemoveFilter004 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_RemoveFilter with not exist OH_EffectFilter parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_RemoveFilter with not exist OH_EffectFilter parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectRemoveFilter005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRemoveFilter005 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    GTEST_LOG_(INFO) << "OHImageEffectRemoveFilter005 OH_ImageEffect_AddFilter success! filter: " << filter;
    int32_t result = OH_ImageEffect_RemoveFilter(imageEffect, "TestEFilter");
    ASSERT_EQ(result, 0) << "OH_ImageEffect_RemoveFilter failed";

    GTEST_LOG_(INFO) << "OHImageEffectRemoveFilter005 success! result: " << result;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRemoveFilter005 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_SetValue with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_SetValue with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterSetValue001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterSetValue001 start";
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    const char *key = KEY_FILTER_INTENSITY;
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = static_cast<float>(12);
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, key, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_SetValue failed";

    GTEST_LOG_(INFO) << "OHEFilterSetValue001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterSetValue001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_SetValue with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_SetValue with all empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterSetValue002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterSetValue002 start";

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(nullptr, nullptr, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_SetValue failed";

    GTEST_LOG_(INFO) << "OHEFilterSetValue002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterSetValue002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_SetValue with unexpected ImageEffect_Any.dataType parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_SetValue with unexpected ImageEffect_Any.dataType parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterSetValue003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterSetValue003 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    const char *key = KEY_FILTER_INTENSITY;
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_UNKNOWN;
    value.dataValue.charValue = 'A';
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, key, &value);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_SetValue failed";

    GTEST_LOG_(INFO) << "OHEFilterSetValue003 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterSetValue003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_SetValue with not exist key parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_SetValue with not exist key parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterSetValue004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterSetValue004 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    const char *key = "test";
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = static_cast<float>(12);
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, key, &value);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_SetValue failed";

    GTEST_LOG_(INFO) << "OHEFilterSetValue004 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterSetValue004 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_SetValue with empty key parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_SetValue with empty key parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterSetValue005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterSetValue005 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = static_cast<float>(12);
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, nullptr, &value);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_SetValue failed";

    GTEST_LOG_(INFO) << "OHEFilterSetValue005 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterSetValue005 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_SetValue with empty ImageEffect_Any parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_SetValue with empty ImageEffect_Any parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterSetValue006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterSetValue006 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    const char *key = KEY_FILTER_INTENSITY;
    ImageEffect_Any value;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, key, &value);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_SetValue failed";

    GTEST_LOG_(INFO) << "OHEFilterSetValue006 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterSetValue006 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_GetValue with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_GetValue with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterGetValue001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterGetValue001 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    const char *key = KEY_FILTER_INTENSITY;
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = static_cast<float>(12);
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, key, &value);
    ImageEffect_Any result;
    errorCode = OH_EffectFilter_GetValue(filter, key, &result);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_GetValue failed";
    ASSERT_EQ(result.dataValue.floatValue, static_cast<float>(12)) << "OH_EffectFilter_GetValue failed";

    GTEST_LOG_(INFO) << "OHEFilterGetValue001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterGetValue001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_GetValue with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_GetValue with all empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterGetValue002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterGetValue002 start";

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_GetValue(nullptr, nullptr, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_GetValue failed";

    GTEST_LOG_(INFO) << "OHEFilterGetValue002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterGetValue002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_GetValue with not exist key parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_GetValue with not exist key parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterGetValue003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterGetValue003 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    const char *key = KEY_FILTER_INTENSITY;
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = static_cast<float>(12);
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, key, &value);
    ImageEffect_Any result;
    errorCode = OH_EffectFilter_GetValue(filter, "test", &result);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_GetValue failed";
    ASSERT_NE(result.dataValue.floatValue, static_cast<float>(12)) << "OH_EffectFilter_GetValue failed";

    GTEST_LOG_(INFO) << "OHEFilterGetValue003 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterGetValue003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_GetValue with empty OH_EffectFilter parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_GetValue with empty OH_EffectFilter parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterGetValue004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterGetValue004 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    const char *key = KEY_FILTER_INTENSITY;
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = static_cast<float>(12);
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, key, &value);
    ImageEffect_Any result;
    errorCode = OH_EffectFilter_GetValue(nullptr, KEY_FILTER_INTENSITY, &result);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_GetValue failed";
    ASSERT_NE(result.dataValue.floatValue, static_cast<float>(12)) << "OH_EffectFilter_GetValue failed";

    GTEST_LOG_(INFO) << "OHEFilterGetValue004 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterGetValue004 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_GetValue with unobstructed OH_EffectFilter_SetValue func
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_GetValue with unobstructed OH_EffectFilter_SetValue func
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterGetValue005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterGetValue005 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    const char *key = KEY_FILTER_INTENSITY;
    ImageEffect_Any value;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_GetValue(filter, key, &value);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_GetValue failed";
    ASSERT_EQ(value.dataValue.floatValue, 0) << "OH_EffectFilter_GetValue failed";

    GTEST_LOG_(INFO) << "OHEFilterGetValue005 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterGetValue005 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_GetValue by FILTER_NAME
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_GetValue by FILTER_NAME
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterGetValue006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterGetValue006 start";
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);
    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);
    const char *key = "FILTER_NAME";
    ImageEffect_Any value;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_GetValue(filter, key, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_GetValue failed";
    ASSERT_EQ(value.dataType, ImageEffect_DataType::EFFECT_DATA_TYPE_PTR) << "OH_EffectFilter_GetValue failed";
    ASSERT_NE(value.dataValue.ptrValue, nullptr) << "OH_EffectFilter_GetValue failed";
    ASSERT_STREQ(static_cast<char *>(value.dataValue.ptrValue), BRIGHTNESS_EFILTER) <<
        "OH_EffectFilter_GetValue failed";

    GTEST_LOG_(INFO) << "OHEFilterGetValue006 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterGetValue006 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Render with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Render with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterRender001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRender001 start";

    OH_EffectFilter *filter = OH_EffectFilter_Create(BRIGHTNESS_EFILTER);
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Render(filter, pixelmapNative_, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_Render failed";

    GTEST_LOG_(INFO) << "OHEFilterRender001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRender001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Render with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Render with all empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterRender002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRender002 start";

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Render(nullptr, nullptr, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_Render failed";

    GTEST_LOG_(INFO) << "OHEFilterRender002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRender002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Render with empty OH_EffectFilter parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Render with empty OH_EffectFilter parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterRender003, TestSize.Level1)
{
    InSequence s;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRender003 start";

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Render(nullptr, pixelmapNative_, pixelmapNative_);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_Render failed";

    GTEST_LOG_(INFO) << "OHEFilterRender003 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRender003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Render with empty inputPixelmap, outputPixelmap parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Render with empty inputPixelmap, outputPixelmap parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterRender004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRender004 start";

    OH_EffectFilter *filter = OH_EffectFilter_Create(BRIGHTNESS_EFILTER);
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Render(filter, nullptr, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_Render failed";

    GTEST_LOG_(INFO) << "OHEFilterRender004 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRender004 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Render with empty outputPixelmap parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Render with empty outputPixelmap parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterRender005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRender005 start";
    InSequence s;

    OH_EffectFilter *filter = OH_EffectFilter_Create(BRIGHTNESS_EFILTER);

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Render(filter, pixelmapNative_, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_Render failed";

    GTEST_LOG_(INFO) << "OHEFilterRender005 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRender005 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Render with empty inputPixelmap parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Render with empty inputPixelmap parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterRender006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRender006 start";
    InSequence s;

    OH_EffectFilter *filter = OH_EffectFilter_Create(BRIGHTNESS_EFILTER);
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Render(filter, nullptr, pixelmapNative_);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_Render failed";

    GTEST_LOG_(INFO) << "OHEFilterRender006 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRender006 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Release with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Release with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterRelease001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRelease001 start";

    OH_EffectFilter *filter = OH_EffectFilter_Create(BRIGHTNESS_EFILTER);
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Release(filter);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_Release failed";

    GTEST_LOG_(INFO) << "OHEFilterRelease001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRelease001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Release with empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Release with empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterRelease002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRelease002 start";

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Release(nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_Release failed";

    GTEST_LOG_(INFO) << "OHEFilterRelease002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRelease002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Release with not exist OH_EffectFilter parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Release with not exist OH_EffectFilter parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterRelease003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRelease003 start";

    OH_EffectFilter *filter = OH_EffectFilter_Create("TestEFilter");
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Release(filter);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_Release failed";

    GTEST_LOG_(INFO) << "OHEFilterRelease003 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRelease003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffectSingleFilter with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffectSingleFilter with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, ImageEffectSingleFilterUnittest001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectSingleFilterUnittest001 start";
    InSequence s;

    OH_EffectFilter *filter = OH_EffectFilter_Create(BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr) << "ImageEffectSingleFilterUnittest001 OH_EffectFilter_Create failed";
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSingleFilterUnittest001 OH_EffectFilter_SetValue failed";

    errorCode = OH_EffectFilter_Render(filter, pixelmapNative_, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSingleFilterUnittest001 OH_EffectFilter_Render failed";

    errorCode = OH_EffectFilter_Release(filter);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSingleFilterUnittest001 OH_EffectFilter_Release failed";

    GTEST_LOG_(INFO) << "ImageEffectSingleFilterUnittest001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectSingleFilterUnittest001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffectSingleFilter submethod OH_EffectFilter_Create with not exist OH_EffectFilter parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffectSingleFilter submethod OH_EffectFilter_Create with not exist OH_EffectFilter
 * parameter
 */
HWTEST_F(ImageEffectCApiUnittest, ImageEffectSingleFilterUnittest002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectSingleFilterUnittest002 start";
    InSequence s;

    OH_EffectFilter *filter = OH_EffectFilter_Create("TestEFilter");
    ASSERT_EQ(filter, nullptr) << "ImageEffectSingleFilterUnittest002 OH_EffectFilter_Create failed";
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSingleFilterUnittest002 OH_EffectFilter_SetValue failed";

    errorCode = OH_EffectFilter_Render(filter, pixelmapNative_, pixelmapNative_);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSingleFilterUnittest002 OH_EffectFilter_Render failed";

    errorCode = OH_EffectFilter_Release(filter);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSingleFilterUnittest002 OH_EffectFilter_Release failed";

    GTEST_LOG_(INFO) << "ImageEffectSingleFilterUnittest002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectSingleFilterUnittest002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffectSingleFilter submethod OH_EffectFilter_SetValue with not exist key parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffectSingleFilter submethod OH_EffectFilter_SetValue with not exist key parameter
 */
HWTEST_F(ImageEffectCApiUnittest, ImageEffectSingleFilterUnittest003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectSingleFilterUnittest003 start";
    InSequence s;

    OH_EffectFilter *filter = OH_EffectFilter_Create(BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr) << "ImageEffectSingleFilterUnittest003 OH_EffectFilter_Create failed";
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, "testRatio", &value);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSingleFilterUnittest003 OH_EffectFilter_SetValue failed";

    errorCode = OH_EffectFilter_Render(filter, pixelmapNative_, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSingleFilterUnittest003 OH_EffectFilter_Render failed";

    errorCode = OH_EffectFilter_Release(filter);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSingleFilterUnittest003 OH_EffectFilter_Release failed";

    GTEST_LOG_(INFO) << "ImageEffectSingleFilterUnittest003 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectSingleFilterUnittest003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffectSingleFilter submethod OH_EffectFilter_Render unobstructed
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffectSingleFilter submethod OH_EffectFilter_Render unobstructed
 */
HWTEST_F(ImageEffectCApiUnittest, ImageEffectSingleFilterUnittest004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectSingleFilterUnittest004 start";

    OH_EffectFilter *filter = OH_EffectFilter_Create(BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr) << "ImageEffectSingleFilterUnittest004 OH_EffectFilter_Create failed";
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSingleFilterUnittest004 OH_EffectFilter_SetValue failed";

    errorCode = OH_EffectFilter_Release(filter);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSingleFilterUnittest004 OH_EffectFilter_Release failed";

    GTEST_LOG_(INFO) << "ImageEffectSingleFilterUnittest004 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectSingleFilterUnittest004 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputPixelmap with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputPixelmap with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSetInputPixelmap001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetInputPixelmap001 start";
    InSequence s;

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_SetInputPixelmap failed";

    GTEST_LOG_(INFO) << "OHImageEffectSetInputPixelmap001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetInputPixelmap001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputPixelmap with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputPixelmap with all empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSetInputPixelmap002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetInputPixelmap002 start";

    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputPixelmap(nullptr, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_SetInputPixelmap failed";

    GTEST_LOG_(INFO) << "OHImageEffectSetInputPixelmap002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetInputPixelmap002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputPixelmap with empty OH_PixelmapNative parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputPixelmap with empty OH_PixelmapNative parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSetInputPixelmap003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetInputPixelmap003 start";
    InSequence s;

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_SetInputPixelmap failed";

    GTEST_LOG_(INFO) << "OHImageEffectSetInputPixelmap003 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetInputPixelmap003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputPixelmap with empty OH_ImageEffect parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputPixelmap with empty OH_ImageEffect parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSetInputPixelmap004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetInputPixelmap004 start";
    InSequence s;

    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputPixelmap(nullptr, pixelmapNative_);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_SetInputPixelmap failed";

    GTEST_LOG_(INFO) << "OHImageEffectSetInputPixelmap004 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetInputPixelmap004 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputPixelmap with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputPixelmap with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSetOutputPixelMap001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputPixelMap001 start";
    InSequence s;

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetOutputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_SetOutputPixelmap failed";

    GTEST_LOG_(INFO) << "OHImageEffectSetOutputPixelMap001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputPixelMap001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputPixelmap with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputPixelmap with all empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSetOutputPixelMap002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputPixelMap002 start";

    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetOutputPixelmap(nullptr, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_SetOutputPixelmap failed";

    GTEST_LOG_(INFO) << "OHImageEffectSetOutputPixelMap002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputPixelMap002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputPixelmap with empty OH_PixelmapNative parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputPixelmap with empty OH_PixelmapNative parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSetOutputPixelMap003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputPixelMap003 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetOutputPixelmap(imageEffect, nullptr);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_SetOutputPixelmap failed";

    GTEST_LOG_(INFO) << "OHImageEffectSetOutputPixelMap003 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputPixelMap003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputPixelmap with empty OH_ImageEffect parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputPixelmap with empty OH_ImageEffect parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSetOutputPixelMap004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputPixelMap004 start";
    InSequence s;

    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetOutputPixelmap(nullptr, pixelmapNative_);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_SetOutputPixelmap failed";

    GTEST_LOG_(INFO) << "OHImageEffectSetOutputPixelMap004 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputPixelMap004 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Start with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Start with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectStart001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectStart001 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_Start failed";

    GTEST_LOG_(INFO) << "OHImageEffectStart001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectStart001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Start with empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Start with empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectStart002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectStart002 start";

    ImageEffect_ErrorCode errorCode = OH_ImageEffect_Start(nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_Start failed";

    GTEST_LOG_(INFO) << "OHImageEffectStart002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectStart002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Release with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Release with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectRelease001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRelease001 start";
  
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OHImageEffectRelease001 failed";

    GTEST_LOG_(INFO) << "OHImageEffectRelease001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRelease001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Release with empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Release with empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectRelease002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRelease002 start";
  
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_Release(nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_Release002 failed";

    GTEST_LOG_(INFO) << "OHImageEffectRelease002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRelease002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffectStandardFilter with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffectStandardFilter with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, ImageEffectStandardFilterUnittest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectStandardFilterUnittest001 start";
    InSequence s;

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr) << "ImageEffectStandardFilterUnittest001 OH_ImageEffect_Create failed";

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr) << "ImageEffectStandardFilterUnittest001 OH_ImageEffect_AddFilter failed";

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest001 OH_EffectFilter_SetValue failed";

    errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest001 OH_ImageEffect_SetInputPixelmap failed";
    errorCode = OH_ImageEffect_SetOutputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest001 OH_ImageEffect_SetOutputPixelmap failed";

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest001 OH_ImageEffect_Start failed";

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest001 OH_ImageEffect_Release failed";

    GTEST_LOG_(INFO) << "ImageEffectStandardFilterUnittest001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectStandardFilterUnittest001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffectStandardFilter submethod OH_ImageEffect_AddFilter with not exist OH_EffectFilter parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffectStandardFilter submethod OH_ImageEffect_AddFilter with not exist OH_EffectFilter
 * parameter
 */
HWTEST_F(ImageEffectCApiUnittest, ImageEffectStandardFilterUnittest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectStandardFilterUnittest002 start";
    InSequence s;

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr) << "ImageEffectStandardFilterUnittest002 OH_ImageEffect_Create failed";

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, "TestEFilter");
    ASSERT_EQ(filter, nullptr) << "ImageEffectStandardFilterUnittest002 OH_ImageEffect_AddFilter failed";

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest002 OH_EffectFilter_SetValue failed";

    errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest002 OH_ImageEffect_SetInputPixelmap failed";
    errorCode = OH_ImageEffect_SetOutputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest002 OH_ImageEffect_SetOutputPixelmap failed";

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest002 OH_ImageEffect_Start failed";

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest002 OH_ImageEffect_Release failed";

    GTEST_LOG_(INFO) << "ImageEffectStandardFilterUnittest002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectStandardFilterUnittest002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffectStandardFilter submethod OH_EffectFilter_SetValue with not exist key parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffectStandardFilter submethod OH_EffectFilter_SetValue with not exist key parameter
 */
HWTEST_F(ImageEffectCApiUnittest, ImageEffectStandardFilterUnittest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectStandardFilterUnittest003 start";
    InSequence s;

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr) << "ImageEffectStandardFilterUnittest003 OH_ImageEffect_Create failed";

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr) << "ImageEffectStandardFilterUnittest003 OH_ImageEffect_AddFilter failed";

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, "test", &value);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest003 OH_EffectFilter_SetValue failed";

    errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest003 OH_ImageEffect_SetInputPixelmap failed";
    errorCode = OH_ImageEffect_SetOutputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest003 OH_ImageEffect_SetOutputPixelmap failed";

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest003 OH_ImageEffect_Start failed";

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest003 OH_ImageEffect_Release failed";

    GTEST_LOG_(INFO) << "ImageEffectStandardFilterUnittest003 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectStandardFilterUnittest003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffectStandardFilter submethod OH_ImageEffect_Start with empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffectStandardFilter submethod OH_ImageEffect_Start with empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, ImageEffectStandardFilterUnittest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectStandardFilterUnittest004 start";
    InSequence s;

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr) << "ImageEffectStandardFilterUnittest004 OH_ImageEffect_Create failed";

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr) << "ImageEffectStandardFilterUnittest004 OH_ImageEffect_AddFilter failed";

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest004 OH_EffectFilter_SetValue failed";

    errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest004 OH_ImageEffect_SetInputPixelmap failed";
    errorCode = OH_ImageEffect_SetOutputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest004 OH_ImageEffect_SetOutputPixelmap failed";

    errorCode = OH_ImageEffect_Start(nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest004 OH_ImageEffect_Start failed";

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest004 OH_ImageEffect_Release failed";

    GTEST_LOG_(INFO) << "ImageEffectStandardFilterUnittest004 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectStandardFilterUnittest004 END";
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffectStandardFilter submethod OH_ImageEffect_Release with empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffectStandardFilter submethod OH_ImageEffect_Release with empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, ImageEffectStandardFilterUnittest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectStandardFilterUnittest005 start";
    InSequence s;

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr) << "ImageEffectStandardFilterUnittest005 OH_ImageEffect_Create failed";

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr) << "ImageEffectStandardFilterUnittest005 OH_ImageEffect_AddFilter failed";

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest005 OH_EffectFilter_SetValue failed";

    errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest005 OH_ImageEffect_SetInputPixelmap failed";
    errorCode = OH_ImageEffect_SetOutputPixelmap(imageEffect, pixelmapNative_);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest005 OH_ImageEffect_SetOutputPixelmap failed";

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest005 OH_ImageEffect_Start failed";

    errorCode = OH_ImageEffect_Release(nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectStandardFilterUnittest004 OH_ImageEffect_Release failed";

    GTEST_LOG_(INFO) << "ImageEffectStandardFilterUnittest005 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectStandardFilterUnittest005 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Register with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Register with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterRegister001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRegister001 start";

    ImageEffect_FilterDelegate delegate = {
        .setValue = [](OH_EffectFilter *filter, const char *key, const ImageEffect_Any *value) { return true; },
        .render = [](OH_EffectFilter *filter, OH_EffectBufferInfo *src, OH_EffectFilterDelegate_PushData pushData) {
            pushData(filter, src);
            return true;
        },
        .save = [](OH_EffectFilter *filter, char **info) { return true; },
        .restore = [](const char *info) { return OH_EffectFilter_Create("CustomBrightnessEFilter"); }
    };

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Register(filterInfo_, &delegate);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "OHEFilterRegister001 OH_EffectFilter_Register failed";

    GTEST_LOG_(INFO) << "OHEFilterRegister001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRegister001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Register with ImageEffect_FilterDelegate not exist OH_EffectFilter parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Register with ImageEffect_FilterDelegate not exist OH_EffectFilter parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterRegister002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRegister002 start";

    ImageEffect_FilterDelegate delegate = {
        .setValue = [](OH_EffectFilter *filter, const char *key, const ImageEffect_Any *value) { return true; },
        .render = [](OH_EffectFilter *filter, OH_EffectBufferInfo *src, OH_EffectFilterDelegate_PushData pushData) {
            pushData(filter, src);
            return true;
        },
        .save = [](OH_EffectFilter *filter, char **info) { return true; },
        .restore = [](const char *info) { return OH_EffectFilter_Create("CustomTestEFilter"); }
    };

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Register(filterInfo_, &delegate);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "OHEFilterRegister002 OH_EffectFilter_Register failed";

    GTEST_LOG_(INFO) << "OHEFilterRegister002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRegister002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Register with ImageEffect_FilterDelegate all false parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Register with ImageEffect_FilterDelegate all false parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterRegister003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRegister003 start";

    ImageEffect_FilterDelegate delegate = {
        .setValue = [](OH_EffectFilter *filter, const char *key, const ImageEffect_Any *value) { return false; },
        .render = [](OH_EffectFilter *filter, OH_EffectBufferInfo *src, OH_EffectFilterDelegate_PushData pushData) {
            return false;
        },
        .save = [](OH_EffectFilter *filter, char **info) { return false; },
        .restore = [](const char *info) { return OH_EffectFilter_Create("CustomBrightnessEFilter"); }
    };

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Register(filterInfo_, &delegate);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "OHEFilterRegister003 OH_EffectFilter_Register failed";

    GTEST_LOG_(INFO) << "OHEFilterRegister003 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRegister003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_Register with OH_EffectInfo not exist OH_EffectFilter parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_Register with OH_EffectInfo not exist OH_EffectFilter parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterRegister004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRegister004 start";

    ImageEffect_FilterDelegate delegate = {
        .setValue = [](OH_EffectFilter *filter, const char *key, const ImageEffect_Any *value) { return true; },
        .render = [](OH_EffectFilter *filter, OH_EffectBufferInfo *src, OH_EffectFilterDelegate_PushData pushData) {
            pushData(filter, src);
            return true;
        },
        .save = [](OH_EffectFilter *filter, char **info) { return true; },
        .restore = [](const char *info) { return OH_EffectFilter_Create("CustomBrightnessEFilter"); }
    };

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_Register(filterInfo_, &delegate);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "OHEFilterRegister004 OH_EffectFilter_Register failed";

    GTEST_LOG_(INFO) << "OHEFilterRegister004 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterRegister004 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilterInfo with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilterInfo with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterLookupFilterInfo001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterLookupFilterInfo001 start";

    OH_EffectFilterInfo *filterInfo = OH_EffectFilterInfo_Create();
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_LookupFilterInfo(BRIGHTNESS_EFILTER, filterInfo);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_LookupFilterInfo failed";
    OH_EffectFilterInfo_Release(filterInfo);
    GTEST_LOG_(INFO) << "OHEFilterLookupFilterInfo001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterLookupFilterInfo001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilterInfo with empty key parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilterInfo with empty key parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterLookupFilterInfo002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterLookupFilterInfo002 start";

    OH_EffectFilterInfo *filterInfo = OH_EffectFilterInfo_Create();
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_LookupFilterInfo(nullptr, filterInfo);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OHEFilterLookupFilterInfo failed";
    OH_EffectFilterInfo_Release(filterInfo);
    GTEST_LOG_(INFO) << "OHEFilterLookupFilterInfo002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterLookupFilterInfo002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilterInfo with not esist key parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilterInfo with not esist key parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterLookupFilterInfo003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterLookupFilterInfo003 start";

    OH_EffectFilterInfo *filterInfo = OH_EffectFilterInfo_Create();
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_LookupFilterInfo("TestEFilter", filterInfo);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OHEFilterLookupFilterInfo003 failed";
    OH_EffectFilterInfo_Release(filterInfo);
    GTEST_LOG_(INFO) << "OHEFilterLookupFilterInfo003 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterLookupFilterInfo003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilterInfo with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilterInfo with all empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterLookupFilterInfo004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterLookupFilterInfo004 start";

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_LookupFilterInfo(nullptr, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_LookupFilterInfo failed";

    GTEST_LOG_(INFO) << "OHEFilterLookupFilterInfo004 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterLookupFilterInfo004 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_EffectFilter_LookupFilterInfo with empty OH_EffectInfo parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_EffectFilter_LookupFilterInfo with empty OH_EffectInfo parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHEFilterLookupFilterInfo005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterLookupFilterInfo005 start";

    ImageEffect_ErrorCode errorCode = OH_EffectFilter_LookupFilterInfo(BRIGHTNESS_EFILTER, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_EffectFilter_LookupFilterInfo failed";

    GTEST_LOG_(INFO) << "OHEFilterLookupFilterInfo005 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHEFilterLookupFilterInfo005 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Stop with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Stop with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectStop001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectStop001 start";
  
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_Stop(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_Stop failed";

    GTEST_LOG_(INFO) << "OHImageEffectStop001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectStop001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Stop with empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Stop with empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectStop002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectStop002 start";
  
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_Stop(nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_Stop failed";

    GTEST_LOG_(INFO) << "OHImageEffectStop002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectStop002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Save with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Save with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSave001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSave001 start";
  
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    char *imageEffectInfo = nullptr;
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_Save(imageEffect, &imageEffectInfo);

    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_Save failed";

    GTEST_LOG_(INFO) << "OHImageEffectSave001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSave001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Save with empty OH_ImageEffect parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Save with empty OH_ImageEffect parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSave002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSave002 start";

    char *imageEffectInfo = nullptr;
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_Save(nullptr, &imageEffectInfo);

    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_Save failed";

    GTEST_LOG_(INFO) << "OHImageEffectSave002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSave002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Restore with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Restore with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectRestore001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRestore001 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr) << "OH_ImageEffect_Restore failed";
    char *imageEffectInfo = nullptr;
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_Save(imageEffect, &imageEffectInfo);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_Restore failed";
    imageEffect = OH_ImageEffect_Restore(imageEffectInfo);
    ASSERT_NE(imageEffect, nullptr) << "OH_ImageEffect_Restore failed";

    GTEST_LOG_(INFO) << "OHImageEffectRestore001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRestore001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Restore submethods OH_ImageEffect_Save with empty OH_ImageEffect parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Restore submethods OH_ImageEffect_Save with empty OH_ImageEffect parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectRestore002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRestore002 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr) << "OH_ImageEffect_Restore failed";
    char *imageEffectInfo = nullptr;
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_Save(nullptr, &imageEffectInfo);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_Restore failed";
    imageEffect = OH_ImageEffect_Restore(imageEffectInfo);
    ASSERT_EQ(imageEffect, nullptr) << "OH_ImageEffect_Restore failed";

    GTEST_LOG_(INFO) << "OHImageEffectRestore002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRestore002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_Restore with empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_Restore with empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectRestore003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRestore003 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr) << "OH_ImageEffect_Restore failed";
    char *imageEffectInfo = nullptr;
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_Save(imageEffect, &imageEffectInfo);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_Restore failed";
    imageEffect = OH_ImageEffect_Restore(nullptr);
    ASSERT_EQ(imageEffect, nullptr) << "OH_ImageEffect_Restore failed";

    GTEST_LOG_(INFO) << "OHImageEffectRestore003 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectRestore003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputSurface with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputSurface with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSetOutputSurface001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputSurface001 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetOutputSurface(imageEffect, nativeWindow_);

    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_SetOutputSurface failed";

    GTEST_LOG_(INFO) << "OHImageEffectSetOutputSurface001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputSurface001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputSurface with all empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputSurface with all empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSetOutputSurface002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputSurface002 start";

    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetOutputSurface(nullptr, nullptr);

    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_SetOutputSurface failed";

    GTEST_LOG_(INFO) << "OHImageEffectSetOutputSurface002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputSurface002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputSurface with empty OH_ImageEffect parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputSurface with empty OH_ImageEffect parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSetOutputSurface003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputSurface003 start";

    OHNativeWindow *nativeWindow = new OHNativeWindow();
    nativeWindow->surface = nullptr;
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetOutputSurface(nullptr, nativeWindow);

    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_SetOutputSurface failed";

    GTEST_LOG_(INFO) << "OHImageEffectSetOutputSurface003 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputSurface003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetOutputSurface with empty surfaceId parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetOutputSurface with empty surfaceId parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSetOutputSurface004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputSurface004 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetOutputSurface(imageEffect, nullptr);

    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_SetOutputSurface failed";

    GTEST_LOG_(INFO) << "OHImageEffectSetOutputSurface004 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetOutputSurface004 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_GetInputSurface with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_GetInputSurface with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectGetInputSurface001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectGetInputSurface001 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    OHNativeWindow *nativeWindow = nullptr;
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_GetInputSurface(imageEffect, &nativeWindow);

    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_GetInputSurface failed";
    ASSERT_NE(nativeWindow, nullptr) << "OH_ImageEffect_GetInputSurface failed";

    GTEST_LOG_(INFO) << "OHImageEffectGetInputSurface001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectGetInputSurface001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_GetInputSurface with empty OH_ImageEffect parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_GetInputSurface with empty OH_ImageEffect parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectGetInputSurface002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectGetInputSurface002 start";

    OHNativeWindow *nativeWindow = nullptr;
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_GetInputSurface(nullptr, &nativeWindow);

    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_GetInputSurface failed";
    ASSERT_EQ(nativeWindow, nullptr) << "OH_ImageEffect_GetInputSurface failed";

    GTEST_LOG_(INFO) << "OHImageEffectGetInputSurface002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectGetInputSurface002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffectSaveAndRestore with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffectSaveAndRestore with normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, ImageEffectSaveAndRestoreUnittest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectSaveAndRestoreUnittest001 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr) << "ImageEffectSaveAndRestoreUnittest001 OH_ImageEffect_Create failed";

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr) << "ImageEffectSaveAndRestoreUnittest001 OH_ImageEffect_AddFilter failed";

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest001 OH_EffectFilter_SetValue failed";

    char *imageEffectInfo = nullptr;
    errorCode = OH_ImageEffect_Save(imageEffect, &imageEffectInfo);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest001 OH_EffectFilter_SetValue failed";

    ASSERT_NE(imageEffectInfo, nullptr) << "ImageEffectSaveAndRestoreUnittest001 imageEffectInfo is null";
    std::string info = imageEffectInfo;

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest001 OH_ImageEffect_Release failed";

    imageEffect = OH_ImageEffect_Restore(info.c_str());
    ASSERT_NE(imageEffect, nullptr) << "ImageEffectSaveAndRestoreUnittest001 OH_ImageEffect_Restore failed";

    OH_EffectFilter *restoreFilter = OH_ImageEffect_GetFilter(imageEffect, 0);
    ASSERT_NE(restoreFilter, nullptr) << "ImageEffectSaveAndRestoreUnittest001: OH_ImageEffect_GetFilter failed";

    ImageEffect_Any restoreValue;
    OH_EffectFilter_GetValue(restoreFilter, KEY_FILTER_INTENSITY, &restoreValue);
    ASSERT_FLOAT_EQ(restoreValue.dataValue.floatValue, 100.f) <<
        "ImageEffectSaveAndRestoreUnittest001 OH_EffectFilter_GetValue failed";

    errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, pixelmapNative_);
        ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest001 OH_ImageEffect_SetInputPixelmap failed";
    
    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest001 OH_ImageEffect_Start failed";

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest001 OH_ImageEffect_Release failed";

    GTEST_LOG_(INFO) << "ImageEffectSaveAndRestoreUnittest001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest ImageEffectSaveAndRestoreUnittest001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffectSaveAndRestore unobstructed func OH_ImageEffect_Create
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffectSaveAndRestore unobstructed func OH_ImageEffect_Create
 */
HWTEST_F(ImageEffectCApiUnittest, ImageEffectSaveAndRestoreUnittest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectSaveAndRestoreUnittest002 start";

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(nullptr, BRIGHTNESS_EFILTER);
    ASSERT_EQ(filter, nullptr) << "ImageEffectSaveAndRestoreUnittest002 OH_ImageEffect_AddFilter failed";

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest002 OH_EffectFilter_SetValue failed";

    char *imageEffectInfo = nullptr;
    errorCode = OH_ImageEffect_Save(nullptr, &imageEffectInfo);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest002 OH_EffectFilter_SetValue failed";

    errorCode = OH_ImageEffect_Release(nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest00 OH_ImageEffect_Release failed";

    errorCode = OH_ImageEffect_SetInputPixelmap(nullptr, pixelmapNative_);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest002 OH_ImageEffect_SetInputPixelmap failed";
    
    errorCode = OH_ImageEffect_Start(nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest002 OH_ImageEffect_Start failed";

    errorCode = OH_ImageEffect_Release(nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest002 OH_ImageEffect_Release failed";

    GTEST_LOG_(INFO) << "ImageEffectSaveAndRestoreUnittest002 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectSaveAndRestoreUnittest002 END";
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffectSaveAndRestore unobstructed func OH_ImageEffect_AddFilter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffectSaveAndRestore unobstructed func OH_ImageEffect_AddFilter
 */
HWTEST_F(ImageEffectCApiUnittest, ImageEffectSaveAndRestoreUnittest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest ImageEffectSaveAndRestoreUnittest003 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr) << "ImageEffectSaveAndRestoreUnittest003 OH_ImageEffect_Create failed";

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(nullptr, KEY_FILTER_INTENSITY, &value);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest003 OH_EffectFilter_SetValue failed";

    char *imageEffectInfo = nullptr;
    errorCode = OH_ImageEffect_Save(imageEffect, &imageEffectInfo);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest003 OH_EffectFilter_SetValue failed";
    ASSERT_NE(imageEffectInfo, nullptr);
    std::string infoStr = imageEffectInfo;

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest003 OH_ImageEffect_Release failed";

    imageEffect = OH_ImageEffect_Restore(infoStr.c_str());
    ASSERT_NE(imageEffect, nullptr) << "ImageEffectSaveAndRestoreUnittest00 OH_ImageEffect_Restore failed";

    errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, pixelmapNative_);
        ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest003 OH_ImageEffect_SetInputPixelmap failed";
    
    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest003 OH_ImageEffect_Start failed";

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest003 OH_ImageEffect_Release failed";

    GTEST_LOG_(INFO) << "ImageEffectSaveAndRestoreUnittest003 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest ImageEffectSaveAndRestoreUnittest003 END";
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffectSaveAndRestore with empty inputPixelmap parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffectSaveAndRestore with empty inputPixelmap parameter
 */
HWTEST_F(ImageEffectCApiUnittest, ImageEffectSaveAndRestoreUnittest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest ImageEffectSaveAndRestoreUnittest004 start";

    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr) << "ImageEffectSaveAndRestoreUnittest004 OH_ImageEffect_Create failed";

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr) << "ImageEffectSaveAndRestoreUnittest004 OH_ImageEffect_AddFilter failed";

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest004 OH_EffectFilter_SetValue failed";

    char *imageEffectInfo = nullptr;
    errorCode = OH_ImageEffect_Save(imageEffect, &imageEffectInfo);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest004 OH_EffectFilter_SetValue failed";

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest004 OH_ImageEffect_Release failed";

    imageEffect = OH_ImageEffect_Restore(imageEffectInfo);
    ASSERT_NE(imageEffect, nullptr) << "ImageEffectSaveAndRestoreUnittest004 OH_ImageEffect_Restore failed";

    errorCode = OH_ImageEffect_SetInputPixelmap(imageEffect, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest004 OH_ImageEffect_SetInputPixelmap failed";
    
    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest004 OH_ImageEffect_Start failed";

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) <<
        "ImageEffectSaveAndRestoreUnittest004 OH_ImageEffect_Release failed";

    GTEST_LOG_(INFO) << "ImageEffectSaveAndRestoreUnittest004 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: ImageEffectSaveAndRestoreUnittest004 END";
}

/**
 * Feature: ImageEffect
 * Function: Test ImageEffectSaveAndRestore with ContrastEFilter normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ImageEffectSaveAndRestore with ContrastEFilter normal parameter
 */
HWTEST_F(ImageEffectCApiUnittest, ImageEffectSaveAndRestoreUnittest005, TestSize.Level1)
{
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EffectFilter *filter = OH_ImageEffect_AddFilter(imageEffect, CONTRAST_EFILTER);
    ASSERT_NE(filter, nullptr);

    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    ImageEffect_ErrorCode errorCode = OH_EffectFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    char *imageEffectInfo = nullptr;
    errorCode = OH_ImageEffect_Save(imageEffect, &imageEffectInfo);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    ASSERT_NE(imageEffectInfo, nullptr);
    std::string info = imageEffectInfo;

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS);

    imageEffect = OH_ImageEffect_Restore(info.c_str());
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
 * Function: Test OH_ImageEffect_SetInputUri with empty uri parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputUri with empty uri parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectSetInputUri001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetInputUri001 start";
    OH_ImageEffect *imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr) << "OHImageEffectSetInputUri001 OH_ImageEffect_Create failed";

    ImageEffect_ErrorCode errorCode = OH_ImageEffect_SetInputUri(imageEffect, nullptr);
    ASSERT_NE(errorCode, ImageEffect_ErrorCode::EFFECT_SUCCESS) << "OH_ImageEffect_SetInputUri failed";

    GTEST_LOG_(INFO) << "OHImageEffectSetInputUri001 success! result: " << errorCode;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectSetInputUri001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_FiltersSize with empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_FiltersSize with empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OHImageEffectFiltersSize001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectFiltersSize001 start";
    int32_t res = OH_ImageEffect_GetFilterCount(nullptr);
    ASSERT_EQ(res, 0) << "OHImageEffectFiltersSize001 OH_ImageEffect_FiltersSize failed";

    GTEST_LOG_(INFO) << "OHImageEffectFiltersSize001 success! result: " << res;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OHImageEffectFiltersSize001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_GetFilter with empty parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_GetFilter with empty parameter
 */
HWTEST_F(ImageEffectCApiUnittest, OH_ImageEffect_GetFilter001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OH_ImageEffect_GetFilter001 start";
    OH_EffectFilter *filter = OH_ImageEffect_GetFilter(nullptr, 0);
    ASSERT_EQ(filter, nullptr) << "OH_ImageEffect_GetFilter001 OH_ImageEffect_GetFilter failed";

    GTEST_LOG_(INFO) << "OH_ImageEffect_GetFilter001 success! result: " << filter;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OH_ImageEffect_GetFilter001 END";
}

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_GetFilter with empty parameter and negative index
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_GetFilter with empty parameter and negative index
 */
HWTEST_F(ImageEffectCApiUnittest, OH_ImageEffect_GetFilter002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OH_ImageEffect_GetFilter002 start";
    OH_EffectFilter *filter = OH_ImageEffect_GetFilter(nullptr, -1);
    ASSERT_EQ(filter, nullptr) << "OH_ImageEffect_GetFilter002 OH_ImageEffect_GetFilter failed";

    GTEST_LOG_(INFO) << "OH_ImageEffect_GetFilter002 success! result: " << filter;
    GTEST_LOG_(INFO) << "ImageEffectCApiUnittest: OH_ImageEffect_GetFilter002 END";
}
} // namespace Effect
} // namespace Media
} // namespace OHOS