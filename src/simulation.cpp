#include "simulation.h"

#include <glm/ext/scalar_constants.hpp>
#include <random>

#include "gfx/common.h"
#include "platform.h"

namespace {
enum SimKernel : u32 {
    KernelUpdatePositions = 0,
    KernelExternalForces,
    KernelReorderCopyback,
    KernelReorder,
    KernelCalculateDensities,
    KernelCalculatePressureForces,
    // KernelCalculateViscosityForces,
    KernelUpdateSpatialHash
};

std::vector<glm::vec2> SpawnParticlesInBox(const vfs::Simulation2D::BoundingBox& box, u32 count) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution dist;

    auto p = std::vector<glm::vec2>(count);

    for (u32 i = 0; i < count; i++) {
        float r1 = dist(rng);
        float r2 = dist(rng);

        float x = box.pos.x + r1 * box.size.x;
        float y = box.pos.y + r2 * box.size.y;

        p[i] = glm::vec2{x, y};
    }

    return p;
}

std::vector<glm::vec3> SpawnParticlesInBox(const vfs::Simulation3D::BoundingBox& box, u32 count) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution dist;

    auto p = std::vector<glm::vec3>(count);

    for (u32 i = 0; i < count; i++) {
        float r1 = dist(rng);
        float r2 = dist(rng);
        float r3 = dist(rng);

        float x = box.pos.x + r1 * box.size.x;
        float y = box.pos.y + r2 * box.size.y;
        float z = box.pos.z + r3 * box.size.z;

        p[i] = glm::vec3{x, y, z};
    }

    return p;
}

}  // namespace

namespace vfs {

void Simulation2D::Init(const gfx::CoreCtx& ctx) {
    par = SimulationParameters{
        .n_particles = 16324,
        .iterations = 3,
        .time_scale = 1.0f,
        .fixed_dt = 1.0f / 120.0f,
    };

    SetBoundingBoxSize(17.1, 9.3);

    for (auto& bufs : frame_buffers) {
        bufs = {
            .position_buffer = CreateDataBuffer<glm::vec2>(ctx, par.n_particles),
            .velocity_buffer = CreateDataBuffer<glm::vec2>(ctx, par.n_particles),
            .density_buffer = CreateDataBuffer<glm::vec2>(ctx, par.n_particles),
        };
    }

    predicted_positions = CreateDataBuffer<glm::vec2>(ctx, par.n_particles);

    spatial_keys = CreateDataBuffer<u32>(ctx, par.n_particles);
    spatial_indices = CreateDataBuffer<u32>(ctx, par.n_particles);
    spatial_offsets = CreateDataBuffer<u32>(ctx, par.n_particles);

    sort_target_position = CreateDataBuffer<glm::vec2>(ctx, par.n_particles);
    sort_target_pred_position = CreateDataBuffer<glm::vec2>(ctx, par.n_particles);
    sort_target_velocity = CreateDataBuffer<glm::vec2>(ctx, par.n_particles);

    desc_manager.Init(ctx, sizeof(SimulationUniformData));
    desc_manager.SetUniformData(&sim_uniform_data);

    simulation_pipeline.Init(ctx, {
                                      .desc_manager = &desc_manager,
                                      .push_const_size = sizeof(SimulationPushConstants),
                                      .shader_path = "shaders/compiled/simulation.slang.spv",
                                      .kernels =
                                          {
                                              "update_positions",
                                              "external_forces",
                                              "reorder_copyback",
                                              "reorder",
                                              "calculate_densities",
                                              "calculate_pressure_forces",
                                              // "calculate_viscosity_forces",
                                              "update_spatial_hash",
                                          },
                                  });

    sort.Init(ctx);
    offset.Init(ctx);
    SetInitialData();
}

void Simulation2D::SetParticleState(const gfx::Device& gfx,
                                    const std::vector<glm::vec2>& pos,
                                    const std::vector<glm::vec2>& vel) {
    for (auto& frame : frame_buffers) {
        gfx.SetDataVec(frame.position_buffer, pos);
        gfx.SetDataVec(frame.velocity_buffer, vel);
        gfx.SetDataVal(frame.density_buffer, glm::vec2(0.0f));
    }
}

void Simulation2D::SetParticleInBox(const gfx::Device& gfx, const BoundingBox& box) {
    auto pos = SpawnParticlesInBox(box, par.n_particles);
    auto vel = std::vector<glm::vec2>(par.n_particles, glm::vec2(0.0f));
    SetParticleState(gfx, pos, vel);
}

void Simulation2D::SetInitialData() {
    const float smoothing_radius = 0.2;
    sim_uniform_data = SimulationUniformData{
        .gravity = -13.0f,
        .damping_factor = 0.5f,
        .smoothing_radius = smoothing_radius,
        .target_density = 234.0f,
        .pressure_multiplier = 225.0f,
        .near_pressure_multiplier = 18.0f,
        .viscosity_strenght = 0.03f,

        .box = bounding_box,

        .poly6_scale = 4.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 8)),
        .spiky_pow3_scale = 10.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 5)),
        .spiky_pow2_scale = 6.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 4)),
        .spiky_pow3_diff_scale = 30.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 5)),
        .spiky_pow2_diff_scale = 12.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 4)),

        .predicted_positions = predicted_positions.device_addr,

        .spatial_keys = spatial_keys.device_addr,
        .spatial_offsets = spatial_offsets.device_addr,
        .sorted_indices = spatial_indices.device_addr,

        .sort_target_positions = sort_target_position.device_addr,
        .sort_target_pred_positions = sort_target_pred_position.device_addr,
        .sort_target_velocities = sort_target_velocity.device_addr,
    };

    UpdateUniforms();
}

