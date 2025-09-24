#include "lague_model.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/glm.hpp>

#include "imgui.h"
#include "models/model.h"
#include "platform.h"

namespace vfs {
enum SimKernel : u32 {
    KernelUpdatePositions = 0,
    KernelExternalForces,
    KernelReorderCopyback,
    KernelReorder,
    KernelCalculateDensities,
    KernelCalculatePressureForces,
};

void LagueModel::Init(const gfx::CoreCtx& ctx) {
    parameters = {
        .n_particles = 400000,
        .iterations = 3,
        .time_scale = 2.0f,
        .fixed_dt = 1.0f / 120.0f,
    };

    SetBoundingBoxSize({23.0f, 10.0f, 10.0f});

    buffers = CreateDataBuffers(ctx);

    predicted_positions = CreateDataBuffer<glm::vec3>(ctx, parameters.n_particles);

    sort_target_position = CreateDataBuffer<glm::vec3>(ctx, parameters.n_particles);
    sort_target_pred_position = CreateDataBuffer<glm::vec3>(ctx, parameters.n_particles);
    sort_target_velocity = CreateDataBuffer<glm::vec3>(ctx, parameters.n_particles);

    desc_manager.Init(ctx, sizeof(UniformData));
    spatial_hash.Init(ctx, parameters.n_particles, 0.2);

    pipeline.Init(ctx, {
                           .desc_manager = &desc_manager,
                           .push_const_size = sizeof(SPHModel::PushConstants),
                           .shader_path = "shaders/compiled/simulation_3d.slang.spv",
                           .kernels =
                               {
                                   "update_positions",
                                   "external_forces",
                                   "reorder_copyback",
                                   "reorder",
                                   "calculate_densities",
                                   "calculate_pressure_forces",
                               },
                       });

    SetInitialData();
}

SPHModel::DataBuffers LagueModel::CreateDataBuffers(const gfx::CoreCtx& ctx) const {
    return {
        .position_buffer = CreateDataBuffer<glm::vec3>(ctx, parameters.n_particles),
        .velocity_buffer = CreateDataBuffer<glm::vec3>(ctx, parameters.n_particles),
        .density_buffer = CreateDataBuffer<glm::vec2>(ctx, parameters.n_particles),
    };
}

void LagueModel::SetInitialData() {
    const float smoothing_radius = 0.2;
    uniform_data = UniformData{
        .gravity = -10.0f,
        .damping_factor = 0.05f,
        .smoothing_radius = smoothing_radius,
        .target_density = 630.0f,
        .pressure_multiplier = 288.0f,
        .near_pressure_multiplier = 2.16f,
        .viscosity_strenght = 0.03f,

        .box = bounding_box.value(),

        .spiky_pow3_scale = 15.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 6)),
        .spiky_pow2_scale =
            15.0f / (2.0f * glm::pi<float>() * (float)std::pow(smoothing_radius, 5)),
        .spiky_pow3_diff_scale = 45.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 6)),
        .spiky_pow2_diff_scale = 15.0f / (glm::pi<float>() * (float)std::pow(smoothing_radius, 5)),

        .predicted_positions = predicted_positions.device_addr,

        .spatial_keys = spatial_hash.SpatialKeysAddr(),
        .spatial_offsets = spatial_hash.SpatialOffsetsAddr(),
        .sorted_indices = spatial_hash.SpatialIndicesAddr(),

        .sort_target_positions = sort_target_position.device_addr,
        .sort_target_pred_positions = sort_target_pred_position.device_addr,
        .sort_target_velocities = sort_target_velocity.device_addr,
    };

    desc_manager.SetUniformData(&uniform_data);
}

void LagueModel::ScheduleUpdateUniforms() {
    update_uniforms = true;
}

