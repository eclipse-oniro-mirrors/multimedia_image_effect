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

#ifndef RENDER_SURFACE_H
#define RENDER_SURFACE_H

#include "render_attribute.h"
#include "render_lib_header.h"
#include "render_object.h"
#include "base/render_base.h"
#include "image_effect_marco_define.h"

namespace OHOS {
namespace Media {
namespace Effect {
class RenderSurface : public RenderObject {
public:
    enum class SurfaceType {
        SURFACE_TYPE_NULL,
        SURFACE_TYPE_ON_SCREEN,
        SURFACE_TYPE_OFF_SCREEN
    };
    IMAGE_EFFECT_EXPORT explicit RenderSurface(const std::string &tag);
    IMAGE_EFFECT_EXPORT ~RenderSurface() override;

    virtual void SetAttrib(const RenderAttribute& attrib);
    virtual RenderAttribute GetAttrib();
    virtual bool Create(void *window);
    bool Init() override;
    bool Release() override;

    void *GetRawSurface() const;
    SurfaceType GetSurfaceType() const;

private:
    EGLDisplay display_;
    EGLConfig config_;
    EGLSurface surface_;
    RenderAttribute attribute_;
    SurfaceType surfaceType_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif