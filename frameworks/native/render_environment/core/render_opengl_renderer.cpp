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

#include "base/math/math_utils.h"
#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
RenderOpenglRenderer::RenderOpenglRenderer() {}

RenderOpenglRenderer::~RenderOpenglRenderer() {}

void RenderOpenglRenderer::Draw(GLuint texId, RenderMesh *mesh, RenderGeneralProgram *shader,
    RenderFrameBuffer *frameBuffer, RenderViewport *viewport)
{
    CHECK_AND_RETURN_LOG(frameBuffer != nullptr, "RenderOpenglRenderer Draw failed! Framebuffer is null");
    CHECK_AND_RETURN_LOG(viewport != nullptr, "RenderOpenglRenderer Draw failed! Viewport is null");
    CHECK_AND_RETURN_LOG(shader != nullptr, "RenderOpenglRenderer Draw failed! Shader is null");
    CHECK_AND_RETURN_LOG(mesh != nullptr, "RenderOpenglRenderer Draw failed! Mesh is null");
    frameBuffer->Bind();
    glViewport(viewport->leftBottomX_, viewport->leftBottomY_, viewport->width_, viewport->height_);
    shader->Bind();
    mesh->Bind(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    glDrawArrays(mesh->primitiveType_, mesh->startVertex_, mesh->vertexNum_);
    shader->Unbind();
    frameBuffer->UnBind();
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}

void RenderOpenglRenderer::Draw(GLuint texId, GLuint fbo, RenderMesh *mesh, RenderGeneralProgram *shader,
    RenderViewport *viewport, GLenum target)
{
    CHECK_AND_RETURN_LOG(viewport != nullptr, "RenderOpenglRenderer Draw failed! Viewport is null");
    CHECK_AND_RETURN_LOG(shader != nullptr, "RenderOpenglRenderer Draw failed! Shader is null");
    CHECK_AND_RETURN_LOG(mesh != nullptr, "RenderOpenglRenderer Draw failed! Mesh is null");
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(viewport->leftBottomX_, viewport->leftBottomY_, viewport->width_, viewport->height_);
    shader->Bind();
    mesh->Bind(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, texId);
    glDrawArrays(mesh->primitiveType_, mesh->startVertex_, mesh->vertexNum_);
    shader->Unbind();
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}

void ProcessTransform(GraphicTransformType type, RenderGeneralProgram *shader)
{
    glm::mat4 trans = glm::mat4(1.0f);
    switch (type) {
        case GRAPHIC_ROTATE_90:
            trans = glm::rotate(trans, glm::radians(90.0f), glm::vec3(0.0, 0.0, 1.0));
            shader->SetUniform("transform", MathUtils::NativePtr(trans));
            break;
        case GRAPHIC_FLIP_H_ROT90:
            trans = glm::rotate(trans, glm::radians(90.0f), glm::vec3(0.0, 0.0, 1.0));
            shader->SetUniform("transform", MathUtils::NativePtr(trans));
            shader->SetUniform("flipH", 1.0f);
            break;
        case GRAPHIC_FLIP_V_ROT90:
            trans = glm::rotate(trans, glm::radians(90.0f), glm::vec3(0.0, 0.0, 1.0));
            shader->SetUniform("transform", MathUtils::NativePtr(trans));
            shader->SetUniform("flipV", 1.0f);
            break;
        case GRAPHIC_ROTATE_180:
            trans = glm::rotate(trans, glm::radians(180.0f), glm::vec3(0.0, 0.0, 1.0));
            shader->SetUniform("transform", MathUtils::NativePtr(trans));
            break;
        case GRAPHIC_FLIP_H_ROT180:
            trans = glm::rotate(trans, glm::radians(180.0f), glm::vec3(0.0, 0.0, 1.0));
            shader->SetUniform("transform", MathUtils::NativePtr(trans));
            shader->SetUniform("flipH", 1.0f);
            break;
        case GRAPHIC_FLIP_V_ROT180:
            trans = glm::rotate(trans, glm::radians(180.0f), glm::vec3(0.0, 0.0, 1.0));
            shader->SetUniform("transform", MathUtils::NativePtr(trans));
            shader->SetUniform("flipV", 1.0f);
            break;
        case GRAPHIC_ROTATE_270:
            trans = glm::rotate(trans, glm::radians(270.0f), glm::vec3(0.0, 0.0, 1.0));
            shader->SetUniform("transform", MathUtils::NativePtr(trans));
            break;
        case GRAPHIC_FLIP_H_ROT270:
            trans = glm::rotate(trans, glm::radians(270.0f), glm::vec3(0.0, 0.0, 1.0));
            shader->SetUniform("transform", MathUtils::NativePtr(trans));
            shader->SetUniform("flipH", 1.0f);
            break;
        case GRAPHIC_FLIP_V_ROT270:
            trans = glm::rotate(trans, glm::radians(270.0f), glm::vec3(0.0, 0.0, 1.0));
            shader->SetUniform("transform", MathUtils::NativePtr(trans));
            shader->SetUniform("flipV", 1.0f);
            break;
        default:
            break;
    }
}

void RenderOpenglRenderer::DrawOnScreenWithTransform(GLuint texId, RenderMesh *mesh, RenderGeneralProgram *shader,
    RenderViewport *viewport, GraphicTransformType type, GLenum target)
{
    glViewport(viewport->leftBottomX_, viewport->leftBottomY_, viewport->width_, viewport->height_);
    shader->Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, texId);
    ProcessTransform(type, shader);
    mesh->Bind(shader);
    glDrawArrays(mesh->primitiveType_, mesh->startVertex_, mesh->vertexNum_);
    shader->Unbind();
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