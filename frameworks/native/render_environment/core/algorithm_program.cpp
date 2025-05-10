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

#include "algorithm_program.h"

#include "base/math/math_utils.h"
#include "graphic/gl_utils.h"

namespace OHOS {
namespace Media {
namespace Effect {
AlgorithmProgram::AlgorithmProgram(const std::string &vertex, const std::string &fragment)
{
    shader_ = new RenderGeneralProgram(vertex.c_str(), fragment.c_str());
    shader_->Init();
    vertexShaderCode_ = vertex;
    fragmentShaderCode_ = fragment;
}

AlgorithmProgram::~AlgorithmProgram()
{
    if (shader_) {
        shader_->Release();
        delete shader_;
        shader_ = nullptr;
    }
}

void AlgorithmProgram::UpdateShader(const std::string &vertex, const std::string &fragment)
{
    if (strcmp(vertex.c_str(), vertexShaderCode_.c_str()) == 0 &&
        strcmp(fragment.c_str(), fragmentShaderCode_.c_str()) == 0) {
        return;
    }
    if (shader_) {
        shader_->Release();
        delete shader_;
        shader_ = nullptr;
    }
    shader_ = new RenderGeneralProgram(vertex.c_str(), fragment.c_str());
    shader_->Init();
    vertexShaderCode_ = vertex;
    fragmentShaderCode_ = fragment;
}

void AlgorithmProgram::Bind()
{
    shader_->Bind();
}

void AlgorithmProgram::Unbind()
{
    shader_->Unbind();
}

int AlgorithmProgram::GetAttributeLocation(const std::string &attributeName)
{
    return shader_->GetAttributeLocation(attributeName.c_str());
}

int AlgorithmProgram::GetUniformLocation(const std::string &uniformName)
{
    return shader_->GetUniformLocation(uniformName.c_str());
}

void AlgorithmProgram::SetInt(const std::string &name, int value)
{
    shader_->SetUniform(name.c_str(), value);
}

void AlgorithmProgram::SetFloat(const std::string &name, float value)
{
    shader_->SetUniform(name.c_str(), value);
}

void AlgorithmProgram::SetMat4(const std::string &name, const void *value)
{
    shader_->SetUniform(name.c_str(), value);
}

void AlgorithmProgram::BindTexture(const std::string &name, int unitId, int textureId, GLenum target)
{
    glActiveTexture(GL_TEXTURE0 + unitId);
    glBindTexture(target, textureId);
    glUniform1i(GetUniformLocation(name), unitId);
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}

void AlgorithmProgram::UnBindTexture(int unitId, GLenum target)
{
    glActiveTexture(GL_TEXTURE0 + unitId);
    glBindTexture(target, GL_NONE);
}

RenderGeneralProgram *AlgorithmProgram::GetShader()
{
    return shader_;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS