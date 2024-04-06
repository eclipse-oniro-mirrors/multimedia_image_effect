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

#ifndef IM_RENDER_SURFACE_H
#define IM_RENDER_SURFACE_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>

#include "IMRenderAttribute.h"
#include "RenderComponent.h"

namespace OHOS {
namespace Media {
namespace Effect {
class IMRenderSurface : public RenderComponent {
public:
    enum class SurfaceType {
        SURFACE_TYPE_NULL,
        SURFACE_TYPE_NO_SCREEN,
        SURFACE_TYPE_OFF_SCREEN
    };
    explicit IMRenderSurface(const std::string &tag);
    ~IMRenderSurface() override;

    virtual void SetAttrib(const IMRenderAttribute& attrib);
    virtual IMRenderAttribute GetAttrib();
    virtual bool Create(void *window);
    bool Init() override;
    bool Release() override;

    void *GetRawSurface() const;
    SurfaceType GetSurfaceType() const;

private:
    EGLDisplay m_display;
    EGLConfig m_config;
    EGLSurface m_surface;
    IMRenderAttribute m_attribute;
    SurfaceType m_surfaceType;
};
}
}
}
#endif