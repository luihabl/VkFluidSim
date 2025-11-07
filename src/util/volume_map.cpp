#include "volume_map.h"

#include "util/gaussian_quadrature.h"
#include "util/kernel.h"

void vfs::GenerateVolumeMap(const MeshSDF& sdf,
                            float support_radius,
                            LinearLagrangeDiscreteGrid& volume_map) {
    const auto box = sdf.GetBox();
    const auto domain = AABB{.pos_min = box.pos, .pos_max = box.pos + box.size};
    auto kernel = CubicSplineKernel(support_radius);

    auto volume_map_func = [&](const glm::vec3& x) -> double {
        const double dist = sdf.Interpolate(x);

        if (dist > 2.0 * support_radius) {
            return 0.0;
        }

        auto integrand = [&](const glm::vec3& xi) -> double {
            if (glm::length2(xi) > support_radius * support_radius) {
                return 0.0;
            }

            auto dist_i = sdf.Interpolate(x + xi);

            if (dist_i <= 0.0) {
                return 1.0;
            }

            else if (dist_i < support_radius) {
                return kernel.W(dist_i) / kernel.WZero();
            }

            else {
                return 0;
            }
        };

        return 0.8 * GaussLegendreQuadrature3D(domain, 16, integrand);
    };

    volume_map.Init(sdf.GetResolution(), domain, volume_map_func);
}
