#pragma once

#include <glm/gtc/constants.hpp>

#include "gfx/mesh.h"
#include "util/mesh_bvh.h"
#include "util/mesh_pseudonormals.h"

namespace vfs {

class MeshSDF {
public:
    void Init(const gfx::CPUMesh& mesh,
              glm::uvec3 resolution,
              gfx::BoundingBox bounding_box = {},
              double tolerance = 0.05);
    void Build();
    void Clean();

    const MeshBVH& GetBVH() { return bvh; }
    const MeshPseudonormals& GetPseudonormals() { return pseudonormals; }
    gfx::BoundingBox GetBox() { return box; }
    auto GetResolution() { return resolution; }
    const std::vector<f64>& GetSDF() { return sdf_grid; }

private:
    const gfx::CPUMesh* mesh{nullptr};
    MeshBVH bvh;
    MeshPseudonormals pseudonormals;

    // SDF grid (may be refactored into another class)
    gfx::BoundingBox box;
    glm::uvec3 resolution;
    std::vector<f64> sdf_grid;
    f64 tolerance{0.0};
};

}  // namespace vfs
