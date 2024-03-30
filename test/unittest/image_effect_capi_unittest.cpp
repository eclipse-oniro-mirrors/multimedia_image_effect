/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include "native_image_effect.h"
#include "native_efilter.h"
#include "mock_native_pixel_map.h"
#include "efilter_factory.h"
#include "brightness_efilter.h"
#include "contrast_efilter.h"
#include "test_common.h"
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
void ImageEffectCApiUnittest::SetUpTestCase() {}

void ImageEffectCApiUnittest::TearDownTestCase() {}

void ImageEffectCApiUnittest::SetUp()
{
    mockPixelMap_ = new MockPixelMap();
    mockPixelMapNapi_ = new MockPixelMapNapi();
    ExternLoader::Instance()->InitExt();
    EFilterFactory::Instance()->ResisterEFilter<BrightnessEFilter>(BRIGHTNESS_EFILTER);
    EFilterFactory::Instance()->ResisterEFilter<ContrastEFilter>(CONTRAST_EFILTER);
}

void ImageEffectCApiUnittest::TearDown()
{
    Mock::AllowLeak(mockPixelMap_);
    Mock::AllowLeak(mockPixelMapNapi_);
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

    OH_EFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    OH_Any value;
    value.dataType = OH_DataType::TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    OH_EffectErrorCode errorCode = OH_EFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, OH_EffectErrorCode::EFFECT_ERR_SUCCESS);

    NativePixelMap inputPixel = {
        .napi = mockPixelMapNapi_
    };
    errorCode = OH_ImageEffect_SetInputPixelMap(imageEffect, &inputPixel);
    ASSERT_EQ(errorCode, OH_EffectErrorCode::EFFECT_ERR_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, OH_EffectErrorCode::EFFECT_ERR_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, OH_EffectErrorCode::EFFECT_ERR_SUCCESS);
}

} // namespace Effect
} // namespace Media
} // namespace OHOS