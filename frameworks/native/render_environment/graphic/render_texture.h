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

#ifndef RENDER_TEXTURE_H
#define RENDER_TEXTURE_H

#include "base/render_base.h"
#include "graphic/gl_utils.h"
#include "graphic/render_object.h"
#include "effect_buffer.h"
#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
class RenderTexture : public RenderObject {
public:
    RenderTexture(GLsizei w, GLsizei h, GLenum interFmt = GL_RGBA8)
    {
        internalFormat_ = interFmt;
        width_ = w;
        height_ = h;
    }

    ~RenderTexture() = default;

    unsigned int Width()
    {
        return width_;
    }

    unsigned int Height()
    {
        return height_;
    }

    GLenum Format()
    {
        return internalFormat_;
    }

    void SetName(unsigned int name)
    {
        name_ = name;
    }

    unsigned int GetName()
    {
        return name_;
    }

    bool Init() override
    {
        if (!IsReady()) {
            name_ = GLUtils::CreateTexture2D(width_, height_, 1, internalFormat_, GL_LINEAR, GL_LINEAR,
                GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
            SetReady(true);
        }
        return true;
    }

    bool Release() override
    {
        GLUtils::DeleteTexture(name_);
        name_ = GL_ZERO;
        width_ = 0;
        height_ = 0;
        SetReady(false);
        return true;
    }

private:
    GLuint name_{ GL_ZERO };
    GLsizei width_{ 0 };
    GLsizei height_{ 0 };
    GLenum internalFormat_;
};

class TextureSizeMeasurer {
public:
    size_t operator () (const RenderTexturePtr &tex)
    {
        size_t width = static_cast<size_t>(tex->Width());
        size_t height = static_cast<size_t>(tex->Height());
        CHECK_AND_RETURN_RET_LOG(width != 0 && height !=0, 0,
            "TextureSizeMeasurer: width or height is null! width=%{public}zu, height=%{public}zu", width, height);
        CHECK_AND_RETURN_RET_LOG(width <= std::numeric_limits<size_t>::max() / height, 0,
            "TextureSizeMeasurer: Integer overflow! width=%{public}zu, height=%{public}zu", width, height);
        CHECK_AND_RETURN_RET_LOG(GLUtils::GetInternalFormatPixelByteSize(tex->Format()) <=
            std::numeric_limits<size_t>::max() / (width * height), 0,
            "TextureSizeMeasurer: Integer overflow! width=%{public}zu, height=%{public}zu, format=%{public}zu",
            width, height, GLUtils::GetInternalFormatPixelByteSize(tex->Format()));

        return (width * height * GLUtils::GetInternalFormatPixelByteSize(tex->Format()));
    }
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif