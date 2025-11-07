#include "simulation.h"

#include "gfx/descriptor.h"
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
void Simulation::Init(const gfx::CoreCtx& ctx) {
    parameters = SimulationParameters{
        .gravity = {0.0f, -9.81f, 0.0f},
        .smooth_radius = 0.2f,
    };

    ClearDescManager(ctx);
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

    for (auto& d : descriptors) {
        d.buffer.data_buffer.Destroy();
    }

    desc_manager.Clear(ctx);
}

void Simulation::SetGlobalParameters(const SimulationParameters& p) {
    parameters = p;
    if (desc_manager.desc_set != VK_NULL_HANDLE)
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

u32 Simulation::AddUniformDescriptor(const gfx::CoreCtx& ctx, u32 size) {
    descriptors.push_back(gfx::DescriptorManager::CreateUniformInfo(ctx, size));
    return descriptors.size() - 1;
}

u32 Simulation::AddDescriptorInfo(gfx::DescriptorManager::DescriptorInfo&& info) {
    descriptors.push_back(std::move(info));
    return descriptors.size() - 1;
}

void Simulation::InitDescriptorManager(const gfx::CoreCtx& ctx) {
    desc_manager.Init(ctx, descriptors);
    GetDescManager().SetUniformData(global_parameter_id, &parameters);
}

void Simulation::ClearDescManager(const gfx::CoreCtx& ctx) {
    if (desc_manager.desc_set != VK_NULL_HANDLE)
        desc_manager.Clear(ctx);

    for (auto& d : descriptors) {
        d.buffer.data_buffer.Destroy();
    }

    descriptors = {};

    global_parameter_id = AddUniformDescriptor(ctx, sizeof(SimulationParameters));
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

        scene->DrawDebugUI();
    }
}

}  // namespace vfs
