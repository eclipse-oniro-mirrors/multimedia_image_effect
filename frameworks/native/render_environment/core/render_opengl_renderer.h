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

#ifndef RENDER_OPENGL_RENDERER
#define RENDER_OPENGL_RENDERER

#include "base/render_base.h"
#include "core/render_mesh.h"
#include "core/render_viewport.h"
#include "graphic/render_context.h"
#include "graphic/render_frame_buffer.h"
#include "core/render_default_data.h"

namespace OHOS {
namespace Media {
namespace Effect {
class RenderOpenglRenderer {
public:
    explicit RenderOpenglRenderer();
    virtual ~RenderOpenglRenderer();

    void Draw(GLuint texId, GLuint fbo, RenderMesh *mesh, RenderGeneralProgram *shader, RenderViewport *viewport,
        GLenum target);

    void Draw(GLuint texId, RenderMesh *mesh, RenderGeneralProgram *shader, RenderFrameBuffer *frameBuffer,
        RenderViewport *viewport);

    void DrawOnScreen(GLuint texId, RenderMesh *mesh, RenderGeneralProgram *shader, RenderViewport *viewport,
        const void *trans, GLenum target);
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // RENDER_OPENGL_RENDERER