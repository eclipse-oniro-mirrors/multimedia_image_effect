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

HWTEST_F(TestRenderEnvironment, TestRenderEnvironment001, TestSize.Level1)
{
    std::shared_ptr<EffectBuffer> output;
    renderEnvironment->outType_ = DataType::NATIVE_WINDOW;
    effectBuffer->bufferInfo_->width_ = 2 * WIDTH;
    renderEnvironment->GenTex(effectBuffer, output);

    effectBuffer->bufferInfo_->formatType_ = IEffectFormat::DEFAULT;
    renderEnvironment->GenTex(effectBuffer, output);

    RenderTexturePtr texptr = renderEnvironment->RequestBuffer(WIDTH, HEIGHT);
    EXPECT_NE(texptr, nullptr);
    renderEnvironment->ConvertTextureToBuffer(texptr, effectBuffer.get());

    GLenum internalFormat = GL_RG8;
    size_t ret = GLUtils::GetInternalFormatPixelByteSize(internalFormat);
    EXPECT_EQ(ret, 0);

    internalFormat = GL_R8;
    ret = GLUtils::GetInternalFormatPixelByteSize(internalFormat);
    EXPECT_EQ(ret, 1);

    internalFormat = GL_RGB565;
    ret = GLUtils::GetInternalFormatPixelByteSize(internalFormat);
    EXPECT_EQ(ret, 2);

    internalFormat = GL_RGBA4;
    ret = GLUtils::GetInternalFormatPixelByteSize(internalFormat);
    EXPECT_EQ(ret, 2);

    internalFormat = GL_RGBA16F;
    ret = GLUtils::GetInternalFormatPixelByteSize(internalFormat);
    EXPECT_EQ(ret, 8);
}

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

HWTEST_F(TestRenderEnvironment, TestRenderEnvironment003, TestSize.Level1)
{
    std::shared_ptr<EffectBuffer> input;
    RenderTexturePtr texptr = renderEnvironment->RequestBuffer(WIDTH, HEIGHT);
    EXPECT_NE(texptr, nullptr);

    effectBuffer->bufferInfo_->tex_ = texptr;
    renderEnvironment->ConvertYUV2RGBA(effectBuffer, input);
    effectBuffer->bufferInfo_->surfaceBuffer_ = nullptr;
    std::shared_ptr<EffectBuffer> buffer =
        renderEnvironment->ConvertBufferToTexture(effectBuffer.get());
    EXPECT_NE(buffer, nullptr);
    
    bool result = renderEnvironment->IfNeedGenMainTex();
    EXPECT_EQ(result, true);

    RenderContext *context = renderEnvironment->GetContext();
    EXPECT_NE(context, nullptr);

    ResourceCache *ceCache = renderEnvironment->GetResourceCache();
    EXPECT_NE(ceCache, nullptr);
}

HWTEST_F(TestRenderEnvironment, TestRenderEnvironment004, TestSize.Level1)
{
    RenderTexturePtr texptr = renderEnvironment->RequestBuffer(WIDTH, HEIGHT);
    EXPECT_NE(texptr, nullptr);

    IEffectFormat format = IEffectFormat::YUVNV21;
    GLuint tex = renderEnvironment->ConvertFromYUVToRGB(effectBuffer.get(), format);
    EXPECT_NE(tex, 0);
    renderEnvironment->ConvertFromRGBToYUV(texptr, format, effectBuffer->buffer_);

    format = IEffectFormat::YUVNV12;
    renderEnvironment->ConvertFromRGBToYUV(texptr, format, effectBuffer->buffer_);
}

HWTEST_F(TestRenderEnvironment, TestRenderEnvironment005, TestSize.Level1)
{
    std::shared_ptr<RenderContext> renderContext = std::make_shared<RenderContext>();
    bool result = renderContext->SwapBuffers(renderEnvironment->screenSurface_);
    EXPECT_EQ(result, false);

    result = renderContext->Init();
    EXPECT_EQ(result, true);

    result = renderContext->SwapBuffers(renderEnvironment->screenSurface_);
    EXPECT_EQ(result, false);
}

HWTEST_F(TestRenderEnvironment, TestRenderEnvironment006, TestSize.Level1)
{
    std::shared_ptr<EffectContext> effectContext = std::make_shared<EffectContext>();
    effectContext->renderEnvironment_ = renderEnvironment;
    effectContext->memoryManager_ = std::make_shared<EffectMemoryManager>();
    MemoryInfo memInfo = {
        .bufferInfo = {
            .width_ = effectBuffer->bufferInfo_->width_,
            .height_ = effectBuffer->bufferInfo_->height_,
            .len_ = effectBuffer->bufferInfo_->len_,
            .formatType_ = effectBuffer->bufferInfo_->formatType_,
        },
        .bufferType = BufferType::DMA_BUFFER,
    };
    MemoryData *memoryData = effectContext->memoryManager_->AllocMemory(effectBuffer->buffer_, memInfo);
    MemoryInfo &allocMemInfo = memoryData->memoryInfo;
    SurfaceBuffer *buffer = (allocMemInfo.bufferType == BufferType::DMA_BUFFER) ?
        static_cast<SurfaceBuffer *>(allocMemInfo.extra) : nullptr;

    std::shared_ptr<EffectBuffer> input;
    RenderTexturePtr texptr = renderEnvironment->RequestBuffer(WIDTH, HEIGHT);
    EXPECT_NE(texptr, nullptr);

    effectBuffer->bufferInfo_->tex_ = texptr;
    effectBuffer->bufferInfo_->surfaceBuffer_ = buffer;
    effectBuffer->bufferInfo_->formatType_ = IEffectFormat::YUVNV12;

    renderEnvironment->ConvertYUV2RGBA(effectBuffer, input);
    renderEnvironment->DrawTexFromSurfaceBuffer(texptr, buffer);

    IEffectFormat format = IEffectFormat::RGBA8888;
    renderEnvironment->DrawFlipSurfaceBufferFromTex(texptr, buffer, format);

    format = IEffectFormat::YUVNV12;
    renderEnvironment->DrawFlipSurfaceBufferFromTex(texptr, buffer, format);
    renderEnvironment->DrawSurfaceBufferFromTex(texptr, buffer, format);
}
HWTEST_F(TestRenderEnvironment, TestRenderEnvironment007, TestSize.Level1) {
    std::string tag = "TestTag";
    std::shared_ptr<RenderSurface> renderSurface = std::make_shared<RenderSurface>(tag);

    bool result = renderSurface->Init();
    EXPECT_EQ(result, true);

    void *window = nullptr;
    result = renderSurface->Create(window);
    EXPECT_EQ(result, false);
    
    result = renderSurface->Release();
    EXPECT_EQ(result, true);

    RenderSurface::SurfaceType type = renderSurface->GetSurfaceType();
    EXPECT_EQ(type, RenderSurface::SurfaceType::SURFACE_TYPE_NULL);
}

