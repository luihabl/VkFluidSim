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

    const MeshBVH& GetBVH() const { return bvh; }
    const MeshPseudonormals& GetPseudonormals() const { return pseudonormals; }
    gfx::BoundingBox GetBox() const { return box; }
    auto GetResolution() const { return resolution; }
    f64 Interpolate(const glm::vec3& x) const { return discrete_grid.Interpolate(x); }
    const std::vector<f64>& GetSDF() const { return discrete_grid.GetGrid(); }

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
