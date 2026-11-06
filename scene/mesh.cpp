// Procedural mesh builders. Winding is counter-clockwise front-facing with
// backface culling enabled; every builder emits outward-facing normals.

#include "mesh.hpp"

#include <cmath>

namespace enigma::scene {
namespace {

constexpr float kTwoPi = 6.28318530717958647692f;

struct FaceDefinition {
    float normal[3];
    float tangent[3];
};

// For each face the bitangent is cross(normal, tangent); the corner order
// (-t-b, +t-b, +t+b, -t+b) then yields the outward normal via the
// cross-product rule cross(t, cross(n, t)) == n for orthonormal n, t.
constexpr FaceDefinition kBoxFaces[6] = {
    {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
};

void pushVertex(MeshData& mesh, float px, float py, float pz, float nx,
                float ny, float nz) {
    mesh.vertices.push_back(px);
    mesh.vertices.push_back(py);
    mesh.vertices.push_back(pz);
    mesh.vertices.push_back(nx);
    mesh.vertices.push_back(ny);
    mesh.vertices.push_back(nz);
}

// Cyclic permutation (x, y, z) -> (z, x, y): determinant +1, so it maps the
// canonical X axis onto Y (applied twice: onto Z) without flipping winding.
void permuteCyclic(float& x, float& y, float& z) {
    const float oldX = x;
    const float oldY = y;
    x = z;
    y = oldX;
    z = oldY;
}

}  // namespace

MeshData makeBox(float sizeX, float sizeY, float sizeZ) {
    MeshData mesh;
    mesh.vertices.reserve(24 * kVertexFloats);
    mesh.indices.reserve(36);

    const float half[3] = {sizeX * 0.5f, sizeY * 0.5f, sizeZ * 0.5f};
    for (const FaceDefinition& face : kBoxFaces) {
        const float* n = face.normal;
        const float* t = face.tangent;
        const float b[3] = {
            n[1] * t[2] - n[2] * t[1],
            n[2] * t[0] - n[0] * t[2],
            n[0] * t[1] - n[1] * t[0],
        };
        // Half extent along the face normal selects the box dimension.
        float normalExtent = half[0];
        if (n[1] != 0.0f) {
            normalExtent = half[1];
        } else if (n[2] != 0.0f) {
            normalExtent = half[2];
        }
        float tangentExtent = half[0];
        if (t[1] != 0.0f) {
            tangentExtent = half[1];
        } else if (t[2] != 0.0f) {
            tangentExtent = half[2];
        }
        // Bitangent is always the remaining axis.
        float bitangentExtent = half[0];
        if (b[1] != 0.0f) {
            bitangentExtent = half[1];
        } else if (b[2] != 0.0f) {
            bitangentExtent = half[2];
        }

        const uint32_t base =
            static_cast<uint32_t>(mesh.vertices.size() / kVertexFloats);
        const float cornerSigns[4][2] = {{-1.0f, -1.0f}, {1.0f, -1.0f},
                                         {1.0f, 1.0f}, {-1.0f, 1.0f}};
        for (const auto& signs : cornerSigns) {
            const float su = signs[0] * tangentExtent;
            const float sv = signs[1] * bitangentExtent;
            pushVertex(mesh, n[0] * normalExtent + t[0] * su + b[0] * sv,
                       n[1] * normalExtent + t[1] * su + b[1] * sv,
                       n[2] * normalExtent + t[2] * su + b[2] * sv, n[0], n[1],
                       n[2]);
        }
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
    }
    return mesh;
}

MeshData makeCylinder(int radialSegments, float radius, float length,
                      Axis axis) {
    MeshData mesh;
    const auto segments = static_cast<uint32_t>(radialSegments);
    mesh.vertices.reserve((4 * segments + 2) * kVertexFloats);
    mesh.indices.reserve(segments * 12);

    const float halfLength = length * 0.5f;

    // Side: two rings with smooth radial normals.
    for (uint32_t ring = 0; ring < 2; ++ring) {
        const float x = (ring == 0) ? -halfLength : halfLength;
        for (uint32_t i = 0; i < segments; ++i) {
            const float theta = kTwoPi * static_cast<float>(i) /
                                static_cast<float>(segments);
            const float c = std::cos(theta);
            const float s = std::sin(theta);
            pushVertex(mesh, x, radius * c, radius * s, 0.0f, c, s);
        }
    }
    for (uint32_t i = 0; i < segments; ++i) {
        const uint32_t next = (i + 1) % segments;
        const uint32_t a = i;
        const uint32_t an = next;
        const uint32_t b = segments + i;
        const uint32_t bn = segments + next;
        mesh.indices.push_back(a);
        mesh.indices.push_back(bn);
        mesh.indices.push_back(b);
        mesh.indices.push_back(a);
        mesh.indices.push_back(an);
        mesh.indices.push_back(bn);
    }

    // +X cap: center + ring, flat normal (1, 0, 0).
    const uint32_t centerPlus =
        static_cast<uint32_t>(mesh.vertices.size() / kVertexFloats);
    pushVertex(mesh, halfLength, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    for (uint32_t i = 0; i < segments; ++i) {
        const float theta = kTwoPi * static_cast<float>(i) /
                            static_cast<float>(segments);
        pushVertex(mesh, halfLength, radius * std::cos(theta),
                   radius * std::sin(theta), 1.0f, 0.0f, 0.0f);
    }
    for (uint32_t i = 0; i < segments; ++i) {
        mesh.indices.push_back(centerPlus);
        mesh.indices.push_back(centerPlus + 1 + i);
        mesh.indices.push_back(centerPlus + 1 + (i + 1) % segments);
    }

    // -X cap: reversed fan so the flat normal points (-1, 0, 0).
    const uint32_t centerMinus =
        static_cast<uint32_t>(mesh.vertices.size() / kVertexFloats);
    pushVertex(mesh, -halfLength, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
    for (uint32_t i = 0; i < segments; ++i) {
        const float theta = kTwoPi * static_cast<float>(i) /
                            static_cast<float>(segments);
        pushVertex(mesh, -halfLength, radius * std::cos(theta),
                   radius * std::sin(theta), -1.0f, 0.0f, 0.0f);
    }
    for (uint32_t i = 0; i < segments; ++i) {
        mesh.indices.push_back(centerMinus);
        mesh.indices.push_back(centerMinus + 1 + (i + 1) % segments);
        mesh.indices.push_back(centerMinus + 1 + i);
    }

    if (axis != Axis::X) {
        const int permutations = (axis == Axis::Y) ? 1 : 2;
        for (std::size_t vertex = 0; vertex < mesh.vertices.size();
             vertex += kVertexFloats) {
            for (int pass = 0; pass < permutations; ++pass) {
                permuteCyclic(mesh.vertices[vertex + 0],
                              mesh.vertices[vertex + 1],
                              mesh.vertices[vertex + 2]);
                permuteCyclic(mesh.vertices[vertex + 3],
                              mesh.vertices[vertex + 4],
                              mesh.vertices[vertex + 5]);
            }
        }
    }
    return mesh;
}

void appendMesh(MeshData& destination, const MeshData& source, float offsetX,
                float offsetY, float offsetZ) {
    const uint32_t baseIndex = static_cast<uint32_t>(
        destination.vertices.size() / kVertexFloats);
    destination.vertices.reserve(destination.vertices.size() +
                                 source.vertices.size());
    for (std::size_t vertex = 0; vertex < source.vertices.size();
         vertex += kVertexFloats) {
        destination.vertices.push_back(source.vertices[vertex + 0] + offsetX);
        destination.vertices.push_back(source.vertices[vertex + 1] + offsetY);
        destination.vertices.push_back(source.vertices[vertex + 2] + offsetZ);
        destination.vertices.push_back(source.vertices[vertex + 3]);
        destination.vertices.push_back(source.vertices[vertex + 4]);
        destination.vertices.push_back(source.vertices[vertex + 5]);
    }
    destination.indices.reserve(destination.indices.size() +
                                source.indices.size());
    for (const uint32_t index : source.indices) {
        destination.indices.push_back(baseIndex + index);
    }
}

}  // namespace enigma::scene
