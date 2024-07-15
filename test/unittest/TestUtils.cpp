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
#include "nlohmann/json.hpp"
#include "json_helper.h"

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

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny001, TestSize.Level1) {
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_INT32;
    value.dataValue.int32Value = 123;
    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(&value, any);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    int actualValue = Plugin::AnyCast<int>(any);
    ASSERT_EQ(actualValue, 123);
}

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny002, TestSize.Level1) {
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_FLOAT;
    value.dataValue.floatValue = 123.45f;
    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(&value, any);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    float actualValue = Plugin::AnyCast<float>(any);
    ASSERT_EQ(actualValue, 123.45f);
}

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny003, TestSize.Level1) {
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_DOUBLE;
    value.dataValue.doubleValue = 123.45;
    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(&value, any);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    double actualValue = Plugin::AnyCast<double>(any);
    ASSERT_EQ(actualValue, 123.45);
}

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny004, TestSize.Level1) {
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_CHAR;
    value.dataValue.charValue = 'A';
    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(&value, any);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    ASSERT_EQ(Plugin::AnyCast<char>(any), 'A');
}

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny005, TestSize.Level1) {
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_LONG;
    value.dataValue.longValue = 123456789L;
    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(&value, any);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    ASSERT_EQ(Plugin::AnyCast<long>(any), 123456789L);
}

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny006, TestSize.Level1) {
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_PTR;
    value.dataValue.ptrValue = (void*)0x12345678;
    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(&value, any);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    ASSERT_EQ(Plugin::AnyCast<void*>(any), reinterpret_cast<void*>(0x12345678));
}

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny007, TestSize.Level1) {
    ImageEffect_Any value;
    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(&value, any);
    ASSERT_NE(result, ErrorCode::SUCCESS);
}

HWTEST_F(TestUtils, NativeCommonUtilsParseOHAny008, TestSize.Level1) {
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_BOOL;
    value.dataValue.boolValue = true;
    Plugin::Any any;
    EXPECT_EQ(NativeCommonUtils::ParseOHAny(&value, any), ErrorCode::SUCCESS);
}

HWTEST_F(TestUtils, NativeCommonUtilsSwitchToOHAny001, TestSize.Level1) {
    Plugin::Any any = 10.0;
    ImageEffect_Any value;
    value.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_DOUBLE;
    ErrorCode result = NativeCommonUtils::SwitchToOHAny(any, &value);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
    EXPECT_DOUBLE_EQ(value.dataValue.doubleValue, 10.0);

    Plugin::Any anyChar = 'a';
    ImageEffect_Any valueChar;
    valueChar.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_CHAR;
    result = NativeCommonUtils::SwitchToOHAny(anyChar, &valueChar);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
    EXPECT_EQ(valueChar.dataValue.charValue, 'a');

    Plugin::Any anyLong = 10L;
    ImageEffect_Any valueLong;
    valueLong.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_LONG;
    result = NativeCommonUtils::SwitchToOHAny(anyLong, &valueLong);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
    EXPECT_EQ(valueLong.dataValue.longValue, 10L);

    Plugin::Any anyPtr = (void*)10;
    ImageEffect_Any valuePtr;
    valuePtr.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_PTR;
    result = NativeCommonUtils::SwitchToOHAny(anyPtr, &valuePtr);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
    EXPECT_EQ(valuePtr.dataValue.ptrValue, (void*)10);

    Plugin::Any anyBool = true;
    ImageEffect_Any valueBool;
    valueBool.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_BOOL;
    result = NativeCommonUtils::SwitchToOHAny(anyBool, &valueBool);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
    EXPECT_EQ(valueBool.dataValue.boolValue, true);

    Plugin::Any anyUnknown = std::string("Unsupported");
    ImageEffect_Any valueUnknown;
    valueUnknown.dataType = ImageEffect_DataType::EFFECT_DATA_TYPE_UNKNOWN;
    result = NativeCommonUtils::SwitchToOHAny(anyUnknown, &valueUnknown);
    EXPECT_EQ(result, ErrorCode::ERR_NOT_SUPPORT_SWITCH_TO_OHANY);
}

HWTEST_F(TestUtils, JsonHelper001, TestSize.Level1) {
    nlohmann::json json = {
        {"stringKey", "testString"},
        {"floatKey", 1.23f},
        {"intKey", 123},
        {"arrayKey", nlohmann::json::array({1, 2, 3})}
    };

    std::string stringValue;
    float floatValue;
    int32_t intValue;
    nlohmann::json arrayValue;

    ErrorCode result = JsonHelper::GetStringValue(json, "stringKey", stringValue);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
    result = JsonHelper::GetFloatValue(json, "floatKey", floatValue);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
    result = JsonHelper::GetInt32Value(json, "intKey", intValue);
    EXPECT_EQ(result, ErrorCode::SUCCESS);
    result = JsonHelper::GetArray(json, "arrayKey", arrayValue);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    result = JsonHelper::GetStringValue(json, "nonExistKey", stringValue);
    EXPECT_NE(result, ErrorCode::SUCCESS);
    result = JsonHelper::GetFloatValue(json, "nonExistKey", floatValue);
    EXPECT_NE(result, ErrorCode::SUCCESS);
    result = JsonHelper::GetInt32Value(json, "nonExistKey", intValue);
    EXPECT_NE(result, ErrorCode::SUCCESS);
    result = JsonHelper::GetArray(json, "nonExistKey", arrayValue);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    result = JsonHelper::GetStringValue(json, "intKey", stringValue);
    EXPECT_NE(result, ErrorCode::SUCCESS);
    result = JsonHelper::GetFloatValue(json, "stringKey", floatValue);
    EXPECT_NE(result, ErrorCode::SUCCESS);
    result = JsonHelper::GetInt32Value(json, "arrayKey", intValue);
    EXPECT_NE(result, ErrorCode::SUCCESS);
    result = JsonHelper::GetArray(json, "floatKey", arrayValue);
    EXPECT_NE(result, ErrorCode::SUCCESS);
}
}
}
}
}