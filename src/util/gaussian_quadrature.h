#pragma once

#include <functional>
#include <glm/glm.hpp>

#include "geometry.h"

namespace vfs {
double GaussLegendreQuadrature3D(const AABB& domain,
                                 u32 n,
                                 std::function<double(const glm::dvec3&)> integrand);
}  // namespace vfs
