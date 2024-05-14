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

#ifndef RENDER_ATTRIBUTE_H
#define RENDER_ATTRIBUTE_H

#include "render_lib_header.h"
#include "base/render_base.h"

namespace OHOS {
namespace Media {
namespace Effect {
const int COLOR_LENGTH = 8;
class RenderAttribute {
public:
    RenderAttribute() = default;
    ~RenderAttribute() = default;
    std::vector<int> ToEGLAttribList() const;

private:
    int redBits_ = COLOR_LENGTH;
    int greenBits_ = COLOR_LENGTH;
    int bluBits_ = COLOR_LENGTH;
    int alphaBits_ = COLOR_LENGTH;
    int depthBits_ = 0;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif