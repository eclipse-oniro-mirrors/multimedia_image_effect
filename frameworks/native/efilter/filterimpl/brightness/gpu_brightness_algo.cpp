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

#include "gpu_brightness_algo.h"
#include <string>

#include "common_utils.h"
#include "effect_log.h"
#include "graphic/gl_utils.h"
#include "effect_trace.h"

namespace OHOS {
namespace Media {
namespace Effect {
constexpr int MAX_BRIGHTNESS = 100;

const std::string VS_CONTENT = "attribute vec4 aPosition;\n"
    "attribute vec4 aTextureCoord;\n"
    "varying vec2 textureCoordinate;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = aPosition;\n"
    "    textureCoordinate = aTextureCoord.xy;\n"
    "}\n";
const std::string FS_CONTENT = "precision highp float;\n"
    "uniform sampler2D Texture;\n"
    "varying vec2 textureCoordinate;\n"
    "uniform float ratio;\n"
    "void main() {\n"
    "    vec4 curColor = texture2D(Texture, textureCoordinate);\n"
    "    vec3 res = curColor.xyz;\n"
    "    float scale = pow(2.4, ratio);\n"
    "    float eps = 1.0e-5;\n"
    "    res = clamp(1.0 - res, 0.0, 1.0) + eps;\n"
    "    float nr = 1.0 - pow(res.x, scale);\n"
    "    float ng = 1.0 - pow(res.y, scale);\n"
    "    float nb = 1.0 - pow(res.z, scale);\n"
    "    gl_FragColor = clamp((vec4(nr, ng, nb, 1.0)), 0.0, 1.0);\n"
    "}";

ErrorCode GpuBrightnessAlgo::Release()
{
    if (shader_) {
        delete shader_;
        shader_ = nullptr;
    }

    if (renderMesh_) {
        delete renderMesh_;
        renderMesh_ = nullptr;
    }

    if (fbo_ != 0) {
        GLUtils::DeleteFboOnly(fbo_);
    }
    return ErrorCode::SUCCESS;
}

ErrorCode GpuBrightnessAlgo::Init()
{
    fbo_ = GLUtils::CreateFramebuffer();
    vertexShaderCode_ = VS_CONTENT;
    fragmentShaderCode_ = FS_CONTENT;
    renderMesh_ = new RenderMesh(DEFAULT_VERTEX_DATA);
    renderEffectData_ = std::make_shared<BrightnessFilterData>();
    return ErrorCode::SUCCESS;
}

void GpuBrightnessAlgo::PreDraw(GLenum target)
{
    if (shader_ != nullptr && target == GL_TEXTURE_2D) {
        if (renderEffectData_->inputTexture_ != nullptr) {
            shader_->BindTexture("Texture", 0, renderEffectData_->inputTexture_->GetName(), target);
        }
        shader_->SetFloat("ratio", renderEffectData_->ratio);
    }
}

void GpuBrightnessAlgo::PostDraw(GLenum target)
{
    if (renderEffectData_->inputTexture_ != nullptr) {
        if (target == GL_TEXTURE_2D) {
            shader_->UnBindTexture(0, target);
        }
        renderEffectData_->inputTexture_.reset();
    }
}

float GpuBrightnessAlgo::ParseBrightness(std::map<std::string, Plugin::Any> &value)
{
    float brightness = 0.f;
    ErrorCode res = CommonUtils::GetValue("FilterIntensity", value, brightness);
    if (res != ErrorCode::SUCCESS) {
        EFFECT_LOGW("get value fail! res=%{public}d. use default value: %{public}f", res, brightness);
    }
    EFFECT_LOGI("get value success! brightness=%{public}f", brightness);
    return brightness;
}

ErrorCode GpuBrightnessAlgo::OnApplyRGBA8888(EffectBuffer *src, EffectBuffer *dst,
    std::map<std::string, Plugin::Any> &value, const std::shared_ptr<EffectContext> &context)
{
    EFFECT_LOGI("GpuBrightnessFilterOperator::OnApplyRGBA8888 enter!");
    CHECK_AND_RETURN_RET_LOG(src != nullptr && dst != nullptr, ErrorCode::ERR_INPUT_NULL, "input para is null!");
    if (context->renderEnvironment_->GetEGLStatus() != EGLStatus::READY) {
        context->renderEnvironment_->Init();
    }
    if (!context->renderEnvironment_->IsPrepared()) {
        context->renderEnvironment_->Prepare();
    }
    EffectBuffer *inEffectBuffer = nullptr;
    if (src->extraInfo_->dataType != DataType::TEX) {
        context->renderEnvironment_->BeginFrame();
        inEffectBuffer = context->renderEnvironment_->ConvertBufferToTexture(src).get();
    } else {
        inEffectBuffer = src;
    }
    Init();
    renderEffectData_->inputTexture_ = inEffectBuffer->tex;
    renderEffectData_->outputHeight_ = inEffectBuffer->tex->Height();
    renderEffectData_->outputWidth_ = inEffectBuffer->tex->Width();
    renderEffectData_->ratio = ParseBrightness(value) / MAX_BRIGHTNESS;

    RenderTexturePtr tex = context->renderEnvironment_->RequestBuffer(renderEffectData_->outputWidth_,
        renderEffectData_->outputHeight_);
    Render(GL_TEXTURE_2D, tex);
    if (dst->extraInfo_->dataType != DataType::TEX) {
        context->renderEnvironment_->ConvertTextureToBuffer(tex, dst);
    } else {
        dst->bufferInfo_->width_ = tex->Width();
        dst->bufferInfo_->height_ = tex->Height();
        dst->bufferInfo_->rowStride_ = tex->Width() * RGBA_SIZE_PER_PIXEL;
        dst->bufferInfo_->len_ = tex->Width() * tex->Height() * RGBA_SIZE_PER_PIXEL;
        dst->bufferInfo_->formatType_ = IEffectFormat::RGBA8888;
        dst->tex = tex;
    }
    return ErrorCode::SUCCESS;
}

void GpuBrightnessAlgo::Render(GLenum target, RenderTexturePtr tex)
{
    if (shader_ == nullptr) {
        shader_ = new AlgorithmProgram(context_, vertexShaderCode_, fragmentShaderCode_);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, tex->GetName(), 0);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glViewport(0, 0, renderEffectData_->outputWidth_, renderEffectData_->outputHeight_);
    shader_->Bind();

    CHECK_AND_RETURN_LOG(renderMesh_ != nullptr, "Render: renderMesh is null!");
    renderMesh_->Bind(shader_->GetShader());

    PreDraw(target);
    glDrawArrays(renderMesh_->primitiveType_, 0, renderMesh_->vertexNum_);
    PostDraw(target);
    shader_->Unbind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
} // namespace Effect
} // namespace Media
} // namespace OHOS