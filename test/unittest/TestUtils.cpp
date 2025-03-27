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
#include "effect_json_helper.h"
#include "common_utils.h"
#include "string_helper.h"
#include "format_helper.h"
#include "native_effect_base.h"
#include "memcpy_helper.h"
#include "event_report.h"
#include "mock_producer_surface.h"

using namespace testing::ext;
namespace {
    const float YUV_BYTES_PER_PIXEL = 1.5f;
    const int32_t P10_BYTES_PER_LUMA = 2;
    const u_int32_t RGBA_BYTES_PER_PIXEL = 4;
    constexpr uint32_t WIDTH = 1920;
    constexpr uint32_t HEIGHT = 1080;
    constexpr uint32_t MAX_ALLOC_SIZE = 600 * 1024 * 1024;
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
    EffectJsonPtr root = EffectJsonHelper::CreateObject();
    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(root->IsObject());
    ASSERT_TRUE(root->Put("stringKey", "testString"));
    ASSERT_TRUE(root->Put("floatKey", 1.23f));
    ASSERT_TRUE(root->Put("intKey", 123));
    EffectJsonPtr intRoot = EffectJsonHelper::CreateArray();
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

    result = formatHelper->CalculateDataRowCount(height, IEffectFormat::DEFAULT);
    ASSERT_EQ(result, height);

    result = formatHelper->CalculateRowStride(width, IEffectFormat::YCRCB_P010);
    ASSERT_EQ(result, width * P10_BYTES_PER_LUMA);

    result = formatHelper->CalculateRowStride(width, IEffectFormat::DEFAULT);
    ASSERT_EQ(result, width);
}

std::shared_ptr<void> AllocBuffer(size_t size)
{
    if (size <= 0 || size > MAX_ALLOC_SIZE) {
        return nullptr;
    }

    void *buffer = malloc(size);
    if (buffer == nullptr) {
        return nullptr;
    }

    std::shared_ptr<void> bufferPtr(buffer, [](void *buffer) {
        if (buffer != nullptr) {
            free(buffer);
        }
    });
    return bufferPtr;
}

FormatConverterInfo CreateConverterInfo(IEffectFormat format, void *addr, uint32_t rowStride)
{
    return {
        .bufferInfo = {
            .width_ = WIDTH,
            .height_ = HEIGHT,
            .len_ = FormatHelper::CalculateSize(WIDTH, HEIGHT, format),
            .formatType_ = format,
            .rowStride_ = rowStride,
        },
        .buffer = addr,
    };
}

HWTEST_F(TestUtils, FormatHelper002, TestSize.Level1)
{
    std::unordered_set<IEffectFormat> formats = FormatHelper::GetAllSupportedFormats();
    ASSERT_NE(formats.size(), 0);

    ASSERT_TRUE(FormatHelper::IsSupportConvert(IEffectFormat::RGBA8888, IEffectFormat::YUVNV21));

    std::shared_ptr<void> rgbaBuffer = AllocBuffer(FormatHelper::CalculateSize(WIDTH, HEIGHT, IEffectFormat::RGBA8888));
    FormatConverterInfo rgbaConverterInfo = CreateConverterInfo(IEffectFormat::RGBA8888, rgbaBuffer.get(),
        RGBA_BYTES_PER_PIXEL * WIDTH);
    std::shared_ptr<void> nv12Buffer = AllocBuffer(FormatHelper::CalculateSize(WIDTH, HEIGHT, IEffectFormat::YUVNV12));
    FormatConverterInfo nv12ConverterInfo = CreateConverterInfo(IEffectFormat::YUVNV12, nv12Buffer.get(), WIDTH);
    std::shared_ptr<void> nv21Buffer = AllocBuffer(FormatHelper::CalculateSize(WIDTH, HEIGHT, IEffectFormat::YUVNV21));
    FormatConverterInfo nv21ConverterInfo = CreateConverterInfo(IEffectFormat::YUVNV21, nv21Buffer.get(), WIDTH);

    ErrorCode res = FormatHelper::ConvertFormat(rgbaConverterInfo, nv12ConverterInfo);
    ASSERT_EQ(res, ErrorCode::SUCCESS);

    res = FormatHelper::ConvertFormat(rgbaConverterInfo, nv21ConverterInfo);
    ASSERT_EQ(res, ErrorCode::SUCCESS);

    res = FormatHelper::ConvertFormat(nv12ConverterInfo, rgbaConverterInfo);
    ASSERT_EQ(res, ErrorCode::SUCCESS);

    res = FormatHelper::ConvertFormat(nv21ConverterInfo, rgbaConverterInfo);
    ASSERT_EQ(res, ErrorCode::SUCCESS);

    res = FormatHelper::ConvertFormat(nv12ConverterInfo, nv21ConverterInfo);
    ASSERT_NE(res, ErrorCode::SUCCESS);

    std::shared_ptr<void> rgbaBuffer2 = AllocBuffer(FormatHelper::CalculateSize(WIDTH / 2,
        HEIGHT, IEffectFormat::RGBA8888));
    FormatConverterInfo rgbaConverterInfo2 = CreateConverterInfo(IEffectFormat::RGBA8888, rgbaBuffer2.get(),
         RGBA_BYTES_PER_PIXEL * WIDTH);

    res = FormatHelper::ConvertFormat(rgbaConverterInfo2, nv12ConverterInfo);
    ASSERT_EQ(res, ErrorCode::SUCCESS);
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
    std::string expected = "ERROR_PERMISSION_DENIED";
    std::string actual = GetErrorName(code);
    ASSERT_EQ(expected, actual);

    code = ErrorCode::ERR_INPUT_NULL;
    expected = "Unknow error type";
    actual = GetErrorName(code);
    ASSERT_EQ(expected, actual);
}

