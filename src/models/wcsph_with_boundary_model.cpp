#include "wcsph_with_boundary_model.h"

#include "imgui.h"
#include "models/model.h"
#include "platform.h"
#include "simulation.h"

namespace vfs {

enum SimKernel : u32 {
    KernelUpdatePositions = 0,
    KernelExternalAccel,
    KernelCalculateDensities,
    KernelCalculatePressureAccel,
    KernelCalculateViscousAccel,
};

WCSPHWithBoundaryModel::WCSPHWithBoundaryModel(
    const std::vector<gfx::DescriptorManager::DescriptorInfo>& desc_info,
    const SPHModel::Parameters* base_par,
    const Parameters* par)
    : SPHModel(base_par) {
    if (par) {
        parameters = *par;
    } else {
        parameters = {
            .wall_stiffness = 1e4,
            .stiffness = 1000,
            .expoent = 7,
            .viscosity_strenght = 0.01,
            .boundary_object = {},
        };
    }

    this->desc_info = desc_info;
}

void WCSPHWithBoundaryModel::Init(const gfx::CoreCtx& ctx) {
    SPHModel::Init(ctx);
    InitBufferReorder(ctx);

    auto& sim = Simulation::Get();
    parameter_id = sim.AddUniformDescriptor(ctx, sizeof(Parameters));

    for (const auto& info : desc_info) {
        sim.AddDescriptorInfo(info);
    }

    sim.InitDescriptorManager(ctx);
    sim.GetDescManager().SetUniformData(parameter_id, &parameters);

    pipeline.Init(ctx, {
                           .push_const_size = sizeof(SPHModel::PushConstants),
                           .set = sim.GetDescManager().Set(),
                           .layout = sim.GetDescManager().Layout(),
                           .shader_path = "shaders/compiled/wcsph_with_boundary_model.slang.spv",
                           .kernels =
                               {
                                   "UpdatePositions",
                                   "ExternalAccel",
                                   "CalculateDensities",
                                   "CalculatePressureAccel",
                                   "CalculateViscousAccel",
                               },
                       });

    UpdateAllUniforms();
}

void WCSPHWithBoundaryModel::Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) {
    SPHModel::Step(ctx, cmd);

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

        pipeline.Compute(cmd, KernelExternalAccel, n_groups, &push);
        ComputeToComputePipelineBarrier(cmd);

        RunSpatialHash(cmd, ctx);
        ComputeToComputePipelineBarrier(cmd);

        pipeline.Compute(cmd, KernelCalculateDensities, n_groups, &push);
        ComputeToComputePipelineBarrier(cmd);

        pipeline.Compute(cmd, KernelCalculatePressureAccel, n_groups, &push);
        ComputeToComputePipelineBarrier(cmd);

        pipeline.Compute(cmd, KernelCalculateViscousAccel, n_groups, &push);
        ComputeToComputePipelineBarrier(cmd);

        pipeline.Compute(cmd, KernelUpdatePositions, n_groups, &push);
    }
}

void WCSPHWithBoundaryModel::DrawDebugUI() {
    SPHModel::DrawDebugUI();

    auto& sim = Simulation::Get();

    if (ImGui::CollapsingHeader("WCSPH model")) {
        if (ImGui::DragFloat("Wall stiffness", &parameters.wall_stiffness, 1e2, 0.0f, 1e6)) {
            sim.GetDescManager().SetUniformData(parameter_id, &parameters);
        }

        if (ImGui::SliderFloat("Stiffness", &parameters.stiffness, 0.0f, 5000.0f)) {
            sim.GetDescManager().SetUniformData(parameter_id, &parameters);
        }

        if (ImGui::DragFloat("Expoent", &parameters.expoent, 1.0f, 0.0f, 10.0f)) {
            sim.GetDescManager().SetUniformData(parameter_id, &parameters);
        }

        if (ImGui::DragFloat("Viscosity", &parameters.viscosity_strenght, 0.001f, 0.0f, 1.0f)) {
            sim.GetDescManager().SetUniformData(parameter_id, &parameters);
        }
    }
}
}  // namespace vfs
