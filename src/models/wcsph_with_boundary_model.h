#pragma once

#include <glm/fwd.hpp>

#include "gfx/common.h"
#include "gfx/descriptor.h"
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
    };

    struct Parameters {
        float wall_stiffness;
        float stiffness;
        float expoent;
        float viscosity_strenght;
        BoundaryObjectInfo boundary_object;
    };

    WCSPHWithBoundaryModel(const std::vector<gfx::DescriptorManager::DescriptorInfo>& desc_info,
                           const SPHModel::Parameters* sph_parameters = nullptr,
                           const Parameters* parameters = nullptr);

    void Init(const gfx::CoreCtx& ctx) override;
    void Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) override;
    void DrawDebugUI() override;

private:
    u32 parameter_id{0};
    Parameters parameters;
    std::vector<gfx::DescriptorManager::DescriptorInfo> desc_info;
};

}  // namespace vfs
