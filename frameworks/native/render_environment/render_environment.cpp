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

#include "render_environment.h"

#include <GLES2/gl2ext.h>

#include "effect_trace.h"
#include "effect_log.h"
#include "effect_memory.h"
#include "format_helper.h"
#include "base/math/math_utils.h"

namespace OHOS {
namespace Media {
namespace Effect {
const char* const DEFAULT_FSHADER = "#extension GL_OES_EGL_image_external : require\n"
    "precision highp float;\n"
    "uniform samplerExternalOES inputTexture;\n"
    "varying vec2 textureCoordinate;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = texture2D(inputTexture, textureCoordinate);\n"
    "}\n";

constexpr const static int RGB_PLANE_SIZE = 3;
constexpr const static int G_POS = 1;
constexpr const static int B_POS = 2;
constexpr const static int UV_PLANE_SIZE = 2;

EGLStatus RenderEnvironment::GetEGLStatus() const
{
    return isEGLReady;
}

void RenderEnvironment::Init()
{
    EFFECT_LOGI("RenderEnvironment start init");
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglInitialize(display, nullptr, nullptr)) {
        EFFECT_LOGE("EGL Initialize failed");
    }
    param_ = new RenderParam();
    param_->context_ = new RenderContext();

    if (param_->context_->Init()) {
        isEGLReady = EGLStatus::READY;
    }
}

void RenderEnvironment::Prepare()
{
    if (param_->context_->MakeCurrent(screenSurface_)) {
        param_->renderer_ = new RenderOpenglRenderer();
        InitDefaultShaderMT(param_);
        InitDefaultMeshMT(param_);
        param_->threadReady_ = true;
    } else {
        param_->threadReady_ = false;
    }
}

RenderMesh *RenderEnvironment::CreateMeshMT(RenderParam *param, bool isBackGround, RenderGeneralProgram *shader)
{
    const std::vector<std::vector<float>> meshData = isBackGround ? DEFAULT_FLIP_VERTEX_DATA : DEFAULT_VERTEX_DATA;
    RenderMesh *mesh = new RenderMesh(meshData);
    mesh->Bind(shader);
    return mesh;
}

void RenderEnvironment::InitDefaultMeshMT(RenderParam *param)
{
    param->meshBase_ = CreateMeshMT(param, false, param->shaderBase_);
    param->meshBaseFlip_ = CreateMeshMT(param, true, param->shaderBase_);
    param->meshBaseDMA_ = CreateMeshMT(param, false, param->shaderBaseDMA_);
    param->meshBaseFlipYUVDMA_ = CreateMeshMT(param, true, param->shaderBaseYUVDMA2RGB2D_);
    param->meshBaseYUVDMA_ = CreateMeshMT(param, false, param->shaderBaseYUVDMA2RGB2D_);
    param->meshBaseDrawFrame_ = CreateMeshMT(param, false, param->shaderBaseDrawFrame_);
    param->meshBaseDrawFrameYUV_ = CreateMeshMT(param, true, param->shaderBaseDrawFrameYUV_);
}

void RenderEnvironment::InitDefaultShaderMT(RenderParam *param)
{
    param->shaderBase_ = new RenderGeneralProgram(param->context_, DEFAULT_VERTEX_SHADER_SCREEN_CODE,
        DEFAULT_FRAGMENT_SHADER_CODE);
    param->shaderBase_->Init();
    param->shaderBaseDMA_ = new RenderGeneralProgram(param->context_, DEFAULT_VERTEX_SHADER_SCREEN_CODE,
        DEFAULT_FSHADER);
    param->shaderBaseDMA_->Init();
    param->shaderBaseYUVDMA_ = new RenderGeneralProgram(param->context_, DEFAULT_YUV_VERTEX_SHADER,
        DEFAULT_YUV_SHADER_CODE);
    param->shaderBaseYUVDMA_->Init();
    param->shaderBaseYUVDMA2RGB2D_ = new RenderGeneralProgram(param->context_, DEFAULT_YUV_VERTEX_SHADER,
        DEFAULT_YUV_RGBA_SHADER_CODE);
    param->shaderBaseYUVDMA2RGB2D_->Init();
    param->shaderBaseRGB2D2YUVDMA_ = new RenderGeneralProgram(param->context_, DEFAULT_YUV_VERTEX_SHADER,
        DEFAULT_RGBA_YUV_SHADER_CODE);
    param->shaderBaseRGB2D2YUVDMA_->Init();
    param->shaderBaseDrawFrame_ = new RenderGeneralProgram(param->context_, TRANSFORM_VERTEX_SHADER_SCREEN_CODE,
        DEFAULT_FRAGMENT_SHADER_CODE);
    param->shaderBaseDrawFrame_->Init();
    param->shaderBaseDrawFrameYUV_ = new RenderGeneralProgram(param->context_, TRANSFORM_YUV_VERTEX_SHADER,
        DEFAULT_YUV_RGBA_SHADER_CODE);
    param->shaderBaseDrawFrameYUV_->Init();
}

void RenderEnvironment::InitEngine(OHNativeWindow *window)
{
    EFFECT_LOGI("RenderEnvironment InitEngine start");
    if (window_ != nullptr) {
        return;
    }
    window_ = window;
    screenSurface_ = new RenderSurface(std::string());
    screenSurface_->SetAttrib(attribute_);
    screenSurface_->Create(window);
}

bool RenderEnvironment::BeginFrame()
{
    return param_->context_->MakeCurrent(screenSurface_);
}

RenderTexturePtr RenderEnvironment::RequestBuffer(int width, int height)
{
    RenderTexturePtr renderTex = param_->resCache_->RequestTexture(param_->context_, width, height, GL_RGBA8);
    return renderTex;
}

bool RenderEnvironment::IsPrepared() const
{
    return param_->threadReady_;
}

void RenderEnvironment::GenMainTex(const std::shared_ptr<EffectBuffer> &source, std::shared_ptr<EffectBuffer> &output)
{
    std::shared_ptr<BufferInfo> info =source->bufferInfo_;
    int width = static_cast<int>(info->width_);
    int height = static_cast<int>(info->height_);
    RenderTexturePtr renderTex;
    bool needRender = true;
    renderTex = param_->resCache_->GetTexGlobalCache("Main");
    if (renderTex == nullptr || hasInputChanged) {
        renderTex = param_->resCache_->RequestTexture(param_->context_, width, height, GL_RGBA8);
        param_->resCache_->AddTexGlobalCache("Main", renderTex);
    } else {
        needRender = false;
    }

    if (needRender || hasInputChanged) {
        DrawImageToFBO(renderTex, source.get());
        hasInputChanged = false;
    }

    if (outType_ == DataType::NATIVE_WINDOW) {
        RenderTexturePtr tempTex = param_->resCache_->RequestTexture(param_->context_, width, height, GL_RGBA8);
        DrawFlipTex(renderTex, tempTex);
        renderTex = tempTex;
    }
    output = GenTexEffectBuffer(source);
    output->bufferInfo_->formatType_ = IEffectFormat::RGBA8888;
    output->tex = renderTex;
}

void RenderEnvironment::DrawFlipTex(RenderTexturePtr input, RenderTexturePtr output)
{
    GLuint tempFbo = GLUtils::CreateFramebuffer(output->GetName());
    RenderViewport vp(0, 0, input->Width(), input->Height());
    param_->renderer_->Draw(input->GetName(), tempFbo, param_->meshBaseFlip_, param_->shaderBase_, &vp, GL_TEXTURE_2D);
    GLUtils::DeleteFboOnly(tempFbo);
}

std::shared_ptr<EffectBuffer> RenderEnvironment::ConvertBufferToTexture(EffectBuffer *source)
{
    std::shared_ptr<BufferInfo> info = source->bufferInfo_;
    int width = static_cast<int>(info->width_);
    int height = static_cast<int>(info->height_);
    RenderTexturePtr renderTex = param_->resCache_->RequestTexture(param_->context_, width, height, GL_RGBA8);
    DrawImageToFBO(renderTex, source);

    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    bufferInfo->width_ = info->width_;
    bufferInfo->height_ = info->height_;
    bufferInfo->rowStride_ = info->rowStride_;
    bufferInfo->len_ = info->len_;
    bufferInfo->formatType_ = info->formatType_;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_unique<ExtraInfo>();
    extraInfo->dataType = DataType::TEX;
    std::shared_ptr<EffectBuffer> output = std::make_shared<EffectBuffer>(bufferInfo, nullptr, extraInfo);
    output->tex = renderTex;
    return output;
}

void RenderEnvironment::NotifyInputChanged()
{
    hasInputChanged = true;
}

bool RenderEnvironment::IfNeedGenMainTex() const
{
    return hasInputChanged;
}

void RenderEnvironment::UpdateCanvas()
{
    if (window_ != nullptr) {
        OH_NativeWindow_NativeWindowHandleOpt(window_, GET_BUFFER_GEOMETRY, &canvasHeight, &canvasWidth);
        param_->viewport_.Set(0, 0, canvasWidth, canvasHeight);
    }
}

void RenderEnvironment::DrawImageToFBO(RenderTexturePtr renderTex, const EffectBuffer *source)
{
    GLuint tempFbo = GLUtils::CreateFramebuffer(renderTex->GetName());
    GLuint tex = GenTexFromEffectBuffer(source);
    GLenum target = source->extraInfo_->surfaceBuffer != nullptr ? GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D;

    RenderGeneralProgram *program;
    RenderMesh *mesh;
    if (target == GL_TEXTURE_EXTERNAL_OES) {
        if (source->bufferInfo_->formatType_ == IEffectFormat::RGBA8888) {
            program = param_->shaderBaseDMA_;
            mesh = param_->meshBaseDMA_;
        } else {
            program = param_->shaderBaseYUVDMA2RGB2D_;
            mesh = param_->meshBaseYUVDMA_;
        }
    } else {
        program = param_->shaderBase_;
        mesh = param_->meshBase_;
    }

    RenderViewport vp(0, 0, renderTex->Width(), renderTex->Height());
    param_->renderer_->Draw(tex, tempFbo, mesh, program, &vp, target);
    glBindTexture(target, 0);
    GLUtils::DeleteTexture(tex);
    GLUtils::DeleteFboOnly(tempFbo);
}

GLuint RenderEnvironment::GenTexFromEffectBuffer(const EffectBuffer *source)
{
    GLuint tex;
    int width = static_cast<int>(source->bufferInfo_->width_);
    int height = static_cast<int>(source->bufferInfo_->height_);
    if (source->bufferInfo_->formatType_ == IEffectFormat::RGBA8888) {
        int stride = static_cast<int>(source->bufferInfo_->rowStride_) / 4;
        if (source->extraInfo_->surfaceBuffer != nullptr) {
            source->extraInfo_->surfaceBuffer->FlushCache();
            tex = GLUtils::CreateTextureFromSurfaceBuffer(source->extraInfo_->surfaceBuffer);
        } else {
            tex = GLUtils::CreateTexWithStorage(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
            glBindTexture(GL_TEXTURE_2D, tex);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            if (width == stride) {
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, source->buffer_);
            } else {
                glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, source->buffer_);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            }
        }
    } else {
        if (source->extraInfo_->surfaceBuffer != nullptr) {
            source->extraInfo_->surfaceBuffer->FlushCache();
            tex = GLUtils::CreateTextureFromSurfaceBuffer(source->extraInfo_->surfaceBuffer);
        } else {
            tex = ConvertFromYUVToRGB(source, source->bufferInfo_->formatType_);
        }
    }
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
    return tex;
}

GLuint RenderEnvironment::ConvertFromYUVToRGB(const EffectBuffer *source, IEffectFormat format)
{
    uint32_t width = source->bufferInfo_->width_;
    uint32_t height = source->bufferInfo_->height_;
    auto *srcNV12 = static_cast<unsigned char *>(source->buffer_);
    uint8_t *srcNV12UV = srcNV12 + width * height;
    unsigned char *data = new unsigned char[width * height * RGBA_SIZE_PER_PIXEL];
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            uint32_t nv_index = i / UV_PLANE_SIZE * width + j - j % UV_PLANE_SIZE; // 2 mean u/v split factor
            uint32_t y_index = i * width + j;

            uint8_t y;
            uint8_t u;
            uint8_t v;
            if (format == IEffectFormat::YUVNV12) {
                y = srcNV12[y_index];
                u = srcNV12UV[nv_index];
                v = srcNV12UV[nv_index + 1];
            } else {
                y = srcNV12[y_index];
                v = srcNV12UV[nv_index];
                u = srcNV12UV[nv_index + 1];
            }
            uint8_t r = FormatHelper::YuvToR(y, u, v);
            uint8_t g = FormatHelper::YuvToG(y, u, v);
            uint8_t b = FormatHelper::YuvToB(y, u, v);
            uint32_t rgb_index = i * width * RGB_PLANE_SIZE + j * RGB_PLANE_SIZE;
            data[rgb_index] = r;
            data[rgb_index + G_POS] = g;
            data[rgb_index + B_POS] = b;
        }
    }
    GLuint tex = GLUtils::CreateTexWithStorage(GL_TEXTURE_2D, 1, GL_RGB, width, height);
    delete[] data;
    return tex;
}

