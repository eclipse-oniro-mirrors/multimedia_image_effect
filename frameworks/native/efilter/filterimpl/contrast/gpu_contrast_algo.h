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

#ifndef IMAGE_EFFECT_GPU_CONTRAST_ALGO_H
#define IMAGE_EFFECT_GPU_CONTRAST_ALGO_H

#include "effect_buffer.h"
#include "error_code.h"
#include "any.h"
#include "image_effect_marco_define.h"

#include "core/render_default_data.h"
#include "core/algorithm_program.h"
#include "render_environment.h"
#include "effect_context.h"

namespace OHOS {
namespace Media {
namespace Effect {
class ContrastFilterData {
public:
    RenderTexturePtr inputTexture_ = nullptr;
    unsigned int outputWidth_;
    unsigned int outputHeight_;
    float ratio;
};
using ContrastFilterDataPtr = std::shared_ptr<ContrastFilterData>;
class GpuContrastAlgo {
public:
    IMAGE_EFFECT_EXPORT
    ErrorCode OnApplyRGBA8888(EffectBuffer *src, EffectBuffer *dst, std::map<std::string, Plugin::Any> &value,
        const std::shared_ptr<EffectContext> &context);
    ErrorCode Release();
    ErrorCode Init();
    void Render(GLenum target, RenderTexturePtr tex);
private:
    float ParseContrast(std::map<std::string, Plugin::Any> &value);
    ContrastFilterDataPtr renderEffectData_;
    void PreDraw(GLenum target);
    void PostDraw(GLenum target);
    RenderContext *context_{ nullptr };
    std::string vertexShaderCode_;
    std::string fragmentShaderCode_;
    GLuint fbo_{ 0 };
    AlgorithmProgram *shader_{ nullptr };
    RenderMesh *renderMesh_{ nullptr };
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_GPU_CONTRAST_ALGO_H