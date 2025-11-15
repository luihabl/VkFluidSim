#include "mesh_pseudonormals.h"

#include <unordered_map>

namespace vfs {

void MeshPseudonormals::Init(const gfx::CPUMesh& mesh) {
    this->mesh = &mesh;
}

void MeshPseudonormals::Build() {
    u32 n_triangles = mesh->position_indices.size() / 3;
    u32 n_vertices = mesh->vertices.size();

    triangle_pseudonormals.resize(n_triangles);
    edge_pseudonormals.resize(n_triangles);
    vertex_pseudonormals.resize(n_vertices, {0, 0, 0});

    std::unordered_map<u32, glm::vec3> edge_normals;
    std::unordered_map<u32, u32> edge_count;

    auto GetEdgeKey = [n_vertices](auto ia, auto ib) {
        return std::max(ia, ib) + std::min(ia, ib) * n_vertices;
    };

    auto AddEdgeNormal = [&edge_normals, &edge_count, &GetEdgeKey](auto ia, auto ib,
                                                                   const auto& n) {
        auto key = GetEdgeKey(ia, ib);
        if (!edge_normals.contains(key)) {
            edge_normals[key] = {0.0f, 0.0f, 0.0f};
            edge_count[key] = 0;
        }
        edge_normals[key] += n;
        edge_count[key]++;
    };

    for (u32 i = 0; i < n_triangles; i++) {
        const auto idx_a = mesh->position_indices[i * 3 + 0];
        const auto idx_b = mesh->position_indices[i * 3 + 1];
        const auto idx_c = mesh->position_indices[i * 3 + 2];

        const auto& a = mesh->vertices[idx_a].pos;
        const auto& b = mesh->vertices[idx_b].pos;
        const auto& c = mesh->vertices[idx_c].pos;

        // Triangle pseudonormals
        const auto triangle_normal = glm::normalize(glm::cross(b - a, c - a));
        triangle_pseudonormals[i] = triangle_normal;

        // Vertex pseudonormals
        // It is defied as a weighted average of the triangle normals of the three triangles which
        // share the edge. The weights are simply the incident angle (angle between the vertices).
        const auto alpha_0 =
            std::acos(glm::abs(glm::dot(glm::normalize(b - a), glm::normalize(c - a))));
        const auto alpha_1 =
            std::acos(glm::abs(glm::dot(glm::normalize(a - b), glm::normalize(c - b))));
        const auto alpha_2 =
            std::acos(glm::abs(glm::dot(glm::normalize(b - c), glm::normalize(a - c))));

        vertex_pseudonormals[idx_a] += alpha_0 * triangle_normal;
        vertex_pseudonormals[idx_b] += alpha_1 * triangle_normal;
        vertex_pseudonormals[idx_c] += alpha_2 * triangle_normal;

        // Edge pseudonormals
        // It is defined as an average of the triangle normals which share the edge.
        AddEdgeNormal(idx_a, idx_b, triangle_normal);
        AddEdgeNormal(idx_b, idx_c, triangle_normal);
        AddEdgeNormal(idx_a, idx_c, triangle_normal);
    }

    // Normalize and consolidate
    for (auto& vn : vertex_pseudonormals) {
        vn = glm::normalize(vn);
    }

    for (u32 i = 0; i < n_triangles; i++) {
        const auto idx_a = mesh->position_indices[i * 3 + 0];
        const auto idx_b = mesh->position_indices[i * 3 + 1];
        const auto idx_c = mesh->position_indices[i * 3 + 2];

        edge_pseudonormals[i][0] = glm::normalize(edge_normals.at(GetEdgeKey(idx_a, idx_b)));
        edge_pseudonormals[i][1] = glm::normalize(edge_normals.at(GetEdgeKey(idx_b, idx_c)));
        edge_pseudonormals[i][2] = glm::normalize(edge_normals.at(GetEdgeKey(idx_a, idx_c)));
    }

    u32 single_edge_count = 0;
    u32 triple_edge_count = 0;

    for (const auto& [key, count] : edge_count) {
        if (count == 1) {
            single_edge_count++;
        }

        if (count > 2) {
            triple_edge_count++;
        }
    }

    if (single_edge_count > 0)
        fmt::println("[Pseudonormals] warning: found {} single edges", single_edge_count);

    if (triple_edge_count > 0)
        fmt::println("[Pseudonormals] warning: found {} triple edges", triple_edge_count);
}
}  // namespace vfs
