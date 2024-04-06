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

#include "effect_log.h"
#include "GLUtils.h"

namespace OHOS {
namespace Media {
namespace Effect {
const int MSG_LENGTH = 512;

ErrorCode GLUtils::CreateTexture(int width, int height, const void *data, GLuint &textureId)
{
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
    glBindTexture(GL_TEXTURE_2D, GL_NONE);
    if (!CheckError(__FILE__, __LINE__)) {
        DeleteTexture(textureId);
        textureId = 0;
        return ErrorCode::ERR_GL_CREATE_TEXTURE_FAILED;
    }
    return ErrorCode::SUCCESS;
}

void GLUtils::DeleteTexture(unsigned int texture)
{
    if (texture == 0) {
        return;
    }
    glDeleteTextures(1, &texture);
}

ErrorCode GLUtils::CreateFramebuffer(GLuint &fboId, unsigned int textureId)
{
    glGenFramebuffers(1, &fboId);
    if (textureId != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            EFFECT_LOGE("CreateFramebuffer glCheckFramebufferStatus != GL_FRAMEBUFFER_COMPLETE");
            glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
            glDeleteFramebuffers(1, &fboId);
            return ErrorCode::ERR_GL_FRAMEBUFFER_NOT_COMPLETE;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    }
    return ErrorCode::SUCCESS;
}

unsigned int GLUtils::LoadShader(const std::string &src, unsigned int shaderType)
{
    const char *tempSrc = src.c_str();
    unsigned int shader = glCreateShader(shaderType);
    if (shader == 0) {
        EFFECT_LOGE("Could Not Create Shader");
    }
    glShaderSource(shader, 1, &tempSrc, nullptr);
    glCompileShader(shader);
    int status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLchar message[MSG_LENGTH] = {0};
        glGetShaderInfoLog(shader, MSG_LENGTH - 1, nullptr, &message[0]);
        EFFECT_LOGE("LoadShader Error: %{public}s", message);
        glDeleteShader(shader);
        shader = 0;
    }
    return shader;
}

unsigned int GLUtils::CreateProgram(const std::string &vss, const std::string &fss)
{
    unsigned int vs = LoadShader(vss, GL_VERTEX_SHADER);
    unsigned int fs = LoadShader(fss, GL_FRAGMENT_SHADER);
    if (vs == 0 || fs == 0) {
        return 0;
    }
    unsigned int program = glCreateProgram();
    if (program == 0) {
        EFFECT_LOGE("CreateProgram Failed");
    }
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    CheckError(__FILE__, __LINE__);
    int status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLchar message[MSG_LENGTH] = {0};
        glGetShaderInfoLog(program, MSG_LENGTH - 1, nullptr, &message[0]);
        EFFECT_LOGE("LoadShader Error: %{public}s", message);
        glDeleteProgram(program);
        program = 0;
    }
    if (vs > 0) {
        glDetachShader(program, vs);
        glDeleteShader(vs);
    }
    if (fs > 0) {
        glDetachShader(program, fs);
        glDeleteShader(fs);
    }
    return program;
}

bool GLUtils::CheckError(const char *file, int line)
{
    GLenum status = glGetError();
    if (status != GL_NO_ERROR) {
        EFFECT_LOGE("GL Error: 0x%{public}x, [%{public}s : %{public}d]", status, file, line);
        return false;
    }
    return true;
}
}
}
}