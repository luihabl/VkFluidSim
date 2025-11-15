#pragma once

#include <glm/fwd.hpp>

#include "gfx/common.h"
#include "gfx/mesh.h"
#include "models/model.h"
namespace vfs {
/*
 * This model is based on the work by M. Becker and M. Teschner, “Weakly compressible SPH for free
 * surface flows,” in Proceedings of the 2007 ACM SIGGRAPH/eurographics symposium on computer
 * animation, in Sca ’07. San Diego, California: Eurographics Association, 2007, pp. 209–217.
 */
class WCSPHWithBoundaryModel final : public SPHModel {
public:
    struct BoundaryObjectInfo {
        glm::mat4x4 transform;
        glm::mat4x4 rotation;
        gfx::BoundingBox box;

        VkDeviceAddress sdf_grid;
        VkDeviceAddress volume_map_grid;
        glm::uvec3 resolution;
    };

    struct Parameters {
        float wall_stiffness;
        float stiffness;
        float expoent;
        float viscosity_strenght;
        VkDeviceAddress boundary_objects{0};
        u32 n_boundary_objects{0};
    };

    WCSPHWithBoundaryModel(const SPHModel::Parameters* sph_parameters = nullptr,
                           const Parameters* parameters = nullptr);

    void Init(const gfx::CoreCtx& ctx) override;
    void Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) override;
    void DrawDebugUI() override;

private:
    u32 parameter_id{0};
    Parameters parameters;

    u32 vm_buf_id{0};
    gfx::Buffer boundary_density;
    gfx::Buffer boundary_gradient;
};

}  // namespace vfs