HWTEST_F(TestUtils, MemcpyHelperCopyData001, TestSize.Level1)
{
    MemoryData *src = new MemoryData();
    MemoryData *dst = src;
    MemcpyHelper::CopyData(src, dst);
    EXPECT_EQ(src, dst);

    dst = new MemoryData();
    MemcpyHelper::CopyData(src, dst);
    EXPECT_NE(src, dst);

    MemcpyHelper::CopyData(src, src);
    EXPECT_EQ(src, src);

    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    void *add = nullptr;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_unique<ExtraInfo>();
    std::shared_ptr<EffectBuffer> dst2 = std::make_unique<EffectBuffer>(bufferInfo, add, extraInfo);
    CopyInfo info = {
        .bufferInfo = *dst2->bufferInfo_,
        .data = static_cast<uint8_t *>(dst2->buffer_),
    };
    MemcpyHelper::CopyData(info, nullptr);
    MemcpyHelper::CopyData(info, dst2.get());

    delete src;
    src = nullptr;
    delete dst;
    dst = nullptr;
}

HWTEST_F(TestUtils, MemcpyHelperCopyData002, TestSize.Level1)
{
    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    void *add = nullptr;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_unique<ExtraInfo>();
    std::shared_ptr<EffectBuffer> src = std::make_unique<EffectBuffer>(bufferInfo, add, extraInfo);
    std::shared_ptr<EffectBuffer> dst = src;
   
    MemcpyHelper::CopyData(src.get(), dst.get());
    EXPECT_EQ(src.get(), dst.get());
}

HWTEST_F(TestUtils, NativeCommonUtilsSwitchToOHEffectInfo001, TestSize.Level1)
{
    EffectInfo effectInfo;
    std::shared_ptr<OH_EffectFilterInfo> ohFilterInfo = std::make_shared<OH_EffectFilterInfo>();
    std::vector<IPType> ipType;
    ipType.emplace_back(IPType::DEFAULT);
    effectInfo.formats_.emplace(IEffectFormat::DEFAULT, ipType);

    NativeCommonUtils::SwitchToOHEffectInfo(&effectInfo, ohFilterInfo.get());
    ASSERT_NE(ohFilterInfo->supportedBufferTypes.size(), 1);
    ASSERT_EQ(ohFilterInfo->supportedFormats.size(), 1);
    ASSERT_EQ(*(ohFilterInfo->supportedFormats.begin()), ImageEffect_Format::EFFECT_PIXEL_FORMAT_UNKNOWN);
}

HWTEST_F(TestUtils, NativeCommonUtilsGetSupportedFormats001, TestSize.Level1)
{
    std::shared_ptr<OH_EffectFilterInfo> ohFilterInfo = nullptr;
    uint32_t result = NativeCommonUtils::GetSupportedFormats(ohFilterInfo.get());
    ASSERT_EQ(result, 0);

    ohFilterInfo = std::make_shared<OH_EffectFilterInfo>();
    ohFilterInfo->supportedFormats.emplace(ImageEffect_Format::EFFECT_PIXEL_FORMAT_YCRCB_P010);
    result = NativeCommonUtils::GetSupportedFormats(ohFilterInfo.get());
    ASSERT_EQ(result, 0);
}

HWTEST_F(TestUtils, NativeCommonUtilsConverStartResult001, TestSize.Level1)
{
    ErrorCode errorCode = ErrorCode::ERR_ALLOC_MEMORY_FAIL;
    ImageEffect_ErrorCode result = NativeCommonUtils::ConvertStartResult(errorCode);
    ASSERT_EQ(result, ImageEffect_ErrorCode::EFFECT_ALLOCATE_MEMORY_FAILED);
}

HWTEST_F(TestUtils, ReportHiSysEvent_001, TestSize.Level1)
{
    const EventInfo eventInfo = {
        .errorInfo = {
	    .errorCode = 0,
	    .errorMsg = "test",
	}
    };
    EventReport::ReportHiSysEvent("not_find_test", eventInfo);
}


}
}
}
}
