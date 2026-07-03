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

#ifndef RENDERVECTOR_H
#define RENDERVECTOR_H

#include <cmath>
#include <algorithm>
#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {

struct Vec2 {
    union {
        struct { float x, y; };
        struct { float r, g; };
        struct { float s, t; };
        float data[2];
    };

    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float v) : x(v), y(v) {}
    Vec2(float x, float y) : x(x), y(y) {}

    float& operator[](int i) { return data[i]; }
    const float& operator[](int i) const { return data[i]; }

    Vec2 operator+(const Vec2& rhs) const {
        return Vec2(x + rhs.x, y + rhs.y);
    }

    Vec2 operator-(const Vec2& rhs) const {
        return Vec2(x - rhs.x, y - rhs.y);
    }

    Vec2 operator*(const Vec2& rhs) const {
        return Vec2(x * rhs.x, y * rhs.y);
    }

    Vec2 operator/(const Vec2& rhs) const {
        return Vec2(x / rhs.x, y / rhs.y);
    }

    Vec2& operator+=(const Vec2& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    Vec2& operator-=(const Vec2& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    Vec2& operator*=(const Vec2& rhs) {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    Vec2& operator/=(const Vec2& rhs) { 
        x /= rhs.x;
        y /= rhs.y;
        return *this;
    }

    Vec2 operator*(float s) const {
        return Vec2(x * s, y * s);
    }

    Vec2 operator/(float s) const {
        return Vec2(x / s, y / s);
    }

    bool operator==(const Vec2& rhs) const {
        return x == rhs.x && y == rhs.y;
    }

    bool operator!=(const Vec2& rhs) const {
        return !(*this == rhs);
    }

    float Dot(const Vec2& rhs) const {
        return x * rhs.x + y * rhs.y;
    }

    float Length() const {
        return std::sqrt(x * x + y * y);
    }

    Vec2 Normalize() const {
        float len = Length();
        if (len > 0.0f) {
            return Vec2(x / len, y / len);
        }
        return Vec2(0.0f);
    }

    void Print(const char* name = "Vec2") const
    {
        EFFECT_LOGI("%s: (%.6f, %.6f)", name, x, y);
    }

    const float* Data() const { return data; }
};

struct Vec3 {
    union {
        struct { float x, y, z; };
        struct { float r, g, b; };
        struct { float s, t, p; };
        float data[3];
    };

    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float v) : x(v), y(v), z(v) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    float& operator[](int i) { return data[i]; }
    const float& operator[](int i) const { return data[i]; }

    Vec3 operator+(const Vec3& rhs) const {
        return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    Vec3 operator-(const Vec3& rhs) const {
        return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    Vec3 operator*(float s) const {
        return Vec3(x * s, y * s, z * s);
    }

    Vec3 operator/(float s) const {
        return Vec3(x / s, y / s, z / s);
    }

    bool operator==(const Vec3& rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

    bool operator!=(const Vec3& rhs) const {
        return !(*this == rhs);
    }

    float Dot(const Vec3& rhs) const {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    Vec3 Cross(const Vec3& rhs) const {
        return Vec3 (
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        );
    }

    float Length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vec3 Normalize() const {
        float len = Length();
        if (len > 0.0f) {
            return Vec3(x / len, y / len, z / len);
        }
        return Vec3(0.0f);
    }

    void Print(const char* name = "Vec3") const
    {
        EFFECT_LOGI("%s: (%.6f, %.6f, %.6f)", name, x, y, z);
    }

    const float* Data() const { return data; }
};

struct Vec4 {
    union {
        struct { float x, y, z, w; };
        struct { float r, g, b, a; };
        struct { float s, t, p, q; };
        float data[4];
    };

    Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vec4(float v) : x(v), y(v), z(v), w(v) {}
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

    float& operator[](int i) { return data[i]; }
    const float& operator[](int i) const { return data[i]; }

    Vec4 operator+(const Vec4& rhs) const {
        return Vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); 
    }

    Vec4 operator-(const Vec4& rhs) const {
        return Vec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); 
    }

    Vec4 operator*(float s) const {
        return Vec4(x * s, y * s, z * s, w * s);
    }

    Vec4 operator/(float s) const {
        return Vec4(x / s, y / s, z / s, w / s);
    }

    bool operator==(const Vec4& rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
    }

    bool operator!=(const Vec4& rhs) const {
        return !(*this == rhs);
    }

    float Dot(const Vec4& rhs) const {
        return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
    }

    float Length() const {
        return std::sqrt(x * x + y * y + z * z + w * w);
    }

    Vec4 Normalize() const {
        float len = Length();
        if (len > 0.0f) {
            return Vec4(x / len, y / len, z / len, w / len);
        }
        return Vec4(0.0f);
    }

    void Print(const char* name = "Vec4") const
    {
        EFFECT_LOGI("%s: (%.6f, %.6f, %.6f, %.6f)", name, x, y, z, w);
    }

    const float* Data() const { return data; }
};

} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // RENDERVECTOR_H