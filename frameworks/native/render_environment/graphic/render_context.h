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

#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H

#include "base/render_base.h"
#include "graphic/render_attribute.h"
#include "graphic/render_lib_header.h"
#include "graphic/render_object.h"
#include "graphic/render_surface.h"
#include "image_effect_marco_define.h"

namespace OHOS {
namespace Media {
namespace Effect {
class RenderContext : public RenderObject {
public:
    IMAGE_EFFECT_EXPORT RenderContext();
    IMAGE_EFFECT_EXPORT ~RenderContext();

    virtual bool Create(RenderContext *sharedContext);
    bool Init() override;
    bool Release() override;
    virtual bool MakeCurrent(const RenderSurface *surface);
    virtual bool ReleaseCurrent();
    virtual bool SwapBuffers(const RenderSurface *surface);

private:
    EGLDisplay display_;
    EGLContext context_;
    RenderAttribute attribute_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif