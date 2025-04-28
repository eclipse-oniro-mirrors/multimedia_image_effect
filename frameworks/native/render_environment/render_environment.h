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

#ifndef RENDER_ENVIRONMENT_H
#define RENDER_ENVIRONMENT_H

#include <external_window.h>
#include <GLES3/gl3.h>

#include "any.h"
#include "error_code.h"

#include "base/render_base.h"
#include "core/render_opengl_renderer.h"
#include "core/render_default_data.h"
#include "core/render_mesh.h"
#include "core/render_resource_cache.h"
#include "core/render_viewport.h"
#include "graphic/render_frame_buffer.h"
#include "graphic/render_general_program.h"
#include "effect_buffer.h"
#include "image_effect_marco_define.h"

namespace OHOS {
namespace Media {
namespace Effect {
class RenderParam {
public:
    RenderContext *context_ = nullptr;
    RenderOpenglRenderer *renderer_ = nullptr;
    RenderMesh *meshBase_ = nullptr;
    RenderMesh *meshBaseFlip_ = nullptr;
    RenderMesh *meshBaseDMA_ = nullptr;
    RenderMesh *meshBaseFlipYUVDMA_ = nullptr;
    RenderMesh *meshBaseYUVDMA_ = nullptr;
    RenderMesh *meshBaseDrawFrame_ = nullptr;
    RenderMesh *meshBaseDrawFrameYUV_ = nullptr;
    RenderGeneralProgram *shaderBase_ = nullptr;
    RenderGeneralProgram *shaderBaseDMA_ = nullptr;
    RenderGeneralProgram *shaderBaseYUVDMA_ = nullptr;
    RenderGeneralProgram *shaderBaseYUVDMA2RGB2D_ = nullptr;
    RenderGeneralProgram *shaderBaseRGB2D2YUVDMA_ = nullptr;
    RenderGeneralProgram *shaderBaseDrawFrame_ = nullptr;
    RenderGeneralProgram *shaderBaseDrawFrameYUV_ = nullptr;
    ResourceCache *resCache_ = nullptr;
    RenderViewport viewport_;
    bool threadReady_ = false;

    RenderParam()
    {
        resCache_ = new ResourceCache;
    }

    ~RenderParam()
    {
        if (renderer_) {
            delete renderer_;
            renderer_ = nullptr;
        }
        if (meshBase_) {
            delete meshBase_;
            meshBase_ = nullptr;
        }
        if (meshBaseFlip_) {
            delete meshBaseFlip_;
            meshBaseFlip_ = nullptr;
        }
        if (meshBaseDMA_) {
            delete meshBaseDMA_;
            meshBaseDMA_ = nullptr;
        }
        if (meshBaseFlipYUVDMA_) {
            delete meshBaseFlipYUVDMA_;
            meshBaseFlipYUVDMA_ = nullptr;
        }
        if (meshBaseYUVDMA_) {
            delete meshBaseYUVDMA_;
            meshBaseYUVDMA_ = nullptr;
        }
        if (meshBaseDrawFrame_) {
            delete meshBaseDrawFrame_;
            meshBaseDrawFrame_ = nullptr;
        }
        if (meshBaseDrawFrameYUV_) {
            delete meshBaseDrawFrameYUV_;
            meshBaseDrawFrameYUV_ = nullptr;
        }

        ReleaseShaderBase();

        if (resCache_) {
            delete resCache_;
            resCache_ = nullptr;
        }

        if (context_) {
            context_->ReleaseCurrent();
            context_->Release();
            delete context_;
            context_ = nullptr;
        }
    }
private:
    void ReleaseShaderBase()
    {
        if (shaderBase_) {
            shaderBase_->Release();
            delete shaderBase_;
            shaderBase_ = nullptr;
        }
        if (shaderBaseDMA_) {
            shaderBaseDMA_->Release();
            delete shaderBaseDMA_;
            shaderBaseDMA_ = nullptr;
        }
        if (shaderBaseYUVDMA_) {
            shaderBaseYUVDMA_->Release();
            delete shaderBaseYUVDMA_;
            shaderBaseYUVDMA_ = nullptr;
        }
        if (shaderBaseYUVDMA2RGB2D_) {
            shaderBaseYUVDMA2RGB2D_->Release();
            delete shaderBaseYUVDMA2RGB2D_;
            shaderBaseYUVDMA2RGB2D_ = nullptr;
        }
        if (shaderBaseRGB2D2YUVDMA_) {
            shaderBaseRGB2D2YUVDMA_->Release();
            delete shaderBaseRGB2D2YUVDMA_;
            shaderBaseRGB2D2YUVDMA_ = nullptr;
        }
        if (shaderBaseDrawFrame_) {
            shaderBaseDrawFrame_->Release();
            delete shaderBaseDrawFrame_;
            shaderBaseDrawFrame_ = nullptr;
        }
        if (shaderBaseDrawFrameYUV_) {
            shaderBaseDrawFrameYUV_->Release();
            delete shaderBaseDrawFrameYUV_;
            shaderBaseDrawFrameYUV_ = nullptr;
        }
    }
};
enum EGLStatus {READY, UNREADY};
class RenderEnvironment {
public:
    IMAGE_EFFECT_EXPORT RenderEnvironment() = default;
    IMAGE_EFFECT_EXPORT ~RenderEnvironment() = default;
    IMAGE_EFFECT_EXPORT void Init();
    IMAGE_EFFECT_EXPORT void Prepare();
    void InitEngine(OHNativeWindow *window);
    void NotifyInputChanged();
    IMAGE_EFFECT_EXPORT bool IfNeedGenMainTex() const;
    RenderTexturePtr ReCreateTexture(RenderTexturePtr renderTex, int width, int height, bool isHdr10) const;
    IMAGE_EFFECT_EXPORT std::unordered_map<std::string, RenderTexturePtr> GenHdr8GainMapTexs(
        const std::shared_ptr<EffectBuffer> &source);
    IMAGE_EFFECT_EXPORT RenderTexturePtr GenMainTex(const std::shared_ptr<EffectBuffer> &source, bool isHdr10 = false);

