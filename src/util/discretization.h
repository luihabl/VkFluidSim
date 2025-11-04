#pragma once
#include <functional>
#include <glm/glm.hpp>

#include "util/geometry.h"

namespace vfs {
class LinearLagrangeDiscreteGrid {
public:
    void Init(const glm::uvec3& resolution,
              const AABB& domain,
              std::function<double(const glm::dvec3&)> func);
    double Interpolate(const glm::vec3& pos) const;
    const std::vector<f64>& GetGrid() const { return grid; }

private:
    double GridVal(const glm::uvec3& idx) const;

    std::vector<f64> grid;
    AABB domain;
    glm::dvec3 step;
    glm::uvec3 resolution;
};
}  // namespace vfs
