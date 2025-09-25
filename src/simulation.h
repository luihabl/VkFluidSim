#pragma once

#include <memory>

#include "scenes/scene.h"

namespace vfs {

struct SimulationParameters {
    glm::vec3 gravity;
    float smooth_radius;
};

class SimulationBuilder;

class Simulation {
public:
    friend SimulationBuilder;

    void Init();
    void Step(VkCommandBuffer cmd);
    void Clear();

    static Simulation& Get();

    const SimulationParameters& GetGlobalParameters() { return parameters; }
    void SetGlobalParameters(const SimulationParameters& p);
    void SetOnParametersChangedCallback(std::function<void()>&& callback);

    void SetScene(std::unique_ptr<SceneBase>&& scene);
    SceneBase* GetScene();

private:
    Simulation() = default;
    Simulation(const Simulation&) = delete;
    Simulation(Simulation&&) = delete;
    Simulation& operator=(const Simulation&) = delete;
    Simulation& operator=(Simulation&&) = delete;

    std::function<void()> on_parameters_changed;
    SimulationParameters parameters;
    std::unique_ptr<SceneBase> scene;

    float current_time{0.0f};
    u32 current_step{0};
};
}  // namespace vfs
