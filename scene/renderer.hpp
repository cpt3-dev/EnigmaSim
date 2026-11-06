#pragma once
// Scene renderer for the Enigma machine. Builds all geometry once as
// persistent indexed VAO/VBO/EBO meshes and redraws them every frame with
// per-part model matrices. The visuals are driven entirely by
// core::MachineConfig (rotor choices, ring settings, positions).
//
// Rotor letter engravings and the keyboard are not implemented yet: they
// depend on a text-rendering module.

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "enigma.hpp"
#include "gl_objects.hpp"
#include "math.hpp"
#include "mesh.hpp"

namespace enigma::scene {

struct Material {
    float diffuse[3];
    float specular[3];
    float shininess;
    float emissive[3];
    float alpha;
};

// One uploaded mesh: vertex buffer + index buffer + VAO, built once.
class GpuMesh {
public:
    explicit GpuMesh(const MeshData& data);
    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;
    GpuMesh(GpuMesh&&) noexcept = default;
    GpuMesh& operator=(GpuMesh&&) noexcept = default;

    void draw() const;

private:
    gl::Buffer vertexBuffer_;
    gl::Buffer indexBuffer_;
    gl::VertexArray vertexArray_;
    gl::GLsizei indexCount_ = 0;
};

class EnigmaRenderer {
public:
    // Requires a current OpenGL 3.3 Core context with gl::load() completed.
    // Returns false and fills errorLog on shader or upload failure.
    bool initialize(std::string& errorLog);

    // Rebuilds configuration-dependent layout (rotor count, casing width)
    // and snaps the rotor animation to the new ground positions.
    void applyConfig(const core::MachineConfig& config);

    // Advances the rotor-angle easing and draws the machine. No heap
    // allocation happens per frame.
    void render(float deltaSeconds, const core::Machine& machine,
                const math::Mat4& view, const math::Mat4& projection,
                const math::Vec3& cameraPosition, bool cutaway);

private:
    struct Uniforms {
        gl::GLint model = -1;
        gl::GLint view = -1;
        gl::GLint projection = -1;
        gl::GLint cameraPosition = -1;
        gl::GLint lightDirection = -1;
        gl::GLint lightColor = -1;
        gl::GLint ambientColor = -1;
        gl::GLint diffuseColor = -1;
        gl::GLint specularColor = -1;
        gl::GLint shininess = -1;
        gl::GLint emissiveColor = -1;
        gl::GLint alpha = -1;
    };

    void buildMeshes(int rotorCount);
    void drawPart(int meshIndex, const math::Mat4& model,
                  const Material& material);

    gl::ShaderProgram program_;
    Uniforms uniforms_;
    std::vector<GpuMesh> meshes_;

    int rotorCount_ = core::kMinRotors;
    std::array<float, core::kMaxRotors> slotX_{};
    float casingWidth_ = 0.0f;
    float reflectorX_ = 0.0f;

    std::array<float, core::kMaxRotors> currentCoreAngle_{};
    std::array<float, core::kMaxRotors> currentCollarAngle_{};
    std::array<float, core::kMaxRotors> targetCoreAngle_{};
    std::array<float, core::kMaxRotors> targetCollarAngle_{};
    std::array<float, core::kMaxRotors> ringOffsetAngle_{};
    int lastKeypressCount_ = 0;

    math::Mat4 lampPanelModel_;
    math::Mat4 plugPanelModel_;
    std::array<math::Vec3, core::kAlphabetSize> lampLocal_{};
    std::array<math::Vec3, core::kAlphabetSize> socketLocal_{};
};

}  // namespace enigma::scene
