#include "mesh_sdf.h"

#include <glm/ext.hpp>

#include "util/geometry.h"

namespace {
u32 GetIndex3D(const glm::uvec3& size, const glm::uvec3& idx) {
    return idx.z + size.z * idx.y + size.z * size.y * idx.x;
}
}  // namespace

namespace vfs {
void MeshSDF::Init(const gfx::CPUMesh& mesh,
                   glm::uvec3 resolution,
                   gfx::BoundingBox bounding_box,
                   double tolerance) {
    this->box = bounding_box;
    this->resolution = resolution;
    this->mesh = &mesh;
    this->tolerance = tolerance;

    bvh.Init(mesh);
    pseudonormals.Init(mesh);
}

void MeshSDF::Build() {
    bvh.Build();
    pseudonormals.Build();

    if (box.pos == glm::vec3(0) && box.size == glm::vec3(0)) {
        auto root_box = bvh.GetNodes().front().box;
        auto size = root_box.pos_max - root_box.pos_min;
        auto pos = root_box.pos_min;

        this->box.size = size + 2.0f * (f32)tolerance;
        this->box.pos = pos - (f32)tolerance;
    }

    sdf_grid.resize(resolution.x * resolution.y * resolution.z, 0);
    auto step = box.size / (glm::vec3)(resolution - 1u);

    // NOTE: this is being constructed for a rectangular grid, not necessarily what is best for an
    // SDF. Make a way to define this more generally for a list of points for example.
    for (u32 i = 0; i < resolution.x; i++) {
        for (u32 j = 0; j < resolution.y; j++) {
            for (u32 k = 0; k < resolution.z; k++) {
                auto pos = step * glm::vec3(i, j, k) + box.pos;

                auto signed_distance = SignedDistanceToMesh(bvh, pseudonormals, pos);
                sdf_grid[GetIndex3D(resolution, {i, j, k})] =
                    signed_distance.signed_distance - tolerance;
            }
        }
    }
}

void MeshSDF::Clean() {}

}  // namespace vfs
