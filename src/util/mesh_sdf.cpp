#include "mesh_sdf.h"

#include <glm/ext.hpp>
#include <glm/fwd.hpp>

#include "util/geometry.h"

namespace {  // k(z), j(y), i(x)
u32 GetIndex3D(const glm::uvec3& size, const glm::uvec3& idx) {
    return idx.x + size.x * idx.y + size.x * size.y * idx.z;
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

    discrete_grid.Init(resolution, {.pos_min = box.pos, .pos_max = box.pos + box.size},
                       [this](const glm::dvec3& x) {
                           auto spos = SignedDistanceToMesh(bvh, pseudonormals, x);
                           return spos.signed_distance - tolerance;
                       });
}

void MeshSDF::Clean() {}

}  // namespace vfs
