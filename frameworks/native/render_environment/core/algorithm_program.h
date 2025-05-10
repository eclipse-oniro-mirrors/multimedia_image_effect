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

#ifndef ALGORITHM_PROGRAM_H
#define ALGORITHM_PROGRAM_H

#include "base/render_base.h"
#include "core/render_resource_cache.h"
#include "graphic/render_context.h"
#include "graphic/render_general_program.h"

namespace OHOS {
namespace Media {
namespace Effect {
class AlgorithmProgram {
public:
    AlgorithmProgram(const std::string &vertex, const std::string &fragment);
    ~AlgorithmProgram();
    void UpdateShader(const std::string &vertex, const std::string &fragment);
    void Bind();
    void Unbind();
    int GetAttributeLocation(const std::string &attributeName);
    int GetUniformLocation(const std::string &uniformName);
    void SetInt(const std::string &name, int value);
    void SetFloat(const std::string &name, float value);
    void SetMat4(const std::string &name, const void* value);
    void BindTexture(const std::string &name, int unitId, int textureId, GLenum target = GL_TEXTURE_2D);
    void UnBindTexture(int unitId, GLenum target = GL_TEXTURE_2D);
    RenderGeneralProgram *GetShader();
private:
    RenderGeneralProgram *shader_ = nullptr;
    std::string vertexShaderCode_;
    std::string fragmentShaderCode_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif