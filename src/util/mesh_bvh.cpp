#include "mesh_bvh.h"

#include <glm/ext.hpp>
#include <glm/ext/vector_common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>

#include "gfx/mesh.h"

namespace vfs {

void MeshBVH::Init(gfx::CPUMesh* mesh) {
    this->mesh = mesh;
}

void MeshBVH::Build() {
    u32 n_triangles = mesh->indices.size() / 3;
    if (n_triangles == 0) {
        fmt::println("Empty mesh");
        return;
    }

    if ((n_triangles * 3) != mesh->indices.size()) {
        fmt::println("Mesh is not triangular");
        return;
    }

    triangle_start_idx.resize(n_triangles);
    triangle_centroids.resize(n_triangles);
    for (u32 i = 0; i < n_triangles; i++) {
        triangle_start_idx[i] = i * 3;

        const auto& [v0, v1, v2] = GetTriangleAtIdx(i);
        triangle_centroids[i] = (v0 + v1 + v2) / 3.0f;
    }

    nodes.reserve(2 * n_triangles - 1);

    nodes.push_back({
        .triangle_start = 0,
        .triangle_count = n_triangles,
    });
    UpdateBounds(0);
    Split(0);
}

MeshBVH::Axis MeshBVH::ChooseSplitPosition(const Node& node) {
    glm::vec3 extent = node.aabb_max - node.aabb_min;
    auto* e = glm::value_ptr(extent);
    u32 axis = std::distance(e, std::max_element(e, e + 3));
    return {.axis = axis, .pos = node.aabb_min[axis] + extent[axis] / 2.0f};
}

void MeshBVH::Split(u32 node_idx) {
    auto& node = nodes[node_idx];

    if (node.triangle_count <= 2)
        return;

    auto split_pos = ChooseSplitPosition(node);

    u32 i = node.triangle_start;
    u32 j = i + node.triangle_count - 1;

    while (i <= j) {
        if (triangle_centroids[i][split_pos.axis] < split_pos.pos) {
            i++;
        } else {
            std::swap(triangle_centroids[i], triangle_centroids[j]);
            std::swap(triangle_start_idx[i], triangle_start_idx[j]);
            j--;
        }
    }

    u32 left_count = i - node.triangle_start;

    if (left_count == 0 || left_count == node.triangle_count) {
        fmt::println("Early stop, triangle count: {} [parent count: {}]", left_count,
                     node.triangle_count);
        return;
    }

    nodes.push_back(Node{
        .triangle_start = node.triangle_start,
        .triangle_count = left_count,
    });

    u32 child_a_idx = nodes.size() - 1;
    node.child_a = child_a_idx;

    nodes.push_back(Node{
        .triangle_start = i,
        .triangle_count = node.triangle_count - left_count,
    });

    u32 child_b_idx = nodes.size() - 1;
    node.child_b = child_b_idx;

    UpdateBounds(child_a_idx);
    UpdateBounds(child_b_idx);

    Split(child_a_idx);
    Split(child_b_idx);
}

void MeshBVH::UpdateBounds(u32 node_idx) {
    auto& node = nodes[node_idx];
    node.aabb_max = glm::vec3(std::numeric_limits<float>::lowest());
    node.aabb_min = glm::vec3(std::numeric_limits<float>::max());

    for (u32 first = node.triangle_start, i = 0; i < node.triangle_count; i++) {
        const auto& [v0, v1, v2] = GetTriangleAtIdx(i + first);

        node.aabb_min = glm::min(node.aabb_min, v0);
        node.aabb_min = glm::min(node.aabb_min, v1);
        node.aabb_min = glm::min(node.aabb_min, v2);

        node.aabb_max = glm::max(node.aabb_max, v0);
        node.aabb_max = glm::max(node.aabb_max, v1);
        node.aabb_max = glm::max(node.aabb_max, v2);
    }
}

std::tuple<const glm::vec3&, const glm::vec3&, const glm::vec3&> MeshBVH::GetTriangleAtIdx(
    u32 idx) {
    const u32 t_idx = triangle_start_idx[idx];

    return {
        mesh->vertices[mesh->indices[t_idx + 0]].pos,
        mesh->vertices[mesh->indices[t_idx + 1]].pos,
        mesh->vertices[mesh->indices[t_idx + 2]].pos,
    };
}

const glm::vec3& MeshBVH::GetCentroidAtIdx(u32 idx) {
    return triangle_centroids[idx];
}

}  // namespace vfs