void RenderEnvironment::ConvertFromRGBToYUV(RenderTexturePtr input, IEffectFormat format, void *data)
{
    uint32_t width = input->Width();
    uint32_t height = input->Height();
    unsigned char *rgbData = new unsigned char[width * height * RGBA_SIZE_PER_PIXEL];
    ReadPixelsFromTex(input, rgbData, width, height, width);
    auto *srcNV12 = static_cast<unsigned char *>(data);
    uint8_t *srcNV12UV = srcNV12 + width * height;
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            uint32_t y_index = i * width + j;
            uint32_t nv_index = i / UV_PLANE_SIZE * width + j - j % UV_PLANE_SIZE; // 2 mean u/v split factor
            uint32_t rgb_index = i * width * RGBA_SIZE_PER_PIXEL + j * RGBA_SIZE_PER_PIXEL;
            uint8_t r = rgbData[rgb_index];
            uint8_t g = rgbData[rgb_index + G_POS];
            uint8_t b = rgbData[rgb_index + B_POS];
            srcNV12[y_index] = FormatHelper::RGBToY(r, g, b);
            if (format == IEffectFormat::YUVNV12) {
                srcNV12UV[nv_index] = FormatHelper::RGBToU(r, g, b);
                srcNV12UV[nv_index + 1] = FormatHelper::RGBToV(r, g, b);
            } else {
                srcNV12UV[nv_index] = FormatHelper::RGBToV(r, g, b);
                srcNV12UV[nv_index + 1] = FormatHelper::RGBToU(r, g, b);
            }
        }
    }
    delete[] rgbData;
}

