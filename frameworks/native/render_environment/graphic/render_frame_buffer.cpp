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

#include "render_frame_buffer.h"
#include "base/render_base.h"
#include "graphic/gl_utils.h"
#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
RenderFrameBuffer::RenderFrameBuffer(RenderContext *ctx, ResourceCache *cache, int width, int height,GLenum interFmt)
{
    CHECK_AND_RETURN_LOG(ctx != nullptr && cache != nullptr && texture_ != nullptr,
        "RenderFrameBuffer struct fail, ctx or cache or texture_ is null.");
    texture_ = cache->RequestTexture(ctx, width, height, interFmt);
    context_ = ctx;
    cache_ = cache;
    fboId_ = GLUtils::CreateFramebuffer(texture_->GetName());
    GLUtils::CheckError(__FILE_NAME__, __LINE__);
}

RenderFrameBuffer::~RenderFrameBuffer()
{
    if (fboId_) {
        GLUtils::DeleteFboOnly(fboId_);
    }
}

void RenderFrameBuffer::Resize(int width, int height)
{
    CHECK_AND_RETURN_LOG(texture_ != nullptr, "RenderFrameBuffer Resize fail, texture_ is null.");
    if ((width != (int)texture_->Width()) || (height != (int)texture_->Height())) {
        GLenum fmt = texture_->Format();
        texture_.reset();
        texture_ = cache_->RequestTexture(context_, width, height, fmt);
        glBindFramebuffer(GL_FRAMEBUFFER, fboId_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_->GetName(), 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        GLUtils::CheckError(__FILE_NAME__, __LINE__);
    }
}

RenderTexturePtr RenderFrameBuffer::Texture() const
{
    return texture_;
}

void RenderFrameBuffer::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fboId_);
}

void RenderFrameBuffer::UnBind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
}

int RenderFrameBuffer::Width()
{
    return texture_->Width();
}

int RenderFrameBuffer::Height()
{
    return texture_->Height();
}
} // namespace Effect
} // namespace Media
} // namespace OHOS