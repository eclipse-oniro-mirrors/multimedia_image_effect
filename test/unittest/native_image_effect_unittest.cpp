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

#include <vector>
#include "gtest/gtest.h"

#include "native_effect_errors.h"
#include "native_image_effect.h"
#include "mock_surface_buffer.h"
#include "mock_surface_buffer_yuv_nv21.h"
#include "mock_surface_buffer_yuv_nv12.h"
#include "native_effect_filter.h"
#include "image_type.h"
#include "efilter_factory.h"
#include "brightness_efilter.h"
#include "contrast_efilter.h"
#include "mock_pixel_map_napi.h"
#include "mock_native_pixel_map.h"
#include "surface_utils.h"
#include "test_common.h"
#include "external_loader.h"

using namespace testing::ext;
using namespace OHOS::Media;
using ::testing::InSequence;
using ::testing::Mock;

static std::string g_jpgPath;
static std::string g_notJpgPath;
static std::string g_jpgUri;
static std::string g_notJpgUri;
static uint64_t g_uniqueId = 0;

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
        sptr<Surface> consumer = Surface::CreateSurfaceAsConsumer("UnitTest");
        SurfaceUtils::GetInstance()->Add(g_uniqueId, consumer);
    }

    static void TearDownTestCase()
    {
        SurfaceUtils::GetInstance()->Remove(g_uniqueId);
    }

    void SetUp() override
    {
        mockSurfaceBuffer_ = new MockSurfaceBuffer();
        mockSurfaceBufferYuvNv21_ = new MockSurfaceBufferYuvNv21();
        mockSurfaceBufferYuvNv12_ = new MockSurfaceBufferYuvNv12();
        ExternLoader::Instance()->InitExt();
        EFilterFactory::Instance()->functions_.clear();
        EFilterFactory::Instance()->ResisterEFilter<BrightnessEFilter>(BRIGHTNESS_EFILTER);
        EFilterFactory::Instance()->ResisterEFilter<ContrastEFilter>(CONTRAST_EFILTER);
        EFilterFactory::Instance()->ResisterEFilter<CustomTestEFilter>(CUSTOM_TEST_EFILTER);
        mockPixelMapNapi_ = new MockPixelMapNapi();
    }

    void TearDown() override
    {
        delete mockSurfaceBuffer_;
        delete mockSurfaceBufferYuvNv21_;
        delete mockSurfaceBufferYuvNv12_;
        Mock::AllowLeak(mockPixelMapNapi_);

        mockSurfaceBuffer_ = nullptr;
        mockSurfaceBufferYuvNv21_ = nullptr;
        mockSurfaceBufferYuvNv12_ = nullptr;
    }

    MockSurfaceBuffer *mockSurfaceBuffer_;
    MockSurfaceBuffer *mockSurfaceBufferYuvNv21_;
    MockSurfaceBuffer *mockSurfaceBufferYuvNv12_;
    MockPixelMapNapi *mockPixelMapNapi_;

    OH_EFilterDelegate delegate_ = {
        .setValue = [](OH_EFilter *filter, const char *key, const OH_Any *value) { return true; },
        .render = [](OH_EFilter *filter, OH_EffectBuffer *src, OH_EFilterDelegate_PushData pushData) {
            pushData(filter, src);
            return true;
        },
        .save = [](OH_EFilter *filter, char **info) {
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
            OH_EFilter *filter = OH_EFilter_Create(CUSTOM_BRIGHTNESS_EFILTER);
            OH_Any value;
            value.dataType = OH_DataType::TYPE_FLOAT;
            value.dataValue.floatValue = 50.f;
            OH_EFilter_SetValue(filter, BRIGHTNESS_EFILTER, &value);
            return filter;
        }
    };
};

/**
 * Feature: ImageEffect
 * Function: Test OH_ImageEffect_SetInputPath with normal parameter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OH_ImageEffect_SetInputPath with normal parameter
 */
HWTEST_F(NativeImageEffectUnittest, OHImageEffectSetInputPath001, TestSize.Level1)
{
    OH_ImageEffect * imageEffect = OH_ImageEffect_Create(IMAGE_EFFECT_NAME);
    ASSERT_NE(imageEffect, nullptr);

    OH_EFilter *filter = OH_ImageEffect_AddFilter(imageEffect, BRIGHTNESS_EFILTER);
    ASSERT_NE(filter, nullptr);

    OH_Any value;
    value.dataType = OH_DataType::TYPE_FLOAT;
    value.dataValue.floatValue = 100.f;
    OH_EffectErrorCode errorCode = OH_EFilter_SetValue(filter, KEY_FILTER_INTENSITY, &value);
    ASSERT_EQ(errorCode, OH_EffectErrorCode::EFFECT_ERR_SUCCESS);

    errorCode = OH_ImageEffect_SetInputPath(imageEffect, g_jpgPath.c_str());
    ASSERT_EQ(errorCode, OH_EffectErrorCode::EFFECT_ERR_SUCCESS);

    errorCode = OH_ImageEffect_Start(imageEffect);
    ASSERT_EQ(errorCode, OH_EffectErrorCode::EFFECT_ERR_SUCCESS);

    errorCode = OH_ImageEffect_Release(imageEffect);
    ASSERT_EQ(errorCode, OH_EffectErrorCode::EFFECT_ERR_SUCCESS);
}

}
} // namespace Effect
} // namespace Media
} // namespace OHOS