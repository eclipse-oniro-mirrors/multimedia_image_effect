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

#include "effect_json_helper.h"

using namespace testing::ext;
using namespace OHOS::Media::Effect;

namespace OHOS {
namespace Media {
namespace Effect {
namespace Test {
namespace {
    constexpr char const *IMAGE_EFFECT = "imageEffect";
    constexpr char const *FILTERS = "filters";
    constexpr char const *NAME = "name";
    constexpr char const *BRIGHTNESS = "Brightness";
    constexpr char const *VALUES = "values";
    constexpr char const *FILTER_INTENSITY = "FilterIntensity";
    constexpr char const *CONTRAST = "Contrast";
    constexpr char const *IMAGE_EDIT = "imageEdit";
    constexpr char const *FLOAT_TEST = "FloatTest";
    constexpr char const *INT_TEST = "IntTest";
    constexpr char const *UINT_TEST = "UintTest";
    constexpr char const *DOUBLE_TEST = "DoubleTest";
    constexpr char const *BOOL_TEST = "BoolTest";
    constexpr char const *STRING_TEST = "StringTest";
    constexpr char const *TEST_STR = "testStr";
} // namespace

class TestJsonHelper : public testing::Test {
public:
    TestJsonHelper() = default;

    ~TestJsonHelper() override = default;

    static void SetUpTestCase() {}

    static void TearDownTestCase() {}

    void SetUp() override
    {
        root_ = EffectJsonHelper::CreateObject();
    }

    void TearDown() override
    {
        root_ = nullptr;
    }

