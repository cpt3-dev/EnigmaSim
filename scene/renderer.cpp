// Blinn-Phong renderer for the procedural Enigma scene. Shaders are
// embedded string constants; no external asset files.

#include "renderer.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace enigma::scene {
namespace {

// Mesh slots inside EnigmaRenderer::meshes_.
constexpr int kMeshRotorCore = 0;
constexpr int kMeshRotorCollar = 1;
constexpr int kMeshReflectorDisc = 2;
constexpr int kMeshReflectorCover = 3;
constexpr int kMeshLampPanel = 4;
constexpr int kMeshLamp = 5;
constexpr int kMeshPlugPanel = 6;
constexpr int kMeshPlugSocket = 7;
constexpr int kMeshCasing = 8;
constexpr int kMeshCount = 9;

// -- Geometry constants ------------------------------------------------------
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr float kAngleStep = kTwoPi / static_cast<float>(core::kAlphabetSize);

constexpr float kRotorRadius = 0.42f;
constexpr float kRotorLength = 0.46f;
constexpr float kRotorSpacing = 0.62f;
constexpr int kRotorDivisions = core::kAlphabetSize;  // 26

constexpr float kCollarRadius = 0.52f;
constexpr float kCollarLength = 0.16f;
constexpr float kCollarOffsetX = 0.16f;  // collar sits right of each rotor core
constexpr float kCollarTabSizeX = 0.14f;
constexpr float kCollarTabSizeY = 0.10f;
constexpr float kCollarTabSizeZ = 0.18f;

constexpr float kReflectorRadius = 0.50f;
constexpr float kReflectorLength = 0.22f;
constexpr float kReflectorGap = 0.38f;  // gap between reflector and left rotor
constexpr float kReflectorCoverSizeX = 0.18f;
constexpr float kReflectorCoverSizeY = 0.95f;
constexpr float kReflectorCoverSizeZ = 0.95f;

constexpr float kCasingHeight = 1.6f;
constexpr float kCasingDepth = 2.6f;
constexpr float kCasingSidePadding = 0.95f;
constexpr float kCasingTopY = kCasingHeight * 0.5f;
constexpr float kRotorAxisY = kCasingTopY;  // rotors half-sunk into the casing
constexpr float kRotorRowZ = -0.62f;

constexpr float kLampPanelWidth = 2.5f;
constexpr float kLampPanelThickness = 0.1f;
constexpr float kLampPanelDepth = 1.05f;
constexpr float kLampPanelY = kCasingTopY + 0.04f;
constexpr float kLampPanelZ = 0.60f;
constexpr float kLampPanelSlopeRad = math::radians(26.0f);
constexpr float kLampRadius = 0.075f;
constexpr float kLampLength = 0.07f;
constexpr int kLampSegments = 16;
constexpr float kLampSpacing = 0.24f;
constexpr float kLampRowSpacing = 0.26f;
constexpr float kLampSurfaceOffset =
    kLampPanelThickness * 0.5f + kLampLength * 0.5f - 0.015f;

constexpr float kPlugPanelWidth = 2.95f;
constexpr float kPlugPanelHeight = 0.55f;
constexpr float kPlugPanelThickness = 0.08f;
constexpr float kPlugPanelY = -0.30f;
constexpr float kPlugPanelZ =
    kCasingDepth * 0.5f + kPlugPanelThickness * 0.5f - 0.02f;
constexpr float kSocketRadius = 0.055f;
constexpr float kSocketLength = 0.10f;
constexpr int kSocketSegments = 12;
constexpr float kPlugSpacing = 0.21f;
constexpr float kPlugRowOffset = 0.13f;
constexpr float kSocketSurfaceOffset =
    kPlugPanelThickness * 0.5f + kSocketLength * 0.5f - 0.02f;

constexpr float kCutawayAlpha = 0.35f;
constexpr float kRotorEaseRate = 14.0f;  // 1/s exponential approach

// Historical lampboard layout, top row first.
constexpr char kLampRowTop[] = "QWERTZUIO";
constexpr char kLampRowMiddle[] = "ASDFGHJK";
constexpr char kLampRowBottom[] = "PYXCVBNML";

// -- Materials ---------------------------------------------------------------
constexpr Material kBakeliteMaterial = {
    {0.16f, 0.11f, 0.08f}, {0.30f, 0.28f, 0.26f}, 24.0f,
    {0.0f, 0.0f, 0.0f},    1.0f};
constexpr Material kMetalMaterial = {
    {0.60f, 0.62f, 0.64f}, {0.90f, 0.90f, 0.90f}, 64.0f,
    {0.0f, 0.0f, 0.0f},    1.0f};
constexpr Material kRotorMetalMaterial = {
    {0.50f, 0.52f, 0.55f}, {0.85f, 0.85f, 0.85f}, 48.0f,
    {0.0f, 0.0f, 0.0f},    1.0f};
constexpr Material kCollarMaterial = {
    {0.10f, 0.10f, 0.11f}, {0.55f, 0.55f, 0.55f}, 40.0f,
    {0.0f, 0.0f, 0.0f},    1.0f};
constexpr Material kPanelMaterial = {
    {0.22f, 0.15f, 0.10f}, {0.25f, 0.22f, 0.20f}, 20.0f,
    {0.0f, 0.0f, 0.0f},    1.0f};
// Emissive stays zero until a future input pipeline lights the pressed key.
constexpr Material kLampMaterial = {
    {0.30f, 0.27f, 0.16f}, {0.90f, 0.90f, 0.85f}, 80.0f,
    {0.0f, 0.0f, 0.0f},    1.0f};
constexpr Material kSocketMaterial = {
    {0.07f, 0.07f, 0.08f}, {0.40f, 0.40f, 0.40f}, 32.0f,
    {0.0f, 0.0f, 0.0f},    1.0f};

// -- Lighting ----------------------------------------------------------------
const math::Vec3 kLightDirection =
    math::normalize(math::Vec3{-0.40f, -0.75f, -0.50f});
constexpr float kLightColor[3] = {0.95f, 0.92f, 0.86f};
constexpr float kAmbientColor[3] = {0.22f, 0.21f, 0.20f};

// -- Embedded GLSL 330 core shaders -------------------------------------------
const char* kVertexShaderSource = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
out vec3 vWorldPosition;
out vec3 vNormal;
void main() {
    const vec4 world = uModel * vec4(aPosition, 1.0);
    vWorldPosition = world.xyz;
    vNormal = mat3(uModel) * aNormal;
    gl_Position = uProjection * uView * world;
})";

const char* kFragmentShaderSource = R"(#version 330 core
in vec3 vWorldPosition;
in vec3 vNormal;
uniform vec3 uCameraPosition;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uDiffuseColor;
uniform vec3 uSpecularColor;
uniform float uShininess;
uniform vec3 uEmissiveColor;
uniform float uAlpha;
out vec4 fragColor;
void main() {
    const vec3 normal = normalize(vNormal);
    const vec3 toLight = normalize(-uLightDirection);
    const float lambert = max(dot(normal, toLight), 0.0);
    const vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);
    const vec3 halfDirection = normalize(toLight + viewDirection);
    const float blinn = pow(max(dot(normal, halfDirection), 0.0), uShininess);
    const vec3 color = uAmbientColor * uDiffuseColor
                     + uLightColor * uDiffuseColor * lambert
                     + uLightColor * uSpecularColor * blinn
                     + uEmissiveColor;
    fragColor = vec4(color, uAlpha);
})";

float angleOfLetter(char letter) {
    return static_cast<float>(letter - core::kFirstLetter) * kAngleStep;
}

// Wraps an angle difference to [-pi, pi] so easing takes the short way.
float shortestAngle(float difference) {
    float wrapped = std::fmod(difference, kTwoPi);
    if (wrapped > math::kPi) {
        wrapped -= kTwoPi;
    } else if (wrapped < -math::kPi) {
        wrapped += kTwoPi;
    }
    return wrapped;
}

void fillRowPositions(std::array<math::Vec3, core::kAlphabetSize>& targets,
                      int& nextSlot, const char* row, float z, float y) {
    const int count = static_cast<int>(std::strlen(row));
    for (int i = 0; i < count; ++i) {
        const float x =
            (static_cast<float>(i) - static_cast<float>(count - 1) * 0.5f) *
            kLampSpacing;
        targets[static_cast<std::size_t>(nextSlot++)] = {x, y, z};
    }
}

}  // namespace

GpuMesh::GpuMesh(const MeshData& data)
    : indexCount_(static_cast<gl::GLsizei>(data.indices.size())) {
    vertexBuffer_.upload(
        static_cast<gl::GLsizeiptr>(data.vertices.size() * sizeof(float)),
        data.vertices.data());
    indexBuffer_.uploadElements(
        static_cast<gl::GLsizeiptr>(data.indices.size() *
                                    sizeof(std::uint32_t)),
        data.indices.data());
    vertexArray_.bind();
    vertexBuffer_.bind();
    vertexArray_.bindAttribute(0, kPositionComponents, kVertexStrideBytes, 0);
    vertexArray_.bindAttribute(1, kNormalComponents, kVertexStrideBytes,
                               static_cast<std::size_t>(kNormalOffsetBytes));
    vertexArray_.bindIndexBuffer(indexBuffer_.id());
    gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
}

void GpuMesh::draw() const {
    vertexArray_.bind();
    gl::glDrawElements(gl::GL_TRIANGLES, indexCount_, gl::GL_UNSIGNED_INT,
                       nullptr);
}

bool EnigmaRenderer::initialize(std::string& errorLog) {
    if (!program_.build(kVertexShaderSource, kFragmentShaderSource)) {
        errorLog = program_.log();
        return false;
    }
    uniforms_.model = program_.uniform("uModel");
    uniforms_.view = program_.uniform("uView");
    uniforms_.projection = program_.uniform("uProjection");
    uniforms_.cameraPosition = program_.uniform("uCameraPosition");
    uniforms_.lightDirection = program_.uniform("uLightDirection");
    uniforms_.lightColor = program_.uniform("uLightColor");
    uniforms_.ambientColor = program_.uniform("uAmbientColor");
    uniforms_.diffuseColor = program_.uniform("uDiffuseColor");
    uniforms_.specularColor = program_.uniform("uSpecularColor");
    uniforms_.shininess = program_.uniform("uShininess");
    uniforms_.emissiveColor = program_.uniform("uEmissiveColor");
    uniforms_.alpha = program_.uniform("uAlpha");

    buildMeshes(rotorCount_);

    lampPanelModel_ = math::translation({0.0f, kLampPanelY, kLampPanelZ}) *
                      math::rotationX(kLampPanelSlopeRad);
    plugPanelModel_ = math::translation({0.0f, kPlugPanelY, kPlugPanelZ});

    int lampSlot = 0;
    fillRowPositions(lampLocal_, lampSlot, kLampRowTop, -kLampRowSpacing,
                     kLampSurfaceOffset);
    fillRowPositions(lampLocal_, lampSlot, kLampRowMiddle, 0.0f,
                     kLampSurfaceOffset);
    fillRowPositions(lampLocal_, lampSlot, kLampRowBottom, kLampRowSpacing,
                     kLampSurfaceOffset);

    const int socketsPerRow = core::kAlphabetSize / 2;
    for (int i = 0; i < socketsPerRow; ++i) {
        const float x = (static_cast<float>(i) -
                         static_cast<float>(socketsPerRow - 1) * 0.5f) *
                        kPlugSpacing;
        socketLocal_[static_cast<std::size_t>(i)] = {x, kPlugRowOffset,
                                                     kSocketSurfaceOffset};
        socketLocal_[static_cast<std::size_t>(socketsPerRow + i)] = {
            x, -kPlugRowOffset, kSocketSurfaceOffset};
    }
    return true;
}

void EnigmaRenderer::buildMeshes(int rotorCount) {
    meshes_.clear();
    meshes_.reserve(kMeshCount);

    meshes_.push_back(
        GpuMesh(makeCylinder(kRotorDivisions, kRotorRadius, kRotorLength,
                             Axis::X)));

    MeshData collar =
        makeCylinder(kRotorDivisions, kCollarRadius, kCollarLength, Axis::X);
    // Index tab on the rim makes the Ringstellung rotation visible.
    appendMesh(collar, makeBox(kCollarTabSizeX, kCollarTabSizeY,
                               kCollarTabSizeZ),
               0.0f, kCollarRadius + kCollarTabSizeY * 0.5f, 0.0f);
    meshes_.push_back(GpuMesh(collar));

    meshes_.push_back(
        GpuMesh(makeCylinder(kRotorDivisions, kReflectorRadius,
                             kReflectorLength, Axis::X)));
    meshes_.push_back(GpuMesh(makeBox(kReflectorCoverSizeX,
                                      kReflectorCoverSizeY,
                                      kReflectorCoverSizeZ)));

    meshes_.push_back(GpuMesh(makeBox(kLampPanelWidth, kLampPanelThickness,
                                      kLampPanelDepth)));
    meshes_.push_back(GpuMesh(makeCylinder(kLampSegments, kLampRadius,
                                           kLampLength, Axis::Y)));

    meshes_.push_back(GpuMesh(makeBox(kPlugPanelWidth, kPlugPanelHeight,
                                      kPlugPanelThickness)));
    meshes_.push_back(GpuMesh(makeCylinder(kSocketSegments, kSocketRadius,
                                           kSocketLength, Axis::Z)));

    casingWidth_ = kRotorSpacing * static_cast<float>(rotorCount - 1) +
                   2.0f * kCasingSidePadding;
    meshes_.push_back(
        GpuMesh(makeBox(casingWidth_, kCasingHeight, kCasingDepth)));
}

void EnigmaRenderer::applyConfig(const core::MachineConfig& config) {
    rotorCount_ = core::rotorCountFor(config.type);
    for (int i = 0; i < rotorCount_; ++i) {
        slotX_[static_cast<std::size_t>(i)] =
            (static_cast<float>(i) -
             static_cast<float>(rotorCount_ - 1) * 0.5f) *
            kRotorSpacing;
    }
    reflectorX_ = slotX_[0] - kReflectorGap;

    const float width = kRotorSpacing * static_cast<float>(rotorCount_ - 1) +
                        2.0f * kCasingSidePadding;
    if (std::fabs(width - casingWidth_) > 1e-4f) {
        // Only the casing depends on the rotor count; rebuild that mesh
        // alone. This happens on config switches, never per frame.
        casingWidth_ = width;
        meshes_[kMeshCasing] =
            GpuMesh(makeBox(casingWidth_, kCasingHeight, kCasingDepth));
    }

    for (int i = 0; i < rotorCount_; ++i) {
        const std::size_t slot = static_cast<std::size_t>(i);
        ringOffsetAngle_[slot] = angleOfLetter(config.rotors[i].ring);
        const float coreAngle = angleOfLetter(config.rotors[i].position);
        currentCoreAngle_[slot] = coreAngle;
        targetCoreAngle_[slot] = coreAngle;
        currentCollarAngle_[slot] = coreAngle + ringOffsetAngle_[slot];
        targetCollarAngle_[slot] = coreAngle + ringOffsetAngle_[slot];
    }
    lastKeypressCount_ = 0;
}

void EnigmaRenderer::render(float deltaSeconds, const core::Machine& machine,
                            const math::Mat4& view,
                            const math::Mat4& projection,
                            const math::Vec3& cameraPosition, bool cutaway) {
    // Positions only change on keypresses; rebuild targets only then so the
    // steady-state frame performs no heap allocations.
    if (machine.keypressCount() != lastKeypressCount_) {
        lastKeypressCount_ = machine.keypressCount();
        const std::string positions = machine.positions();
        for (int i = 0; i < rotorCount_ && i < core::kMaxRotors; ++i) {
            const std::size_t slot = static_cast<std::size_t>(i);
            const float coreAngle =
                angleOfLetter(positions[static_cast<std::size_t>(i)]);
            targetCoreAngle_[slot] = coreAngle;
            targetCollarAngle_[slot] = coreAngle + ringOffsetAngle_[slot];
        }
    }

    const float ease = std::min(1.0f, deltaSeconds * kRotorEaseRate);
    for (int i = 0; i < rotorCount_; ++i) {
        const std::size_t slot = static_cast<std::size_t>(i);
        currentCoreAngle_[slot] +=
            shortestAngle(targetCoreAngle_[slot] - currentCoreAngle_[slot]) *
            ease;
        currentCollarAngle_[slot] +=
            shortestAngle(targetCollarAngle_[slot] -
                          currentCollarAngle_[slot]) *
            ease;
    }

    program_.use();
    program_.setMatrix4(uniforms_.view, view.m);
    program_.setMatrix4(uniforms_.projection, projection.m);
    program_.setVec3(uniforms_.cameraPosition, cameraPosition.x,
                     cameraPosition.y, cameraPosition.z);
    program_.setVec3(uniforms_.lightDirection, kLightDirection.x,
                     kLightDirection.y, kLightDirection.z);
    program_.setVec3(uniforms_.lightColor, kLightColor[0], kLightColor[1],
                     kLightColor[2]);
    program_.setVec3(uniforms_.ambientColor, kAmbientColor[0],
                     kAmbientColor[1], kAmbientColor[2]);

    gl::glEnable(gl::GL_DEPTH_TEST);
    gl::glEnable(gl::GL_CULL_FACE);
    gl::glDisable(gl::GL_BLEND);
    gl::glDepthMask(gl::GL_TRUE_B);

    // Rotor cores and independently rotated Ringstellung collars.
    for (int i = 0; i < rotorCount_; ++i) {
        const std::size_t slot = static_cast<std::size_t>(i);
        const math::Mat4 coreModel =
            math::translation({slotX_[slot], kRotorAxisY, kRotorRowZ}) *
            math::rotationX(currentCoreAngle_[slot]);
        drawPart(kMeshRotorCore, coreModel, kRotorMetalMaterial);

        const math::Mat4 collarModel =
            math::translation(
                {slotX_[slot] + kCollarOffsetX, kRotorAxisY, kRotorRowZ}) *
            math::rotationX(currentCollarAngle_[slot]);
        drawPart(kMeshRotorCollar, collarModel, kCollarMaterial);
    }

    // Reflector at the left end of the rotor row.
    drawPart(kMeshReflectorDisc,
             math::translation({reflectorX_, kRotorAxisY, kRotorRowZ}),
             kMetalMaterial);
    const float coverX = reflectorX_ - (kReflectorLength +
                                        kReflectorCoverSizeX) * 0.5f;
    drawPart(kMeshReflectorCover,
             math::translation({coverX, kRotorAxisY, kRotorRowZ}),
             kBakeliteMaterial);

    // Lampboard on the sloped front deck.
    drawPart(kMeshLampPanel, lampPanelModel_, kPanelMaterial);
    for (int i = 0; i < core::kAlphabetSize; ++i) {
        const std::size_t lamp = static_cast<std::size_t>(i);
        drawPart(kMeshLamp,
                 lampPanelModel_ * math::translation(lampLocal_[lamp]),
                 kLampMaterial);
    }

    // Plugboard on the front face.
    drawPart(kMeshPlugPanel, plugPanelModel_, kPanelMaterial);
    for (int i = 0; i < core::kAlphabetSize; ++i) {
        const std::size_t socket = static_cast<std::size_t>(i);
        drawPart(kMeshPlugSocket,
                 plugPanelModel_ * math::translation(socketLocal_[socket]),
                 kSocketMaterial);
    }

    // Casing last; in cutaway mode it becomes translucent so the rotor
    // halves and reflector inside the body stay visible.
    if (cutaway) {
        Material casingMaterial = kBakeliteMaterial;
        casingMaterial.alpha = kCutawayAlpha;
        gl::glEnable(gl::GL_BLEND);
        gl::glBlendFunc(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA);
        gl::glDepthMask(gl::GL_FALSE);
        gl::glDisable(gl::GL_CULL_FACE);
        drawPart(kMeshCasing, math::identity(), casingMaterial);
        gl::glDepthMask(gl::GL_TRUE_B);
        gl::glDisable(gl::GL_BLEND);
        gl::glEnable(gl::GL_CULL_FACE);
    } else {
        drawPart(kMeshCasing, math::identity(), kBakeliteMaterial);
    }
}

void EnigmaRenderer::drawPart(int meshIndex, const math::Mat4& model,
                              const Material& material) {
    program_.setMatrix4(uniforms_.model, model.m);
    program_.setVec3(uniforms_.diffuseColor, material.diffuse[0],
                     material.diffuse[1], material.diffuse[2]);
    program_.setVec3(uniforms_.specularColor, material.specular[0],
                     material.specular[1], material.specular[2]);
    program_.setFloat(uniforms_.shininess, material.shininess);
    program_.setVec3(uniforms_.emissiveColor, material.emissive[0],
                     material.emissive[1], material.emissive[2]);
    program_.setFloat(uniforms_.alpha, material.alpha);
    meshes_[static_cast<std::size_t>(meshIndex)].draw();
}

}  // namespace enigma::scene