HWTEST_F(TestRenderEnvironment, TestRenderEnvironment008, TestSize.Level1) {
    unsigned char *bitmap = new unsigned char[WIDTH * HEIGHT * 4];

    int temp = renderEnvironment->GenTextureWithPixels(bitmap, WIDTH, HEIGHT, WIDTH);
    EXPECT_NE(temp, 0);

    temp = renderEnvironment->GenTextureWithPixels(bitmap, WIDTH - 10, HEIGHT, WIDTH);
    EXPECT_NE(temp, 0);
    delete[] bitmap;
    bitmap = nullptr;
}

HWTEST_F(TestRenderEnvironment, RenderFrameBuffer_001, TestSize.Level1) {
    ResourceCache *ceCache = renderEnvironment->GetResourceCache();
    RenderFrameBuffer *renderFrameBuffer = new RenderFrameBuffer(ceCache, WIDTH, HEIGHT);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    renderFrameBuffer->Resize(WIDTH/2, HEIGHT);
    renderFrameBuffer->Resize(WIDTH, HEIGHT);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    renderFrameBuffer->Bind();
    renderFrameBuffer->UnBind();

    RenderTexturePtr ptr = renderFrameBuffer->Texture();
    EXPECT_NE(ptr, nullptr);

    delete renderFrameBuffer;
    renderFrameBuffer = nullptr;
}

HWTEST_F(TestRenderEnvironment, DrawFlipTex_001, TestSize.Level1) {
    RenderTexturePtr inputTexptr = renderEnvironment->RequestBuffer(WIDTH, HEIGHT);
    RenderTexturePtr outputTexptr = renderEnvironment->RequestBuffer(WIDTH, HEIGHT);
    EXPECT_NE(inputTexptr, nullptr);
    EXPECT_NE(outputTexptr, nullptr);
    ASSERT_NO_THROW(renderEnvironment->DrawFlipTex(inputTexptr, outputTexptr));
}

HWTEST_F(TestRenderEnvironment, DrawOesTexture2DFromTexture_001, TestSize.Level1) {
    RenderTexturePtr inputTexptr = renderEnvironment->RequestBuffer(WIDTH, HEIGHT);
    EXPECT_NE(inputTexptr, nullptr);

    GLuint outputTex = 0;
    renderEnvironment->DrawOesTexture2DFromTexture(inputTexptr, outputTex, WIDTH, HEIGHT, IEffectFormat::RGBA8888);

    outputTex = 1;
    ASSERT_NO_THROW(renderEnvironment->DrawOesTexture2DFromTexture(inputTexptr,
        outputTex, WIDTH, HEIGHT, IEffectFormat::RGBA8888));

    ASSERT_NO_THROW(renderEnvironment->DrawOesTexture2DFromTexture(inputTexptr,
        outputTex, WIDTH, HEIGHT, IEffectFormat::YUVNV21));
}

HWTEST_F(TestRenderEnvironment, DrawSurfaceBufferFromSurfaceBuffer_001, TestSize.Level1) {
    RenderTexturePtr inputTexptr = renderEnvironment->RequestBuffer(WIDTH, HEIGHT);
    EXPECT_NE(inputTexptr, nullptr);

    sptr<SurfaceBuffer> inBuffer;
    MockProducerSurface::AllocDmaMemory(inBuffer);
    sptr<SurfaceBuffer> outBuffer;
    MockProducerSurface::AllocDmaMemory(outBuffer);

    renderEnvironment->DrawSurfaceBufferFromSurfaceBuffer(nullptr, outBuffer, IEffectFormat::RGBA8888);

    ASSERT_NO_THROW(renderEnvironment->DrawSurfaceBufferFromSurfaceBuffer(inBuffer,
        outBuffer, IEffectFormat::RGBA8888));

    ASSERT_NO_THROW(renderEnvironment->DrawSurfaceBufferFromSurfaceBuffer(inBuffer,
        outBuffer, IEffectFormat::YUVNV21));

    MockProducerSurface::ReleaseDmaBuffer(inBuffer);
    MockProducerSurface::ReleaseDmaBuffer(outBuffer);
}
}
}
}
}