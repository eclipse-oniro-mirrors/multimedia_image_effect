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

#include "IMRenderAttribute.h"

namespace OHOS {
namespace Media {
namespace Effect {
std::vector<int> IMRenderAttribute::ToEGLAttribList() const
{
    int configAttribs[] = {
        EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
        EGL_RED_SIZE, redBits_,
        EGL_GREEN_SIZE, greenBits_,
        EGL_BLUE_SIZE, bluBits_,
        EGL_ALPHA_SIZE, alphaBits_,
        EGL_DEPTH_SIZE, depthBits_,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_NONE
    };
    std::vector<int> attribs(configAttribs, configAttribs + sizeof(configAttribs) / sizeof(EGLint));
    return attribs;
}
}
}
}