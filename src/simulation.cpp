#include "simulation.h"

namespace vfs {
namespace {
Simulation g_simulation;
}

Simulation& Simulation::Get() {
    return g_simulation;
}
void Simulation::Init() {
    parameters = SimulationParameters{
        .gravity = {0.0f, -9.81f, 0.0f},
        .smooth_radius = 0.2f,
    };
}

void Simulation::Step(VkCommandBuffer cmd) {
    if (scene) {
        scene->Step(cmd);
        current_step++;
    }
}

void Simulation::Clear() {
    if (scene)
        scene->Clear();
}

void Simulation::SetOnParametersChangedCallback(std::function<void()>&& callback) {
    on_parameters_changed = std::move(callback);
}
void Simulation::SetGlobalParameters(const SimulationParameters& p) {
    parameters = p;
    if (on_parameters_changed)
        on_parameters_changed();
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
}  // namespace vfs
