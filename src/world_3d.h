#pragma once

#include <glm/fwd.hpp>

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"
#include "pipeline.h"
#include "platform.h"
#include "renderer.h"
#include "sort.h"
#include "ui.h"

namespace vfs {
class World3D {
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
        float damping_factor = 0.05f;
        float smoothing_radius = 0.35f;
        float target_density = 10.0f;
        float pressure_multiplier = 500.0f;
        float near_pressure_multiplier = 500.0f;
        float viscosity_strenght = 500.0f;

        glm::vec3 box_pos;
        glm::vec3 box_size;

        glm::mat4x4 world_to_local;
        glm::mat4x4 local_to_world;

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

    ComputePipeline simulation_pipeline;

    GPUCountSort sort;
    SpatialOffset offset;

    gfx::Buffer predicted_positions;

    gfx::Buffer spatial_keys;
    gfx::Buffer spatial_indices;
    gfx::Buffer spatial_offsets;

    gfx::Buffer sort_target_position;
    gfx::Buffer sort_target_pred_position;
    gfx::Buffer sort_target_velocity;

    gfx::GPUMesh particle_mesh;

    glm::vec3 box_pos;
    glm::vec3 box_size;

    glm::vec3 camera_pos;
    glm::vec3 camera_dir;

    UI ui;

    float time_scale = 1.0f;
    int iterations = 3;
    int n_particles;
    float spacing = 0.2f;
    float scale = 1.5e-2;
    float fixed_dt = 1 / 120.0f;
    int current_frame = 0;
    bool paused{true};
    bool update_uniform_data{false};
    SimulationUniformData sim_uniform_data;
    std::vector<float> gpu_times;
    std::span<u64> gpu_timestamps;

    void SetInitialData();
    void ResetSimulation();
    void DrawUI(VkCommandBuffer cmd);
    void CopyBuffersToNextFrame(VkCommandBuffer cmd);
    void RunSimulationStep(VkCommandBuffer cmd);
    void SetBox(glm::vec3 size, glm::vec3 pos = {0.0f, 0.0f, 0.0f});
    void InitSimulationPipelines();
    void UpdateUniforms();
};
}  // namespace vfs
