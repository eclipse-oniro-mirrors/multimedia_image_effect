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

#include "render_context.h"

namespace OHOS {
namespace Media {
namespace Effect {
RenderContext::RenderContext() : display_(EGL_NO_DISPLAY), context_(EGL_NO_CONTEXT) {}

RenderContext::~RenderContext() {}

bool RenderContext::Create(RenderContext *sharedContext)
{
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) {
        EFFECT_LOGE("RenderContext: unable to get EGL display.");
        return false;
    }
    int attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    int *attribList = attribs;
    context_ = eglCreateContext(display_, EGL_NO_CONFIG_KHR, sharedContext, attribList);
    if (context_ == nullptr) {
        EFFECT_LOGE("RenderContext: unable to create egl context, code: %{public}d", eglGetError());
        return false;
    }

    eglSwapInterval(display_, 0);

    SetReady(true);
    return true;
}

bool RenderContext::Init()
{
    return Create(nullptr);
}

bool RenderContext::Release()
{
    if (IsReady()) {
        EGLBoolean ret = eglDestroyContext(display_, context_);
        if (ret != EGL_TRUE) {
            EGLint error = eglGetError();
            EFFECT_LOGE("RenderSurface Release Failed, code: %{public}d", error);
            return false;
        }
        SetReady(false);
    }
    return true;
}

bool RenderContext::MakeCurrent(const RenderSurface *surface)
{
    if (!IsReady()) {
        EFFECT_LOGE("EGL MakeCurrent failed");
        return false;
    }
    EGLSurface rawSurface = EGL_NO_SURFACE;
    if (surface) {
        rawSurface = static_cast<EGLSurface>(surface->GetRawSurface());
    }
    EGLBoolean ret = eglMakeCurrent(display_, rawSurface, rawSurface, context_);
    if (ret != EGL_TRUE) {
        EGLint error = eglGetError();
        EFFECT_LOGE("RenderSurface eglMakeCurrent failed, code: %{public}d", error);
        return false;
    }
    eglSwapInterval(display_, 0);
    return true;
}

bool RenderContext::ReleaseCurrent()
{
    if (!IsReady()) {
        EFFECT_LOGE("EGL ReleaseCurrent failed");
        return false;
    }
    EGLBoolean ret = eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (ret != EGL_TRUE) {
        EGLint error = eglGetError();
        EFFECT_LOGE("RenderSurface ReleaseCurrent failed, code: %{public}d", error);
        return false;
    }
    return true;
}

bool RenderContext::SwapBuffers(const RenderSurface *surface)
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
    EGLBoolean ret = eglSwapBuffers(display_, rawSurface);
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