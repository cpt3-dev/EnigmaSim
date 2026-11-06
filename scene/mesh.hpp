#pragma once
// Procedural indexed mesh generation for the Enigma scene. All geometry is
// generated in source; no external asset files. Vertex layout is
// interleaved position (3 floats) + normal (3 floats).

#include <cstdint>
#include <vector>

namespace enigma::scene {

constexpr int kPositionComponents = 3;
constexpr int kNormalComponents = 3;
constexpr int kVertexFloats = kPositionComponents + kNormalComponents;
constexpr int kVertexStrideBytes =
    kVertexFloats * static_cast<int>(sizeof(float));
constexpr int kNormalOffsetBytes =
    kPositionComponents * static_cast<int>(sizeof(float));

enum class Axis { X, Y, Z };

struct MeshData {
    std::vector<float> vertices;    // interleaved position + normal
    std::vector<uint32_t> indices;  // triangle list
};

// Axis-aligned box centered at the origin, 24 vertices / 36 indices.
[[nodiscard]] MeshData makeBox(float sizeX, float sizeY, float sizeZ);

// Cylinder centered at the origin along the given axis, with both end caps.
// radialSegments equals the letter count (26) for rotor parts.
[[nodiscard]] MeshData makeCylinder(int radialSegments, float radius,
                                    float length, Axis axis);

// Appends src to destination, translating positions by the given offset.
// Used to compose multi-part meshes (e.g. collar cylinder + index tab).
void appendMesh(MeshData& destination, const MeshData& source, float offsetX,
                float offsetY, float offsetZ);

}  // namespace enigma::scene
