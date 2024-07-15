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

#include "effect_memory.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Effect {
namespace Test {

constexpr int BUFFER_SIZE = 1024;

constexpr uint32_t WIDTH = 1920;
constexpr uint32_t HEIGHT = 1080;
constexpr IEffectFormat FORMATE_TYPE = IEffectFormat::RGBA8888;
constexpr uint32_t ROW_STRIDE = WIDTH * 4;
constexpr uint32_t LEN = ROW_STRIDE * HEIGHT;

class TestEffectMemoryManager : public testing::Test {
public:
    TestEffectMemoryManager() = default;

    ~TestEffectMemoryManager() override = default;
    static void SetUpTestCase() {}

    static void TearDownTestCase() {}

    void SetUp() override
    {
        buffer = malloc(BUFFER_SIZE);
        std::shared_ptr<HeapMemoryData> memoryData = std::make_unique<HeapMemoryData>();
        memoryData->data = buffer;
        memoryData_ = memoryData;
    }

    void TearDown() override
    {
        memoryData_ = nullptr;
        free(buffer);
        buffer = nullptr;
    }

    void *buffer = nullptr;
    std::shared_ptr<HeapMemoryData> memoryData_;
};

HWTEST_F(TestEffectMemoryManager, TestEffectMemoryManager001, TestSize.Level1) {

    std::unique_ptr<HeapMemory> heapMemory = std::make_unique<HeapMemory>();
    ErrorCode result = heapMemory->Release();
    ASSERT_NE(result, ErrorCode::SUCCESS);

    MemoryInfo memoryInfo;
    memoryInfo.bufferInfo.width_ = WIDTH;
    memoryInfo.bufferInfo.height_ = HEIGHT;
    memoryInfo.bufferInfo.len_ = LEN;
    memoryInfo.bufferInfo.rowStride_ = ROW_STRIDE;
    memoryInfo.bufferInfo.formatType_ = FORMATE_TYPE;

    std::shared_ptr<MemoryData> memoryData = heapMemory->Alloc(memoryInfo);
    ASSERT_NE(memoryData, nullptr);
    result = heapMemory->Release();
    ASSERT_EQ(result, ErrorCode::SUCCESS);

    std::shared_ptr<DmaMemory> dmaMemory = std::make_shared<DmaMemory>();
    result = dmaMemory->Release();
    ASSERT_NE(result, ErrorCode::SUCCESS);

    memoryData = dmaMemory->Alloc(memoryInfo);
    ASSERT_NE(memoryData, nullptr);
    result = dmaMemory->Release();
    ASSERT_EQ(result, ErrorCode::SUCCESS);

    std::shared_ptr<SharedMemory> sharedMemory = std::make_shared<SharedMemory>();
    result = sharedMemory->Release();
    ASSERT_NE(result, ErrorCode::SUCCESS);

    memoryData = sharedMemory->Alloc(memoryInfo);
    ASSERT_NE(memoryData, nullptr);
    result = sharedMemory->Release();
    ASSERT_EQ(result, ErrorCode::SUCCESS);

    EffectMemory *effectMemory = new EffectMemory();
    BufferType bufferType = BufferType::SHARED_MEMORY;
    std::unique_ptr<AbsMemory> absMemory = effectMemory->CreateMemory(bufferType);
    ASSERT_NE(absMemory, nullptr);
    bufferType = BufferType::DEFAULT;
    absMemory = effectMemory->CreateMemory(bufferType);
    ASSERT_EQ(absMemory, nullptr);
}
}
}
}
}