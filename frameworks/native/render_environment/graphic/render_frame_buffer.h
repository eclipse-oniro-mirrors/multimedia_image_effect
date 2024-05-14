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

#ifndef RENDER_FRAME_BUFFER_H
#define RENDER_FRAME_BUFFER_H

#include "base/render_base.h"
#include "core/render_resource_cache.h"
#include "graphic/render_context.h"
#include "graphic/render_texture.h"

namespace OHOS {
namespace Media {
namespace Effect {
class ResourceCache;
class RenderFrameBuffer {
public:
    RenderFrameBuffer(RenderContext *ctx, ResourceCache *cache, int width, int height, GLenum interFmt = GL_RGBA8);
    ~RenderFrameBuffer();

    void Resize(int width, int height);
    RenderTexturePtr Texture();
    void Bind();
    void UnBind();
    int Width();
    int Height();

private:
    unsigned int fboId_ = GL_NONE;
    RenderTexturePtr texture_;
    RenderContext *context_{ nullptr };
    ResourceCache *cache_{ nullptr };
};

using RenderFrameBufferPtr = std::shared_ptr<RenderFrameBuffer>;
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // RENDER_FRAME_BUFFER_H