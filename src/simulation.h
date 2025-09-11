#pragma once

#include <glm/glm.hpp>

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "pipeline.h"
#include "sort.h"

namespace vfs {

class Simulation2D {
public:
    struct BoundingBox {
        glm::vec2 size;
        glm::vec2 pos;
    };

    struct SimulationUniformData {
        float gravity;
        float damping_factor;
        float smoothing_radius;
        float target_density;
        float pressure_multiplier;
        float near_pressure_multiplier;
        float viscosity_strenght;

        BoundingBox box;

        float poly6_scale;
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

    struct SimulationPushConstants {
        float time;
        float dt;
        unsigned n_particles;
        VkDeviceAddress positions;
        VkDeviceAddress velocities;
        VkDeviceAddress densities;
    };

    struct FrameBuffers {
        gfx::Buffer position_buffer;
        gfx::Buffer velocity_buffer;
        gfx::Buffer density_buffer;
    };

    struct SimulationParameters {
        float time_scale;
        int iterations;
        int n_particles;
        float fixed_dt;
    };

    void Init(const gfx::CoreCtx& ctx);
    void Step(VkCommandBuffer cmd, const gfx::CoreCtx& ctx, u32 frame_idx);
    void ScheduleUpdateUniforms();
    SimulationUniformData& UniformData() { return sim_uniform_data; }
    void SetParticleState(const gfx::Device& gfx,
                          const std::vector<glm::vec2>& pos,
                          const std::vector<glm::vec2>& vel);
    void SetParticleInBox(const gfx::Device& gfx, const BoundingBox& box);
    void SetBoundingBoxSize(float w, float h);

    BoundingBox GetBoundingBox() { return bounding_box; }
    const FrameBuffers& GetFrameData(u32 frame_idx) { return frame_buffers[frame_idx]; }
    const SimulationParameters& GetParameters() { return par; }
    void Clear(const gfx::CoreCtx& ctx);

private:
    bool update_uniforms{false};

    SimulationParameters par;
    BoundingBox bounding_box;

    std::array<FrameBuffers, gfx::FRAME_COUNT> frame_buffers;
    DescriptorManager desc_manager;
    ComputePipeline simulation_pipeline;
    SimulationUniformData sim_uniform_data;

    gfx::Buffer predicted_positions;

    gfx::Buffer spatial_keys;
    gfx::Buffer spatial_indices;
    gfx::Buffer spatial_offsets;

    gfx::Buffer sort_target_position;
    gfx::Buffer sort_target_pred_position;
    gfx::Buffer sort_target_velocity;

    GPUCountSort sort;
    SpatialOffset offset;

    void UpdateUniforms();
    void SetInitialData();
    void CopyBuffersToNextFrame(VkCommandBuffer cmd, u32 current_frame);
};

}  // namespace vfs
