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

#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <utility>

#include "samples/RhythmGame/third_party/glm/geometric.hpp"
#include "samples/RhythmGame/third_party/glm/gtc/matrix_transform.hpp"
#include "samples/RhythmGame/third_party/glm/gtx/matrix_transform_2d.hpp"
#include "samples/RhythmGame/third_party/glm/gtc/type_ptr.hpp"
#include "samples/RhythmGame/third_party/glm/trigonometric.hpp"

#include "render_vector.h"
#include "render_matrix.h"
#include "base/render_base.h"

namespace OHOS {
namespace Media {
namespace Effect {
class MathUtils {
public:
    constexpr const static double PI = 3.14159265358979323846264338327f;
    
    template<class T> static T Radians(T degrees)
    {
        return glm::radians(degrees);
    }

    template<class R, class TL, class TR> static void Multiply(R &result, const TL &left, const TR &right)
    {
        static_assert(std::is_same_v<R, decltype(std::declval<TL>() * std::declval<TR>())>,
            "MathUtils::Multiply(): Wrong R Type!");
        result = left * right;
    }

    template<class T, class... Vs> static void Rotate(T &result, const T &src, float radians, const Vs &... v)
    {
        result = glm::rotate(src, radians, v...);
    }

    static Mat4x4 Perspective(float fov, float aspect, float nearV, float farV)
    {
        return glm::perspective(fov, aspect, nearV, farV);
    }

    template<class T> static T Inversed(const T &in)
    {
        return glm::inverse(in);
    }

    template<class T> static const void *NativePtr(const T &v)
    {
        return reinterpret_cast<const void *>(glm::value_ptr(v));
    }
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif