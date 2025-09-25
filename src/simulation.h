#pragma once

#include <memory>

#include "scenes/scene.h"

namespace vfs {

struct SimulationParameters {
    glm::vec3 gravity;
    float smooth_radius;
};

class Simulation {
public:
    void Init();
    void Step(VkCommandBuffer cmd);
    void Clear();

    static Simulation& Get();

    const SimulationParameters& GlobalParameters() { return parameters; }
    void SetGlobalParameters(const SimulationParameters& p);
    void SetOnParametersChangedCallback(std::function<void()>&& callback);

    void SetScene(std::unique_ptr<SceneBase>&& scene);
    SceneBase* GetScene();

private:
    std::function<void()> on_parameters_changed;
    SimulationParameters parameters;
    std::unique_ptr<SceneBase> scene;

    float current_time{0.0f};
    u32 current_step{0};
};
}  // namespace vfs
