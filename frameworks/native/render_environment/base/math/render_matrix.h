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

#ifndef RENDERMATRIX_H
#define RENDERMATRIX_H

#include <cmath>
#include "render_vector.h"
#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
struct Mat2x2 {
    union {
        float data[4];
        struct {
            float m00, m01;
            float m10, m11;
        };
    };

    Mat2x2()
    {
        std::fill(data, data + 4, 0.0f);
    }
    Mat2x2(float v)
    {
        std::fill(data, data + 4, 0.0f);
        data[0] = data[3] = v;
    }
    Mat2x2(float m00, float m01, float m10, float m11)
        : m00(m00), m01(m01), m10(m10), m11(m11) {}

    float& operator[](int i) { return data[i]; }
    const float& operator[](int i) const { return data[i]; }

    Mat2x2 operator*(const Mat2x2& rhs) const
    {
        Mat2x2 result;
        result.data[0] = data[0] * rhs.data[0] + data[2] * rhs.data[1];
        result.data[1] = data[1] * rhs.data[0] + data[3] * rhs.data[1];
        result.data[2] = data[0] * rhs.data[2] + data[2] * rhs.data[3];
        result.data[3] = data[1] * rhs.data[2] + data[3] * rhs.data[3];
        return result;
    }

    Vec2 operator*(const Vec2& v) const
    {
        return Vec2(
            data[0] * v.x + data[2] * v.y,
            data[1] * v.x + data[3] * v.y);
    }

    Mat2x2 Inverse() const
    {
        Mat2x2 inv;
        float det = data[0] * data[3] - data[1] * data[2];
        if (det == 0.0f) {
            return Mat2x2(1.0f);
        }
        float invDet = 1.0f / det;
        inv.data[0] = data[3] * invDet;
        inv.data[1] = -data[1] * invDet;
        inv.data[2] = -data[2] * invDet;
        inv.data[3] = data[0] * invDet;
        return inv;
    }

    void Print(const char* name = "Mat2x2") const
    {
        EFFECT_LOGI("%{public}s:", name);
        EFFECT_LOGI("%{public}.6f %{public}.6f", data[0], data[1]);
        EFFECT_LOGI("%{public}.6f %{public}.6f", data[2], data[3]);
    }

    const float* Data() const { return data; }
};

struct Mat3x3 {
    union {
        float data[9];
        struct {
            float m00, m01, m02;
            float m10, m11, m12;
            float m20, m21, m22;
        };
    };

    Mat3x3()
    {
        std::fill(data, data + 9, 0.0f);
    }
    Mat3x3(float v)
    {
        std::fill(data, data + 9, 0.0f);
        data[0] = data[4] = data[8] = v;
    }
    Mat3x3(float m00, float m01, float m02,
           float m10, float m11, float m12,
           float m20, float m21, float m22)
        : m00(m00), m01(m01), m02(m02),
          m10(m10), m11(m11), m12(m12),
          m20(m20), m21(m21), m22(m22) {}

    float& operator[](int i) { return data[i]; }
    const float& operator[](int i) const { return data[i]; }

    Mat3x3 operator*(const Mat3x3& rhs) const
    {
        Mat3x3 result;
        for (int col = 0; col < 3; col++) {
            for (int row = 0; row < 3; row++) {
                result.data[col * 3 + row] =
                    data[0 * 3 + row] * rhs.data[col * 3 + 0] +
                    data[1 * 3 + row] * rhs.data[col * 3 + 1] +
                    data[2 * 3 + row] * rhs.data[col * 3 + 2];
            }
        }
        return result;
    }

    Vec3 operator*(const Vec3& v) const
    {
        return Vec3(
            data[0] * v.x + data[3] * v.y + data[6] * v.z,
            data[1] * v.x + data[4] * v.y + data[7] * v.z,
            data[2] * v.x + data[5] * v.y + data[8] * v.z);
    }

    Mat3x3 Inverse() const
    {
        Mat3x3 inv;
        float det = data[0] * (data[4] * data[8] - data[5] * data[7]) -
                    data[1] * (data[3] * data[8] - data[5] * data[6]) +
                    data[2] * (data[3] * data[7] - data[4] * data[6]);
        if (det == 0.0f) {
            return Mat3x3(1.0f);
        }
        float invDet = 1.0f / det;
        inv.data[0] = (data[4] * data[8] - data[5] * data[7]) * invDet;
        inv.data[1] = (data[2] * data[7] - data[1] * data[8]) * invDet;
        inv.data[2] = (data[1] * data[5] - data[2] * data[4]) * invDet;
        inv.data[3] = (data[5] * data[6] - data[3] * data[8]) * invDet;
        inv.data[4] = (data[0] * data[8] - data[2] * data[6]) * invDet;
        inv.data[5] = (data[2] * data[3] - data[0] * data[5]) * invDet;
        inv.data[6] = (data[3] * data[7] - data[4] * data[6]) * invDet;
        inv.data[7] = (data[1] * data[6] - data[0] * data[7]) * invDet;
        inv.data[8] = (data[0] * data[4] - data[1] * data[3]) * invDet;
        return inv;
    }

    void Print(const char* name = "Mat3x3") const
    {
        EFFECT_LOGI("%{public}s:", name);
        EFFECT_LOGI("%{public}.6f %{public}.6f %{public}.6f", data[0], data[1], data[2]);
        EFFECT_LOGI("%{public}.6f %{public}.6f %{public}.6f", data[3], data[4], data[5]);
        EFFECT_LOGI("%{public}.6f %{public}.6f %{public}.6f", data[6], data[7], data[8]);
    }

    const float* Data() const { return data; }
};

