#include "mesh_bvh.h"

#include <glm/ext.hpp>
#include <glm/ext/vector_common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>

#include "gfx/mesh.h"

namespace vfs {

void MeshBVH::Init(gfx::CPUMesh* mesh, SplitType split) {
    this->mesh = mesh;
    split_type = split;
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

    triangles.resize(n_triangles);
    for (u32 i = 0; i < n_triangles; i++) {
        triangles[i].vertex_start_idx = i * 3;

        const auto& [v0, v1, v2] = GetTriangleAtIdx(i);
        triangles[i].centroid = (v0 + v1 + v2) / 3.0f;
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
    switch (split_type) {
        case SplitType::Midplane: {
            glm::vec3 extent = node.box.pos_max - node.box.pos_min;
            auto* e = glm::value_ptr(extent);
            u32 axis = std::distance(e, std::max_element(e, e + 3));
            return {.axis = axis, .pos = node.box.pos_min[axis] + extent[axis] / 2.0f};
        }

        case SplitType::SurfaceAreaHeuristics: {
            int best_axis = -1;
            float best_pos = 0.0f;
            float best_cost = 1e30f;

            for (int axis = 0; axis < 3; axis++) {
                for (u32 i = node.triangle_start; i < node.triangle_start + node.triangle_count;
                     i++) {
                    float candidate_pos = triangles[i].centroid[axis];
                    float cost = SurfaceAreaCost(node, axis, candidate_pos);
                    if (cost < best_cost) {
                        best_pos = candidate_pos;
                        best_axis = axis;
                        best_cost = cost;
                    }
                }
            }

            return {.axis = (u32)best_axis, .pos = best_pos};
        }
    }
}

float MeshBVH::SurfaceAreaCost(const Node& node, int axis, float pos) {
    AABB left_box, right_box;
    u32 left_count{0}, right_count{0};
    for (u32 i = node.triangle_start; i < node.triangle_start + node.triangle_count; i++) {
        const auto& [v0, v1, v2] = GetTriangleAtIdx(i);

        if (triangles[i].centroid[axis] < pos) {
            left_count++;
            left_box.Grow(v0);
            left_box.Grow(v1);
            left_box.Grow(v2);
        } else {
            right_count++;
            right_box.Grow(v0);
            right_box.Grow(v1);
            right_box.Grow(v2);
        }
    }

    f32 cost = (f32)left_count * left_box.Area() + (f32)right_count * right_box.Area();
    return cost > 0 ? cost : 1e30f;
}

void MeshBVH::Split(u32 node_idx) {
    auto& node = nodes[node_idx];

    if (node.triangle_count <= 2)
        return;

    auto split_pos = ChooseSplitPosition(node);

    u32 i = node.triangle_start;
    u32 j = i + node.triangle_count - 1;

    while (i <= j) {
        if (triangles[i].centroid[split_pos.axis] < split_pos.pos) {
            i++;
        } else {
            std::swap(triangles[i], triangles[j--]);
        }
    }

    u32 left_count = i - node.triangle_start;

    if (left_count == 0 || left_count == node.triangle_count) {
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

    for (u32 first = node.triangle_start, i = 0; i < node.triangle_count; i++) {
        const auto& [v0, v1, v2] = GetTriangleAtIdx(i + first);

        node.box.Grow(v0);
        node.box.Grow(v1);
        node.box.Grow(v2);
    }
}

std::tuple<const glm::vec3&, const glm::vec3&, const glm::vec3&> MeshBVH::GetTriangleAtIdx(
    u32 idx) {
    const u32 t_idx = triangles[idx].vertex_start_idx;

    return {
        mesh->vertices[mesh->indices[t_idx + 0]].pos,
        mesh->vertices[mesh->indices[t_idx + 1]].pos,
        mesh->vertices[mesh->indices[t_idx + 2]].pos,
    };
}

const glm::vec3& MeshBVH::GetCentroidAtIdx(u32 idx) {
    return triangles[idx].centroid;
}

void MeshBVH::AABB::Grow(const glm::vec3& p) {
    pos_min = glm::min(pos_min, p);
    pos_max = glm::max(pos_max, p);
}

float MeshBVH::AABB::Area() const {
    const auto e = pos_max - pos_min;
    return e.x * e.y + e.y * e.z + e.z * e.x;
}

}  // namespace vfs
