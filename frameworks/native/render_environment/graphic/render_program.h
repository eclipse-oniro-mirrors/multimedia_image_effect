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

#ifndef RENDER_PROGRAM_H
#define RENDER_PROGRAM_H

#include "base/render_base.h"
#include "graphic/render_context.h"
#include "graphic/render_object.h"
#include "image_effect_marco_define.h"

namespace OHOS {
namespace Media {
namespace Effect {
class RenderProgram : public RenderObject {
public:
    const static unsigned int INVALID_PROGRAM_NAME = 0;
    IMAGE_EFFECT_EXPORT RenderProgram(RenderContext *context);
    IMAGE_EFFECT_EXPORT ~RenderProgram() = default;
    IMAGE_EFFECT_EXPORT void Bind();
    IMAGE_EFFECT_EXPORT static void Unbind();
    IMAGE_EFFECT_EXPORT void SetUniform(const std::string &name, float value);
    IMAGE_EFFECT_EXPORT void SetUniform(const std::string &name, int value);
    void SetUniform(const std::string &name, unsigned int value);
    void SetUniform(const std::string &name, const void *value);

    unsigned int GetName();
    int GetAttributeLocation(const std::string &name);
    IMAGE_EFFECT_EXPORT int GetUniformLocation(const std::string &name);
protected:
    unsigned int program_;
    RenderContext *context_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif