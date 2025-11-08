#pragma once

#include <memory>

#include "gfx/descriptor.h"
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

    void Init(const gfx::CoreCtx& ctx);
    void Step(VkCommandBuffer cmd);
    void Clear(const gfx::CoreCtx& ctx);

    static Simulation& Get();

    const SimulationParameters& GetGlobalParameters() { return parameters; }
    void SetGlobalParameters(const SimulationParameters& p);

    void SetScene(std::unique_ptr<SceneBase>&& scene);
    SceneBase* GetScene();
    void DrawDebugUI();

    u32 AddUniformDescriptor(const gfx::CoreCtx& ctx, u32 size);
    u32 AddDescriptorInfo(const gfx::DescriptorManager::DescriptorInfo& info);

    void InitDescriptorManager(const gfx::CoreCtx& ctx);
    const gfx::DescriptorManager& GetDescManager() { return desc_manager; }
    void ClearDescManager(const gfx::CoreCtx& ctx);

private:
    Simulation() = default;
    Simulation(const Simulation&) = delete;
    Simulation(Simulation&&) = delete;
    Simulation& operator=(const Simulation&) = delete;
    Simulation& operator=(Simulation&&) = delete;

    SimulationParameters parameters;
    std::unique_ptr<SceneBase> scene;

    u32 global_parameter_id;
    gfx::DescriptorManager desc_manager;
    gfx::DescriptorManager::DescriptorInfo global_parameter_info;
    std::vector<gfx::DescriptorManager::DescriptorInfo> descriptors;

    float current_time{0.0f};
    u32 current_step{0};
};
}  // namespace vfs
