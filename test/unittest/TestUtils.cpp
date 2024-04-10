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

#include "native_common_utils.h"
#include "any.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Effect {
namespace Test {

class TestUtils : public testing::Test {
public:
    TestUtils() = default;

    ~TestUtils() override = default;
    static void SetUpTestCase() {}

    static void TearDownTestCase() {}

    void SetUp() override{}

    void TearDown() override{}
};

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny001, TestSize.Level0) {
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_INT32;
    value.dataValue.int32Value = 123;
    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(&value, any);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    int actualValue = Plugin::AnyCast<int>(any);
    ASSERT_EQ(actualValue, 123);
}

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny002, TestSize.Level0) {
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 123.45f;
    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(&value, any);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    float actualValue = Plugin::AnyCast<float>(any);
    ASSERT_EQ(actualValue, 123.45f);
}

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny003, TestSize.Level0) {
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_DOUBLE;
    value.dataValue.doubleValue = 123.45;
    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(&value, any);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    double actualValue = Plugin::AnyCast<double>(any);
    ASSERT_EQ(actualValue, 123.45);
}

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny004, TestSize.Level0) {
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_CHAR;
    value.dataValue.charValue = 'A';
    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(&value, any);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    ASSERT_EQ(Plugin::AnyCast<char>(any), 'A');
}

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny005, TestSize.Level0) {
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_LONG;
    value.dataValue.longValue = 123456789L;
    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(&value, any);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    ASSERT_EQ(Plugin::AnyCast<long>(any), 123456789L);
}

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny006, TestSize.Level0) {
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_PTR;
    value.dataValue.ptrValue = (void*)0x12345678;
    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(&value, any);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    ASSERT_EQ(Plugin::AnyCast<void*>(any), reinterpret_cast<void*>(0x12345678));
}

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny007, TestSize.Level0) {
    ImageEffect_Any value;
    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(&value, any);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}
}
}
}
}