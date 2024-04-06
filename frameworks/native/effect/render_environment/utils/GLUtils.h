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

#include <string>

#include <GLES3/gl3.h>

#include "error_code.h"

namespace OHOS {
namespace Media {
namespace Effect {
class GLUtils {
public:
    static ErrorCode CreateTexture(int width, int height, const void *data, GLuint &textureId);

    static void DeleteTexture(unsigned int texture);

    static ErrorCode CreateFramebuffer(GLuint &fboId, unsigned int textureId = 0);

    static unsigned int LoadShader(const std::string &src, unsigned int shaderType);

    static unsigned int CreateProgram(const std::string &vss, const std::string &fss);

    static bool CheckError(const char* file, int line);
};
}
}
}
#endif
