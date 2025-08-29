#pragma once

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"
#include "pipeline.h"
#include "platform.h"
#include "renderer.h"
#include "sort.h"

namespace vfs {
class World {
public:
    using Event = SDL_Event;

    void Init(Platform& platform);
    void Update(Platform& platform);
    void HandleEvent(Platform& platform, const Event& e);
    void Clear();

private:
    gfx::Device gfx;
    Renderer renderer;

    struct SimulationUniformData {
        float gravity = 0.0f;
        float mass = 1.0f;
        float damping_factor = 0.05f;
        float smoothing_radius = 0.35f;
        float target_density = 10.0f;
        float pressure_multiplier = 500.0f;
        float near_pressure_multiplier = 500.0f;
        float viscosity_strenght = 500.0f;

        glm::vec4 box = {0, 0, 1, 1};

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

    std::array<FrameBuffers, gfx::FRAME_COUNT> frame_buffers;

    DescriptorManager global_desc_manager;

    ComputePipelineSet<SimulationPushConstants> simulation_pipelines;
    u32 sim_pos;
    u32 sim_ext_forces;
    u32 sim_reorder;
    u32 sim_spatial_hash;
    u32 sim_reorder_copyback;
    u32 sim_pressure;
    u32 sim_density;
    u32 sim_viscosity;

    GPUCountSort sort;
    SpatialOffset offset;

    gfx::Buffer predicted_positions;

    gfx::Buffer spatial_keys;
    gfx::Buffer spatial_indices;
    gfx::Buffer spatial_offsets;

    gfx::Buffer sort_target_position;
    gfx::Buffer sort_target_pred_position;
    gfx::Buffer sort_target_velocity;

    gfx::GPUMesh circle_mesh;

    glm::vec4 box;
    glm::vec4 cbox;
    int n_particles = 6000;
    float spacing = 0.2f;
    float scale = 2e-2;
    int current_frame = 0;

    void SetInitialData();
    void CopyBuffersToNextFrame(VkCommandBuffer cmd);
    void RunSimulationStep(VkCommandBuffer cmd);
    void SetBox(float w, float h);
    void InitSimulationPipelines();
};
}  // namespace vfs