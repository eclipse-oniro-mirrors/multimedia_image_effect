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

#ifndef IMAGE_EFFECT_EXT_RENDER_GENERAL_PROGRAM_H
#define IMAGE_EFFECT_EXT_RENDER_GENERAL_PROGRAM_H

#include "base/render_base.h"
#include "graphic/render_program.h"
#include "image_effect_marco_define.h"

namespace OHOS {
namespace Media {
namespace Effect {
class RenderGeneralProgram : public RenderProgram {
public:
    IMAGE_EFFECT_EXPORT RenderGeneralProgram(const std::string &vss, const std::string &fss);
    IMAGE_EFFECT_EXPORT ~RenderGeneralProgram();
    IMAGE_EFFECT_EXPORT bool Init() override;
    IMAGE_EFFECT_EXPORT bool Release() override;

private:
    std::string vss_;
    std::string fss_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif
