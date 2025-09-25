#pragma once

#include <cstddef>

#include "models/model.h"
namespace vfs {

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
    DataBuffers CreateDataBuffers(const gfx::CoreCtx& ctx) const override;
    void DrawDebugUI() override;

private:
    Parameters parameters;

    bool update_uniforms{false};
    u32 parameter_id;
    u32 spatial_hash_buf_id;
    u32 buf_id;

    gfx::Buffer predicted_positions;

    gfx::Buffer sort_target_position;
    gfx::Buffer sort_target_pred_position;
    gfx::Buffer sort_target_velocity;

    void UpdateAllUniforms();
    void ScheduleUpdateUniforms();
    void SetParticlesInBox(const gfx::Device& gfx, const BoundingBox& box);
};

}  // namespace vfs
