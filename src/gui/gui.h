#pragma once

#include <glm/ext/scalar_constants.hpp>
#include <memory>

#include "gfx/gfx.h"
#include "gui/common.h"
#include "gui/imgui_setup.h"
#include "imgui_setup.h"
#include "platform.h"
#include "scenes/scene.h"
#include "simulation_renderer.h"

namespace vfs {
class GUI {
public:
    using Event = SDL_Event;

    void Init(Platform& platform);
    void Update(Platform& platform);
    void HandleEvent(Platform& platform, const Event& e);
    void Clear();

private:
    gfx::Device gfx;

    OrbitCamera camera;
    bool orbit_move{false};
    glm::vec2 initial_mouse_pos{0.0f};
    glm::vec3 last_camera_angles{0.0f};

    SimulationRenderer3D renderer;

    std::unique_ptr<SceneBase> scene;

    ImGui_Setup ui;

    int current_frame = 0;
    bool paused{true};

    void SetCameraPosition();
    void DrawUI(VkCommandBuffer cmd);
};
}  // namespace vfs
