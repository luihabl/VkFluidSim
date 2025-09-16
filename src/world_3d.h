#pragma once

#include "gfx/gfx.h"
#include "platform.h"
#include "renderer.h"

namespace vfs {
class World3D {
public:
    using Event = SDL_Event;

    void Init(Platform& platform);
    void Update(Platform& platform);
    void HandleEvent(Platform& platform, const Event& e);
    void Clear();

private:
    gfx::Device gfx;
    Camera camera;
    SimulationRenderer3D renderer;
    // UI ui;

    int current_frame = 0;
    bool paused{true};

    void ResetSimulation();
    void DrawUI(VkCommandBuffer cmd);
};
}  // namespace vfs
