#include "gui.h"

#include <glm/common.hpp>
#include <glm/ext.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/trigonometric.hpp>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_mouse.h"
#include "gfx/common.h"
#include "imgui.h"
#include "platform.h"
#include "scenes/dam_break_scene.h"

namespace vfs {

struct PushConstants3D {
    glm::mat4 view_proj;
    VkDeviceAddress positions;
    VkDeviceAddress velocities;
};

void GUI::Init(Platform& platform) {
    gfx.Init({
        .name = "Vulkan fluid sim 3D",
        .window = platform.GetWindow(),
        .validation_layers = true,
    });

    scene = std::make_unique<DamBreakScene>(gfx);
    scene->Init();

    auto ext = gfx.GetSwapchainExtent();
    renderer.Init(gfx, scene->GetModel(), ext.width, ext.height);

    ui.Init(gfx);

    auto screen_size = Platform::Info::GetScreenSize();
    camera.SetPerspective(glm::radians(75.0f), 0.1f, 1000.0f, (float)screen_size.x / screen_size.y);
    camera.SetInverseDepth(false);

    camera.SetRadius(25.0f);
    camera.SetAngles({0.0f, -90.0f, 0.0f});
    last_camera_angles = camera.GetAngles();
}

void GUI::SetCameraPosition() {
    constexpr float delta = 0.5f;

    if (orbit_move) {
        glm::vec2 current_mouse_pos;
        SDL_GetMouseState(&current_mouse_pos.x, &current_mouse_pos.y);
        auto mouse_diff = current_mouse_pos - initial_mouse_pos;
        mouse_diff = glm::vec2(mouse_diff.y, mouse_diff.x);
        camera.SetAngles(last_camera_angles + glm::vec3(mouse_diff * delta, 0.0f));
    }
}

void GUI::Update(Platform& platform) {
    SetCameraPosition();

    auto cmd = gfx.BeginFrame();

    if (!paused) {
        scene->Step(cmd);
    }
    renderer.Draw(gfx, cmd, scene->GetModel(), camera);
    DrawUI(cmd);

    gfx.EndFrame(cmd, renderer.GetDrawImage());
}

void GUI::DrawUI(VkCommandBuffer cmd) {
    ui.BeginDraw(gfx, cmd, renderer.GetDrawImage());

#define TEXTV3(str, prop)                                     \
    {                                                         \
        auto a = prop;                                        \
        ImGui::Text(str ": %.2f, %.2f, %.2f", a.x, a.y, a.z); \
    }

    if (ImGui::Begin("Control")) {
        if (paused) {
            if (ImGui::Button("Continue")) {
                paused = false;
            }
        } else {
            if (ImGui::Button("Pause")) {
                paused = true;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset")) {
            scene->Reset();
            paused = true;
        }

        // simulation->DrawDebugUI();

        if (ImGui::CollapsingHeader("Camera")) {
            auto pos = camera.GetPosition();
            TEXTV3("Position", camera.GetPosition());
            // TEXTV3("Global up (+y)", gfx::axis::UP);
            // TEXTV3("Global left (+x)", gfx::axis::LEFT);
            // TEXTV3("Global front (+z)", gfx::axis::FRONT);

            auto angles = glm::eulerAngles(camera.GetRotation());
            ImGui::Text("pitch: %.2f deg", glm::degrees(angles.x));
            ImGui::Text("yaw: %.2f deg", glm::degrees(angles.y));
            ImGui::Text("roll: %.2f deg", glm::degrees(angles.z));

            float fovx = glm::degrees(camera.GetFoV().x);
            if (ImGui::DragFloat("FoV (x)", &fovx, 0.1f, 15.0f, 130.0f)) {
                camera.SetFoVX(glm::radians(fovx));
            }

            float radius = camera.GetRadius();
            if (ImGui::DragFloat("Orbit radius", &radius, 0.1f, 0.0f, 90.0f)) {
                camera.SetRadius(radius);
            }
        }
    }

    ImGui::End();

    ui.EndDraw(cmd);
}

void GUI::HandleEvent(Platform& platform, const Event& e) {
    ui.HandleEvent(e);

    auto& io = ImGui::GetIO();

    if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_Q) {
        platform.ScheduleQuit();
    }

    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT &&
        !io.WantCaptureMouse) {
        orbit_move = true;

        SDL_GetMouseState(&initial_mouse_pos.x, &initial_mouse_pos.y);
    }

    if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT && orbit_move) {
        orbit_move = false;
        last_camera_angles = camera.GetAngles();
    }

    if (e.type == SDL_EVENT_MOUSE_WHEEL && !io.WantCaptureMouse) {
        float amount = e.wheel.y;

        float camera_radius = camera.GetRadius();
        camera_radius += amount * 0.4f;
        camera_radius = glm::clamp(camera_radius, 2.0f, 10000.0f);
        camera.SetRadius(camera_radius);
    }
}

void GUI::Clear() {
    vkDeviceWaitIdle(gfx.GetCoreCtx().device);
    ui.Clear(gfx);
    renderer.Clear(gfx);
    scene->Clear();
    gfx.Clear();
}

}  // namespace vfs
