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

#ifndef RENDER_VIEW_PORT_H
#define RENDER_VIEW_PORT_H

#include "base/render_base.h"

namespace OHOS {
namespace Media {
namespace Effect {
class RenderViewport {
public:
    RenderViewport() : leftBottomX_(0), leftBottomY_(0), width_(0), height_(0) {}

    RenderViewport(int leftBottomX, int leftBottomY, int width, int height)
        : leftBottomX_(leftBottomX), leftBottomY_(leftBottomY), width_(width), height_(height)
    {}

    ~RenderViewport(){};
    void Set(int leftBottomX, int leftBottomY, int width, int height)
    {
        leftBottomX_ = leftBottomX;
        leftBottomY_ = leftBottomY;
        width_ = width;
        height_ = height;
    }
    int leftBottomX_;
    int leftBottomY_;
    int width_;
    int height_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif