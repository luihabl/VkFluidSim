#pragma once

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "platform.h"
#include "renderer.h"
#include "ui.h"

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
    // UI ui;

    int current_frame = 0;
    bool paused{true};

    void ResetSimulation();
    void DrawUI(VkCommandBuffer cmd);

    // === Temporary ===

    // Pipeline
    VkPipeline pipeline;
    VkPipelineLayout layout;

    // Renderer
    Transform transform;
    gfx::Image draw_img;
    gfx::Image depth_img;
    glm::vec4 clear_color;
};
}  // namespace vfs