RenderContext *RenderEnvironment::GetContext()
{
    return param_->context_;
}

ResourceCache *RenderEnvironment::GetResourceCache()
{
    return param_->resCache_;
}

glm::mat4 GetTransformMatrix(GraphicTransformType type)
{
    glm::mat4 trans = glm::mat4(1.0f);
    switch (type) {
        case GRAPHIC_ROTATE_90:
            trans = glm::rotate(trans, glm::radians(90.0f), glm::vec3(0.0, 0.0, 1.0));
            break;
        case GRAPHIC_ROTATE_180:
            trans = glm::rotate(trans, glm::radians(180.0f), glm::vec3(0.0, 0.0, 1.0));
            break;
        case GRAPHIC_ROTATE_270:
            trans = glm::rotate(trans, glm::radians(270.0f), glm::vec3(0.0, 0.0, 1.0));
            break;
        default:
            break;
    }
    return trans;
}

void RenderEnvironment::DrawFrameWithTransform(const std::shared_ptr<EffectBuffer> &buffer, GraphicTransformType type)
{
    if (param_ != nullptr) {
        BeginFrame();
        UpdateCanvas();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glm::mat4 trans = GetTransformMatrix(type);
        if (buffer->tex == nullptr) {
            EFFECT_LOGE("RenderEnvironment DrawFrameWithTransform tex is nullptr");
            return;
        }
        param_->renderer_->DrawOnScreen(buffer->tex->GetName(), param_->meshBaseDrawFrame_,
            param_->shaderBaseDrawFrame_, &param_->viewport_, MathUtils::NativePtr(trans), GL_TEXTURE_2D);

        if (screenSurface_ == nullptr) {
            EFFECT_LOGE("RenderEnvironment screenSurface_ is nullptr");
            return;
        }
        param_->context_->SwapBuffers(screenSurface_);
        GLUtils::CheckError(__FILE_NAME__, __LINE__);
    }
}