struct Mat4x4 {
    union {
        float data[16];
        struct {
            float m00, m01, m02, m03;
            float m10, m11, m12, m13;
            float m20, m21, m22, m23;
            float m30, m31, m32, m33;
        };
    };

    Mat4x4()
    {
        std::fill(data, data + 16, 0.0f);
    }

    Mat4x4(float v)
    {
        std::fill(data, data + 16, 0.0f);
        data[0] = data[5] = data[10] = data[15] = v;
    }

    Mat4x4(float m00, float m01, float m02, float m03,
           float m10, float m11, float m12, float m13,
           float m20, float m21, float m22, float m23,
           float m30, float m31, float m32, float m33)
        : m00(m00), m01(m01), m02(m02), m03(m03),
          m10(m10), m11(m11), m12(m12), m13(m13),
          m20(m20), m21(m21), m22(m22), m23(m23),
          m30(m30), m31(m31), m32(m32), m33(m33) {}

    float& operator[](int i) { return data[i]; }
    const float& operator[](int i) const { return data[i]; }

    Mat4x4 operator*(const Mat4x4& rhs) const
    {
        Mat4x4 result;
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                result.data[col * 4 + row] =
                    data[0 * 4 + row] * rhs.data[col * 4 + 0] +
                    data[1 * 4 + row] * rhs.data[col * 4 + 1] +
                    data[2 * 4 + row] * rhs.data[col * 4 + 2] +
                    data[3 * 4 + row] * rhs.data[col * 4 + 3];
            }
        }
        return result;
    }

    Vec4 operator*(const Vec4& v) const
    {
        return Vec4(
            data[0] * v.x + data[4] * v.y + data[8] * v.z + data[12] * v.w,
            data[1] * v.x + data[5] * v.y + data[9] * v.z + data[13] * v.w,
            data[2] * v.x + data[6] * v.y + data[10] * v.z + data[14] * v.w,
            data[3] * v.x + data[7] * v.y + data[11] * v.z + data[15] * v.w);
    }

    Mat4x4 Inverse() const
    {
        Mat4x4 inv;

        float a00 = data[0];
        float a01 = data[1];
        float a02 = data[2];
        float a03 = data[3];
        float a10 = data[4];
        float a11 = data[5];
        float a12 = data[6];
        float a13 = data[7];
        float a20 = data[8];
        float a21 = data[9];
        float a22 = data[10];
        float a23 = data[11];
        float a30 = data[12];
        float a31 = data[13];
        float a32 = data[14];
        float a33 = data[15];

        float b00 = a00 * a11 - a01 * a10;
        float b01 = a00 * a12 - a02 * a10;
        float b02 = a00 * a13 - a03 * a10;
        float b03 = a01 * a12 - a02 * a11;
        float b04 = a01 * a13 - a03 * a11;
        float b05 = a02 * a13 - a03 * a12;
        float b06 = a20 * a31 - a21 * a30;
        float b07 = a20 * a32 - a22 * a30;
        float b08 = a20 * a33 - a23 * a30;
        float b09 = a21 * a32 - a22 * a31;
        float b10 = a21 * a33 - a23 * a31;
        float b11 = a22 * a33 - a23 * a32;

        float det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
        CHECK_AND_RETURN_RET(det != 0.0f, Mat4x4(1.0f));
        float invDet = 1.0f / det;
        inv.data[0] = (a11 * b11 - a12 * b10 + a13 * b09) * invDet;
        inv.data[1] = (a02 * b10 - a01 * b11 - a03 * b09) * invDet;
        inv.data[2] = (a31 * b05 - a32 * b04 + a33 * b03) * invDet;
        inv.data[3] = (a22 * b04 - a21 * b05 - a23 * b03) * invDet;

        inv.data[4] = (a12 * b08 - a10 * b11 - a13 * b07) * invDet;
        inv.data[5] = (a00 * b11 - a02 * b08 + a03 * b07) * invDet;
        inv.data[6] = (a32 * b02 - a30 * b05 - a33 * b01) * invDet;
        inv.data[7] = (a20 * b05 - a22 * b02 + a23 * b01) * invDet;

        inv.data[8] = (a10 * b10 - a11 * b08 + a13 * b06) * invDet;
        inv.data[9] = (a01 * b08 - a00 * b10 - a03 * b06) * invDet;
        inv.data[10] = (a30 * b04 - a31 * b02 + a33 * b00) * invDet;
        inv.data[11] = (a21 * b02 - a20 * b04 - a23 * b00) * invDet;

        inv.data[12] = (a11 * b07 - a10 * b09 - a12 * b06) * invDet;
        inv.data[13] = (a00 * b09 - a01 * b07 + a02 * b06) * invDet;
        inv.data[14] = (a31 * b01 - a30 * b03 - a32 * b00) * invDet;
        inv.data[15] = (a20 * b03 - a21 * b01 + a22 * b00) * invDet;
        return inv;
    }

    void Print(const char* name = "Mat4x4") const
    {
        EFFECT_LOGI("%{public}s:", name);
        EFFECT_LOGI("%{public}.6f %{public}.6f %{public}.6f %{public}.6f", data[0], data[1], data[2], data[3]);
        EFFECT_LOGI("%{public}.6f %{public}.6f %{public}.6f %{public}.6f", data[4], data[5], data[6], data[7]);
        EFFECT_LOGI("%{public}.6f %{public}.6f %{public}.6f %{public}.6f", data[8], data[9], data[10], data[11]);
        EFFECT_LOGI("%{public}.6f %{public}.6f %{public}.6f %{public}.6f", data[12], data[13], data[14], data[15]);
    }

    const float* Data() const { return data; }
};

} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // RENDERMATRIX_H