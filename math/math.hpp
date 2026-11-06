#pragma once
// Hand-written 3D math for the renderer: Vec3 and column-major Mat4.
// Pure C++20: this header must never include windows.h or any OpenGL header.
//
// Mat4 storage is column-major: element (row r, column c) lives at
// m[c * 4 + r]. This matches glUniformMatrix4fv with transpose = GL_FALSE.

#include <cmath>

namespace enigma::math {

constexpr float kPi = 3.14159265358979323846f;

[[nodiscard]] constexpr float radians(float degrees) {
    return degrees * (kPi / 180.0f);
}

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

[[nodiscard]] constexpr Vec3 operator+(const Vec3& a, const Vec3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

[[nodiscard]] constexpr Vec3 operator-(const Vec3& a, const Vec3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

[[nodiscard]] constexpr Vec3 operator*(const Vec3& v, float scale) {
    return {v.x * scale, v.y * scale, v.z * scale};
}

[[nodiscard]] constexpr Vec3 operator*(float scale, const Vec3& v) {
    return v * scale;
}

[[nodiscard]] constexpr Vec3 operator-(const Vec3& v) {
    return {-v.x, -v.y, -v.z};
}

[[nodiscard]] constexpr float dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

[[nodiscard]] constexpr Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

[[nodiscard]] inline float length(const Vec3& v) {
    return std::sqrt(dot(v, v));
}

[[nodiscard]] inline Vec3 normalize(const Vec3& v) {
    const float len = length(v);
    if (len <= 0.0f) {
        return {0.0f, 0.0f, 0.0f};
    }
    return v * (1.0f / len);
}

// Column-major 4x4 matrix.
struct Mat4 {
    float m[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    [[nodiscard]] float& at(int row, int col) { return m[col * 4 + row]; }
    [[nodiscard]] float at(int row, int col) const { return m[col * 4 + row]; }
};

[[nodiscard]] inline Mat4 identity() {
    return Mat4{};
}

// result = a * b (applies b first, then a).
[[nodiscard]] inline Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 result;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a.at(row, k) * b.at(k, col);
            }
            result.at(row, col) = sum;
        }
    }
    return result;
}

[[nodiscard]] inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    return multiply(a, b);
}

[[nodiscard]] inline Vec3 transformPoint(const Mat4& mat, const Vec3& p) {
    const float x = mat.at(0, 0) * p.x + mat.at(0, 1) * p.y +
                    mat.at(0, 2) * p.z + mat.at(0, 3);
    const float y = mat.at(1, 0) * p.x + mat.at(1, 1) * p.y +
                    mat.at(1, 2) * p.z + mat.at(1, 3);
    const float z = mat.at(2, 0) * p.x + mat.at(2, 1) * p.y +
                    mat.at(2, 2) * p.z + mat.at(2, 3);
    return {x, y, z};
}

[[nodiscard]] inline Mat4 translation(const Vec3& offset) {
    Mat4 result;
    result.at(0, 3) = offset.x;
    result.at(1, 3) = offset.y;
    result.at(2, 3) = offset.z;
    return result;
}

// Rotation around the X axis (the rotor axis in this project).
[[nodiscard]] inline Mat4 rotationX(float angleRadians) {
    const float c = std::cos(angleRadians);
    const float s = std::sin(angleRadians);
    Mat4 result;
    result.at(1, 1) = c;
    result.at(1, 2) = -s;
    result.at(2, 1) = s;
    result.at(2, 2) = c;
    return result;
}

// Standard OpenGL symmetric perspective projection (right-handed, depth
// mapped to [-1, 1]). fovYRadians is the vertical field of view.
[[nodiscard]] inline Mat4 perspective(float fovYRadians, float aspect,
                                      float zNear, float zFar) {
    const float focal = 1.0f / std::tan(fovYRadians * 0.5f);
    Mat4 result;
    for (float& value : result.m) {
        value = 0.0f;
    }
    result.at(0, 0) = focal / aspect;
    result.at(1, 1) = focal;
    result.at(2, 2) = (zFar + zNear) / (zNear - zFar);
    result.at(2, 3) = (2.0f * zFar * zNear) / (zNear - zFar);
    result.at(3, 2) = -1.0f;
    return result;
}

