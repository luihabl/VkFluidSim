#include "mesh_sdf.h"

#include <glm/ext.hpp>

namespace vfs {
void MeshSDF::Init(const gfx::CPUMesh& mesh, gfx::BoundingBox bounding_box, glm::uvec3 resolution) {
    this->box = bounding_box;
    this->resolution = resolution;
    this->mesh = &mesh;

    bvh.Init(mesh);
    pseudonormals.Init(mesh);
}

void MeshSDF::Build() {
    bvh.Build();
    pseudonormals.Build();

    sdf_grid.resize(resolution.x * resolution.y * resolution.z, 0);
    auto step = box.size / (glm::vec3)(resolution - 1u);
}

void MeshSDF::Clean() {}

}  // namespace vfs