void Simulation2D::ScheduleUpdateUniforms() {
    update_uniforms = true;
}

void Simulation2D::UpdateUniforms() {
    desc_manager.SetUniformData(&sim_uniform_data);
}

void Simulation2D::SetBoundingBoxSize(float w, float h) {
    bounding_box = BoundingBox{
        .size = {w, h},
        .pos = {0.0f, 0.0f},
    };

    UniformData().box = bounding_box;
}

void Simulation2D::CopyBuffersToNextFrame(VkCommandBuffer cmd, u32 current_frame) {
    int next_frame = (current_frame + 1) % gfx::FRAME_COUNT;

#define CPY_BUFFER(name)                                                                     \
    {                                                                                        \
        auto cpy_info = VkBufferCopy{                                                        \
            .dstOffset = 0, .srcOffset = 0, .size = frame_buffers[current_frame].name.size}; \
        vkCmdCopyBuffer(cmd, frame_buffers[current_frame].name.buffer,                       \
                        frame_buffers[next_frame].name.buffer, 1, &cpy_info);                \
    }

    CPY_BUFFER(position_buffer);
    CPY_BUFFER(velocity_buffer);
    CPY_BUFFER(density_buffer);
}

void Simulation2D::Step(VkCommandBuffer cmd, const gfx::CoreCtx& ctx, u32 frame_idx) {
    if (update_uniforms) {
        UpdateUniforms();
        update_uniforms = false;
    }

    auto comp_consts = SimulationPushConstants{
        .time = Platform::Info::GetTime(),
        .dt = par.time_scale * par.fixed_dt / (float)par.iterations,
        .n_particles = (uint32_t)par.n_particles,

        .positions = frame_buffers[frame_idx].position_buffer.device_addr,
        .velocities = frame_buffers[frame_idx].velocity_buffer.device_addr,
        .densities = frame_buffers[frame_idx].density_buffer.device_addr,
    };

    auto n_groups = glm::ivec3(par.n_particles / 64 + 1, 1, 1);

    for (int i = 0; i < par.iterations; i++) {
        simulation_pipeline.Compute(cmd, KernelExternalForces, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        // RunSpatial
        simulation_pipeline.Compute(cmd, KernelUpdateSpatialHash, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        sort.Run(cmd, ctx, spatial_indices, spatial_keys, par.n_particles - 1);
        ComputeToComputePipelineBarrier(cmd);

        offset.Run(cmd, ctx, true, spatial_keys, spatial_offsets);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelReorder, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelReorderCopyback, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelCalculateDensities, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelCalculatePressureForces, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        // simulation_pipeline.Compute(cmd, KernelCalculateViscosityForces, n_groups, &comp_consts);
        // ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelUpdatePositions, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);
    }

    CopyBuffersToNextFrame(cmd, frame_idx);
}

void Simulation2D::Clear(const gfx::CoreCtx& ctx) {
    predicted_positions.Destroy();

    spatial_keys.Destroy();
    spatial_indices.Destroy();
    spatial_offsets.Destroy();

    sort_target_position.Destroy();
    sort_target_pred_position.Destroy();
    sort_target_velocity.Destroy();

    sort.Clear(ctx);
    offset.Clear(ctx);

    for (auto& frame : frame_buffers) {
        frame.position_buffer.Destroy();
        frame.velocity_buffer.Destroy();
        frame.density_buffer.Destroy();
    }

    simulation_pipeline.Clear(ctx);
    desc_manager.Clear(ctx);
}

// --- Simulation 3D ---

void Simulation3D::Init(const gfx::CoreCtx& ctx) {
    par = Parameters{
        .n_particles = 400000,
        .iterations = 3,
        .time_scale = 2.0f,
        .fixed_dt = 1.0f / 120.0f,
    };

    SetBoundingBoxSize({23.0f, 10.0f, 10.0f});

    for (auto& bufs : frame_buffers) {
        bufs = {
            .position_buffer = CreateDataBuffer<glm::vec3>(ctx, par.n_particles),
            .velocity_buffer = CreateDataBuffer<glm::vec3>(ctx, par.n_particles),
            .density_buffer = CreateDataBuffer<glm::vec2>(ctx, par.n_particles),
        };
    }

    predicted_positions = CreateDataBuffer<glm::vec3>(ctx, par.n_particles);

    spatial_keys = CreateDataBuffer<u32>(ctx, par.n_particles);
    spatial_indices = CreateDataBuffer<u32>(ctx, par.n_particles);
    spatial_offsets = CreateDataBuffer<u32>(ctx, par.n_particles);

    sort_target_position = CreateDataBuffer<glm::vec3>(ctx, par.n_particles);
    sort_target_pred_position = CreateDataBuffer<glm::vec3>(ctx, par.n_particles);
    sort_target_velocity = CreateDataBuffer<glm::vec3>(ctx, par.n_particles);

    desc_manager.Init(ctx, sizeof(UniformData));
    simulation_pipeline.Init(ctx, {
                                      .desc_manager = &desc_manager,
                                      .push_const_size = sizeof(PushConstants),
                                      .shader_path = "shaders/compiled/simulation_3d.slang.spv",
                                      .kernels =
                                          {
                                              "update_positions",
                                              "external_forces",
                                              "reorder_copyback",
                                              "reorder",
                                              "calculate_densities",
                                              "calculate_pressure_forces",
                                              "update_spatial_hash",
                                          },
                                  });

    sort.Init(ctx);
    offset.Init(ctx);
    SetInitialData();
}

void Simulation3D::SetParticleState(const gfx::Device& gfx,
                                    const std::vector<glm::vec3>& pos,
                                    const std::vector<glm::vec3>& vel) {
    for (auto& frame : frame_buffers) {
        gfx.SetDataVec(frame.position_buffer, pos);
        gfx.SetDataVec(frame.velocity_buffer, vel);
        gfx.SetDataVal(frame.density_buffer, glm::vec2(0.0f));
    }
}

void Simulation3D::SetParticleInBox(const gfx::Device& gfx, const BoundingBox& box) {
    auto pos = SpawnParticlesInBox(box, par.n_particles);
    auto vel = std::vector<glm::vec3>(par.n_particles, glm::vec3(0.0f));
    SetParticleState(gfx, pos, vel);
}

void Simulation3D::SetInitialData() {
    const float smoothing_radius = 0.2;
    uniform_data = UniformData{
        .gravity = -10.0f,
        .damping_factor = 0.05f,
        .smoothing_radius = smoothing_radius,
        .target_density = 630.0f,
        .pressure_multiplier = 288.0f,
        .near_pressure_multiplier = 2.16f,
        .viscosity_strenght = 0.03f,

        .box = bounding_box,

        .spiky_pow3_scale = 15.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 6)),
        .spiky_pow2_scale =
            15.0f / (2.0f * glm::pi<float>() * (float)std::pow(smoothing_radius, 5)),
        .spiky_pow3_diff_scale = 45.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 6)),
        .spiky_pow2_diff_scale = 15.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 5)),

        .predicted_positions = predicted_positions.device_addr,

        .spatial_keys = spatial_keys.device_addr,
        .spatial_offsets = spatial_offsets.device_addr,
        .sorted_indices = spatial_indices.device_addr,

        .sort_target_positions = sort_target_position.device_addr,
        .sort_target_pred_positions = sort_target_pred_position.device_addr,
        .sort_target_velocities = sort_target_velocity.device_addr,
    };

    ScheduleUpdateUniforms();
}

