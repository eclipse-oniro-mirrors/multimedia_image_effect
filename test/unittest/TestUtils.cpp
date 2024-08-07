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
#include "json_helper.h"
#include "common_utils.h"
#include "string_helper.h"
#include "format_helper.h"
#include "native_common_utils.h"

using namespace testing::ext;
namespace
{
    const float YUV_BYTES_PER_PIXEL = 1.5f;
}

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
    EffectJsonPtr root = JsonHelper::CreateObject();
    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(root->IsObject());
    ASSERT_TRUE(root->Put("stringKey", "testString"));
    ASSERT_TRUE(root->Put("floatKey", 1.23f));
    ASSERT_TRUE(root->Put("intKey", 123));
    EffectJsonPtr intRoot = JsonHelper::CreateArray();
    ASSERT_TRUE(intRoot->Add(1));
    ASSERT_TRUE(intRoot->Add(2));
    ASSERT_TRUE(intRoot->Add(3));
    ASSERT_TRUE(root->Put("arrayKey", intRoot));

    ASSERT_TRUE(root->HasElement("stringKey"));
    EffectJsonPtr stringKeyJsonPtr = root->GetElement("stringKey");
    ASSERT_NE(stringKeyJsonPtr, nullptr);
    ASSERT_TRUE(stringKeyJsonPtr->IsString());
    ASSERT_FALSE(stringKeyJsonPtr->IsNumber());
    ASSERT_EQ(stringKeyJsonPtr->GetInt(), 0);
    ASSERT_EQ(stringKeyJsonPtr->GetUInt(), 0);
    ASSERT_EQ(stringKeyJsonPtr->GetFloat(), 0);
    ASSERT_EQ(stringKeyJsonPtr->GetDouble(), 0);
    ASSERT_FALSE(stringKeyJsonPtr->GetBool());
    std::string stringValue = stringKeyJsonPtr->GetString();
    ASSERT_STREQ(stringValue.c_str(), "testString");

    ASSERT_TRUE(root->HasElement("floatKey"));
    EffectJsonPtr floatKeyJsonPtr = root->GetElement("floatKey");
    ASSERT_NE(floatKeyJsonPtr, nullptr);
    ASSERT_TRUE(floatKeyJsonPtr->IsNumber());
    ASSERT_TRUE(floatKeyJsonPtr->GetString().empty());
    ASSERT_FALSE(floatKeyJsonPtr->IsString());
    float floatValue = floatKeyJsonPtr->GetFloat();
    ASSERT_EQ(floatValue, 1.23f);

    ASSERT_FALSE(root->HasElement("nonExistKey"));

    ASSERT_TRUE(root->HasElement("arrayKey"));
    EffectJsonPtr arrayKeyJsonPtr = root->GetElement("arrayKey");
    ASSERT_NE(arrayKeyJsonPtr, nullptr);
    ASSERT_TRUE(arrayKeyJsonPtr->IsArray());
    std::vector<EffectJsonPtr> arrayJsonPtr = arrayKeyJsonPtr->GetArray();
    ASSERT_EQ(arrayJsonPtr.size(), 3);
    ASSERT_EQ(arrayJsonPtr[0]->GetInt(), 1);
    ASSERT_EQ(arrayJsonPtr[1]->GetInt(), 2);
    ASSERT_EQ(arrayJsonPtr[2]->GetInt(), 3);
}

HWTEST_F(TestUtils, NativeCommonUtilsParseJson001, TestSize.Level1) {
    std::string key = "test_key";
    Plugin::Any any = nullptr;
    Json *json = nullptr;
    EffectJsonPtr result = std::make_shared<EffectJson>(json);
    ErrorCode ret = CommonUtils::ParseAnyAndAddToJson(key, any, result);
    ASSERT_EQ(ret, ErrorCode::ERR_ANY_CAST_TYPE_NOT_MATCH);
}
HWTEST_F(TestUtils, NativeCommonUtilsParseNativeWindowData001, TestSize.Level1) {
    std::shared_ptr<BufferInfo> bufferinfo = std::make_unique<BufferInfo>();
    void *addr = nullptr;
    std::shared_ptr<ExtraInfo> extrainfo = std::make_unique<ExtraInfo>();
    std::shared_ptr<EffectBuffer> effectBuffer = std::make_unique<EffectBuffer>(bufferinfo, addr, extrainfo);
    DataType datatype = DataType::UNKNOWN;
    ErrorCode result = CommonUtils::ParseNativeWindowData(effectBuffer, datatype);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}