// Right-handed view matrix looking from eye at target.
[[nodiscard]] inline Mat4 lookAt(const Vec3& eye, const Vec3& target,
                                 const Vec3& up) {
    const Vec3 forward = normalize(target - eye);
    const Vec3 side = normalize(cross(forward, up));
    const Vec3 realUp = cross(side, forward);

    Mat4 result;
    result.at(0, 0) = side.x;
    result.at(0, 1) = side.y;
    result.at(0, 2) = side.z;
    result.at(0, 3) = -dot(side, eye);
    result.at(1, 0) = realUp.x;
    result.at(1, 1) = realUp.y;
    result.at(1, 2) = realUp.z;
    result.at(1, 3) = -dot(realUp, eye);
    result.at(2, 0) = -forward.x;
    result.at(2, 1) = -forward.y;
    result.at(2, 2) = -forward.z;
    result.at(2, 3) = dot(forward, eye);
    return result;
}

// Full 4x4 inverse via cofactor expansion. Returns identity() when the
// matrix is singular (callers only invert well-formed view/model matrices).
[[nodiscard]] inline Mat4 invert(const Mat4& mat) {
    const float* a = mat.m;
    // Pairs of rows for the 2x2 sub-determinants (s0..s5 from rows 0/1,
    // c0..c5 from rows 2/3), following the standard cofactor formulation.
    const float s0 = a[0] * a[5] - a[4] * a[1];
    const float s1 = a[0] * a[9] - a[8] * a[1];
    const float s2 = a[0] * a[13] - a[12] * a[1];
    const float s3 = a[4] * a[9] - a[8] * a[5];
    const float s4 = a[4] * a[13] - a[12] * a[5];
    const float s5 = a[8] * a[13] - a[12] * a[9];

    const float c5 = a[10] * a[15] - a[14] * a[11];
    const float c4 = a[6] * a[15] - a[14] * a[7];
    const float c3 = a[6] * a[11] - a[10] * a[7];
    const float c2 = a[2] * a[15] - a[14] * a[3];
    const float c1 = a[2] * a[11] - a[10] * a[3];
    const float c0 = a[2] * a[7] - a[6] * a[3];

    const float determinant = s0 * c5 - s1 * c4 + s2 * c3 +
                              s3 * c2 - s4 * c1 + s5 * c0;
    if (determinant == 0.0f) {
        return identity();
    }
    const float invDet = 1.0f / determinant;

    Mat4 result;
    float* r = result.m;
    r[0] = (a[5] * c5 - a[9] * c4 + a[13] * c3) * invDet;
    r[1] = (-a[1] * c5 + a[9] * c2 - a[13] * c1) * invDet;
    r[2] = (a[1] * c4 - a[5] * c2 + a[13] * c0) * invDet;
    r[3] = (-a[1] * c3 + a[5] * c1 - a[9] * c0) * invDet;

    r[4] = (-a[4] * c5 + a[8] * c4 - a[12] * c3) * invDet;
    r[5] = (a[0] * c5 - a[8] * c2 + a[12] * c1) * invDet;
    r[6] = (-a[0] * c4 + a[4] * c2 - a[12] * c0) * invDet;
    r[7] = (a[0] * c3 - a[4] * c1 + a[8] * c0) * invDet;

    r[8] = (a[7] * s5 - a[11] * s4 + a[15] * s3) * invDet;
    r[9] = (-a[3] * s5 + a[11] * s2 - a[15] * s1) * invDet;
    r[10] = (a[3] * s4 - a[7] * s2 + a[15] * s0) * invDet;
    r[11] = (-a[3] * s3 + a[7] * s1 - a[11] * s0) * invDet;

    r[12] = (-a[6] * s5 + a[10] * s4 - a[14] * s3) * invDet;
    r[13] = (a[2] * s5 - a[10] * s2 + a[14] * s1) * invDet;
    r[14] = (-a[2] * s4 + a[6] * s2 - a[14] * s0) * invDet;
    r[15] = (a[2] * s3 - a[6] * s1 + a[10] * s0) * invDet;
    return result;
}

}  // namespace enigma::math
