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

#ifndef RENDERROI_H
#define RENDERROI_H

#include <algorithm>

#include "base/render_base.h"

namespace OHOS {
namespace Media {
namespace Effect {
class RenderROI {
public:
    RenderROI(int srcW, int srcH, double roiL, double roiR, double roiB, double roiT)
        : w(srcW), h(srcH), l(roiL), r(roiR), b(roiB), t(roiT)
    {}
    RenderROI() : RenderROI(0, 0, 0.0, 1.0, 0.0, 1.0) {}
    bool IsValid()
    {
        return std::isless(l, r) && std::isless(b, t) && std::isgreater(w, 0.0) && std::isgreater(h, 0.0);
    }
    int Width()
    {
        return (r - l) * w + 0.5f;
    }
    int Height()
    {
        return (t - b) * h + 0.5f;
    }

    int w;
    int h;
    double l;
    double r;
    double b;
    double t;
};

using RenderROIPtr = std::shared_ptr<RenderROI>;
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // RENDERROI_H