void Simulation3D::ScheduleUpdateUniforms() {
    update_uniforms = true;
}

void Simulation3D::UpdateUniforms() {
    desc_manager.SetUniformData(&uniform_data);
}

void Simulation3D::SetBoundingBoxSize(const glm::vec3& size) {
    bounding_box = {
        .size = size,
        .pos = {0.0f, 0.0f, 0.0f},
    };

    uniform_data.box = bounding_box;
}

void Simulation3D::SetSmoothingRadius(float smoothing_radius) {
    uniform_data.smoothing_radius = smoothing_radius;
    uniform_data.spiky_pow3_scale =
        15.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 6));
    uniform_data.spiky_pow2_scale =
        15.0f / (2.0f * glm::pi<float>() * (float)std::pow(smoothing_radius, 5));
    uniform_data.spiky_pow3_diff_scale =
        45.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 6));
    uniform_data.spiky_pow2_diff_scale =
        15.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 5));
}

void Simulation3D::CopyBuffersToNextFrame(VkCommandBuffer cmd, u32 current_frame) {
    int next_frame = (current_frame + 1) % gfx::FRAME_COUNT;

#define CPY_BUFFER(name)                                                                     \
    {                                                                                        \
        auto cpy_info = VkBufferCopy{                                                        \
            .dstOffset = 0, .srcOffset = 0, .size = frame_buffers[current_frame].name.size}; \
        vkCmdCopyBuffer(cmd, frame_buffers[current_frame].name.buffer,                       \
                        frame_buffers[next_frame].name.buffer, 1, &cpy_info);                \
    }

    CPY_BUFFER(position_buffer);
    CPY_BUFFER(velocity_buffer);
    CPY_BUFFER(density_buffer);
}

