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

#include "test_native_buffer_utils.h"

namespace OHOS {
namespace Media {
namespace Effect {
namespace Test {
namespace {
    constexpr int32_t WIDTH = 1920;
    constexpr int32_t HEIGHT = 1080;
    constexpr uint64_t USAGE = 43;
}

std::shared_ptr<OH_NativeBuffer> TestNativeBufferUtils::CreateNativeBuffer(GraphicPixelFormat format)
{
    OH_NativeBuffer_Config config = {
        .width = WIDTH,
        .height = HEIGHT,
        .format = format,
        .usage = USAGE,
    };
    OH_NativeBuffer *nativeBuffer = OH_NativeBuffer_Alloc(&config);
    if (nativeBuffer == nullptr) {
        return nullptr;
    }

    auto res = OH_NativeBuffer_Reference(nativeBuffer);
    if (res != 0) {
        return nullptr;
    }

    std::shared_ptr<OH_NativeBuffer> pNativeBuffer(nativeBuffer, [](OH_NativeBuffer *nb) {
        OH_NativeBuffer_Unreference(nb);
    });

    return pNativeBuffer;
}
}
}
}
}