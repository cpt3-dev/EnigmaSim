#include "camera.hpp"

namespace enigma::scene {
namespace {

constexpr float kDefaultYaw = 0.35f;
constexpr float kDefaultPitch = 0.42f;
constexpr float kDefaultDistance = 6.0f;
constexpr math::Vec3 kDefaultTarget = {0.0f, 0.35f, 0.0f};

constexpr float kOrbitSpeedRadPerPixel = 0.0055f;
constexpr float kPanScalePerPixel = 0.0016f;
constexpr float kZoomFactorPerNotch = 0.85f;
constexpr float kWheelDeltaPerNotch = 120.0f;
constexpr float kMinDistance = 2.2f;
constexpr float kMaxDistance = 14.0f;
constexpr float kMinPitch = -1.35f;
constexpr float kMaxPitch = 1.50f;
constexpr float kMaxTargetExtent = 4.0f;

constexpr math::Vec3 kWorldUp = {0.0f, 1.0f, 0.0f};

float clamp(float value, float minValue, float maxValue) {
    return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}

// Spherical direction from yaw/pitch; yaw 0 looks along +Z.
math::Vec3 direction(float yaw, float pitch) {
    const float cosPitch = std::cos(pitch);
    return {cosPitch * std::sin(yaw), std::sin(pitch),
            cosPitch * std::cos(yaw)};
}

}  // namespace

void OrbitCamera::resetToDefault() {
    target_ = kDefaultTarget;
    yaw_ = kDefaultYaw;
    pitch_ = kDefaultPitch;
    distance_ = kDefaultDistance;
    cutaway_ = false;
}

void OrbitCamera::applyPreset(ViewPreset preset) {
    cutaway_ = false;
    switch (preset) {
        case ViewPreset::Front:
            yaw_ = 0.0f;
            pitch_ = 0.30f;
            distance_ = 5.5f;
            break;
        case ViewPreset::Top:
            yaw_ = 0.0f;
            pitch_ = 1.35f;
            distance_ = 5.5f;
            break;
        case ViewPreset::Cutaway:
            yaw_ = 0.50f;
            pitch_ = 0.35f;
            distance_ = 5.0f;
            cutaway_ = true;
            break;
    }
    target_ = kDefaultTarget;
}

void OrbitCamera::orbit(float deltaXPixels, float deltaYPixels) {
    yaw_ -= deltaXPixels * kOrbitSpeedRadPerPixel;
    // Client-area Y grows downward; dragging up must raise the viewpoint.
    pitch_ = clamp(pitch_ - deltaYPixels * kOrbitSpeedRadPerPixel, kMinPitch,
                   kMaxPitch);
}

void OrbitCamera::pan(float deltaXPixels, float deltaYPixels) {
    const math::Vec3 eyePosition = eye();
    const math::Vec3 forward = math::normalize(target_ - eyePosition);
    const math::Vec3 right = math::normalize(math::cross(forward, kWorldUp));
    const math::Vec3 up = math::cross(right, forward);
    const float scale = distance_ * kPanScalePerPixel;
    target_ = target_ - right * (deltaXPixels * scale) +
              up * (deltaYPixels * scale);
    target_.x = clamp(target_.x, -kMaxTargetExtent, kMaxTargetExtent);
    target_.y = clamp(target_.y, -kMaxTargetExtent, kMaxTargetExtent);
    target_.z = clamp(target_.z, -kMaxTargetExtent, kMaxTargetExtent);
}

void OrbitCamera::zoom(float wheelDelta) {
    const float notches = wheelDelta / kWheelDeltaPerNotch;
    // Rolling forward (positive delta) moves the camera closer.
    distance_ = clamp(distance_ * std::pow(kZoomFactorPerNotch, notches),
                      kMinDistance, kMaxDistance);
}

math::Vec3 OrbitCamera::eye() const {
    return target_ + direction(yaw_, pitch_) * distance_;
}

math::Mat4 OrbitCamera::viewMatrix() const {
    return math::lookAt(eye(), target_, kWorldUp);
}

}  // namespace enigma::scene
