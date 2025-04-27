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

#include <memory>
#include <sync_fence.h>
#include <GLES2/gl2ext.h>

#include "colorspace_helper.h"
#include "common_utils.h"
#include "effect_trace.h"
#include "effect_log.h"
#include "format_helper.h"
#include "metadata_helper.h"
#include "base/math/math_utils.h"

#include "native_window.h"

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

constexpr const static uint32_t RGB_PLANE_SIZE = 3;
constexpr const static int G_POS = 1;
constexpr const static int B_POS = 2;
constexpr const static uint32_t UV_PLANE_SIZE = 2;

EGLStatus RenderEnvironment::GetEGLStatus() const
{
    return isEGLReady;
}

void RenderEnvironment::Init()
{
    EFFECT_TRACE_NAME("RenderEnvironment::Init()");
    EFFECT_LOGI("RenderEnvironment init enter!");
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    needTerminate_ = true;
    if (eglInitialize(display, nullptr, nullptr) == EGL_FALSE) {
        needTerminate_ = false;
        EFFECT_LOGE("EGL Initialize failed");
    }
    param_ = new RenderParam();
    param_->context_ = new RenderContext();

    if (param_->context_->Init()) {
        isEGLReady = EGLStatus::READY;
    }
    EFFECT_LOGI("RenderEnvironment init end!");
}

