#pragma once
// Orbit camera: left-drag orbits, right-drag pans, mouse wheel zooms,
// double-click resets. Preset views (front / top / cutaway) are applied via
// applyPreset(); the cutaway preset additionally flags the casing to be
// rendered translucent.

#include "math.hpp"

namespace enigma::scene {

enum class ViewPreset { Front, Top, Cutaway };

class OrbitCamera {
public:
    OrbitCamera() { resetToDefault(); }

    void resetToDefault();
    void applyPreset(ViewPreset preset);

    // Pixel deltas from drag events.
    void orbit(float deltaXPixels, float deltaYPixels);
    void pan(float deltaXPixels, float deltaYPixels);
    // Raw wheel delta (multiples of 120 per notch).
    void zoom(float wheelDelta);

    [[nodiscard]] math::Mat4 viewMatrix() const;
    [[nodiscard]] math::Vec3 eye() const;
    [[nodiscard]] bool cutawayActive() const { return cutaway_; }

private:
    math::Vec3 target_{};
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float distance_ = 1.0f;
    bool cutaway_ = false;
};

}  // namespace enigma::scene
