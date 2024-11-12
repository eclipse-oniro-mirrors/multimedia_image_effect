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

#include "render_surface.h"
#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
RenderSurface::RenderSurface(const std::string &tag)
    : display_(EGL_NO_DISPLAY),
      config_(EGL_NO_CONFIG_KHR),
      surface_(EGL_NO_SURFACE),
      surfaceType_(SurfaceType::SURFACE_TYPE_NULL)
{}

RenderSurface::~RenderSurface() {}

void RenderSurface::SetAttrib(const RenderAttribute &attrib)
{
    attribute_ = attrib;
}

RenderAttribute RenderSurface::GetAttrib()
{
    return attribute_;
}

bool RenderSurface::Create(void *window)
{
    CHECK_AND_RETURN_RET_LOG(window != nullptr, false, "RenderSurface Create window is null!");
    EGLint retNum = 0;
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    CHECK_AND_RETURN_RET_LOG(display_ != nullptr, false, "RenderSurface eglGetDisplay fail.");
    std::vector<int> attributeList = attribute_.ToEGLAttribList();
    EGLBoolean ret = eglChooseConfig(display_, attributeList.data(), &config_, 1, &retNum);
    if (ret != EGL_TRUE) {
        EGLint error = eglGetError();
        EFFECT_LOGE("RenderSurface create failed, code: %{public}d", error);
        return false;
    }
    EGLint surfaceAttribs[] = {
        EGL_NONE
    };
    EGLNativeWindowType mEglWindow = reinterpret_cast<EGLNativeWindowType>(window);
    surface_ = eglCreateWindowSurface(display_, config_, mEglWindow, surfaceAttribs);
    if (surface_ == EGL_NO_SURFACE) {
        EGLint error = eglGetError();
        EFFECT_LOGE("RenderSurface create failed, code: %{public}d", error);
        return false;
    }
    surfaceType_ = SurfaceType::SURFACE_TYPE_ON_SCREEN;
    SetReady(true);
    return true;
}

bool RenderSurface::Init()
{
    EGLint retNum = 0;
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    std::vector<int> attributeList = attribute_.ToEGLAttribList();
    EGLBoolean ret = eglChooseConfig(display_, attributeList.data(), &config_, 1, &retNum);
    if (ret != EGL_TRUE) {
        EGLint error = eglGetError();
        EFFECT_LOGE("RenderSurface create failed, code: %{public}d", error);
        return false;
    }
    EGLint surfaceAttribs[] = {
        EGL_NONE
    };
    surface_ = eglCreatePbufferSurface(display_, config_, surfaceAttribs);
    if (surface_ == EGL_NO_SURFACE) {
        EGLint error = eglGetError();
        EFFECT_LOGE("RenderSurface create failed, code: %{public}d", error);
        return false;
    }
    surfaceType_ = SurfaceType::SURFACE_TYPE_OFF_SCREEN;
    SetReady(true);
    return true;
}

bool RenderSurface::Release()
{
    if (IsReady()) {
        EGLBoolean ret = eglDestroySurface(display_, surface_);
        if (ret != EGL_TRUE) {
            EGLint error = eglGetError();
            EFFECT_LOGE("RenderSurface create failed, code: %{public}d", error);
            return false;
        }
        surfaceType_ = SurfaceType::SURFACE_TYPE_NULL;
        SetReady(false);
    }
    return true;
}

void *RenderSurface::GetRawSurface() const
{
    return static_cast<void*>(surface_);
}

RenderSurface::SurfaceType RenderSurface::GetSurfaceType() const
{
    return surfaceType_;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS