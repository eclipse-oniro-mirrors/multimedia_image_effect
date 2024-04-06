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
#include "GLUtils.h"

namespace OHOS {
namespace Media {
namespace Effect {
constexpr int MAX_BRIGHTNESS = 100;

constexpr GLfloat VERTICES[] = {
    -1.0f, +1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
    +1.0f, +1.0f, 0.0f, 1.0f, 1.0f,
    +1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
};

const std::string VS_CONTENT = "#version 320 es\n"
    "layout(location = 0) in vec3 a_pos;\n"
    "layout(location = 1) in vec2 a_tex_coord;\n"
    "out vec2 v_texCoord;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(a_pos, 1.0);\n"
    "    v_texCoord = a_tex_coord;\n"
    "}";
const std::string FS_CONTENT = "#version 320 es\n"
    "precision highp float;\n"
    "uniform sampler2D Texture;\n"
    "in vec2 v_texCoord;\n"
    "uniform float ratio;\n"
    "layout(location = 0) out vec4 v_out_color;\n"
    "void main() {\n"
    "    vec4 curColor = texture(Texture, v_texCoord);\n"
    "    vec3 res = curColor.xyz;\n"
    "    float scale = pow(2.4, ratio);\n"
    "    float eps = 1.0e-5;\n"
    "    res = clamp(1.0 - res, 0.0, 1.0) + eps;\n"
    "    float nr = 1.0 - pow(res.x, scale);\n"
    "    float ng = 1.0 - pow(res.y, scale);\n"
    "    float nb = 1.0 - pow(res.z, scale);\n"
    "    v_out_color = clamp((vec4(nr, ng, nb, 1.0)), 0.0, 1.0);\n"
    "}";
	
constexpr int MESH_SIZE = 5;
constexpr int MESH_POS_SIZE = 3;
constexpr int MESH_TEX_POS_SIZE = 2;
constexpr int PORT_SIZE = 4;

ErrorCode GpuBrightnessAlgo::InitMesh()
{
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES), VERTICES, GL_STATIC_DRAW);

    glVertexAttribPointer(0, MESH_POS_SIZE, GL_FLOAT, GL_FALSE, MESH_SIZE * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, MESH_TEX_POS_SIZE, GL_FLOAT, GL_FALSE, MESH_SIZE * sizeof(float),
        reinterpret_cast<void *>(MESH_POS_SIZE * sizeof(float)));
    glEnableVertexAttribArray(1);
    if (!GLUtils::CheckError(__FILE__, __LINE__)) {
        return ErrorCode::ERR_GL_INIT_MESH_FAILED;
    }
    return ErrorCode::SUCCESS;
}

void GpuBrightnessAlgo::Unbind()
{
    glBindTexture(GL_TEXTURE_2D, GL_NONE);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
}

ErrorCode GpuBrightnessAlgo::PreDraw(uint32_t width, uint32_t height, std::map<std::string, Plugin::Any> &value)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width, height);

    glUseProgram(shaderProgram_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId_);
    float brightness = ParseBrightness(value);
    float brightScale = brightness / MAX_BRIGHTNESS;
    glUniform1i(glGetUniformLocation(shaderProgram_, "Texture"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram_, "ratio"), brightScale);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        EFFECT_LOGE("OnApplyRGBA8888 glCheckFramebufferStatus != GL_FRAMEBUFFER_COMPLETE");
        return ErrorCode::ERR_GL_FRAMEBUFFER_NOT_COMPLETE;
    }
    glBindVertexArray(vao_);
    if (!GLUtils::CheckError(__FILE__, __LINE__)) {
        return ErrorCode::ERR_GL_PRE_DRAW_FAILED;
    }
    return ErrorCode::SUCCESS;
}

ErrorCode GpuBrightnessAlgo::Release()
{
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteProgram(shaderProgram_);
    GLUtils::DeleteTexture(texId_);
    glDeleteFramebuffers(1, &fbo_);
    return ErrorCode::SUCCESS;
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
    std::map<std::string, Plugin::Any> &value)
{
    EFFECT_LOGI("GpuBrightnessFilterOperator::OnApplyRGBA8888 enter!");
    CHECK_AND_RETURN_RET_LOG(src != nullptr && dst != nullptr, ErrorCode::ERR_INPUT_NULL,
        "input para is null! src=%{public}p, dst=%{public}p", src, dst);
    auto *srcRgb = static_cast<unsigned char *>(src->buffer_);
    auto *dstRgb = static_cast<unsigned char *>(dst->buffer_);
    uint32_t width = src->bufferInfo_->width_;
    uint32_t height = src->bufferInfo_->height_;

    ErrorCode code = GLUtils::CreateTexture(width, height, srcRgb, texId_);
    if (code != ErrorCode::SUCCESS) {
        EFFECT_LOGE("GpuBrightnessFilterOperator Render CreateTexture Failed");
        return code;
    }

    code = GLUtils::CreateFramebuffer(fbo_, texId_);
    if (code != ErrorCode::SUCCESS) {
        EFFECT_LOGE("GpuBrightnessFilterOperator Render CreateFramebuffer Failed");
        return code;
    }

    if (shaderProgram_ == 0) {
        shaderProgram_ = GLUtils::CreateProgram(VS_CONTENT, FS_CONTENT);
        if (shaderProgram_ == 0) {
            EFFECT_LOGE("GpuBrightnessFilterOperator Render CreateProgram Failed");
            return ErrorCode::ERR_GL_CREATE_PROGRAM_FAILED;
        }
    }

    code = InitMesh();
    if (code != ErrorCode::SUCCESS) {
        EFFECT_LOGE("GpuBrightnessFilterOperator Render InitMesh Failed");
        return code;
    }

    code = PreDraw(width, height, value);
    if (code != ErrorCode::SUCCESS) {
        EFFECT_LOGE("GpuBrightnessFilterOperator Render PreDraw Failed");
        return code;
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, PORT_SIZE);
    if (!GLUtils::CheckError(__FILE__, __LINE__)) {
        EFFECT_LOGE("GpuBrightnessFilterOperator Render Failed");
        return ErrorCode::ERR_GL_DRAW_FAILED;
    }

    glFinish();
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLvoid *>(dstRgb));
    if (!GLUtils::CheckError(__FILE__, __LINE__)) {
        EFFECT_LOGE("GpuBrightnessFilterOperator Render Read Pixels Failed");
        return ErrorCode::ERR_GL_COPY_PIXELS_FAILED;
    }
    Unbind();
    return ErrorCode::SUCCESS;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS