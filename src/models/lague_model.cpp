#include "lague_model.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/glm.hpp>

#include "imgui.h"
#include "models/model.h"
#include "platform.h"
#include "simulation.h"

namespace vfs {
enum SimKernel : u32 {
    KernelUpdatePositions = 0,
    KernelExternalForces,
    KernelCalculateDensities,
    KernelCalculatePressureForces,
};

struct LagueModelBuffers {
    VkDeviceAddress predicted_positions;
    VkDeviceAddress near_density;
};

LagueModel::LagueModel(const SPHModel::Parameters* base_par, const Parameters* par)
    : SPHModel(base_par) {
    if (par) {
        parameters = *par;
    } else {
        parameters = {
            .wall_damping_factor = 0.95f,
            .pressure_multiplier = 288.0f,
            .near_pressure_multiplier = 2.16f,
            .viscosity_strenght = 0.03f,
        };
    }
}

void LagueModel::Init(const gfx::CoreCtx& ctx) {
    SPHModel::Init(ctx);

    near_density = CreateDataBuffer<float>(ctx, SPHModel::parameters.n_particles);
    predicted_positions = CreateDataBuffer<glm::vec3>(ctx, SPHModel::parameters.n_particles);
    AddBufferToBeReordered(predicted_positions);
    InitBufferReorder(ctx);

    auto& sim = Simulation::Get();

    parameter_id = sim.AddDescriptor(sizeof(Parameters));
    buf_id = sim.AddDescriptor(sizeof(LagueModelBuffers));

    sim.InitDescriptorManager(ctx);

    pipeline.Init(ctx, {
                           .push_const_size = sizeof(SPHModel::PushConstants),
                           .set = sim.GetDescManager().Set(),
                           .layout = sim.GetDescManager().Layout(),
                           .shader_path = "shaders/compiled/lague_model.slang.spv",
                           .kernels =
                               {
                                   "UpdatePositions",
                                   "ExternalForces",
                                   "CalculateDensities",
                                   "CalculatePressureForces",
                               },
                       });

    UpdateAllUniforms();
}

void LagueModel::UpdateAllUniforms() {
    SPHModel::UpdateAllUniforms();

    Simulation::Get().GetDescManager().SetUniformData(parameter_id, &parameters);

    auto lague_model_bufs = LagueModelBuffers{
        .predicted_positions = predicted_positions.device_addr,
        .near_density = near_density.device_addr,
    };

    Simulation::Get().GetDescManager().SetUniformData(buf_id, &lague_model_bufs);
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
    };

    auto n_groups = glm::ivec3(SPHModel::parameters.n_particles / group_size + 1, 1, 1);

    for (int i = 0; i < SPHModel::parameters.iterations; i++) {
        if (i > 0)
            ComputeToComputePipelineBarrier(cmd);

        pipeline.Compute(cmd, KernelExternalForces, n_groups, &push);
        ComputeToComputePipelineBarrier(cmd);

        RunSpatialHash(cmd, ctx, &predicted_positions);
        ComputeToComputePipelineBarrier(cmd);

        pipeline.Compute(cmd, KernelCalculateDensities, n_groups, &push);
        ComputeToComputePipelineBarrier(cmd);

        pipeline.Compute(cmd, KernelCalculatePressureForces, n_groups, &push);
        ComputeToComputePipelineBarrier(cmd);

        // pipeline.Compute(cmd, KernelCalculateViscosityForces, n_groups, &comp_consts);
        // ComputeToComputePipelineBarrier(cmd);

        pipeline.Compute(cmd, KernelUpdatePositions, n_groups, &push);
    }
}

void LagueModel::Clear(const gfx::CoreCtx& ctx) {
    SPHModel::Clear(ctx);
    predicted_positions.Destroy();
    near_density.Destroy();
}

void LagueModel::DrawDebugUI() {
    SPHModel::DrawDebugUI();

    auto& sim = Simulation::Get();

    if (ImGui::CollapsingHeader("Lague model")) {
        if (ImGui::SliderFloat("Wall damping factor", &parameters.wall_damping_factor, 0.0f,
                               1.0f)) {
            sim.GetDescManager().SetUniformData(parameter_id, &parameters);
        }

        if (ImGui::SliderFloat("Pressure multiplier", &parameters.pressure_multiplier, 0.0f,
                               2000.0f)) {
            sim.GetDescManager().SetUniformData(parameter_id, &parameters);
        }

        if (ImGui::SliderFloat("Near pressure multiplier", &parameters.near_pressure_multiplier,
                               0.0f, 2000.0f)) {
            sim.GetDescManager().SetUniformData(parameter_id, &parameters);
        }

        if (ImGui::SliderFloat("Viscosity", &parameters.viscosity_strenght, 0.0f, 1.0f)) {
            sim.GetDescManager().SetUniformData(parameter_id, &parameters);
        }
    }
}

}  // namespace vfs
