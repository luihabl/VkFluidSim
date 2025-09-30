#pragma once

#include <cstddef>

#include "models/model.h"
namespace vfs {

/*
 * Model based on the simulations developed by Sebastian Lague. This model can be seen in
 * https://github.com/SebLague/Fluid-Sim. Most part of this model is based on the work by S. Clavet,
 * P. Beaudoin, and P. Poulin, “Particle-based viscoelastic fluid simulation,” in Proceedings of the
 * 2005 ACM SIGGRAPH/Eurographics symposium on Computer animation, Los Angeles California: ACM, July
 * 2005, pp.  219–228. doi: 10.1145/1073368.1073400.
 */
class LagueModel final : public SPHModel {
public:
    struct Parameters {
        float wall_damping_factor;
        float pressure_multiplier;
        float near_pressure_multiplier;
        float viscosity_strenght;
    };

    LagueModel(const SPHModel::Parameters* base_parameters = nullptr,
               const Parameters* parameters = nullptr);

    void Init(const gfx::CoreCtx& ctx) override;
    void Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) override;
    void Clear(const gfx::CoreCtx& ctx) override;
    void DrawDebugUI() override;

private:
    Parameters parameters;

    bool update_uniforms{false};
    u32 parameter_id;
    u32 buf_id;

    gfx::Buffer predicted_positions;
    gfx::Buffer near_density;

    void UpdateAllUniforms();
    void ScheduleUpdateUniforms();
    void SetParticlesInBox(const gfx::Device& gfx, const BoundingBox& box);
};

}  // namespace vfs