    IMAGE_EFFECT_EXPORT void GenTex(const std::shared_ptr<EffectBuffer> &source,
        std::shared_ptr<EffectBuffer> &output);
    IMAGE_EFFECT_EXPORT std::shared_ptr<EffectBuffer> ConvertBufferToTexture(EffectBuffer *source);
    IMAGE_EFFECT_EXPORT void ConvertTextureToBuffer(RenderTexturePtr source, EffectBuffer *output,
        bool needProcessCache = false);
    IMAGE_EFFECT_EXPORT RenderContext* GetContext();
    IMAGE_EFFECT_EXPORT ResourceCache* GetResourceCache();
    IMAGE_EFFECT_EXPORT bool BeginFrame();

    IMAGE_EFFECT_EXPORT void DrawFrameWithTransform(const std::shared_ptr<EffectBuffer> &buffer,
        GraphicTransformType type);
    IMAGE_EFFECT_EXPORT void DrawFrame(GLuint texId, GraphicTransformType type);
    IMAGE_EFFECT_EXPORT void ConvertYUV2RGBA(std::shared_ptr<EffectBuffer> &source, std::shared_ptr<EffectBuffer> &out);
    void ConvertRGBA2YUV(std::shared_ptr<EffectBuffer> &source, std::shared_ptr<EffectBuffer> &out);
    void Draw2D2OES(RenderTexturePtr source, RenderTexturePtr output);
    void UpdateCanvas();
    IMAGE_EFFECT_EXPORT EGLStatus GetEGLStatus() const;
    IMAGE_EFFECT_EXPORT RenderTexturePtr RequestBuffer(int width, int height, GLenum format = GL_RGBA8);
    bool IsPrepared() const;
    IMAGE_EFFECT_EXPORT DataType GetOutputType() const;
    void SetOutputType(DataType type);
    void ReadPixelsFromTex(RenderTexturePtr tex, void *data, int width, int height, int stride);
    void DrawBufferToTexture(RenderTexturePtr renderTex, const EffectBuffer *source);
    IMAGE_EFFECT_EXPORT GLuint GenTextureWithPixels(void *data, int width, int height, int stride,
        IEffectFormat format = IEffectFormat::RGBA8888);
    IMAGE_EFFECT_EXPORT void DrawFlipSurfaceBufferFromTex(RenderTexturePtr tex,
        SurfaceBuffer *buffer, IEffectFormat format);
    IMAGE_EFFECT_EXPORT void DrawOesTexture2DFromTexture(RenderTexturePtr inputTex, GLuint outputTex, int32_t width,
        int32_t height, IEffectFormat format);
    IMAGE_EFFECT_EXPORT void DrawSurfaceBufferFromSurfaceBuffer(SurfaceBuffer *inBuffer, SurfaceBuffer *outBuffer,
        IEffectFormat format) const;
    IMAGE_EFFECT_EXPORT void DrawSurfaceBufferFromTex(RenderTexturePtr tex,
        SurfaceBuffer *buffer, IEffectFormat format);
    IMAGE_EFFECT_EXPORT bool GetOrCreateTextureFromCache(RenderTexturePtr &renderTex, const std::string &texName,
        int width, int height, bool isHdr10) const;
    IMAGE_EFFECT_EXPORT void DrawTexFromSurfaceBuffer(RenderTexturePtr tex, SurfaceBuffer *buffer,
        IEffectFormat format = IEffectFormat::RGBA8888);
    IMAGE_EFFECT_EXPORT void DrawFlipTex(RenderTexturePtr input, RenderTexturePtr output);
    static std::shared_ptr<EffectBuffer> GenTexEffectBuffer(const std::shared_ptr<EffectBuffer>& input);
    IMAGE_EFFECT_EXPORT GLuint ConvertFromYUVToRGB(const EffectBuffer *source, IEffectFormat format);
    IMAGE_EFFECT_EXPORT void ConvertFromRGBToYUV(RenderTexturePtr input, IEffectFormat format, void *data);
    IMAGE_EFFECT_EXPORT void ReleaseParam();
    IMAGE_EFFECT_EXPORT void Release();
    void SetNativeWindowColorSpace(EffectColorSpace colorSpace);

private:
    RenderParam *param_{ nullptr };
    RenderAttribute attribute_;
    RenderSurface *screenSurface_{ nullptr };
    OHNativeWindow *window_{ nullptr };
    bool hasInputChanged = true;
    int canvasWidth = 0;
    int canvasHeight = 0;
    EGLStatus isEGLReady = EGLStatus::UNREADY;
    DataType outType_ = DataType::UNKNOWN;
    bool needTerminate_ = false;
    void InitDefaultMeshMT(RenderParam *param);
    void InitDefaultShaderMT(RenderParam *param);
    RenderMesh *CreateMeshMT(RenderParam *param, bool isBackGround, RenderGeneralProgram *shader);
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif