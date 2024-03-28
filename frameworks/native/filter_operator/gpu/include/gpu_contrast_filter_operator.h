/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef GPU_CONTRAST_FILTER_H
#define GPU_CONTRAST_FILTER_H

#include "gpu_filter_operator.h"

namespace OHOS {
namespace Media {
namespace Effect {
class GpuContrastFilterOperator : public GpuFilterOperator {
public:
    GpuContrastFilterOperator() : GpuFilterOperator(FORMAT_TYPES){};

    static std::vector<IEffectFormat> GetFormatTypes()
    {
        return FORMAT_TYPES;
    };

    static IPType GetIPType()
    {
        return IP_TYPE;
    };

    ErrorCode OnApplyRGBA8888(EffectBuffer *src, EffectBuffer *dst, std::map<std::string, Plugin::Any> &value) override;

    ErrorCode OnApplyYUVNV21(EffectBuffer *src, EffectBuffer *dst, std::map<std::string, Plugin::Any> &value) override;

    ErrorCode OnApplyYUVNV12(EffectBuffer *src, EffectBuffer *dst, std::map<std::string, Plugin::Any> &value) override;

    ErrorCode InitMesh() override;

    ErrorCode PreDraw(uint32_t width, uint32_t height, std::map<std::string, Plugin::Any> &value) override;

    ErrorCode Release() override;

    void Unbind() override;

private:
    float ParseContrast(std::map<std::string, Plugin::Any> &value);

    static const std::vector<IEffectFormat> FORMAT_TYPES;
    static const IPType IP_TYPE = IPType::GPU;
    unsigned int vbo_ = 0;
    unsigned int vao_ = 0;
    unsigned int fbo_ = 0;
    unsigned int shaderProgram_ = 0;
    unsigned int texId_ = 0;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // GPU_CONTRAST_FILTER_H