void RenderEnvironment::DrawFrame(GLuint texId, GraphicTransformType type)
{
    if (param_ != nullptr) {
        BeginFrame();
        UpdateCanvas();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(1.0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        
        RenderMesh *mesh = new RenderMesh(DEFAULT_FLIP_VERTEX_DATA);
        mesh->Bind(param_->shaderBaseDrawFrameYUV_);
        param_->renderer_->DrawOnScreenWithTransform(texId, mesh,
            param_->shaderBaseDrawFrameYUV_, &param_->viewport_, type, GL_TEXTURE_EXTERNAL_OES);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
        if (screenSurface_ == nullptr) {
            EFFECT_LOGE("RenderEnvironment DrawFrame is nullptr");
            return;
        }
        param_->context_->SwapBuffers(screenSurface_);
        GLUtils::CheckError(__FILE_NAME__, __LINE__);
    }
}

void RenderEnvironment::ConvertTextureToBuffer(RenderTexturePtr source, EffectBuffer *output)
{
    int w = static_cast<int>(source->Width());
    int h = static_cast<int>(source->Height());
    if (output->extraInfo_->surfaceBuffer == nullptr) {
        if (output->bufferInfo_->formatType_ == IEffectFormat::RGBA8888) {
            ReadPixelsFromTex(source, output->buffer_, w, h, output->bufferInfo_->rowStride_ / RGBA_SIZE_PER_PIXEL);
        } else {
            ConvertFromRGBToYUV(source, output->bufferInfo_->formatType_, output->buffer_);
        }
    } else {
        DrawSurfaceBufferFromTex(source, output->extraInfo_->surfaceBuffer, output->bufferInfo_->formatType_);
    }
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}

void RenderEnvironment::ConvertYUV2RGBA(std::shared_ptr<EffectBuffer> &source, std::shared_ptr<EffectBuffer> &out)
{
    int width = static_cast<int>(source->bufferInfo_->width_);
    int height = static_cast<int>(source->bufferInfo_->height_);
    RenderTexturePtr outTex;
    if (source->extraInfo_->surfaceBuffer == nullptr) {
        outTex = std::make_shared<RenderTexture>(param_->context_, width, height, GL_RGBA8);
        outTex->SetName(ConvertFromYUVToRGB(source.get(), source->bufferInfo_->formatType_));
    } else {
        outTex = param_->resCache_->RequestTexture(param_->context_, width, height, GL_RGBA8);
        GLuint sourceTex = GLUtils::CreateTextureFromSurfaceBuffer(source->extraInfo_->surfaceBuffer);
        GLuint tempFbo = GLUtils::CreateFramebufferWithTarget(outTex->GetName(), GL_TEXTURE_2D);
        RenderViewport vp(0, 0, width, height);
        param_->renderer_->Draw(sourceTex, tempFbo, param_->meshBaseFlipYUVDMA_, param_->shaderBaseYUVDMA2RGB2D_, &vp,
            GL_TEXTURE_EXTERNAL_OES);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    }

    out = GenTexEffectBuffer(source);
    out->bufferInfo_->formatType_ = IEffectFormat::RGBA8888;
    out->tex = outTex;
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}

void RenderEnvironment::ConvertRGBA2YUV(std::shared_ptr<EffectBuffer> &source, std::shared_ptr<EffectBuffer> &out)
{
    int width = static_cast<int>(source->bufferInfo_->width_);
    int height = static_cast<int>(source->bufferInfo_->height_);
    RenderTexturePtr sourceTex = source->tex;
    GLuint outTex = GLUtils::CreateTextureFromSurfaceBuffer(out->extraInfo_->surfaceBuffer);
    RenderTexturePtr tex = std::make_shared<RenderTexture>(param_->context_, width, height, GL_RGBA8);
    tex->SetName(outTex);
    Draw2D2OES(sourceTex, tex);
    glFinish();
}

void RenderEnvironment::Draw2D2OES(RenderTexturePtr source, RenderTexturePtr output)
{
    int w = static_cast<int>(source->Width());
    int h = static_cast<int>(source->Height());
    GLuint tempFbo = GLUtils::CreateFramebufferWithTarget(output->GetName(), GL_TEXTURE_EXTERNAL_OES);
    RenderViewport vp(0, 0, w, h);
    param_->renderer_->Draw(source->GetName(), tempFbo, param_->meshBaseDMA_, param_->shaderBaseRGB2D2YUVDMA_, &vp,
        GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}

void RenderEnvironment::ReadPixelsFromTex(RenderTexturePtr tex, void *data, int width, int height, int stride)
{
    GLuint inFbo = GLUtils::CreateFramebuffer(tex->GetName());
    glBindFramebuffer(GL_FRAMEBUFFER, inFbo);
    glPixelStorei(GL_PACK_ROW_LENGTH, stride);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    GLUtils::DeleteFboOnly(inFbo);
}

void RenderEnvironment::DrawSurfaceBufferFromTex(RenderTexturePtr tex, SurfaceBuffer *buffer, IEffectFormat format)
{
    GLuint outTex = GLUtils::CreateTextureFromSurfaceBuffer(buffer);
    GLuint tempFbo = GLUtils::CreateFramebufferWithTarget(outTex, GL_TEXTURE_EXTERNAL_OES);
    RenderViewport vp(0, 0, tex->Width(), tex->Height());
    if (format == IEffectFormat::RGBA8888) {
        param_->renderer_->Draw(tex->GetName(), tempFbo, param_->meshBase_, param_->shaderBase_, &vp, GL_TEXTURE_2D);
    } else {
        param_->renderer_->Draw(tex->GetName(), tempFbo, param_->meshBaseDMA_, param_->shaderBaseRGB2D2YUVDMA_, &vp,
            GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glFinish();
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}

void RenderEnvironment::DrawFlipSurfaceBufferFromTex(RenderTexturePtr tex, SurfaceBuffer *buffer, IEffectFormat format)
{
    GLuint outTex = GLUtils::CreateTextureFromSurfaceBuffer(buffer);
    GLuint tempFbo = GLUtils::CreateFramebufferWithTarget(outTex, GL_TEXTURE_EXTERNAL_OES);
    RenderViewport vp(0, 0, tex->Width(), tex->Height());
    if (format == IEffectFormat::RGBA8888) {
        param_->renderer_->Draw(tex->GetName(), tempFbo, param_->meshBaseFlip_, param_->shaderBase_, &vp,
            GL_TEXTURE_2D);
    } else {
        param_->renderer_->Draw(tex->GetName(), tempFbo, param_->meshBaseFlipYUVDMA_, param_->shaderBaseRGB2D2YUVDMA_,
            &vp, GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glFinish();
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}

void RenderEnvironment::DrawTexFromSurfaceBuffer(RenderTexturePtr tex, SurfaceBuffer *buffer)
{
    GLuint tempFbo = GLUtils::CreateFramebuffer(tex->GetName());
    GLuint input = GLUtils::CreateTextureFromSurfaceBuffer(buffer);
    RenderViewport vp(0, 0, tex->Width(), tex->Height());
    param_->renderer_->Draw(input, tempFbo, param_->meshBaseDMA_, param_->shaderBaseDMA_, &vp,
        GL_TEXTURE_EXTERNAL_OES);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    GLUtils::DeleteTexture(input);
    GLUtils::DeleteFboOnly(tempFbo);
}

std::shared_ptr<EffectBuffer> RenderEnvironment::GenTexEffectBuffer(std::shared_ptr<EffectBuffer> input)
{
    std::shared_ptr<BufferInfo> info = input->bufferInfo_;
    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    bufferInfo->width_ = info->width_;
    bufferInfo->height_ = info->height_;
    bufferInfo->rowStride_ = info->rowStride_;
    bufferInfo->len_ = info->len_;
    bufferInfo->formatType_ = info->formatType_;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_unique<ExtraInfo>();
    extraInfo->dataType = DataType::TEX;
    std::shared_ptr<EffectBuffer> out = std::make_unique<EffectBuffer>(bufferInfo, nullptr, extraInfo);
    return out;
}

DataType RenderEnvironment::GetOutputType() const
{
    return outType_;
}

void RenderEnvironment::SetOutputType(DataType type)
{
    outType_ = type;
}

void RenderEnvironment::Release()
{
    window_ = nullptr;
    if (screenSurface_) {
        screenSurface_->Release();
        delete screenSurface_;
        screenSurface_ = nullptr;
    }
}

void RenderEnvironment::ReleaseParam()
{
    if (param_ == nullptr) {
        return;
    }
    delete param_;
    param_ = nullptr;
    isEGLReady = EGLStatus::UNREADY;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS