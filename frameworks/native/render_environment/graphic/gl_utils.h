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

#ifndef GL_UTILS_H
#define GL_UTILS_H

#include "render_context.h"
#include "base/render_base.h"
#include "surface_buffer.h"
#include "image_effect_marco_define.h"

namespace OHOS {
namespace Media {
namespace Effect {
class GLUtils {
public:
    IMAGE_EFFECT_EXPORT
    static GLuint CreateTexture2D(GLsizei width, GLsizei height, GLsizei levels, GLenum internalFormat,
        GLint minFilter, GLint magFilter, GLint wrapS, GLint wrapT);

    IMAGE_EFFECT_EXPORT static unsigned int CreateFramebuffer(unsigned int textureId = 0);

    static unsigned int CreateFramebufferWithTarget(unsigned int textureId, GLenum target);

    IMAGE_EFFECT_EXPORT static void DeleteTexture(unsigned int textureId);

    IMAGE_EFFECT_EXPORT static void DeleteFboOnly(unsigned int fbo);

    static unsigned int LoadShader(const std::string &src, unsigned int shaderType);

    static unsigned int CreateProgram(const std::string &vss, const std::string &fss);

    IMAGE_EFFECT_EXPORT
    static GLuint CreateTexWithStorage(GLenum target, int levels, GLenum internalFormat, int width, int height);

    IMAGE_EFFECT_EXPORT static size_t GetInternalFormatPixelByteSize(GLenum internalFormat);

    IMAGE_EFFECT_EXPORT static void CheckError(const char *file, int line);

    static EGLImageKHR CreateEGLImage(EGLDisplay display, SurfaceBuffer *buffer);

    static GLuint CreateTextureFromImage(EGLImageKHR img);

    static GLuint CreateTextureFromSurfaceBuffer(SurfaceBuffer *buffer);

    static void DestroyImage(EGLImageKHR img);

    static void DestroySyncKHR(EGLSyncKHR sync);

    static void CreateSyncKHR(EGLSyncKHR &sync);

    static int32_t GetEGLFenceFd(EGLSyncKHR sync);
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // GL_UTILS_H
