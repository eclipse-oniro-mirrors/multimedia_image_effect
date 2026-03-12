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
#include "cpu_contrast_algo.h"
#include "effect_buffer.h"
#include "any.h"
#include "effect_context.h"
#include "securec.h"

using namespace testing::ext;
using namespace OHOS::Media::Effect;

namespace OHOS {
namespace Media {
namespace Effect {
namespace Test {
constexpr uint32_t BYTES_PER_INT = 4;
constexpr uint8_t DEFAULT_PIXEL_VALUE = 128;
constexpr uint32_t MAX_BUFFER_SIZE = 100 * 1024 * 1024;

class TestCpuContrastAlgo : public testing::Test {
public:
    TestCpuContrastAlgo() = default;
    ~TestCpuContrastAlgo() override = default;

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp() override {}
    void TearDown() override {}

protected:
    EffectBuffer* CreateEffectBuffer(uint32_t width, uint32_t height, uint32_t len, uint32_t rowStride)
    {
        if (len == 0 || len > MAX_BUFFER_SIZE) {
            return nullptr;
        }
        
        auto bufferInfo = std::make_shared<BufferInfo>();
        bufferInfo->width_ = width;
        bufferInfo->height_ = height;
        bufferInfo->len_ = len;
        bufferInfo->rowStride_ = rowStride;
        
        void* buffer = malloc(len);
        if (buffer == nullptr) {
            return nullptr;
        }
        
        errno_t result = memset_s(buffer, len, DEFAULT_PIXEL_VALUE, len);
        if (result != 0) {
            free(buffer);
            return nullptr;
        }
        
        auto extraInfo = std::make_shared<ExtraInfo>();
        
        return new EffectBuffer(bufferInfo, buffer, extraInfo);
    }

