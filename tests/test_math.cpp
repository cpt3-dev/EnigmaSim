// Unit tests for the hand-written math module (Vec3 / column-major Mat4).
// The renderer cannot be exercised visually in CI, so the matrix math that
// feeds the MVP uniforms is verified here instead.

#include "math.hpp"
#include "test_framework.hpp"

#include <cmath>
#include <cstdio>
#include <string>

namespace {
namespace math = enigma::math;

constexpr float kEpsilon = 1e-4f;

bool nearEqual(float actual, float expected, float tolerance = kEpsilon) {
    return std::fabs(actual - expected) <= tolerance;
}

bool nearEqual(const math::Vec3& actual, const math::Vec3& expected,
               float tolerance = kEpsilon) {
    return nearEqual(actual.x, expected.x, tolerance) &&
           nearEqual(actual.y, expected.y, tolerance) &&
           nearEqual(actual.z, expected.z, tolerance);
}

bool nearEqual(const math::Mat4& actual, const math::Mat4& expected,
               float tolerance = kEpsilon) {
    for (int i = 0; i < 16; ++i) {
        if (!nearEqual(actual.m[i], expected.m[i], tolerance)) {
            return false;
        }
    }
    return true;
}

}  // namespace

TEST(math_vec3_basics) {
    const math::Vec3 a{1.0f, 2.0f, 3.0f};
    const math::Vec3 b{4.0f, -5.0f, 6.0f};
    CHECK(nearEqual(a + b, {5.0f, -3.0f, 9.0f}));
    CHECK(nearEqual(b - a, {3.0f, -7.0f, 3.0f}));
    CHECK(nearEqual(a * 2.0f, {2.0f, 4.0f, 6.0f}));
    CHECK(nearEqual(math::dot(a, b), 4.0f - 10.0f + 18.0f));
    CHECK(nearEqual(math::cross(math::Vec3{1, 0, 0}, math::Vec3{0, 1, 0}),
                    {0.0f, 0.0f, 1.0f}));
    CHECK(nearEqual(math::length(math::Vec3{3.0f, 4.0f, 0.0f}), 5.0f));
    CHECK(nearEqual(math::normalize(math::Vec3{0.0f, 3.0f, 0.0f}),
                    {0.0f, 1.0f, 0.0f}));
    // Degenerate input must not produce NaN.
    CHECK(nearEqual(math::normalize(math::Vec3{0, 0, 0}), {0.0f, 0.0f, 0.0f}));
}

TEST(math_radians) {
    CHECK(nearEqual(math::radians(180.0f), math::kPi));
    CHECK(nearEqual(math::radians(90.0f), math::kPi * 0.5f));
    CHECK(nearEqual(math::radians(0.0f), 0.0f));
}

TEST(math_mat4_identity_and_multiply) {
    const math::Mat4 identity = math::identity();
    math::Mat4 sample;
    for (int i = 0; i < 16; ++i) {
        sample.m[i] = static_cast<float>(i + 1);
    }
    CHECK(nearEqual(math::multiply(identity, sample), sample));
    CHECK(nearEqual(math::multiply(sample, identity), sample));

    // Translation composed with itself doubles the offset.
    const math::Mat4 step = math::translation({1.0f, -2.0f, 3.0f});
    const math::Mat4 doubled = math::multiply(step, step);
    CHECK(nearEqual(doubled.at(0, 3), 2.0f));
    CHECK(nearEqual(doubled.at(1, 3), -4.0f));
    CHECK(nearEqual(doubled.at(2, 3), 6.0f));
}

TEST(math_mat4_transform_point) {
    const math::Mat4 model =
        math::translation({1.0f, 2.0f, 3.0f}) * math::rotationX(math::kPi * 0.5f);
    // rotationX(pi/2) sends +Y to +Z, then the translation is applied.
    const math::Vec3 moved = math::transformPoint(model, {0.0f, 1.0f, 0.0f});
    CHECK(nearEqual(moved, {1.0f, 2.0f, 4.0f}));
}

TEST(math_rotation_x_is_orthonormal) {
    const math::Mat4 rotation = math::rotationX(0.7f);
    const math::Mat4 product = math::multiply(rotation, math::invert(rotation));
    CHECK(nearEqual(product, math::identity(), 1e-3f));
}

TEST(math_perspective_shape) {
    const float fov = math::radians(90.0f);
    const math::Mat4 projection = math::perspective(fov, 2.0f, 1.0f, 100.0f);
    // fov 90 deg -> focal length 1; aspect 2 halves the horizontal term.
    CHECK(nearEqual(projection.at(1, 1), 1.0f));
    CHECK(nearEqual(projection.at(0, 0), 0.5f));
    CHECK(nearEqual(projection.at(3, 2), -1.0f));
    CHECK(nearEqual(projection.at(3, 3), 0.0f));

    // A point on -Z near plane maps to NDC z = -1 after the divide.
    const math::Vec3 nearPoint =
        math::transformPoint(projection, {0.0f, 0.0f, -1.0f});
    // transformPoint drops w; recompute w = -z_eye = 1 for this point.
    CHECK(nearEqual(nearPoint.z / 1.0f, -1.0f));
}

TEST(math_look_at_views_origin) {
    const math::Mat4 view = math::lookAt({0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, 0.0f},
                                         {0.0f, 1.0f, 0.0f});
    // The camera looks down -Z; the origin must land at view-space z = -5.
    const math::Vec3 originInView = math::transformPoint(view, {0, 0, 0});
    CHECK(nearEqual(originInView, {0.0f, 0.0f, -5.0f}));

    // View must be rigid: inverse equals the transpose rotation part.
    const math::Mat4 roundTrip = math::multiply(view, math::invert(view));
    CHECK(nearEqual(roundTrip, math::identity(), 1e-3f));
}

TEST(math_invert_affine) {
    const math::Mat4 model =
        math::translation({-2.0f, 0.5f, 4.0f}) * math::rotationX(1.1f);
    const math::Mat4 inverse = math::invert(model);
    CHECK(nearEqual(math::multiply(model, inverse), math::identity(), 1e-3f));

    const math::Vec3 point{1.5f, -1.0f, 2.0f};
    const math::Vec3 roundTrip =
        math::transformPoint(model, math::transformPoint(inverse, point));
    CHECK(nearEqual(roundTrip, point, 1e-3f));
}

int main() {
    return testfw::runAll();
}
