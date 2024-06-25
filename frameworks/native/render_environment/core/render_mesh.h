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

#ifndef RENDER_MESH_H
#define RENDER_MESH_H

#include "base/render_base.h"
#include "graphic/render_context.h"
#include "graphic/render_general_program.h"

namespace OHOS {
namespace Media {
namespace Effect {
constexpr int RENDERMESH_VERTEX_SIZE = 3;
constexpr int RENDERMESH_TEXCOORD_SIZE = 2;
constexpr int RENDERMESH_ONE_VEC_SIZE = 5;
constexpr int RENDERMESH_BUFFER_SIZE = 2;
class RenderMesh {
public:
    explicit RenderMesh(const std::vector<std::vector<float>> &meshData);
    ~RenderMesh();
    void Bind(RenderGeneralProgram *shader);

    GLuint *vboIds_ = nullptr;
    GLuint vaoId_ = 0;
    GLenum primitiveType_ = GL_TRIANGLE_STRIP;
    GLint startVertex_ = 0;
    GLsizei vertexNum_ = 4;
    std::vector<std::vector<float>> meshData_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif