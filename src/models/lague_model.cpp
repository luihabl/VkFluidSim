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

struct LagueModelBuffers {
    VkDeviceAddress predicted_positions;
    VkDeviceAddress sort_target_positions;
    VkDeviceAddress sort_target_pred_positions;
    VkDeviceAddress sort_target_velocities;
};

LagueModel::LagueModel(const SPHModel::Parameters* base_par, const Parameters* par)
    : SPHModel(base_par) {
    if (par) {
        parameters = *par;
    } else {
        parameters = {
            .wall_damping_factor = 0.05f,
            .pressure_multiplier = 288.0f,
            .near_pressure_multiplier = 2.16f,
            .viscosity_strenght = 0.03f,
        };
    }
}

void LagueModel::Init(const gfx::CoreCtx& ctx) {
    SPHModel::Init(ctx);

    predicted_positions = CreateDataBuffer<glm::vec3>(ctx, SPHModel::parameters.n_particles);

    sort_target_position = CreateDataBuffer<glm::vec3>(ctx, SPHModel::parameters.n_particles);
    sort_target_pred_position = CreateDataBuffer<glm::vec3>(ctx, SPHModel::parameters.n_particles);
    sort_target_velocity = CreateDataBuffer<glm::vec3>(ctx, SPHModel::parameters.n_particles);

    parameter_id = AddDescriptor(sizeof(Parameters));
    buf_id = AddDescriptor(sizeof(LagueModelBuffers));

    InitDescriptorManager(ctx);

    pipeline.Init(ctx, {
                           .push_const_size = sizeof(SPHModel::PushConstants),
                           .set = GetDescManager().Set(),
                           .layout = GetDescManager().Layout(),
                           .shader_path = "shaders/compiled/lague_model.slang.spv",
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

    UpdateAllUniforms();
}

void LagueModel::UpdateAllUniforms() {
    SPHModel::UpdateAllUniforms();

    GetDescManager().SetUniformData(parameter_id, &parameters);

    auto model_bufs = LagueModelBuffers{
        .predicted_positions = predicted_positions.device_addr,
        .sort_target_positions = sort_target_position.device_addr,
        .sort_target_pred_positions = sort_target_pred_position.device_addr,
        .sort_target_velocities = sort_target_velocity.device_addr,
    };

    GetDescManager().SetUniformData(buf_id, &model_bufs);
}

void LagueModel::ScheduleUpdateUniforms() {
    update_uniforms = true;
}

void LagueModel::Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) {
    SPHModel::Step(ctx, cmd);

    if (update_uniforms) {
        UpdateAllUniforms();
        update_uniforms = false;
    }

    auto push = PushConstants{
        .time = Platform::Info::GetTime(),
        .dt = SPHModel::parameters.time_scale * SPHModel::parameters.fixed_dt /
              (float)SPHModel::parameters.iterations,
        .n_particles = (uint32_t)SPHModel::parameters.n_particles,

        .positions = buffers.position_buffer.device_addr,
        .velocities = buffers.velocity_buffer.device_addr,
        .densities = buffers.density_buffer.device_addr,
    };

    auto n_groups = glm::ivec3(SPHModel::parameters.n_particles / group_size + 1, 1, 1);

    for (int i = 0; i < SPHModel::parameters.iterations; i++) {
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
        if (i < SPHModel::parameters.iterations - 1)
            ComputeToComputePipelineBarrier(cmd);
    }
}

void LagueModel::Clear(const gfx::CoreCtx& ctx) {
    SPHModel::Clear(ctx);

    predicted_positions.Destroy();

    sort_target_position.Destroy();
    sort_target_pred_position.Destroy();
    sort_target_velocity.Destroy();

    buffers.position_buffer.Destroy();
    buffers.velocity_buffer.Destroy();
    buffers.density_buffer.Destroy();

    pipeline.Clear(ctx);
}

void LagueModel::DrawDebugUI() {
    // if (ImGui::CollapsingHeader("Simulation")) {
    //     glm::vec3 size = uniform_data.box.size;
    //     if (ImGui::DragFloat3("Bounding box", &uniform_data.box.size.x, 0.1f, 0.5f, 50.0f)) {
    //         ScheduleUpdateUniforms();
    //     }

    //     if (ImGui::SliderFloat("Gravity", &uniform_data.gravity, -25.0f, 25.0f)) {
    //         ScheduleUpdateUniforms();
    //     }

    //     if (ImGui::SliderFloat("Wall damping factor", &uniform_data.damping_factor, 0.0f, 1.0f))
    //     {
    //         ScheduleUpdateUniforms();
    //     }

    //     if (ImGui::SliderFloat("Pressure multiplier", &uniform_data.pressure_multiplier, 0.0f,
    //                            2000.0f)) {
    //         ScheduleUpdateUniforms();
    //     }

    //     float radius = uniform_data.smoothing_radius;
    //     if (ImGui::SliderFloat("Smoothing radius", &radius, 0.0f, 0.6f)) {
    //         SetSmoothingRadius(radius);
    //         ScheduleUpdateUniforms();
    //     }

    //     if (ImGui::SliderFloat("Viscosity", &uniform_data.viscosity_strenght, 0.0f, 1.0f)) {
    //         ScheduleUpdateUniforms();
    //     }

    //     if (ImGui::SliderFloat("Target density", &uniform_data.target_density, 0.0f, 2000.0f)) {
    //         ScheduleUpdateUniforms();
    //     }
    // }
}

}  // namespace vfs