void RenderEnvironment::Prepare()
{
    EFFECT_TRACE_NAME("RenderEnvironment::Prepare()");
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

void RenderEnvironment::SetNativeWindowColorSpace(EffectColorSpace colorSpace)
{
    OH_NativeBuffer_ColorSpace bufferColorSpace = ColorSpaceHelper::ConvertToNativeBufferColorSpace(colorSpace);
    OH_NativeBuffer_ColorSpace currentColorSpace;
    OH_NativeWindow_GetColorSpace(window_, &currentColorSpace);
    if (bufferColorSpace != currentColorSpace) {
        OH_NativeWindow_SetColorSpace(window_, bufferColorSpace);
    }
}

bool RenderEnvironment::BeginFrame()
{
    return param_->context_->MakeCurrent(screenSurface_);
}

RenderTexturePtr RenderEnvironment::RequestBuffer(int width, int height, GLenum format)
{
    RenderTexturePtr renderTex = param_->resCache_->RequestTexture(param_->context_, width, height, format);
    return renderTex;
}

bool RenderEnvironment::IsPrepared() const
{
    return param_->threadReady_;
}

RenderTexturePtr RenderEnvironment::ReCreateTexture(RenderTexturePtr renderTex, int width, int height,
    bool isHdr10) const
{
    RenderTexturePtr tempTex = param_->resCache_->RequestTexture(param_->context_, width, height,
        isHdr10 ? GL_RGB10_A2 : GL_RGBA8);
    GLuint tempFbo = GLUtils::CreateFramebuffer(tempTex->GetName());
    RenderViewport vp(0, 0, renderTex->Width(), renderTex->Height());

    param_->renderer_->Draw(renderTex->GetName(), tempFbo, param_->meshBase_, param_->shaderBase_, &vp, GL_TEXTURE_2D);
    GLUtils::DeleteFboOnly(tempFbo);

    return tempTex;
}

bool RenderEnvironment::GetOrCreateTextureFromCache(RenderTexturePtr& renderTex, const std::string& texName,
    int width, int height, bool isHdr10) const
{
    renderTex = param_->resCache_->GetTexGlobalCache(texName);
    if (renderTex == nullptr || hasInputChanged) {
        renderTex = param_->resCache_->RequestTexture(param_->context_, width, height,
            isHdr10 ? GL_RGB10_A2 : GL_RGBA8);
        param_->resCache_->AddTexGlobalCache(texName, renderTex);
        return true;
    }
    return false;
}

RenderTexturePtr RenderEnvironment::GenMainTex(const std::shared_ptr<EffectBuffer> &source, bool isHdr10)
{
    std::shared_ptr<BufferInfo> info = source->bufferInfo_;
    RenderTexturePtr renderTex = nullptr;
    int width = static_cast<int>(info->width_);
    int height = static_cast<int>(info->height_);

    bool needRender = GetOrCreateTextureFromCache(renderTex, "Main", width, height, isHdr10);
    if (needRender || hasInputChanged) {
        DrawBufferToTexture(renderTex, source.get());
        hasInputChanged = false;
    }

    return ReCreateTexture(renderTex, width, height, isHdr10);
}

std::unordered_map<std::string, RenderTexturePtr> RenderEnvironment::GenHdr8GainMapTexs(
    const std::shared_ptr<EffectBuffer> &source)
{
    std::unordered_map<std::string, RenderTexturePtr> texMap{};

    auto mainTexInfo = source->bufferInfo_;
    auto gainMapBufferInfo = source->auxiliaryBufferInfos->at(EffectPixelmapType::GAINMAP);
    auto gainMapExtraInfo = std::make_shared<ExtraInfo>();
    auto gainMapBuffer = std::make_shared<EffectBuffer>(gainMapBufferInfo, nullptr, gainMapExtraInfo);
    gainMapBuffer->tex = mainTexInfo->gainMapTex_;
    RenderTexturePtr primaryTex = nullptr;
    RenderTexturePtr gainMapTex = nullptr;

    bool needRender = GetOrCreateTextureFromCache(primaryTex, "Primary", static_cast<int>(mainTexInfo->width_),
        static_cast<int>(mainTexInfo->height_), false);
    needRender |= GetOrCreateTextureFromCache(gainMapTex, "GainMap",
        static_cast<int>(gainMapBufferInfo->width_), static_cast<int>(gainMapBufferInfo->height_), false);
    if (needRender || hasInputChanged) {
        DrawBufferToTexture(primaryTex, source.get());
        DrawBufferToTexture(gainMapTex, gainMapBuffer.get());
        hasInputChanged = false;
    }

    texMap.insert({"Primary", ReCreateTexture(primaryTex, static_cast<int>(mainTexInfo->width_),
        static_cast<int>(mainTexInfo->height_), false)});
    texMap.insert({"GainMap", ReCreateTexture(gainMapTex, static_cast<int>(gainMapBufferInfo->width_),
        static_cast<int>(gainMapBufferInfo->height_), false)});

    return texMap;
}

void RenderEnvironment::GenTex(const std::shared_ptr<EffectBuffer> &source, std::shared_ptr<EffectBuffer> &output)
{
    auto info = source->bufferInfo_;
    output = GenTexEffectBuffer(source);

    switch (info->hdrFormat_) {
        case HdrFormat::HDR8_GAINMAP: {
            auto texMap = GenHdr8GainMapTexs(source);
            output->tex = texMap["Primary"];
            output->bufferInfo_->gainMapTex_ = texMap["GainMap"];
            break;
        }
        case HdrFormat::HDR10:
            output->tex = GenMainTex(source, true);
            break;
        case HdrFormat::SDR:
        case HdrFormat::DEFAULT:
        default:
            output->tex = GenMainTex(source, false);
            break;
    }
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
    GLenum interFmt = source->bufferInfo_->hdrFormat_ == HdrFormat::HDR10 ? GL_RGB10_A2 : GL_RGBA8;
    RenderTexturePtr renderTex = param_->resCache_->RequestTexture(param_->context_, static_cast<int>(info->width_),
        static_cast<int>(info->height_), interFmt);
    DrawBufferToTexture(renderTex, source);

    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    CommonUtils::CopyBufferInfo(*info, *bufferInfo);
    std::shared_ptr<ExtraInfo> extraInfo = std::make_unique<ExtraInfo>();
    CommonUtils::CopyExtraInfo(*source->extraInfo_, *extraInfo);
    extraInfo->dataType = DataType::TEX;
    std::shared_ptr<EffectBuffer> output = std::make_shared<EffectBuffer>(bufferInfo, nullptr, extraInfo);
    output->tex = renderTex;
    if (source->bufferInfo_->hdrFormat_ == HdrFormat::HDR8_GAINMAP && source->auxiliaryBufferInfos != nullptr) {
        output->auxiliaryBufferInfos =
            std::make_unique<std::unordered_map<EffectPixelmapType, std::shared_ptr<BufferInfo>>>();
        for (const auto& entry : *source->auxiliaryBufferInfos) {
            std::shared_ptr<BufferInfo> auxiliaryBufferInfo = std::make_shared<BufferInfo>();
            CommonUtils::CopyBufferInfo(*(entry.second), *auxiliaryBufferInfo);
            output->auxiliaryBufferInfos->emplace(entry.first, auxiliaryBufferInfo);
        }

        auto gainBuffer = source -> auxiliaryBufferInfos->find(EffectPixelmapType::GAINMAP);
        if (gainBuffer != source -> auxiliaryBufferInfos -> end()) {
            auto gainBufferInfo = gainBuffer->second;
            if (gainBufferInfo != nullptr) {
                std::shared_ptr<ExtraInfo> gainExtraInfo = std::make_unique<ExtraInfo>();
                auto tempGainBuffer =
                    std::make_shared<EffectBuffer>(gainBufferInfo, gainBufferInfo->addr_, gainExtraInfo);
                RenderTexturePtr gainTex = param_->resCache_->RequestTexture(param_->context_,
                    static_cast<int>(gainBufferInfo->width_), static_cast<int>(gainBufferInfo->height_), GL_RGBA8);
                DrawBufferToTexture(gainTex, tempGainBuffer.get());
                output->bufferInfo_->gainMapTex_ = gainTex;
            }
        }
    }
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

void RenderEnvironment::DrawBufferToTexture(RenderTexturePtr renderTex, const EffectBuffer *source)
{
    CHECK_AND_RETURN_LOG(source != nullptr, "DrawBufferToTexture: source is null!");
    int width = static_cast<int>(source->bufferInfo_->width_);
    int height = static_cast<int>(source->bufferInfo_->height_);
    IEffectFormat format = source->bufferInfo_->formatType_;
    if (source->bufferInfo_->surfaceBuffer_ != nullptr) {
        source->bufferInfo_->surfaceBuffer_->FlushCache();
        DrawTexFromSurfaceBuffer(renderTex, source->bufferInfo_->surfaceBuffer_, format);
    } else {
        GLuint tex;
        CHECK_AND_RETURN_LOG(renderTex != nullptr, "DrawBufferToTexture: renderTex is null!");
        GLuint tempFbo = GLUtils::CreateFramebuffer(renderTex->GetName());
        if (source->bufferInfo_->formatType_ == IEffectFormat::RGBA8888 ||
            source->bufferInfo_->formatType_ == IEffectFormat::RGBA_1010102) {
            int stride = static_cast<int>(source->bufferInfo_->rowStride_ / 4);
            tex = GenTextureWithPixels(source->buffer_, width, height, stride, format);
        } else {
            tex = ConvertFromYUVToRGB(source, format);
        }
        RenderViewport vp(0, 0, renderTex->Width(), renderTex->Height());
        param_->renderer_->Draw(tex, tempFbo, param_->meshBase_, param_->shaderBase_, &vp, GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        GLUtils::DeleteTexture(tex);
        GLUtils::DeleteFboOnly(tempFbo);
    }
}

GLuint RenderEnvironment::ConvertFromYUVToRGB(const EffectBuffer *source, IEffectFormat format)
{
    int width = static_cast<int>(source->bufferInfo_->width_);
    int height = static_cast<int>(source->bufferInfo_->height_);
    auto *srcNV12 = static_cast<unsigned char *>(source->buffer_);
    uint8_t *srcNV12UV = srcNV12 + width * height;
    auto data = std::make_unique<unsigned char[]>(width * height * RGBA_SIZE_PER_PIXEL);
    for (uint32_t i = 0; i < static_cast<uint32_t>(height); i++) {
        for (uint32_t j = 0; j < static_cast<uint32_t>(width); j++) {
            uint32_t nvIndex =
                i / UV_PLANE_SIZE * static_cast<uint32_t>(width) + j - j % UV_PLANE_SIZE; // 2 mean u/v split factor
            uint32_t yIndex = i * static_cast<uint32_t>(width) + j;
            uint8_t y;
            uint8_t u;
            uint8_t v;
            if (format == IEffectFormat::YUVNV12) {
                y = srcNV12[yIndex];
                u = srcNV12UV[nvIndex];
                v = srcNV12UV[nvIndex + 1];
            } else {
                y = srcNV12[yIndex];
                v = srcNV12UV[nvIndex];
                u = srcNV12UV[nvIndex + 1];
            }
            uint8_t r = FormatHelper::YuvToR(y, u, v);
            uint8_t g = FormatHelper::YuvToG(y, u, v);
            uint8_t b = FormatHelper::YuvToB(y, u, v);
            uint32_t rgbIndex = i * static_cast<uint32_t>(width) * RGB_PLANE_SIZE + j * RGB_PLANE_SIZE;
            data[rgbIndex] = r;
            data[rgbIndex + G_POS] = g;
            data[rgbIndex + B_POS] = b;
        }
    }
    GLuint tex = GLUtils::CreateTexWithStorage(GL_TEXTURE_2D, 1, GL_RGB, width, height);
    return tex;
}

void RenderEnvironment::ConvertFromRGBToYUV(RenderTexturePtr input, IEffectFormat format, void *data)
{
    int width = static_cast<int>(input->Width());
    int height = static_cast<int>(input->Height());
    auto rgbData = std::make_unique<unsigned char[]>(width * height * RGBA_SIZE_PER_PIXEL);
    ReadPixelsFromTex(input, rgbData.get(), width, height, width);
    auto *srcNV12 = static_cast<unsigned char *>(data);
    uint8_t *srcNV12UV = srcNV12 + static_cast<uint32_t>(width * height);
    for (uint32_t i = 0; i < static_cast<uint32_t>(height); i++) {
        for (uint32_t j = 0; j < static_cast<uint32_t>(width); j++) {
            uint32_t yIndex = i * static_cast<uint32_t>(width) + j;
            uint32_t nvIndex =
                i / UV_PLANE_SIZE * static_cast<uint32_t>(width) + j - j % UV_PLANE_SIZE; // 2 mean u/v split factor
            uint32_t rgbIndex = i * static_cast<uint32_t>(width) * static_cast<uint32_t>(RGBA_SIZE_PER_PIXEL) +
                j * static_cast<uint32_t>(RGBA_SIZE_PER_PIXEL);
            uint8_t r = rgbData[rgbIndex];
            uint8_t g = rgbData[rgbIndex + G_POS];
            uint8_t b = rgbData[rgbIndex + B_POS];
            srcNV12[yIndex] = FormatHelper::RGBToY(r, g, b);
            if (format == IEffectFormat::YUVNV12) {
                srcNV12UV[nvIndex] = FormatHelper::RGBToU(r, g, b);
                srcNV12UV[nvIndex + 1] = FormatHelper::RGBToV(r, g, b);
            } else {
                srcNV12UV[nvIndex] = FormatHelper::RGBToV(r, g, b);
                srcNV12UV[nvIndex + 1] = FormatHelper::RGBToU(r, g, b);
            }
        }
    }
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

        auto mesh = std::make_shared<RenderMesh>(DEFAULT_FLIP_VERTEX_DATA);
        mesh->Bind(param_->shaderBaseDrawFrameYUV_);
        param_->renderer_->DrawOnScreenWithTransform(texId, mesh.get(),
            param_->shaderBaseDrawFrameYUV_, &param_->viewport_, type, GL_TEXTURE_EXTERNAL_OES);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
        if (screenSurface_ == nullptr) {
            EFFECT_LOGE("RenderEnvironment screenSurface_ is nullptr");
            return;
        }
        param_->context_->SwapBuffers(screenSurface_);
        GLUtils::CheckError(__FILE_NAME__, __LINE__);
    }
}

void RenderEnvironment::ConvertTextureToBuffer(RenderTexturePtr source, EffectBuffer *output, bool needProcessCache)
{
    int w = static_cast<int>(source->Width());
    int h = static_cast<int>(source->Height());
    if (output->bufferInfo_->surfaceBuffer_ == nullptr) {
        if (output->bufferInfo_->formatType_ == IEffectFormat::RGBA8888 ||
            output->bufferInfo_->formatType_ == IEffectFormat::RGBA_1010102) {
            ReadPixelsFromTex(source, output->buffer_, w, h, output->bufferInfo_->rowStride_ / RGBA_SIZE_PER_PIXEL);
        } else {
            ConvertFromRGBToYUV(source, output->bufferInfo_->formatType_, output->buffer_);
        }
    } else {
        DrawSurfaceBufferFromTex(source, output->bufferInfo_->surfaceBuffer_, output->bufferInfo_->formatType_);
        if (needProcessCache) {
            output->bufferInfo_->surfaceBuffer_->InvalidateCache();
        }
    }
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}

void RenderEnvironment::ConvertYUV2RGBA(std::shared_ptr<EffectBuffer> &source, std::shared_ptr<EffectBuffer> &out)
{
    int width = static_cast<int>(source->bufferInfo_->width_);
    int height = static_cast<int>(source->bufferInfo_->height_);
    RenderTexturePtr outTex;
    if (source->bufferInfo_->surfaceBuffer_ == nullptr) {
        outTex = std::make_shared<RenderTexture>(param_->context_, width, height, GL_RGBA8);
        outTex->SetName(ConvertFromYUVToRGB(source.get(), source->bufferInfo_->formatType_));
    } else {
        outTex = param_->resCache_->RequestTexture(param_->context_, width, height, GL_RGBA8);
        EGLImageKHR img = GLUtils::CreateEGLImage(eglGetDisplay(EGL_DEFAULT_DISPLAY),
            source->bufferInfo_->surfaceBuffer_);
        GLuint sourceTex = GLUtils::CreateTextureFromImage(img);
        GLuint tempFbo = GLUtils::CreateFramebufferWithTarget(outTex->GetName(), GL_TEXTURE_2D);
        RenderViewport vp(0, 0, width, height);
        param_->renderer_->Draw(sourceTex, tempFbo, param_->meshBaseFlipYUVDMA_, param_->shaderBaseYUVDMA2RGB2D_, &vp,
            GL_TEXTURE_EXTERNAL_OES);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
        GLUtils::DestroyImage(img);
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
    EGLImageKHR img = GLUtils::CreateEGLImage(eglGetDisplay(EGL_DEFAULT_DISPLAY), out->bufferInfo_->surfaceBuffer_);
    GLuint outTex = GLUtils::CreateTextureFromImage(img);
    RenderTexturePtr tex = std::make_shared<RenderTexture>(param_->context_, width, height, GL_RGBA8);
    tex->SetName(outTex);
    Draw2D2OES(sourceTex, tex);
    glFinish();
    GLUtils::DestroyImage(img);
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
    GLenum type = tex->Format() == GL_RGB10_A2 ? GL_UNSIGNED_INT_2_10_10_10_REV : GL_UNSIGNED_BYTE;
    glReadPixels(0, 0, width, height, GL_RGBA, type, data);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    GLUtils::DeleteFboOnly(inFbo);
}

GLuint RenderEnvironment::GenTextureWithPixels(void *data, int width, int height, int stride, IEffectFormat format)
{
    GLuint tex = GLUtils::CreateTexWithStorage(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLenum type = format == IEffectFormat::RGBA_1010102 ? GL_UNSIGNED_INT_2_10_10_10_REV : GL_UNSIGNED_BYTE;
    if (width == stride) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, type, data);
    } else {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, type, data);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }
    return tex;
}

void RenderEnvironment::DrawSurfaceBufferFromTex(RenderTexturePtr tex, SurfaceBuffer *buffer, IEffectFormat format)
{
    EGLImageKHR img = GLUtils::CreateEGLImage(eglGetDisplay(EGL_DEFAULT_DISPLAY), buffer);
    GLuint outTex = GLUtils::CreateTextureFromImage(img);
    GLuint tempFbo = GLUtils::CreateFramebufferWithTarget(outTex, GL_TEXTURE_EXTERNAL_OES);
    RenderViewport vp(0, 0, tex->Width(), tex->Height());
    if (format == IEffectFormat::RGBA8888 || format == IEffectFormat::RGBA_1010102) {
        param_->renderer_->Draw(tex->GetName(), tempFbo, param_->meshBase_, param_->shaderBase_, &vp, GL_TEXTURE_2D);
    } else {
        param_->renderer_->Draw(tex->GetName(), tempFbo, param_->meshBaseDMA_, param_->shaderBaseRGB2D2YUVDMA_, &vp,
            GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glFinish();
    GLUtils::DeleteTexture(outTex);
    GLUtils::DeleteFboOnly(tempFbo);
    GLUtils::DestroyImage(img);
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}

void RenderEnvironment::DrawFlipSurfaceBufferFromTex(RenderTexturePtr tex, SurfaceBuffer *buffer, IEffectFormat format)
{
    EGLImageKHR img = GLUtils::CreateEGLImage(eglGetDisplay(EGL_DEFAULT_DISPLAY), buffer);
    GLuint outTex = GLUtils::CreateTextureFromImage(img);
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
    GLUtils::DeleteTexture(outTex);
    GLUtils::DeleteFboOnly(tempFbo);
    GLUtils::DestroyImage(img);
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}

void RenderEnvironment::DrawOesTexture2DFromTexture(RenderTexturePtr inputTex, GLuint outputTex, int32_t width,
    int32_t height, IEffectFormat format)
{
    CHECK_AND_RETURN_LOG(inputTex && outputTex != 0,
        "DrawSurfaceBufferFromSurfaceBuffer: inputTex or outputTex is nullptr");
    GLuint outputFbo = GLUtils::CreateFramebufferWithTarget(outputTex, GL_TEXTURE_EXTERNAL_OES);
    RenderViewport vp(0, 0, width, height);
    if (format == IEffectFormat::RGBA8888 || format == IEffectFormat::RGBA_1010102) {
        param_->renderer_->Draw(inputTex->GetName(), outputFbo,
            param_->meshBase_, param_->shaderBase_, &vp, GL_TEXTURE_2D);
    } else {
        param_->renderer_->Draw(inputTex->GetName(), outputFbo,
            param_->meshBaseDMA_, param_->shaderBaseRGB2D2YUVDMA_, &vp, GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    GLUtils::DeleteFboOnly(outputFbo);
}

void RenderEnvironment::DrawSurfaceBufferFromSurfaceBuffer(SurfaceBuffer *inBuffer, SurfaceBuffer *outBuffer,
    IEffectFormat format) const
{
    CHECK_AND_RETURN_LOG(inBuffer && outBuffer, "DrawSurfaceBufferFromSurfaceBuffer: inBuffer or outBuffer is nullptr");
    EGLImageKHR inputImg = GLUtils::CreateEGLImage(eglGetDisplay(EGL_DEFAULT_DISPLAY), inBuffer);
    GLuint inputTex = GLUtils::CreateTextureFromImage(inputImg);

    EGLImageKHR outputImg = GLUtils::CreateEGLImage(eglGetDisplay(EGL_DEFAULT_DISPLAY), outBuffer);
    GLuint outputTex = GLUtils::CreateTextureFromImage(outputImg);
    GLuint outputFbo = GLUtils::CreateFramebufferWithTarget(outputTex, GL_TEXTURE_EXTERNAL_OES);

    RenderViewport vp(0, 0, outBuffer->GetWidth(), outBuffer->GetHeight());
    RenderGeneralProgram *program;
    RenderMesh *mesh;
    if (format == IEffectFormat::RGBA8888 || format == IEffectFormat::RGBA_1010102) {
        program = param_->shaderBaseDMA_;
        mesh = param_->meshBaseDMA_;
    } else {
        program = param_->shaderBaseYUVDMA2RGB2D_;
        mesh = param_->meshBaseYUVDMA_;
    }
    param_->renderer_->Draw(inputTex, outputFbo, mesh, program, &vp, GL_TEXTURE_EXTERNAL_OES);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    GLUtils::DeleteTexture(inputTex);
    GLUtils::DeleteTexture(outputTex);
    GLUtils::DeleteFboOnly(outputFbo);
    GLUtils::DestroyImage(inputImg);
    GLUtils::DestroyImage(outputImg);
}

void RenderEnvironment::DrawTexFromSurfaceBuffer(RenderTexturePtr tex, SurfaceBuffer *buffer, IEffectFormat format)
{
    CHECK_AND_RETURN_LOG(tex != nullptr, "DrawTexFromSurfaceBuffer: tex is null!");
    GLuint tempFbo = GLUtils::CreateFramebuffer(tex->GetName());
    EGLImageKHR img = GLUtils::CreateEGLImage(eglGetDisplay(EGL_DEFAULT_DISPLAY), buffer);
    GLuint input = GLUtils::CreateTextureFromImage(img);
    RenderViewport vp(0, 0, tex->Width(), tex->Height());
    RenderGeneralProgram *program;
    RenderMesh *mesh;
    if (format == IEffectFormat::RGBA8888 || format == IEffectFormat::RGBA_1010102) {
        program = param_->shaderBaseDMA_;
        mesh = param_->meshBaseDMA_;
    } else {
        program = param_->shaderBaseYUVDMA2RGB2D_;
        mesh = param_->meshBaseYUVDMA_;
    }
    param_->renderer_->Draw(input, tempFbo, mesh, program, &vp, GL_TEXTURE_EXTERNAL_OES);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    GLUtils::DeleteTexture(input);
    GLUtils::DeleteFboOnly(tempFbo);
    GLUtils::DestroyImage(img);
}

std::shared_ptr<EffectBuffer> RenderEnvironment::GenTexEffectBuffer(const std::shared_ptr<EffectBuffer>& input)
{
    EFFECT_LOGD("GenTexEffectBuffer: dataType = %{public}d", input->extraInfo_->dataType);
    auto info = input->bufferInfo_;
    auto bufferInfo = std::make_shared<BufferInfo>();
    CommonUtils::CopyBufferInfo(*info, *bufferInfo);

    auto extraInfo = std::make_shared<ExtraInfo>();
    CommonUtils::CopyExtraInfo(*input->extraInfo_, *extraInfo);
    extraInfo->dataType = DataType::TEX;

    auto out = std::make_shared<EffectBuffer>(bufferInfo, nullptr, extraInfo);
    if (bufferInfo->hdrFormat_ != HdrFormat::HDR8_GAINMAP) return out;

    if (input->auxiliaryBufferInfos != nullptr) {
        out->auxiliaryBufferInfos = std::make_shared<std::unordered_map<
                EffectPixelmapType, std::shared_ptr<BufferInfo>>>();
        for (const auto& [pixType, effectBufferInfo] : *input->auxiliaryBufferInfos) {
            auto auxiliaryBufferInfo = std::make_shared<BufferInfo>();
            CommonUtils::CopyBufferInfo(*effectBufferInfo, *auxiliaryBufferInfo);

            out->auxiliaryBufferInfos->emplace(pixType, auxiliaryBufferInfo);
        }
    }
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
    if (needTerminate_) {
        eglTerminate(eglGetDisplay(EGL_DEFAULT_DISPLAY));
        needTerminate_ = false;
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