#include "simulation.h"

#include "imgui.h"

namespace vfs {

class SimulationBuilder {
public:
    static Simulation Build() { return {}; }
};

namespace {
Simulation g_simulation{SimulationBuilder::Build()};
}

Simulation& Simulation::Get() {
    return g_simulation;
}
void Simulation::Init() {
    parameters = SimulationParameters{
        .gravity = {0.0f, -9.81f, 0.0f},
        .smooth_radius = 0.2f,
    };

    global_parameter_id = AddDescriptor(sizeof(SimulationParameters));
}

void Simulation::Step(VkCommandBuffer cmd) {
    if (scene) {
        scene->Step(cmd);
        current_step++;
    }
}

void Simulation::Clear(const gfx::CoreCtx& ctx) {
    if (scene)
        scene->Clear();

    desc_manager.Clear(ctx);
}

void Simulation::SetGlobalParameters(const SimulationParameters& p) {
    parameters = p;
    GetDescManager().SetUniformData(global_parameter_id, &parameters);
}
void Simulation::SetScene(std::unique_ptr<SceneBase>&& s) {
    if (scene)
        scene->Clear();
    scene = std::move(s);
    scene->Init();
};
SceneBase* Simulation::GetScene() {
    return scene.get();
};

u32 Simulation::AddDescriptor(u32 size, gfx::DescriptorManager::DescType type) {
    descriptors.push_back({.size = size, .type = type});
    return descriptors.size() - 1;
}

void Simulation::InitDescriptorManager(const gfx::CoreCtx& ctx) {
    desc_manager.Init(ctx, descriptors);
    GetDescManager().SetUniformData(global_parameter_id, &parameters);
}

void Simulation::ClearDescManager(const gfx::CoreCtx& ctx) {
    desc_manager.Clear(ctx);
    descriptors = {};
    global_parameter_id = AddDescriptor(sizeof(SimulationParameters));
}

void Simulation::DrawDebugUI() {
    if (scene) {
        if (ImGui::CollapsingHeader("Simulation")) {
            if (ImGui::DragFloat3("Gravity", &parameters.gravity.x, 0.1f, -20.0f, 20.0f)) {
                GetDescManager().SetUniformData(global_parameter_id, &parameters);
            }

            if (ImGui::DragFloat("Smooth radius", &parameters.smooth_radius, 0.01f, 0.0f, 50.0f)) {
                GetDescManager().SetUniformData(global_parameter_id, &parameters);
            }
        }

        scene->GetModel()->DrawDebugUI();
    }
}

}  // namespace vfs
