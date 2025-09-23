#pragma once

#include <glm/ext/scalar_constants.hpp>

#include "gfx/gfx.h"
#include "models/model.h"
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
    bool orbit_move{false};
    glm::vec2 initial_mouse_pos{0.0f};
    glm::vec3 last_camera_angles{0.0f};
    glm::vec3 camera_angles{0.0f};
    float camera_radius{0.0f};

    SimulationRenderer3D renderer;
    std::unique_ptr<SPHModel> simulation;

    UI ui;

    int current_frame = 0;
    bool paused{true};

    void ResetSimulation();
    void SetCameraPosition();
    void DrawUI(VkCommandBuffer cmd);
};
}  // namespace vfs
