#pragma once

#include "models/model.h"
#include "sort.h"
namespace vfs {

class LagueModel final : public SPHModel {
public:
    struct UniformData {
        float gravity;
        float damping_factor;
        float smoothing_radius;
        float target_density;
        float pressure_multiplier;
        float near_pressure_multiplier;
        float viscosity_strenght;

        BoundingBox box;

        float spiky_pow3_scale;
        float spiky_pow2_scale;
        float spiky_pow3_diff_scale;
        float spiky_pow2_diff_scale;

        VkDeviceAddress predicted_positions;

        VkDeviceAddress spatial_keys;
        VkDeviceAddress spatial_offsets;
        VkDeviceAddress sorted_indices;

        VkDeviceAddress sort_target_positions;
        VkDeviceAddress sort_target_pred_positions;
        VkDeviceAddress sort_target_velocities;
    };

    void Init(const gfx::CoreCtx& ctx) override;
    void Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) override;
    void Clear(const gfx::CoreCtx& ctx) override;
    DataBuffers CreateDataBuffers(const gfx::CoreCtx& ctx) const override;
    void DrawDebugUI() override;

private:
    bool update_uniforms{false};

    UniformData uniform_data;

    gfx::Buffer predicted_positions;

    gfx::Buffer spatial_keys;
    gfx::Buffer spatial_indices;
    gfx::Buffer spatial_offsets;

    gfx::Buffer sort_target_position;
    gfx::Buffer sort_target_pred_position;
    gfx::Buffer sort_target_velocity;

    GPUCountSort sort;
    SpatialOffset offset;

    void SetInitialData();
    void ScheduleUpdateUniforms();
    void SetParticlesInBox(const gfx::Device& gfx, const BoundingBox& box);
    void SetSmoothingRadius(float radius);
};

}  // namespace vfs
