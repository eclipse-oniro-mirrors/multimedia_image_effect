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

#include <securec.h>
#include <cstdint>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

#include "effect_memory.h"
#include "effect_memory_manager.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Effect {
// 供单元测试验证共享内存移交图时导出的独立映射(common_utils.cpp 实现)
ErrorCode GetSharedMemoryExportResource(SharedMemoryData *sharedMemoryData, uint32_t size, void **addr);
} // namespace Effect
} // namespace Media
} // namespace OHOS

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
    EXPECT_NE(absMemory, nullptr);
    bufferType = BufferType::DEFAULT;
    absMemory = effectMemory->CreateMemory(bufferType);
    EXPECT_EQ(absMemory, nullptr);
    delete effectMemory;
    effectMemory = nullptr;
}

// 新增测试用例: SharedMemory 基于 Ashmem 类分配的完整生命周期（成功路径）
HWTEST_F(TestEffectMemoryManager, SharedMemoryAllocRelease001, TestSize.Level1) {
    constexpr uint32_t testWidth = 16;
    constexpr uint32_t testHeight = 16;
    constexpr uint32_t testRowStride = testWidth * 4;
    constexpr uint32_t testLen = testRowStride * testHeight;

    std::shared_ptr<SharedMemory> sharedMemory = std::make_shared<SharedMemory>();
    MemoryInfo memoryInfo;
    memoryInfo.bufferInfo.width_ = testWidth;
    memoryInfo.bufferInfo.height_ = testHeight;
    memoryInfo.bufferInfo.len_ = testLen;
    memoryInfo.bufferInfo.rowStride_ = testRowStride;
    memoryInfo.bufferInfo.formatType_ = IEffectFormat::RGBA8888;

    std::shared_ptr<MemoryData> memoryData = sharedMemory->Alloc(memoryInfo);
    ASSERT_NE(memoryData, nullptr);
    ASSERT_NE(memoryData->data, nullptr);
    ASSERT_EQ(memoryData->memoryInfo.bufferType, BufferType::SHARED_MEMORY);

    // Ashmem 对象统一持有内部fd 与用户态映射(RAII)
    auto sharedData = std::static_pointer_cast<SharedMemoryData>(memoryData);
    ASSERT_NE(sharedData->ashmem, nullptr);
    ASSERT_GE(sharedData->ashmem->GetAshmemFd(), 0);
    // 为外部消费方导出的独立 fd 副本有效
    ASSERT_NE(sharedData->fdPtr, nullptr);
    ASSERT_GE(*sharedData->fdPtr, 0);
    ASSERT_EQ(static_cast<int *>(memoryData->memoryInfo.extra), sharedData->fdPtr);

    // 映射具备读写权限: 写入后可以读回
    auto *buf = static_cast<uint8_t *>(memoryData->data);
    memset_s(buf, testLen, 0xA5, testLen);
    EXPECT_EQ(buf[0], 0xA5);
    EXPECT_EQ(buf[testLen - 1], 0xA5);

    ErrorCode result = sharedMemory->Release();
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

// 新增测试用例: SharedMemory 导出 fd 与内部 ashmem fd 相互独立
HWTEST_F(TestEffectMemoryManager, SharedMemoryExportFdIndependence001, TestSize.Level1) {
    constexpr uint32_t testWidth = 16;
    constexpr uint32_t testHeight = 16;
    constexpr uint32_t testRowStride = testWidth * 4;
    constexpr uint32_t testLen = testRowStride * testHeight;

    std::shared_ptr<SharedMemory> sharedMemory = std::make_shared<SharedMemory>();
    MemoryInfo memoryInfo;
    memoryInfo.bufferInfo.width_ = testWidth;
    memoryInfo.bufferInfo.height_ = testHeight;
    memoryInfo.bufferInfo.len_ = testLen;
    memoryInfo.bufferInfo.rowStride_ = testRowStride;
    memoryInfo.bufferInfo.formatType_ = IEffectFormat::RGBA8888;

    std::shared_ptr<MemoryData> memoryData = sharedMemory->Alloc(memoryInfo);
    ASSERT_NE(memoryData, nullptr);
    auto sharedData = std::static_pointer_cast<SharedMemoryData>(memoryData);
    ASSERT_NE(sharedData->ashmem, nullptr);

    // 导出 fd 是内部 ashmem fd 的独立副本
    int internalFd = sharedData->ashmem->GetAshmemFd();
    ASSERT_GE(internalFd, 0);
    ASSERT_NE(*sharedData->fdPtr, internalFd);

    // 关闭导出 fd 不影响内部 fd 与内部映射
    sharedData->fdTransferred = true;
    ::close(*sharedData->fdPtr);
    delete sharedData->fdPtr;
    sharedData->fdPtr = nullptr;
    auto *buf = static_cast<uint8_t *>(memoryData->data);
    memset_s(buf, testLen, 0x3C, testLen);
    EXPECT_EQ(buf[testLen - 1], 0x3C);
    EXPECT_EQ(sharedData->ashmem->GetAshmemFd(), internalFd);

    // Relesase 仅释放导出 fd 的堆内存与内部 ashmem(close), 不会重复关闭
    ErrorCode result = sharedMemory->Release();
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}

// 新增测试用例: SharedMemory 非法尺寸分配与未分配时释放
HWTEST_F(TestEffectMemoryManager, SharedMemoryAllocInvalidSize001, TestSize.Level1) {
    std::shared_ptr<SharedMemory> sharedMemory = std::make_shared<SharedMemory>();

    MemoryInfo memoryInfo;
    memoryInfo.bufferInfo.width_ = 16;
    memoryInfo.bufferInfo.height_ = 16;
    memoryInfo.bufferInfo.len_ = 0; // 非法尺寸
    memoryInfo.bufferInfo.rowStride_ = 16 * 4;
    memoryInfo.bufferInfo.formatType_ = IEffectFormat::RGBA8888;
    std::shared_ptr<MemoryData> memoryData = sharedMemory->Alloc(memoryInfo);
    EXPECT_EQ(memoryData, nullptr);

    // 未分配成功时 Release 返回异常
    ErrorCode result = sharedMemory->Release();
    EXPECT_NE(result, ErrorCode::SUCCESS);
}

// 新增测试用例: 共享内存移交像素图的独立映射与接管语义
HWTEST_F(TestEffectMemoryManager, SharedMemoryExportResource001, TestSize.Level1) {
    constexpr uint32_t testWidth = 16;
    constexpr uint32_t testHeight = 16;
    constexpr uint32_t testRowStride = testWidth * 4;
    constexpr uint32_t testLen = testRowStride * testHeight;

    std::shared_ptr<SharedMemory> sharedMemory = std::make_shared<SharedMemory>();
    MemoryInfo memoryInfo;
    memoryInfo.bufferInfo.width_ = testWidth;
    memoryInfo.bufferInfo.height_ = testHeight;
    memoryInfo.bufferInfo.len_ = testLen;
    memoryInfo.bufferInfo.rowStride_ = testRowStride;
    memoryInfo.bufferInfo.formatType_ = IEffectFormat::RGBA8888;

    std::shared_ptr<MemoryData> memoryData = sharedMemory->Alloc(memoryInfo);
    ASSERT_NE(memoryData, nullptr);
    auto sharedData = std::static_pointer_cast<SharedMemoryData>(memoryData);

    // 移交像素图前标记为不再自动释放，模拟 ModifyPixelMapProperty 的接管语义
    sharedData->memoryInfo.isAutoRelease = false;

    // 为像素图建立独立映射，避免像素图析构 munmap 与内部 Ashmem 映射互相释放
    void *exportAddr = nullptr;
    ErrorCode res = GetSharedMemoryExportResource(sharedData.get(), testLen, &exportAddr);
    ASSERT_EQ(res, ErrorCode::SUCCESS);
    ASSERT_NE(exportAddr, nullptr);
    ASSERT_NE(exportAddr, sharedData->data);

    // 独立映射与内部映射指向同一块共享内存: 内部写入，导出映射可见
    memset_s(sharedData->data, testLen, 0x5A, testLen);
    EXPECT_EQ(static_cast<uint8_t *>(exportAddr)[0], 0x5A);
    EXPECT_EQ(static_cast<uint8_t *>(exportAddr)[testLen - 1], 0x5A);

    // 模拟 PixelMap::ReleaseSharedMemory: munmap + close(导出 fd)
    sharedData->fdTransferred = true;
    ::munmap(exportAddr, testLen);
    ::close(*sharedData->fdPtr);
    delete sharedData->fdPtr;
    sharedData->fdPtr = nullptr;

    // 释放共享内存: 导出 fd 已由消费方关闭，此处仅删除堆内存并释放 ashmem
    ErrorCode result = sharedMemory->Release();
    ASSERT_EQ(result, ErrorCode::SUCCESS);
}
} // namespace Test
} // namespace Effect
} // namespace Media
} // OHOS