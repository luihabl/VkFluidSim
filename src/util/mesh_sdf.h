#pragma once

#include <glm/gtc/constants.hpp>

#include "gfx/mesh.h"
#include "util/discretization.h"
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
    f64 Interpolate(const glm::vec3& x) const { return discrete_grid.Interpolate(x); }
    const std::vector<f64>& GetSDF() { return discrete_grid.GetGrid(); }

private:
    const gfx::CPUMesh* mesh{nullptr};
    MeshBVH bvh;
    MeshPseudonormals pseudonormals;

    // SDF grid (may be refactored into another class)
    gfx::BoundingBox box;
    glm::uvec3 resolution;
    LinearLagrangeDiscreteGrid discrete_grid;
    f64 tolerance{0.0};
};

}  // namespace vfs
