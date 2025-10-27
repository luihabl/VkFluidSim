#pragma once

#include <glm/gtc/constants.hpp>

#include "gfx/mesh.h"
#include "util/mesh_bvh.h"
#include "util/mesh_pseudonormals.h"

namespace vfs {

class MeshSDF {
public:
    void Init(const gfx::CPUMesh& mesh,
              gfx::BoundingBox bounding_box,
              glm::uvec3 resolution = {15, 15, 15});
    void Build();
    void Clean();

    const MeshBVH& GetBVH() { return bvh; }
    gfx::BoundingBox GetBox() { return box; }
    auto GetResolution() { return resolution; }
    const std::vector<f32>& GetSDF() { return sdf_grid; }

private:
    const gfx::CPUMesh* mesh{nullptr};
    MeshBVH bvh;
    MeshPseudonormals pseudonormals;

    // SDF grid (may be refactored into another class)
    gfx::BoundingBox box;
    glm::uvec3 resolution;
    std::vector<f32> sdf_grid;
};

}  // namespace vfs
