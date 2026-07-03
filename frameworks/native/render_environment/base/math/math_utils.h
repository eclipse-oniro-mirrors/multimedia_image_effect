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
#include <cmath>

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
        return degrees * static_cast<T>(PI / 180.0);
    }

    template<class R, class TL, class TR> static void Multiply(R &result, const TL &left, const TR &right)
    {
        static_assert(std::is_same_v<R, decltype(std::declval<TL>() * std::declval<TR>())>,
            "MathUtils::Multiply(): Wrong R Type!");
        result = left * right;
    }

    template<class T, class... Vs> static void Rotate(T &result, const T &src, float radians, const Vs &... v)
    {
        Vec3 axis(v...);
        float c = std::cos(radians);
        float s = std::sin(radians);

        float len = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
        if (len > 0.0f) {
            axis.x /= len;
            axis.y /= len;
            axis.z /= len;
        }

        Mat4x4 rot;
        rot.data[0] = c + axis.x * axis.x * (1.0f - c);
        rot.data[1] = axis.y * axis.x * (1.0f - c) + axis.z * s;
        rot.data[2] = axis.z * axis.x * (1.0f - c) - axis.y * s;
        rot.data[3] = 0.0f;

        rot.data[4] = axis.x * axis.y * (1.0f - c) - axis.z * s;
        rot.data[5] = c + axis.y * axis.y * (1.0f - c);
        rot.data[6] = axis.z * axis.y * (1.0f - c) + axis.x * s;
        rot.data[7] = 0.0f;

        rot.data[8] = axis.x * axis.z * (1.0f - c) + axis.y * s;
        rot.data[9] = axis.y * axis.z * (1.0f - c) - axis.x * s;
        rot.data[10] = c + axis.z * axis.z * (1.0f - c);
        rot.data[11] = 0.0f;

        rot.data[12] = 0.0f;
        rot.data[13] = 0.0f;
        rot.data[14] = 0.0f;
        rot.data[15] = 1.0f;

        result = src * rot;
    }

    static Mat4x4 Perspective(float fov, float aspect, float nearV, float farV)
    {
        Mat4x4 result(0.0f);

        float f = 1.0f / std::tan(fov / 2.0f);

        result.data[0] = f / aspect;
        result.data[5] = f;
        result.data[10] = (farV + nearV) / (nearV - farV);
        result.data[11] = -1.0f;
        result.data[14] = (2.0f * farV * nearV) / (nearV - farV);

        return result;
    }

    template<class T> static T Inversed(const T &in)
    {
        return in.Inverse();
    }

    template<class T> static const void *NativePtr(const T &v)
    {
        return reinterpret_cast<const void *>(v.Data());
    }
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif