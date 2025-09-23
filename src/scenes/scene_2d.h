#pragma once

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "models/lague_model_2d.h"
#include "platform.h"
#include "renderers/renderer_2d.h"
#include "ui.h"

namespace vfs {
class World {
public:
    using Event = SDL_Event;

    void Init(Platform& platform);
    void Update(Platform& platform);
    void HandleEvent(Platform& platform, const Event& e);
    void Clear();

private:
    gfx::Device gfx;

    Camera camera;
    SimulationRenderer2D renderer;
    Simulation2D simulation;

    UI ui;

    int current_frame = 0;
    bool paused{true};
    std::vector<float> gpu_times;
    std::span<u64> gpu_timestamps;

    void ResetSimulation();
    void DrawUI(VkCommandBuffer cmd);
};
}  // namespace vfs
