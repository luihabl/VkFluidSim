#include "mesh_bvh.h"

#include <glm/ext.hpp>
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
    for (u32 i = 0; i < n_triangles; i++) {
        triangle_start_idx[i] = i * 3;
    }

    nodes.push_back({
        .bbox = GetBoundingBox(0, n_triangles),
        .triangle_start = 0,
        .triangle_count = n_triangles,
    });

    Split(0);
}

MeshBVH::Axis MeshBVH::ChooseSplitPosition(u32 parent) {
    const auto& bbox = nodes[parent].bbox;

    // TODO: improve this function later for better bounding box splitting
    float vmax = glm::compMax(bbox.size);
    if (bbox.size.x == vmax) {
        return {.axis = 0, .pos = bbox.size.x / 2.0f};
    }
    if (bbox.size.y == vmax) {
        return {.axis = 1, .pos = bbox.size.y / 2.0f};
    }
    if (bbox.size.z == vmax) {
        return {.axis = 2, .pos = bbox.size.z / 2.0f};
    }

    return {};
}

void MeshBVH::GrowBoundingBoxToInclude(gfx::BoundingBox& bbox, u32 triangle_idx) {
    u32 idx = triangle_start_idx[triangle_idx];
    auto a = mesh->vertices[mesh->indices[idx + 0]].pos;
    auto b = mesh->vertices[mesh->indices[idx + 1]].pos;
    auto c = mesh->vertices[mesh->indices[idx + 2]].pos;

    auto min_pos = glm::min(a, b, c);
    auto max_pos = glm::max(a, b, c);
}

void MeshBVH::Split(u32 root, u32 depth) {
    if (depth >= max_depth)
        return;

    auto split_axis = ChooseSplitPosition(root);
}

gfx::BoundingBox MeshBVH::GetBoundingBox(u32 start, u32 count) {
    auto max_pos = glm::vec3{std::numeric_limits<float>::lowest()};
    auto min_pos = glm::vec3{std::numeric_limits<float>::max()};

    for (u32 i = start; i < count; i++) {
        u32 idx = triangle_start_idx[i];

        auto a = mesh->vertices[mesh->indices[idx + 0]].pos;
        auto b = mesh->vertices[mesh->indices[idx + 1]].pos;
        auto c = mesh->vertices[mesh->indices[idx + 2]].pos;

        max_pos = glm::max(max_pos, a);
        max_pos = glm::max(max_pos, b);
        max_pos = glm::max(max_pos, c);

        min_pos = glm::min(min_pos, a);
        min_pos = glm::min(min_pos, b);
        min_pos = glm::min(min_pos, c);
    }

    return {.pos = min_pos, .size = max_pos - min_pos};
}

}  // namespace vfs
