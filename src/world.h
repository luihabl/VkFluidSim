#pragma once

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "gfx/mesh.h"
#include "platform.h"
#include "renderer.h"
#include "simulation.h"
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
    OrthoCamera camera;
    SimulationRenderer2D renderer;
    Simulation2D simulation;

    gfx::GPUMesh particle_mesh;

    UI ui;

    float particle_scale = 1.5e-2;

    int current_frame = 0;
    bool paused{true};
    std::vector<float> gpu_times;
    std::span<u64> gpu_timestamps;

    void ResetSimulation();
    void DrawUI(VkCommandBuffer cmd);
};
}  // namespace vfs
