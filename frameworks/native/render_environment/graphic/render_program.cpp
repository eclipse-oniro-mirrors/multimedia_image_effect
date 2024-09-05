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

#include "graphic/render_program.h"
#include <GLES3/gl3.h>
#include <effect_log.h>

namespace OHOS {
namespace Media {
namespace Effect {
RenderProgram::RenderProgram(RenderContext *context) : program_(0), context_(context) {}

void RenderProgram::SetUniform(const std::string &name, float value)
{
    CHECK_AND_RETURN_LOG(program_ != 0, "RenderProgram SetUniform failed!, name=%{public}s, program=%{public}d",
        name.c_str(), program_);
    GLint location = glGetUniformLocation(program_, name.c_str());
    CHECK_AND_RETURN_LOG(location != -1, "glGetUniformLocation failed!, name=%{public}s", name.c_str());
    glUniform1f(location, value);
}

void RenderProgram::SetUniform(const std::string &name, int value)
{
    CHECK_AND_RETURN_LOG(program_ != 0, "RenderProgram SetUniform failed!, name=%{public}s, program=%{public}d",
        name.c_str(), program_);
    GLint location = glGetUniformLocation(program_, name.c_str());
    CHECK_AND_RETURN_LOG(location != -1, "glGetUniformLocation failed!, name=%{public}s", name.c_str());
    glUniform1i(location, value);
}

void RenderProgram::SetUniform(const std::string &name, unsigned int value)
{
    CHECK_AND_RETURN_LOG(program_ != 0, "RenderProgram SetUniform failed!, name=%{public}s, program=%{public}d",
        name.c_str(), program_);
    GLint location = glGetUniformLocation(program_, name.c_str());
    CHECK_AND_RETURN_LOG(location != -1, "glGetUniformLocation failed!, name=%{public}s", name.c_str());
    glUniform1ui(location, value);
}

void RenderProgram::SetUniform(const std::string &name, const void *value)
{
    CHECK_AND_RETURN_LOG(program_ != 0, "RenderProgram SetUniform failed!, name=%{public}s, program=%{public}d",
        name.c_str(), program_);
    GLint location = glGetUniformLocation(program_, name.c_str());
    CHECK_AND_RETURN_LOG(location != -1, "glGetUniformLocation failed!, name=%{public}s", name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, reinterpret_cast<const GLfloat *>(value));

void RenderProgram::Bind()
{
    glUseProgram(program_);
}

void RenderProgram::Unbind()
{
    glUseProgram(INVALID_PROGRAM_NAME);
}

unsigned int RenderProgram::GetName()
{
    return program_;
}

int RenderProgram::GetAttributeLocation(const std::string &name)
{
    int location = glGetAttribLocation(program_, name.c_str());
    return location;
}

int RenderProgram::GetUniformLocation(const std::string &name)
{
    int location = glGetUniformLocation(program_, name.c_str());
    return location;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS