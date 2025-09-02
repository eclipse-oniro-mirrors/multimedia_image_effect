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

#include "render_environment.h"
#include "effect_context.h"
#include "graphic/render_frame_buffer.h"
#include "mock_producer_surface.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Effect {
namespace Test {

constexpr uint32_t WIDTH = 960;
constexpr uint32_t HEIGHT = 1280;
constexpr IEffectFormat FORMATE_TYPE = IEffectFormat::RGBA8888;
constexpr uint32_t ROW_STRIDE = WIDTH * 4;
constexpr uint32_t LEN = ROW_STRIDE * HEIGHT;

class TestRenderEnvironment : public testing::Test {
public:
    TestRenderEnvironment() = default;

    ~TestRenderEnvironment() override = default;
    static void SetUpTestCase() {}

    static void TearDownTestCase() {}

    void SetUp() override
    {
        renderEnvironment = std::make_shared<RenderEnvironment>();
        renderEnvironment->Init();
        renderEnvironment->Prepare();

        std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
        bufferInfo->width_ = WIDTH;
        bufferInfo->height_ = HEIGHT;
        bufferInfo->rowStride_ = ROW_STRIDE;
        bufferInfo->len_ = LEN;
        bufferInfo->formatType_ = FORMATE_TYPE;
        bufferInfo->surfaceBuffer_ = nullptr;
        void *addr = malloc(bufferInfo->len_);

        std::shared_ptr<ExtraInfo> extraInfo = std::make_unique<ExtraInfo>();
        extraInfo->dataType = DataType::PIXEL_MAP;
        extraInfo->bufferType = BufferType::HEAP_MEMORY;
        effectBuffer = std::make_shared<EffectBuffer>(bufferInfo, addr, extraInfo);

        free(addr);
        addr = nullptr;
    }

    void TearDown() override
    {
        effectBuffer = nullptr;
        if (renderEnvironment == nullptr) {
            return;
        }
        renderEnvironment->ReleaseParam();
        renderEnvironment->Release();
    }

    std::shared_ptr<EffectBuffer> effectBuffer;
    std::shared_ptr<RenderEnvironment> renderEnvironment;
};

HWTEST_F(TestRenderEnvironment, TestRenderEnvironment002, TestSize.Level1)
{
    IEffectFormat format = IEffectFormat::YUVNV12;
    GLuint tex = renderEnvironment->ConvertFromYUVToRGB(effectBuffer.get(), format);
    EXPECT_NE(tex, 0);

    GraphicTransformType type = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    renderEnvironment->DrawFrame(tex, type);
    renderEnvironment->DrawFrameWithTransform(effectBuffer, type);
    effectBuffer->bufferInfo_->tex_ = renderEnvironment->RequestBuffer(WIDTH, HEIGHT);
    EXPECT_NE(effectBuffer->bufferInfo_->tex_, nullptr);

    type = GraphicTransformType::GRAPHIC_ROTATE_90;
    renderEnvironment->DrawFrame(tex, type);
    renderEnvironment->DrawFrameWithTransform(effectBuffer, type);
    type = GraphicTransformType::GRAPHIC_FLIP_H_ROT90;
    renderEnvironment->DrawFrame(tex, type);
    type = GraphicTransformType::GRAPHIC_FLIP_V_ROT90;
    renderEnvironment->DrawFrame(tex, type);

    type = GraphicTransformType::GRAPHIC_ROTATE_180;
    renderEnvironment->DrawFrame(tex, type);
    renderEnvironment->DrawFrameWithTransform(effectBuffer, type);
    type = GraphicTransformType::GRAPHIC_FLIP_H_ROT180;
    renderEnvironment->DrawFrame(tex, type);
    type = GraphicTransformType::GRAPHIC_FLIP_V_ROT180;
    renderEnvironment->DrawFrame(tex, type);

    type = GraphicTransformType::GRAPHIC_ROTATE_270;
    renderEnvironment->DrawFrame(tex, type);
    renderEnvironment->DrawFrameWithTransform(effectBuffer, type);
    type = GraphicTransformType::GRAPHIC_FLIP_H_ROT270;
    renderEnvironment->DrawFrame(tex, type);
    type = GraphicTransformType::GRAPHIC_FLIP_V_ROT270;
    renderEnvironment->DrawFrame(tex, type);
}

}
}
}
}