HWTEST_F(TestUtils, NativeCommonUtilsModifyPixelMapProperty001, TestSize.Level1) {
    PixelMap pixelMap;
    std::shared_ptr<BufferInfo> bufferinfo = std::make_unique<BufferInfo>();
    void *addr = nullptr;
    std::shared_ptr<ExtraInfo> extrainfo = std::make_unique<ExtraInfo>();
    std::shared_ptr<EffectBuffer> buffer = std::make_unique<EffectBuffer>(bufferinfo, addr, extrainfo);
    std::shared_ptr<EffectMemoryManager> memoryManager = std::make_unique<EffectMemoryManager>();
    ErrorCode result = CommonUtils::ModifyPixelMapProperty(&pixelMap, buffer, memoryManager);
    EXPECT_EQ(result, ErrorCode::ERR_ALLOC_MEMORY_FAIL);
}
HWTEST_F(TestUtils, StringHelper001, TestSize.Level1) {
    std::string input = "abc";
    std::string suffix = "abcd";
    std::string srcStr = "abcdef";
    std::string endStr = "def";
    std::shared_ptr<StringHelp> stringHelp = std::make_shared<StringHelp>();
    EXPECT_FALSE(stringHelp->EndsWith(input, suffix));
    EXPECT_FALSE(stringHelp->EndsWithIgnoreCase(input, suffix));
    bool result = stringHelp->EndsWith(srcStr, endStr);
    EXPECT_TRUE(result);
}

HWTEST_F(TestUtils, FormatHelper001, TestSize.Level1) {
    uint32_t height = 1280;
    uint32_t width = 960;
    std::shared_ptr<FormatHelper> formatHelper = std::make_shared<FormatHelper>();
    uint32_t result = formatHelper->CalculateDataRowCount(height, IEffectFormat::YUVNV12);
    ASSERT_EQ(result, height * YUV_BYTES_PER_PIXEL);

    result = formatHelper->CalculateRowStride(width, IEffectFormat::DEFAULT);
    ASSERT_EQ(result, width);
}

HWTEST_F(TestUtils, NativeCommonUtils001, TestSize.Level1) {
    ImageEffect_Format ohFormatType = ImageEffect_Format::EFFECT_PIXEL_FORMAT_RGBA8888;
    IEffectFormat formatType;
    std::shared_ptr<NativeCommonUtils> nativeCommonUtils = std::make_shared<NativeCommonUtils>();
    nativeCommonUtils->SwitchToFormatType(ohFormatType, formatType);
    ASSERT_EQ(formatType, IEffectFormat::RGBA8888);

    ohFormatType = ImageEffect_Format::EFFECT_PIXEL_FORMAT_UNKNOWN;
    nativeCommonUtils->SwitchToFormatType(ohFormatType, formatType);
    ASSERT_EQ(formatType, IEffectFormat::DEFAULT);
}

HWTEST_F(TestUtils, ErrorCode001, TestSize.Level1) {
    ErrorCode code = ErrorCode::ERR_PERMISSION_DENIED;
    const char *expected = "ERROR_PERMISSION_DENIED";
    const char *actual = GetErrorName(code);
    ASSERT_EQ(expected, actual);

    code = ErrorCode::ERR_INPUT_NULL;
    expected = "Unknow error type";
    actual = GetErrorName(code);
    ASSERT_EQ(expected, actual);
}
}
}
}
}