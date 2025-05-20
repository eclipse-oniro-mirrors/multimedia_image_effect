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

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES

#include "gl_utils.h"
#include "surface_buffer.h"
#include "native_window.h"
#include "effect_trace.h"
#include "effect_log.h"

#include <GLES2/gl2ext.h>

#include <cmath>
#include <string>
#include <cstdlib>
#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
constexpr const int MSG_SIZE = 512;
constexpr const int BYTES_OF_RGBA16F = 8;
constexpr const int BYTES_OF_R32F = 4;
constexpr const int BYTES_OF_R8 = 1;
constexpr const int BYTES_OF_RGB565 = 2;
constexpr const int BYTES_OF_RGBA4 = 2;
constexpr const EGLenum FENCE_TYPE = 0x3144;

GLuint GLUtils::CreateTexture2D(GLsizei width, GLsizei height, GLsizei levels, GLenum internalFormat, GLint minFilter,
    GLint magFilter, GLint wrapS, GLint wrapT)
{
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexStorage2D(GL_TEXTURE_2D, levels, internalFormat, width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    glBindTexture(GL_TEXTURE_2D, GL_NONE);
    CheckError(__FILE__, __LINE__);
    return textureId;
}

void GLUtils::CreateDefaultTexture(GLsizei width, GLsizei height, GLenum internalFormat, GLuint textureId)
{
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, width, height);
    glBindTexture(GL_TEXTURE_2D, GL_NONE);
    CheckError(__FILE__, __LINE__);
}

unsigned int GLUtils::GetTexWidth(GLuint texture)
{
    GLint textureWidth = 0;
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &textureWidth);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureWidth;
}

unsigned int GLUtils::GetTexHeight(GLuint texture)
{
    GLint textureHeight = 0;
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &textureHeight);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureHeight;
}

unsigned int GLUtils::GetTexFormat(GLuint texture)
{
    GLint textureFormat = 0;
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &textureFormat);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureFormat;
}

unsigned int GLUtils::CreateFramebuffer(unsigned int textureId)
{
    GLuint fboId;
    glGenFramebuffers(1, &fboId);
    if (textureId != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    }
    return fboId;
}

unsigned int GLUtils::CreateFramebufferWithTarget(unsigned int textureId, GLenum target)
{
    GLuint fboId;
    glGenFramebuffers(1, &fboId);
    if (textureId != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, textureId, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    }
    return fboId;
}

void GLUtils::DeleteFboOnly(unsigned int fbo)
{
    glDeleteFramebuffers(1, &fbo);
}

void GLUtils::DeleteTexture(unsigned int textureId)
{
    if (textureId == 0) {
        return;
    }
    glDeleteTextures(1, &textureId);
}

GLuint GLUtils::CreateTexWithStorage(GLenum target, int levels, GLenum internalFormat,
    int width, int height)
{
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(target, textureId);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexStorage2D(target, levels, internalFormat, width, height);
    glBindTexture(target, 0);
    return textureId;
}

void GLUtils::CheckError(const char *file, int line)
{
    GLenum status = glGetError();
    if (status != GL_NO_ERROR) {
        EFFECT_LOGE("GL Error: 0x%{public}x, [%{public}s : %{public}d]", status, file, line);
    }
}

size_t GLUtils::GetInternalFormatPixelByteSize(GLenum internalFormat)
{
    int ret = 0;
    switch (internalFormat) {
        case GL_RGBA8:
        case GL_R32F:
        case GL_RGB10_A2:
            ret = BYTES_OF_R32F;
            break;
        case GL_RGBA16F:
            ret = BYTES_OF_RGBA16F;
            break;
        case GL_R8:
            ret = BYTES_OF_R8;
            break;
        case GL_RGB565:
            ret = BYTES_OF_RGB565;
            break;
        case GL_RGBA4:
            ret = BYTES_OF_RGBA4;
            break;
        default:
            break;
    }
    return ret;
}

unsigned int GLUtils::LoadShader(const std::string &src, unsigned int shaderType)
{
    const char* tempSrc = src.c_str();
    unsigned int shader = glCreateShader(shaderType);
    if (shader == 0) {
        EFFECT_LOGE("Could Not Create Shader");
    }
    glShaderSource(shader, 1, &tempSrc, nullptr);
    glCompileShader(shader);
    int status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLchar message[MSG_SIZE] = {0};
        glGetShaderInfoLog(shader, MSG_SIZE - 1, nullptr, &message[0]);
        EFFECT_LOGE("LoadShader Error: %{public}s", message);
        glDeleteShader(shader);
        shader = 0;
    }
    CheckError(__FILE__, __LINE__);
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
        GLchar message[MSG_SIZE] = {0};
        glGetShaderInfoLog(program, MSG_SIZE - 1, nullptr, &message[0]);
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

EGLImageKHR GLUtils::CreateEGLImage(EGLDisplay display, SurfaceBuffer *buffer)
{
    NativeWindowBuffer *nBuffer = CreateNativeWindowBufferFromSurfaceBuffer(&buffer);
    EGLint attrsList[] = {
        EGL_IMAGE_PRESERVED,
        EGL_TRUE,
        EGL_NONE,
    };
    EGLint *attrs = attrsList;
    EGLImageKHR img = eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_OHOS, nBuffer, attrs);
    if (img == EGL_NO_IMAGE_KHR) {
        EGLint error = eglGetError();
        EFFECT_LOGE("CreateEGLImage Error: %{public}d", error);
        eglTerminate(display);
    }
    CheckError(__FILE__, __LINE__);
    DestroyNativeWindowBuffer(nBuffer);
    return img;
}

GLuint GLUtils::CreateTextureFromImage(EGLImageKHR img)
{
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, static_cast<GLeglImageOES>(img));
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, GL_NONE);
    CheckError(__FILE__, __LINE__);
    return textureId;
}

void GLUtils::DestroyImage(EGLImageKHR img)
{
    eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), img);
}

void GLUtils::DestroySyncKHR(EGLSyncKHR sync)
{
    if (sync != EGL_NO_SYNC_KHR) {
        eglDestroySyncKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), sync);
    }
}

void GLUtils::CreateSyncKHR(EGLSyncKHR &sync)
{
    DestroySyncKHR(sync);
    sync = eglCreateSyncKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), FENCE_TYPE, nullptr);
}

int32_t GLUtils::GetEGLFenceFd(EGLSyncKHR sync)
{
    int32_t glFenceFd = -1;
    if (sync != EGL_NO_SYNC_KHR) {
        glFenceFd = eglDupNativeFenceFDANDROID(eglGetDisplay(EGL_DEFAULT_DISPLAY), sync);
        glFenceFd = glFenceFd >= 0 ? glFenceFd : -1;
    }

    return glFenceFd;
}

GLuint GLUtils::CreateTextureFromSurfaceBuffer(SurfaceBuffer *buffer)
{
    if (buffer == nullptr) {
        return 0;
    }
    EGLImageKHR img = CreateEGLImage(eglGetDisplay(EGL_DEFAULT_DISPLAY), buffer);
    GLuint tex = CreateTextureFromImage(img);
    return tex;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS