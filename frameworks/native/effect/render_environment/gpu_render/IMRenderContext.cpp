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

#include "IMRenderContext.h"

namespace OHOS {
namespace Media {
namespace Effect {
IMRenderContext::IMRenderContext() : m_display(EGL_NO_DISPLAY), m_context(EGL_NO_CONTEXT) {}

IMRenderContext::~IMRenderContext() {}

bool IMRenderContext::Create(IMRenderContext *sharedContext)
{
    m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_display == EGL_NO_DISPLAY) {
        EFFECT_LOGE("RenderContext: unable to get EGL display.");
        return false;
    }

    EGLint major;
    EGLint minor;
    if (eglInitialize(m_display, &major, &minor) == EGL_FALSE) {
        EFFECT_LOGE("EGL Initialize failed");
        return false;
    }

    if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
        EFFECT_LOGE("EGL Bind OpenGL ES API failed");
        return false;
    }

    int attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    int *attribList = attribs;
    m_context = eglCreateContext(m_display, EGL_NO_CONFIG_KHR, sharedContext, attribList);
    if (m_context == nullptr) {
        EFFECT_LOGE("RenderContext: unable to create egl context, code: %{public}d", eglGetError());
        return false;
    }

    eglSwapInterval(m_display, 0);

    SetReady(true);
    return true;
}

bool IMRenderContext::Init()
{
    return Create(nullptr);
}

bool IMRenderContext::Release()
{
    if (IsReady()) {
        EGLBoolean ret = eglDestroyContext(m_display, m_context);
        if (ret != EGL_TRUE) {
            EGLint error = eglGetError();
            EFFECT_LOGE("RenderSurface create failed, code: %{public}d", error);
            return false;
        }
        SetReady(false);
    }
    return true;
}

bool IMRenderContext::MakeCurrent(const IMRenderSurface *surface)
{
    if (!IsReady()) {
        EFFECT_LOGE("EGL MakeCurrent failed");
        return false;
    }
    EGLSurface rawSurface = EGL_NO_SURFACE;
    if (surface) {
        rawSurface = static_cast<EGLSurface>(surface->GetRawSurface());
    }
    EGLBoolean ret = eglMakeCurrent(m_display, rawSurface, rawSurface, m_context);
    if (ret != EGL_TRUE) {
        EGLint error = eglGetError();
        EFFECT_LOGE("RenderSurface eglMakeCurrent failed, code: %{public}d", error);
        return false;
    }
    eglSwapInterval(m_display, 0);
    return true;
}

bool IMRenderContext::ReleaseCurrent()
{
    if (!IsReady()) {
        EFFECT_LOGE("EGL ReleaseCurrent failed");
        return false;
    }
    EGLBoolean ret = eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (ret != EGL_TRUE) {
        EGLint error = eglGetError();
        EFFECT_LOGE("RenderSurface ReleaseCurrent failed, code: %{public}d", error);
        return false;
    }
    return true;
}

bool IMRenderContext::SwapBuffers(const IMRenderSurface *surface)
{
    if (!IsReady()) {
        EFFECT_LOGE("EGL SwapBuffers failed");
        return false;
    }
    if (surface == nullptr) {
        EFFECT_LOGE("EGL SwapBuffers surface is null");
        return false;
    }
    EGLSurface rawSurface = reinterpret_cast<EGLSurface>(surface->GetRawSurface());
    EGLBoolean ret = eglSwapBuffers(m_display, rawSurface);
    if (ret != EGL_TRUE) {
        EGLint error = eglGetError();
        EFFECT_LOGE("RenderSurface eglSwapBuffers failed, code: %{public}d", error);
        return false;
    }
    return true;
}
}
}
}