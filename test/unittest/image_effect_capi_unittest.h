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

#ifndef IMAGE_EFFECT_CAPI_UNITTEST_H
#define IMAGE_EFFECT_CAPI_UNITTEST_H

#include "gtest/gtest.h"
#include "mock_pixel_map.h"
#include "image_effect_filter.h"
#include "surface.h"

namespace OHOS {
namespace Media {
namespace Effect {
class ImageEffectCApiUnittest : public testing::Test {
public:
    std::shared_ptr<PixelMap> mockPixelMap_;
    OH_PixelmapNative *pixelmapNative_ = nullptr;
    OH_EffectFilterInfo *filterInfo_ = nullptr;

    /* SetUpTestCase:The preset action of the test suite is executed before the first TestCase */
    static void SetUpTestCase();

    /* TearDownTestCase:The test suite cleanup action is executed after the last TestCase */
    static void TearDownTestCase();

    /* SetUp:Execute before each test case */
    void SetUp() override;

    /* TearDown:Execute after each test case */
    void TearDown() override;

    static inline sptr<OHOS::Surface> consumerSurface_;
    static inline sptr<OHOS::Surface> ohSurface_;
    static inline OHNativeWindow *nativeWindow_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_CAPI_UNITTEST_H