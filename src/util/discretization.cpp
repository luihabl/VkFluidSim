#include "discretization.h"

#include <glm/ext.hpp>
#include <glm/fwd.hpp>

#include "gfx/common.h"

namespace {  // k(z), j(y), i(x)
u32 GetIndex3D(const glm::uvec3& size, const glm::uvec3& idx) {
    return idx.x + size.x * idx.y + size.x * size.y * idx.z;
}
}  // namespace

namespace vfs {
void LinearLagrangeDiscreteGrid::Init(const glm::uvec3& resolution,
                                      const AABB& d,
                                      std::function<double(const glm::dvec3&)> func) {
    this->domain = d;
    this->resolution = resolution;

    grid.resize(glm::compMul(resolution), 0.0);
    step = (domain.pos_max - domain.pos_min) / (glm::vec3)(resolution - 1u);

    for (u32 i = 0; i < resolution.x; i++) {
        for (u32 j = 0; j < resolution.y; j++) {
            for (u32 k = 0; k < resolution.z; k++) {
                auto edge_position = step * glm::dvec3(i, j, k) + (glm::dvec3)domain.pos_min;
                grid[GetIndex3D(resolution, {i, j, k})] = func(edge_position);
            }
        }
    }
}

double LinearLagrangeDiscreteGrid::GridVal(const glm::uvec3& idx) const {
    return grid[GetIndex3D(resolution, idx)];
}

double LinearLagrangeDiscreteGrid::Interpolate(const glm::vec3& pos) const {
    auto x = (glm::dvec3)(pos - domain.pos_min) / step;
    const auto cell = glm::floor(x);
    const auto chi = 2.0 * (x - cell) - 1.0;

    constexpr glm::uvec3 offsets[8] = {{0, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 1, 1},
                                       {1, 0, 0}, {1, 1, 0}, {1, 0, 1}, {1, 1, 1}};
    double val = 0.0;
    for (u32 i = 0; i < 8; i++) {
        auto factor = 1.0 + (2.0 * (glm::dvec3)offsets[i] - 1.0) * chi;
        val += GridVal((glm::uvec3)cell + offsets[i]) * glm::compMul(factor);
    }

    return val / 8.0;
}

}  // namespace vfs