    void ReleaseEffectBuffer(EffectBuffer* buffer)
    {
        if (buffer != nullptr) {
            if (buffer->buffer_ != nullptr) {
                free(buffer->buffer_);
            }
            delete buffer;
        }
    }
};

HWTEST_F(TestCpuContrastAlgo, OnApplyRGBA8888001, TestSize.Level1)
{
    uint32_t width = 100;
    uint32_t height = 100;
    uint32_t len = width * height * BYTES_PER_INT;
    
    EffectBuffer* src = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(src, nullptr);
    EffectBuffer* dst = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(dst, nullptr);
    
    std::map<std::string, Any> value;
    value["FilterIntensity"] = 50.0f;
    
    std::shared_ptr<EffectContext> context = nullptr;
    
    ErrorCode result = CpuContrastAlgo::OnApplyRGBA8888(src, dst, value, context);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    
    ReleaseEffectBuffer(src);
    ReleaseEffectBuffer(dst);
}

HWTEST_F(TestCpuContrastAlgo, OnApplyRGBA8888002, TestSize.Level1)
{
    uint32_t width = 100;
    uint32_t height = 100;
    uint32_t len = width * height * BYTES_PER_INT;
    
    EffectBuffer* src = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(src, nullptr);
    EffectBuffer* dst = nullptr;
    
    std::map<std::string, Any> value;
    value["FilterIntensity"] = 50.0f;
    
    std::shared_ptr<EffectContext> context = nullptr;
    
    ErrorCode result = CpuContrastAlgo::OnApplyRGBA8888(src, dst, value, context);
    ASSERT_EQ(result, ErrorCode::ERR_INPUT_NULL);
    
    ReleaseEffectBuffer(src);
}

HWTEST_F(TestCpuContrastAlgo, OnApplyRGBA8888003, TestSize.Level1)
{
    uint32_t width = 100;
    uint32_t height = 100;
    uint32_t len = width * height * BYTES_PER_INT;
    
    EffectBuffer* src = nullptr;
    EffectBuffer* dst = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(dst, nullptr);
    
    std::map<std::string, Any> value;
    value["FilterIntensity"] = 50.0f;
    
    std::shared_ptr<EffectContext> context = nullptr;
    
    ErrorCode result = CpuContrastAlgo::OnApplyRGBA8888(src, dst, value, context);
    ASSERT_EQ(result, ErrorCode::ERR_INPUT_NULL);
    
    ReleaseEffectBuffer(dst);
}

HWTEST_F(TestCpuContrastAlgo, OnApplyRGBA8888004, TestSize.Level1)
{
    uint32_t width = 100;
    uint32_t height = 100;
    uint32_t len = width * height * BYTES_PER_INT;
    
    EffectBuffer* src = CreateEffectBuffer(width, height, len - 1, width * BYTES_PER_INT);
    ASSERT_NE(src, nullptr);
    EffectBuffer* dst = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(dst, nullptr);
    
    std::map<std::string, Any> value;
    value["FilterIntensity"] = 50.0f;
    
    std::shared_ptr<EffectContext> context = nullptr;
    
    ErrorCode result = CpuContrastAlgo::OnApplyRGBA8888(src, dst, value, context);
    ASSERT_EQ(result, ErrorCode::ERR_INVALID_PARAMETER_VALUE);
    
    ReleaseEffectBuffer(src);
    ReleaseEffectBuffer(dst);
}

HWTEST_F(TestCpuContrastAlgo, OnApplyRGBA8888005, TestSize.Level1)
{
    uint32_t width = 100;
    uint32_t height = 100;
    uint32_t len = width * height * BYTES_PER_INT;
    
    EffectBuffer* src = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(src, nullptr);
    EffectBuffer* dst = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(dst, nullptr);
    
    std::map<std::string, Any> value;
    value["FilterIntensity"] = 0.0f;
    
    std::shared_ptr<EffectContext> context = nullptr;
    
    ErrorCode result = CpuContrastAlgo::OnApplyRGBA8888(src, dst, value, context);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    
    ReleaseEffectBuffer(src);
    ReleaseEffectBuffer(dst);
}

HWTEST_F(TestCpuContrastAlgo, OnApplyRGBA8888006, TestSize.Level1)
{
    uint32_t width = 100;
    uint32_t height = 100;
    uint32_t len = width * height * BYTES_PER_INT;
    
    EffectBuffer* src = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(src, nullptr);
    EffectBuffer* dst = src;
    
    std::map<std::string, Any> value;
    value["FilterIntensity"] = 0.0f;
    
    std::shared_ptr<EffectContext> context = nullptr;
    
    ErrorCode result = CpuContrastAlgo::OnApplyRGBA8888(src, dst, value, context);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    
    ReleaseEffectBuffer(src);
}

HWTEST_F(TestCpuContrastAlgo, OnApplyRGBA8888007, TestSize.Level1)
{
    uint32_t width = 100;
    uint32_t height = 100;
    uint32_t len = width * height * BYTES_PER_INT;
    
    EffectBuffer* src = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(src, nullptr);
    EffectBuffer* dst = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(dst, nullptr);
    
    std::map<std::string, Any> value;
    value["FilterIntensity"] = 0.000001f;
    
    std::shared_ptr<EffectContext> context = nullptr;
    
    ErrorCode result = CpuContrastAlgo::OnApplyRGBA8888(src, dst, value, context);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    
    ReleaseEffectBuffer(src);
    ReleaseEffectBuffer(dst);
}

HWTEST_F(TestCpuContrastAlgo, OnApplyRGBA8888008, TestSize.Level1)
{
    uint32_t width = 100;
    uint32_t height = 100;
    uint32_t rowStride = width * BYTES_PER_INT;
    uint32_t len = rowStride * (height - 1) + (width - 1) * BYTES_PER_INT + BYTES_PER_INT - 1;
    
    EffectBuffer* src = CreateEffectBuffer(width, height, len, rowStride);
    ASSERT_NE(src, nullptr);
    EffectBuffer* dst = CreateEffectBuffer(width, height, width * height * BYTES_PER_INT, width * BYTES_PER_INT);
    ASSERT_NE(dst, nullptr);
    
    std::map<std::string, Any> value;
    value["FilterIntensity"] = 50.0f;
    
    std::shared_ptr<EffectContext> context = nullptr;
    
    ErrorCode result = CpuContrastAlgo::OnApplyRGBA8888(src, dst, value, context);
    ASSERT_EQ(result, ErrorCode::ERR_INVALID_PARAMETER_VALUE);
    
    ReleaseEffectBuffer(src);
    ReleaseEffectBuffer(dst);
}

HWTEST_F(TestCpuContrastAlgo, OnApplyRGBA8888009, TestSize.Level1)
{
    uint32_t width = 100;
    uint32_t height = 100;
    uint32_t rowStride = width * BYTES_PER_INT;
    uint32_t len = rowStride * (height - 1) + (width - 1) * BYTES_PER_INT + BYTES_PER_INT - 1;
    
    EffectBuffer* src = CreateEffectBuffer(width, height, width * height * BYTES_PER_INT, width * BYTES_PER_INT);
    ASSERT_NE(src, nullptr);
    EffectBuffer* dst = CreateEffectBuffer(width, height, len, rowStride);
    ASSERT_NE(dst, nullptr);
    
    std::map<std::string, Any> value;
    value["FilterIntensity"] = 50.0f;
    
    std::shared_ptr<EffectContext> context = nullptr;
    
    ErrorCode result = CpuContrastAlgo::OnApplyRGBA8888(src, dst, value, context);
    ASSERT_EQ(result, ErrorCode::ERR_INVALID_PARAMETER_VALUE);
    
    ReleaseEffectBuffer(src);
    ReleaseEffectBuffer(dst);
}

HWTEST_F(TestCpuContrastAlgo, OnApplyRGBA8888010, TestSize.Level1)
{
    uint32_t width = 10;
    uint32_t height = 10;
    uint32_t len = width * height * BYTES_PER_INT;
    
    EffectBuffer* src = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(src, nullptr);
    EffectBuffer* dst = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(dst, nullptr);
    
    std::map<std::string, Any> value;
    value["FilterIntensity"] = 100.0f;
    
    std::shared_ptr<EffectContext> context = nullptr;
    
    ErrorCode result = CpuContrastAlgo::OnApplyRGBA8888(src, dst, value, context);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    
    ReleaseEffectBuffer(src);
    ReleaseEffectBuffer(dst);
}

HWTEST_F(TestCpuContrastAlgo, OnApplyRGBA8888011, TestSize.Level1)
{
    uint32_t width = 10;
    uint32_t height = 10;
    uint32_t len = width * height * BYTES_PER_INT;
    
    EffectBuffer* src = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(src, nullptr);
    EffectBuffer* dst = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(dst, nullptr);
    
    std::map<std::string, Any> value;
    value["FilterIntensity"] = -50.0f;
    
    std::shared_ptr<EffectContext> context = nullptr;
    
    ErrorCode result = CpuContrastAlgo::OnApplyRGBA8888(src, dst, value, context);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    
    ReleaseEffectBuffer(src);
    ReleaseEffectBuffer(dst);
}

HWTEST_F(TestCpuContrastAlgo, OnApplyRGBA8888012, TestSize.Level1)
{
    uint32_t width = 10;
    uint32_t height = 10;
    uint32_t len = width * height * BYTES_PER_INT;
    
    EffectBuffer* src = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(src, nullptr);
    EffectBuffer* dst = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(dst, nullptr);
    
    std::map<std::string, Any> value;
    
    std::shared_ptr<EffectContext> context = nullptr;
    
    ErrorCode result = CpuContrastAlgo::OnApplyRGBA8888(src, dst, value, context);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    
    ReleaseEffectBuffer(src);
    ReleaseEffectBuffer(dst);
}

HWTEST_F(TestCpuContrastAlgo, OnApplyRGBA8888013, TestSize.Level1)
{
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t len = width * height * BYTES_PER_INT;
    
    EffectBuffer* src = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(src, nullptr);
    EffectBuffer* dst = CreateEffectBuffer(width, height, len, width * BYTES_PER_INT);
    ASSERT_NE(dst, nullptr);
    
    std::map<std::string, Any> value;
    value["FilterIntensity"] = 50.0f;
    
    std::shared_ptr<EffectContext> context = nullptr;
    
    ErrorCode result = CpuContrastAlgo::OnApplyRGBA8888(src, dst, value, context);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    
    ReleaseEffectBuffer(src);
    ReleaseEffectBuffer(dst);
}

}
}
}
}