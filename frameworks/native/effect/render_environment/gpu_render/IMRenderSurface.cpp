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

#include "IMRenderSurface.h"

namespace OHOS {
namespace Media {
namespace Effect {
IMRenderSurface::IMRenderSurface(const std::string &tag)
    : m_display(EGL_NO_DISPLAY),
      m_config(EGL_NO_CONFIG_KHR),
      m_surface(EGL_NO_SURFACE),
      m_surfaceType(SurfaceType::SURFACE_TYPE_NULL)
{}

IMRenderSurface::~IMRenderSurface() {}

void IMRenderSurface::SetAttrib(const IMRenderAttribute &attrib)
{
    m_attribute = attrib;
}

IMRenderAttribute IMRenderSurface::GetAttrib()
{
    return m_attribute;
}

bool IMRenderSurface::Create(void *window)
{
    EGLint retNum = 0;
    m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    std::vector<int> attributeList = m_attribute.ToEGLAttribList();
    EGLBoolean ret = eglChooseConfig(m_display, attributeList.data(), &m_config, 1, &retNum);
    if (ret != EGL_TRUE) {
        EGLint error = eglGetError();
        EFFECT_LOGE("RenderSurface create failed, code: %{public}d", error);
        return false;
    }
    EGLint surfaceAttribs[] = {
        EGL_NONE
    };
    EGLNativeWindowType mEglWindow = reinterpret_cast<EGLNativeWindowType>(window);
    m_surface = eglCreateWindowSurface(m_display, m_config, mEglWindow, surfaceAttribs);
    if (m_surface == EGL_NO_SURFACE) {
        EGLint error = eglGetError();
        EFFECT_LOGE("RenderSurface create failed, code: %{public}d", error);
        return false;
    }
    m_surfaceType = SurfaceType::SURFACE_TYPE_NO_SCREEN;
    SetReady(true);
    return true;
}

bool IMRenderSurface::Init()
{
    EGLint retNum = 0;
    m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    std::vector<int> attributeList = m_attribute.ToEGLAttribList();
    EGLBoolean ret = eglChooseConfig(m_display, attributeList.data(), &m_config, 1, &retNum);
    if (ret != EGL_TRUE) {
        EGLint error = eglGetError();
        EFFECT_LOGE("RenderSurface create failed, code: %{public}d", error);
        return false;
    }
    EGLint surfaceAttribs[] = {
        EGL_NONE
    };
    m_surface = eglCreatePbufferSurface(m_display, m_config, surfaceAttribs);
    if (m_surface == EGL_NO_SURFACE) {
        EGLint error = eglGetError();
        EFFECT_LOGE("RenderSurface create failed, code: %{public}d", error);
        return false;
    }
    m_surfaceType = SurfaceType::SURFACE_TYPE_NO_SCREEN;
    SetReady(true);
    return true;
}

bool IMRenderSurface::Release()
{
    if (IsReady()) {
        EGLBoolean ret = eglDestroySurface(m_display, m_surface);
        if (ret != EGL_TRUE) {
            EGLint error = eglGetError();
            EFFECT_LOGE("RenderSurface create failed, code: %{public}d", error);
            return false;
        }
        m_surfaceType = SurfaceType::SURFACE_TYPE_NULL;
        SetReady(false);
    }
    return true;
}

void *IMRenderSurface::GetRawSurface() const
{
    return static_cast<void*>(m_surface);
}

IMRenderSurface::SurfaceType IMRenderSurface::GetSurfaceType() const
{
    return m_surfaceType;
}
}
}
}