#include "wcsph_model.h"

#include "models/model.h"
#include "simulation.h"

namespace vfs {
WCSPHModel::WCSPHModel(const SPHModel::Parameters* base_par, const Parameters* par)
    : SPHModel(base_par) {
    if (par) {
        parameters = *par;
    } else {
        parameters = {
            .wall_damping_factor = 0.05,
            .stiffness = 2000,
            .expoent = 7,
            .viscosity_strenght = 0.03,
        };
    }
}

void WCSPHModel::Init(const gfx::CoreCtx& ctx) {
    SPHModel::Init(ctx);
    InitBufferReorder(ctx);

    auto& sim = Simulation::Get();
    parameter_id = sim.AddDescriptor(sizeof(Parameters));
    sim.InitDescriptorManager(ctx);
    sim.GetDescManager().SetUniformData(parameter_id, &parameters);

    pipeline.Init(ctx, {
                           .push_const_size = sizeof(SPHModel::PushConstants),
                           .set = sim.GetDescManager().Set(),
                           .layout = sim.GetDescManager().Layout(),
                           .shader_path = "shaders/compiled/wcsph_model.slang.spv",
                           .kernels = {},
                       });
}

void WCSPHModel::Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) {}

void WCSPHModel::Clear(const gfx::CoreCtx& ctx) {}

void WCSPHModel::DrawDebugUI() {}
}  // namespace vfs