    EffectJsonPtr root_;
};

HWTEST_F(TestJsonHelper, Parse001, TestSize.Level0)
{
    std::string info = "{\"imageEffect\":{\"filters\":[{\"name\":\"Brightness\",\"values\":{\"FilterIntensity\":"
        "100.0}},{\"name\":\"Contrast\",\"values\":{\"FilterIntensity\":50.0}}],\"name\":\"imageEdit\"}}";

    EffectJsonPtr root = EffectJsonHelper::ParseJsonData(info);
    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(root->HasElement(IMAGE_EFFECT));

    EffectJsonPtr imageEffect = root->GetElement(IMAGE_EFFECT);
    ASSERT_NE(imageEffect, nullptr);
    ASSERT_TRUE(imageEffect->HasElement(FILTERS));
    ASSERT_TRUE(imageEffect->HasElement(NAME));

    EffectJsonPtr filterObject = imageEffect->GetElement(FILTERS);
    ASSERT_NE(filterObject, nullptr);
    ASSERT_TRUE(filterObject->IsArray());
    std::vector<EffectJsonPtr> filters = filterObject->GetArray();
    ASSERT_EQ(filters.size(), 2);

    filters = imageEffect->GetArray(FILTERS);
    ASSERT_EQ(filters.size(), 2);

    ASSERT_NE(filters[0], nullptr);
    ASSERT_STREQ(filters[0]->GetString(NAME).c_str(), BRIGHTNESS);
    EffectJsonPtr values = filters[0]->GetElement(VALUES);
    ASSERT_NE(values, nullptr);
    ASSERT_EQ(values->GetFloat(FILTER_INTENSITY), 100.0f);

    ASSERT_NE(filters[1], nullptr);
    ASSERT_STREQ(filters[1]->GetString(NAME).c_str(), CONTRAST);
    values = filters[1]->GetElement(VALUES);
    ASSERT_NE(values, nullptr);
    ASSERT_EQ(values->GetFloat(FILTER_INTENSITY), 50.0f);

    EffectJsonPtr name = imageEffect->GetElement(NAME);
    ASSERT_NE(name, nullptr);
    ASSERT_STREQ(name->GetString().c_str(), IMAGE_EDIT);
    ASSERT_STREQ(imageEffect->GetString(NAME).c_str(), IMAGE_EDIT);
}

HWTEST_F(TestJsonHelper, Parse002, TestSize.Level0)
{
    std::string info = "{\"imageEffect\":{\"filters\":[{\"name\":\"Brightness\",\"values\":{\"FloatTest\":"
        "100.0,\"IntTest\":-10,\"UintTest\":20,\"DoubleTest\":30.5,\"BoolTest\":true,\"StringTest\":\"testStr\"}},"
        "{\"name\":\"Contrast\",\"values\":{\"FilterIntensity\":50.0}}],\"name\":\"imageEdit\"}}";

    EffectJsonPtr root = EffectJsonHelper::ParseJsonData(info);
    ASSERT_NE(root, nullptr);
    EffectJsonPtr imageEffect = root->GetElement(IMAGE_EFFECT);
    ASSERT_NE(imageEffect, nullptr);
    std::vector<EffectJsonPtr> filters = imageEffect->GetArray(FILTERS);
    ASSERT_EQ(filters.size(), 2);
    EffectJsonPtr values = filters[0]->GetElement(VALUES);
    ASSERT_NE(values, nullptr);

    ASSERT_EQ(values->GetFloat(FLOAT_TEST), 100.0f);
    ASSERT_EQ(values->GetInt(INT_TEST), -10);
    ASSERT_EQ(values->GetUInt(UINT_TEST), 20);
    ASSERT_EQ(values->GetDouble(DOUBLE_TEST), 30.5f);
    ASSERT_EQ(values->GetBool(BOOL_TEST), true);
    ASSERT_EQ(values->GetString(STRING_TEST), TEST_STR);

    EffectJsonPtr filtersObject = imageEffect->GetElement(FILTERS);
    ASSERT_NE(filtersObject, nullptr);
    EffectJsonPtr replaceFilters = EffectJsonHelper::CreateObject();
    ASSERT_TRUE(replaceFilters->Put(NAME, TEST_STR));
    ASSERT_TRUE(imageEffect->Replace(FILTERS, replaceFilters));
    ASSERT_STREQ(replaceFilters->GetString(NAME).c_str(), TEST_STR);
}

HWTEST_F(TestJsonHelper, Create001, TestSize.Level0)
{
    // brightness
    EffectJsonPtr brightnessValue = EffectJsonHelper::CreateObject();
    ASSERT_NE(brightnessValue, nullptr);
    float brightnessFilterIntensity = -100.0f;
    ASSERT_TRUE(brightnessValue->Put(FILTER_INTENSITY, brightnessFilterIntensity));

    EffectJsonPtr brightness = EffectJsonHelper::CreateObject();
    ASSERT_NE(brightness, nullptr);
    ASSERT_TRUE(brightness->Put(NAME, BRIGHTNESS));
    ASSERT_TRUE(brightness->Put(VALUES, brightnessValue));

    // contrast
    EffectJsonPtr contrastValue = EffectJsonHelper::CreateObject();
    ASSERT_NE(contrastValue, nullptr);
    float contrastFilterIntensity = -50.0f;
    ASSERT_TRUE(contrastValue->Put(FILTER_INTENSITY, contrastFilterIntensity));

    EffectJsonPtr contrast = EffectJsonHelper::CreateObject();
    ASSERT_NE(contrast, nullptr);
    ASSERT_TRUE(contrast->Put(NAME, CONTRAST));
    ASSERT_TRUE(contrast->Put(VALUES, contrastValue));

    // array filters
    EffectJsonPtr filters = EffectJsonHelper::CreateArray();
    ASSERT_NE(filters, nullptr);
    ASSERT_TRUE(filters->Add(brightness));
    ASSERT_TRUE(filters->Add(contrast));

    // imageEffect
    EffectJsonPtr imageEffect = EffectJsonHelper::CreateObject();
    ASSERT_NE(imageEffect, nullptr);
    ASSERT_TRUE(imageEffect->Put(FILTERS, filters));
    ASSERT_TRUE(imageEffect->Put(NAME, IMAGE_EDIT));

    EffectJsonPtr root = EffectJsonHelper::CreateObject();
    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(root->Put(IMAGE_EFFECT, imageEffect));

    std::string info = "{\"imageEffect\":{\"filters\":[{\"name\":\"Brightness\",\"values\":{\"FilterIntensity\":"
        "-100}},{\"name\":\"Contrast\",\"values\":{\"FilterIntensity\":-50}}],\"name\":\"imageEdit\"}}";
    ASSERT_STREQ(root->ToString().c_str(), info.c_str());
}

HWTEST_F(TestJsonHelper, CreateObject001, TestSize.Level0)
{
    EffectJsonPtr root = EffectJsonHelper::CreateObject();
    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(root->IsObject());
    float floatValue = 50.0f;
    ASSERT_TRUE(root->Put(FLOAT_TEST, floatValue));
    int32_t intValue = -10;
    ASSERT_TRUE(root->Put(INT_TEST, intValue));
    uint32_t uintValue = 20;
    ASSERT_TRUE(root->Put(UINT_TEST, uintValue));
    double doubleValue = 30.5;
    ASSERT_TRUE(root->Put(DOUBLE_TEST, doubleValue));
    bool boolValue = true;
    ASSERT_TRUE(root->Put(BOOL_TEST, boolValue));
    std::string strValue = TEST_STR;
    ASSERT_TRUE(root->Put(STRING_TEST, strValue));
    ASSERT_FALSE(root->Put(STRING_TEST, (char *)nullptr));

    std::string info = "{\"FloatTest\":50,\"IntTest\":-10,\"UintTest\":20,\"DoubleTest\":30.5,\"BoolTest\":true,"
        "\"StringTest\":\"testStr\"}";

    ASSERT_STREQ(root->ToString().c_str(), info.c_str());
}

HWTEST_F(TestJsonHelper, CreateArray001, TestSize.Level0)
{
    // float
    EffectJsonPtr floatRoot = EffectJsonHelper::CreateArray();
    ASSERT_NE(floatRoot, nullptr);
    float floatValue1 = 50.0f;
    float floatValue2 = -20.0f;
    ASSERT_TRUE(floatRoot->Add(floatValue1));
    ASSERT_TRUE(floatRoot->Add(floatValue2));
    std::vector<EffectJsonPtr> floatValues = floatRoot->GetArray();
    ASSERT_EQ(floatValues.size(), 2);
    ASSERT_EQ(floatValues[0]->GetFloat(), floatValue1);
    ASSERT_EQ(floatValues[1]->GetFloat(), floatValue2);

    // int
    EffectJsonPtr intRoot = EffectJsonHelper::CreateArray();
    ASSERT_NE(intRoot, nullptr);
    int32_t intValue1 = 50;
    int32_t intValue2 = -20;
    int32_t intValue3 = 10;
    ASSERT_TRUE(intRoot->Add(intValue1));
    ASSERT_TRUE(intRoot->Add(intValue2));
    ASSERT_TRUE(intRoot->Add(intValue3));
    std::vector<EffectJsonPtr> intValues = intRoot->GetArray();
    ASSERT_EQ(intValues.size(), 3);
    ASSERT_EQ(intValues[0]->GetInt(), intValue1);
    ASSERT_EQ(intValues[1]->GetInt(), intValue2);
    ASSERT_EQ(intValues[2]->GetInt(), intValue3);

    // uint
    EffectJsonPtr uintRoot = EffectJsonHelper::CreateArray();
    ASSERT_NE(uintRoot, nullptr);
    uint32_t uintValue = 50;
    ASSERT_TRUE(uintRoot->Add(uintValue));
    std::vector<EffectJsonPtr> uintValues = uintRoot->GetArray();
    ASSERT_EQ(uintValues.size(), 1);
    ASSERT_EQ(uintValues[0]->GetUInt(), uintValue);
}

HWTEST_F(TestJsonHelper, CreateArray002, TestSize.Level0)
{
    // double
    EffectJsonPtr doubleRoot = EffectJsonHelper::CreateArray();
    double doubleValue = 30.5;
    ASSERT_TRUE(doubleRoot->Add(doubleValue));
    std::vector<EffectJsonPtr> doubleValues = doubleRoot->GetArray();
    ASSERT_EQ(doubleValues.size(), 1);
    ASSERT_DOUBLE_EQ(doubleValues[0]->GetDouble(), doubleValue);

    // bool
    EffectJsonPtr boolRoot = EffectJsonHelper::CreateArray();
    bool boolValue = true;
    ASSERT_TRUE(boolRoot->Add(boolValue));
    std::vector<EffectJsonPtr> boolValues = boolRoot->GetArray();
    ASSERT_EQ(boolValues.size(), 1);
    ASSERT_DOUBLE_EQ(boolValues[0]->GetBool(), boolValue);

    // string
    EffectJsonPtr stringRoot = EffectJsonHelper::CreateArray();
    ASSERT_TRUE(stringRoot->Add(TEST_STR));
    ASSERT_TRUE(stringRoot->Add(std::string(TEST_STR)));
    std::vector<EffectJsonPtr> strValues = stringRoot->GetArray();
    ASSERT_EQ(strValues.size(), 2);
    ASSERT_STREQ(strValues[0]->GetString().c_str(), TEST_STR);
    ASSERT_STREQ(strValues[1]->GetString().c_str(), TEST_STR);
}

HWTEST_F(TestJsonHelper, Replace001, TestSize.Level0)
{
    std::string info = "{\"imageEffect\":{\"values\":{\"FloatTest\":50.1,\"IntTest\":-10,\"UintTest\":20,"
       "\"DoubleTest\":30.5,\"BoolTest\":true,\"StringTest\":\"testStr\"}}}";

    EffectJsonPtr root = EffectJsonHelper::ParseJsonData(info);
    ASSERT_NE(root, nullptr);
    EffectJsonPtr imageEffect = root->GetElement(IMAGE_EFFECT);
    ASSERT_NE(imageEffect, nullptr);
    EffectJsonPtr values = imageEffect->GetElement(VALUES);
    ASSERT_NE(values, nullptr);

    float floatValue = 70.0f;
    ASSERT_TRUE(values->Replace(FLOAT_TEST, floatValue));
    int32_t intValue = -50;
    ASSERT_TRUE(values->Replace(INT_TEST, intValue));
    uint32_t uintValue = 50;
    ASSERT_TRUE(values->Replace(UINT_TEST, uintValue));
    double doubleValue = 50.4;
    ASSERT_TRUE(values->Replace(DOUBLE_TEST, doubleValue));
    bool boolValue = false;
    ASSERT_TRUE(values->Replace(BOOL_TEST, boolValue));
    std::string strValue = "test";
    ASSERT_TRUE(values->Replace(STRING_TEST, strValue));

    ASSERT_EQ(values->GetFloat(FLOAT_TEST), floatValue);
    ASSERT_EQ(values->GetInt(INT_TEST), intValue);
    ASSERT_EQ(values->GetUInt(UINT_TEST), uintValue);
    ASSERT_DOUBLE_EQ(values->GetDouble(DOUBLE_TEST), doubleValue);
    ASSERT_EQ(values->GetBool(BOOL_TEST), boolValue);
    ASSERT_STREQ(values->GetString(STRING_TEST).c_str(), strValue.c_str());

    std::string strValue1 = "test1";
    const char *strValue1Ptr = strValue1.c_str();
    ASSERT_TRUE(values->Replace(STRING_TEST, strValue1Ptr));
    ASSERT_STREQ(values->GetString(STRING_TEST).c_str(), strValue1Ptr);

    ASSERT_FALSE(values->Replace(std::string(STRING_TEST), (char *)nullptr));
}

HWTEST_F(TestJsonHelper, Abnormal_001, TestSize.Level0)
{
    EffectJsonPtr root = EffectJsonHelper::CreateObject(false);
    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(root->IsObject());
    ASSERT_TRUE(root->Put(INT_TEST, 10));
    ASSERT_TRUE(root->Put(STRING_TEST, TEST_STR));

    EffectJsonPtr intKeyJsonPtr = root->GetElement(INT_TEST);
    ASSERT_NE(intKeyJsonPtr, nullptr);
    ASSERT_TRUE(intKeyJsonPtr->GetString().empty());

    EffectJsonPtr stringKeyJsonPtr = root->GetElement(STRING_TEST);
    ASSERT_NE(stringKeyJsonPtr, nullptr);
    ASSERT_EQ(stringKeyJsonPtr->GetInt(), 0);
    ASSERT_EQ(stringKeyJsonPtr->GetInt(INT_TEST, INT32_MAX), INT32_MAX);
    ASSERT_EQ(stringKeyJsonPtr->GetUInt(), 0);
    ASSERT_EQ(stringKeyJsonPtr->GetUInt(INT_TEST, UINT32_MAX), UINT32_MAX);
    ASSERT_EQ(stringKeyJsonPtr->GetFloat(), 0);
    ASSERT_EQ(stringKeyJsonPtr->GetFloat(INT_TEST, 0), 0);
    ASSERT_EQ(stringKeyJsonPtr->GetDouble(), 0);
    ASSERT_EQ(stringKeyJsonPtr->GetDouble(INT_TEST, 0), 0);
    ASSERT_FALSE(stringKeyJsonPtr->GetBool());
    ASSERT_EQ(stringKeyJsonPtr->GetBool(INT_TEST, true), true);
    ASSERT_EQ(stringKeyJsonPtr->GetArray().size(), 0);
    ASSERT_EQ(stringKeyJsonPtr->GetArray(INT_TEST).size(), 0);

    ASSERT_EQ(root->GetUInt(STRING_TEST, UINT32_MAX), UINT32_MAX);
    ASSERT_EQ(root->GetFloat(STRING_TEST, 0), 0);
    ASSERT_EQ(root->GetDouble(STRING_TEST, 0), 0);
    ASSERT_EQ(root->GetBool(STRING_TEST, true), true);
    ASSERT_EQ(root->GetArray(STRING_TEST).size(), 0);

    EffectJsonPtr boolKeyJsonPtr = root->GetElement(BOOL_TEST);
    ASSERT_EQ(boolKeyJsonPtr, nullptr);
}
} // namespace Test
} // namespace Effect
} // namespace Media
} // namespace OHOS