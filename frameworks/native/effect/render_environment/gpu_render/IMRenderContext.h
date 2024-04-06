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

#ifndef IM_RENDER_CONTEXT_H
#define IM_RENDER_CONTEXT_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>

#include "RenderComponent.h"
#include "IMRenderSurface.h"

namespace OHOS {
namespace Media {
namespace Effect {
class IMRenderContext : public RenderComponent {
public:
    IMRenderContext();
    ~IMRenderContext() override;

    virtual bool Create(IMRenderContext *sharedContext);
    bool Init() override;
    bool Release() override;
    virtual bool MakeCurrent(const IMRenderSurface *surface);
    virtual bool ReleaseCurrent();
    virtual bool SwapBuffers(const IMRenderSurface *surface);

private:
    EGLDisplay m_display;
    EGLContext m_context;
    IMRenderAttribute m_attribute;
};

typedef std::shared_ptr<IMRenderContext> RenderContextPtr;
}
}
}
#endif