void LagueModel::Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) {
    if (update_uniforms) {
        desc_manager.SetUniformData(&uniform_data);
        update_uniforms = false;
    }

    auto push = PushConstants{
        .time = Platform::Info::GetTime(),
        .dt = parameters.time_scale * parameters.fixed_dt / (float)parameters.iterations,
        .n_particles = (uint32_t)parameters.n_particles,

        .positions = buffers.position_buffer.device_addr,
        .velocities = buffers.velocity_buffer.device_addr,
        .densities = buffers.density_buffer.device_addr,
    };

    auto n_groups = glm::ivec3(parameters.n_particles / group_size + 1, 1, 1);

    for (int i = 0; i < parameters.iterations; i++) {
        pipeline.Compute(cmd, KernelExternalForces, n_groups, &push);
        ComputeToComputePipelineBarrier(cmd);

        // RunSpatial
        {
            spatial_hash.Run(ctx, cmd, predicted_positions.device_addr);
            ComputeToComputePipelineBarrier(cmd);

            pipeline.Compute(cmd, KernelReorder, n_groups, &push);
            ComputeToComputePipelineBarrier(cmd);

            pipeline.Compute(cmd, KernelReorderCopyback, n_groups, &push);
            ComputeToComputePipelineBarrier(cmd);
        }

        pipeline.Compute(cmd, KernelCalculateDensities, n_groups, &push);
        ComputeToComputePipelineBarrier(cmd);

        pipeline.Compute(cmd, KernelCalculatePressureForces, n_groups, &push);
        ComputeToComputePipelineBarrier(cmd);

        // pipeline.Compute(cmd, KernelCalculateViscosityForces, n_groups, &comp_consts);
        // ComputeToComputePipelineBarrier(cmd);

        pipeline.Compute(cmd, KernelUpdatePositions, n_groups, &push);
        if (i < parameters.iterations - 1)
            ComputeToComputePipelineBarrier(cmd);
    }
}

void LagueModel::Clear(const gfx::CoreCtx& ctx) {
    predicted_positions.Destroy();

    spatial_hash.Clear(ctx);

    sort_target_position.Destroy();
    sort_target_pred_position.Destroy();
    sort_target_velocity.Destroy();

    buffers.position_buffer.Destroy();
    buffers.velocity_buffer.Destroy();
    buffers.density_buffer.Destroy();

    pipeline.Clear(ctx);
    desc_manager.Clear(ctx);
}

void LagueModel::DrawDebugUI() {
    if (ImGui::CollapsingHeader("Simulation")) {
        glm::vec3 size = uniform_data.box.size;
        if (ImGui::DragFloat3("Bounding box", &uniform_data.box.size.x, 0.1f, 0.5f, 50.0f)) {
            ScheduleUpdateUniforms();
        }

        if (ImGui::SliderFloat("Gravity", &uniform_data.gravity, -25.0f, 25.0f)) {
            ScheduleUpdateUniforms();
        }

        if (ImGui::SliderFloat("Wall damping factor", &uniform_data.damping_factor, 0.0f, 1.0f)) {
            ScheduleUpdateUniforms();
        }

        if (ImGui::SliderFloat("Pressure multiplier", &uniform_data.pressure_multiplier, 0.0f,
                               2000.0f)) {
            ScheduleUpdateUniforms();
        }

        float radius = uniform_data.smoothing_radius;
        if (ImGui::SliderFloat("Smoothing radius", &radius, 0.0f, 0.6f)) {
            SetSmoothingRadius(radius);
            ScheduleUpdateUniforms();
        }

        if (ImGui::SliderFloat("Viscosity", &uniform_data.viscosity_strenght, 0.0f, 1.0f)) {
            ScheduleUpdateUniforms();
        }

        if (ImGui::SliderFloat("Target density", &uniform_data.target_density, 0.0f, 2000.0f)) {
            ScheduleUpdateUniforms();
        }
    }
}

void LagueModel::SetSmoothingRadius(float radius) {
    uniform_data.smoothing_radius = radius;
    uniform_data.spiky_pow3_scale = 15.0f / (glm::pi<float>() * (float)std::pow(radius, 6));
    uniform_data.spiky_pow2_scale = 15.0f / (2.0f * glm::pi<float>() * (float)std::pow(radius, 5));
    uniform_data.spiky_pow3_diff_scale = 45.0f / (glm::pi<float>() * (float)std::pow(radius, 6));
    uniform_data.spiky_pow2_diff_scale = 15.0f / (glm::pi<float>() * (float)std::pow(radius, 5));

    spatial_hash.SetCellSize(radius);
}

}  // namespace vfs