void Simulation3D::Step(VkCommandBuffer cmd, const gfx::CoreCtx& ctx, u32 frame_idx) {
    if (update_uniforms) {
        UpdateUniforms();
        update_uniforms = false;
    }

    auto comp_consts = PushConstants{
        .time = Platform::Info::GetTime(),
        .dt = par.time_scale * par.fixed_dt / (float)par.iterations,
        .n_particles = (uint32_t)par.n_particles,

        .positions = frame_buffers[frame_idx].position_buffer.device_addr,
        .velocities = frame_buffers[frame_idx].velocity_buffer.device_addr,
        .densities = frame_buffers[frame_idx].density_buffer.device_addr,
    };

    auto n_groups = glm::ivec3(par.n_particles / 256 + 1, 1, 1);

    for (int i = 0; i < par.iterations; i++) {
        simulation_pipeline.Compute(cmd, KernelExternalForces, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        // RunSpatial
        simulation_pipeline.Compute(cmd, KernelUpdateSpatialHash, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        sort.Run(cmd, ctx, spatial_indices, spatial_keys, par.n_particles - 1);
        ComputeToComputePipelineBarrier(cmd);

        offset.Run(cmd, ctx, true, spatial_keys, spatial_offsets);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelReorder, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelReorderCopyback, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelCalculateDensities, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelCalculatePressureForces, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);

        // simulation_pipeline.Compute(cmd, KernelCalculateViscosityForces, n_groups, &comp_consts);
        // ComputeToComputePipelineBarrier(cmd);

        simulation_pipeline.Compute(cmd, KernelUpdatePositions, n_groups, &comp_consts);
        ComputeToComputePipelineBarrier(cmd);
    }

    CopyBuffersToNextFrame(cmd, frame_idx);
}

void Simulation3D::Clear(const gfx::CoreCtx& ctx) {
    predicted_positions.Destroy();

    spatial_keys.Destroy();
    spatial_indices.Destroy();
    spatial_offsets.Destroy();

    sort_target_position.Destroy();
    sort_target_pred_position.Destroy();
    sort_target_velocity.Destroy();

    sort.Clear(ctx);
    offset.Clear(ctx);

    for (auto& frame : frame_buffers) {
        frame.position_buffer.Destroy();
        frame.velocity_buffer.Destroy();
        frame.density_buffer.Destroy();
    }

    simulation_pipeline.Clear(ctx);
    desc_manager.Clear(ctx);
}

}  // namespace vfs
