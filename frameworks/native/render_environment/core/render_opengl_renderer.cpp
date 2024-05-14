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

#include "core/render_opengl_renderer.h"

namespace OHOS {
namespace Media {
namespace Effect {
RenderOpenglRenderer::RenderOpenglRenderer() {}

RenderOpenglRenderer::~RenderOpenglRenderer() {}

void RenderOpenglRenderer::Draw(GLuint texId, RenderMesh *mesh, RenderGeneralProgram *shader,
    RenderFrameBuffer *frameBuffer, RenderViewport *viewport)
{
    if (frameBuffer) {
        frameBuffer->Bind();
    }

    if (viewport) {
        glViewport(viewport->leftBottomX_, viewport->leftBottomY_, viewport->width_, viewport->height_);
    }

    if (shader) {
        shader->Bind();
    }

    if (mesh) {
        mesh->Bind(shader);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    glDrawArrays(mesh->primitiveType_, mesh->startVertex_, mesh->vertexNum_);

    if (shader) {
        shader->Unbind();
    }
    frameBuffer->UnBind();
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}

void RenderOpenglRenderer::Draw(GLuint texId, GLuint fbo, RenderMesh *mesh, RenderGeneralProgram *shader,
    RenderViewport *viewport, GLenum target)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    if (viewport) {
        glViewport(viewport->leftBottomX_, viewport->leftBottomY_, viewport->width_, viewport->height_);
    }
    if (shader) {
        shader->Bind();
    }
    if (mesh) {
        mesh->Bind(shader);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, texId);
    glDrawArrays(mesh->primitiveType_, mesh->startVertex_, mesh->vertexNum_);

    if (shader) {
        shader->Unbind();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}

void RenderOpenglRenderer::DrawOnScreen(GLuint texId, RenderMesh *mesh, RenderGeneralProgram *shader,
    RenderViewport *viewport, const void *trans, GLenum target)
{
    glViewport(viewport->leftBottomX_, viewport->leftBottomY_, viewport->width_, viewport->height_);
    shader->Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, texId);
    shader->SetUniform("inputTexture", (int)0);
    shader->SetUniform("transform", trans);
    mesh->Bind(shader);
    glDrawArrays(mesh->primitiveType_, mesh->startVertex_, mesh->vertexNum_);
    shader->Unbind();
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}
} // namespace Effect
} // namespace Media
} // namespace OHOS