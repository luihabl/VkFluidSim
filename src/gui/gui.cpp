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
#include "models/lague_model.h"
#include "platform.h"

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

    simulation = std::make_unique<LagueModel>();
    simulation->Init(gfx.GetCoreCtx());

    auto ext = gfx.GetSwapchainExtent();
    renderer.Init(gfx, simulation.get(), ext.width, ext.height);

    ui.Init(gfx);

    auto screen_size = Platform::Info::GetScreenSize();
    camera.SetPerspective(glm::radians(75.0f), 0.1f, 1000.0f, (float)screen_size.x / screen_size.y);
    camera.SetInverseDepth(false);

    camera.SetPosition(gfx::axis::BACK * 20.0f);
    camera_angles = {0.0f, -90.0f, 0.0f};
    last_camera_angles = camera_angles;

    camera_radius = 25.0f;

    ResetSimulation();
}

void GUI::ResetSimulation() {
    const auto box = simulation->GetBoundingBox().value();
    auto size = box.size;
    size.x /= 5.0f;

    auto pos = box.pos;  // + (box.size - size) / 2.0f;
    simulation->SetParticlesInBox(gfx, {.size = size, .pos = pos});
}

void GUI::SetCameraPosition() {
    constexpr float delta = 0.5f;

    if (orbit_move) {
        glm::vec2 current_mouse_pos;
        SDL_GetMouseState(&current_mouse_pos.x, &current_mouse_pos.y);
        auto mouse_diff = current_mouse_pos - initial_mouse_pos;
        mouse_diff = glm::vec2(mouse_diff.y, mouse_diff.x);
        camera_angles = last_camera_angles + glm::vec3(mouse_diff * delta, 0.0f);
    }

    camera_angles.x = glm::clamp(camera_angles.x, -89.f, 89.f);

    auto pos = camera.GetPosition();

    float pitch = glm::radians(camera_angles.x);
    float yaw = glm::radians(camera_angles.y);
    float roll = glm::radians(camera_angles.z);

    glm::vec3 camera_pos;
    camera_pos.x = cos(yaw) * cos(pitch);
    camera_pos.y = sin(pitch);
    camera_pos.z = sin(yaw) * cos(pitch);

    camera.SetPosition(camera_pos * camera_radius);
    camera.SetTarget({0.0f, 0.0f, 0.0f});
}

void GUI::Update(Platform& platform) {
    SetCameraPosition();

    auto cmd = gfx.BeginFrame();

    if (!paused) {
        simulation->Step(gfx.GetCoreCtx(), cmd);
    }
    renderer.Draw(gfx, cmd, simulation.get(), camera);
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
            ResetSimulation();
            paused = true;
        }

        simulation->DrawDebugUI();

        // if (ImGui::CollapsingHeader("Simulation")) {
        //     glm::vec3 size = simulation.GetUniformData().box.size;
        //     if (ImGui::DragFloat3("Bounding box", &size.x, 0.1f, 0.5f, 50.0f)) {
        //         simulation.GetUniformData().box.size = size;
        //         simulation.ScheduleUpdateUniforms();
        //     }

        //     if (ImGui::SliderFloat("Gravity", &simulation.GetUniformData().gravity, -25.0f,
        //                            25.0f)) {
        //         simulation.ScheduleUpdateUniforms();
        //     }

        //     if (ImGui::SliderFloat("Wall damping factor",
        //                            &simulation.GetUniformData().damping_factor, 0.0f, 1.0f)) {
        //         simulation.ScheduleUpdateUniforms();
        //     }

        //     if (ImGui::SliderFloat("Pressure multiplier",
        //                            &simulation.GetUniformData().pressure_multiplier, 0.0f,
        //                            2000.0f)) {
        //         simulation.ScheduleUpdateUniforms();
        //     }

        //     float radius = simulation.GetUniformData().smoothing_radius;
        //     if (ImGui::SliderFloat("Smoothing radius", &radius, 0.0f, 0.6f)) {
        //         simulation.SetSmoothingRadius(radius);
        //         simulation.ScheduleUpdateUniforms();
        //     }

        //     if (ImGui::SliderFloat("Viscosity", &simulation.GetUniformData().viscosity_strenght,
        //                            0.0f, 1.0f)) {
        //         simulation.ScheduleUpdateUniforms();
        //     }

        //     if (ImGui::SliderFloat("Target density", &simulation.GetUniformData().target_density,
        //                            0.0f, 2000.0f)) {
        //         simulation.ScheduleUpdateUniforms();
        //     }
        // }

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

            ImGui::DragFloat("Orbit radius", &camera_radius, 0.1f, 0.0f, 90.0f);
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
        last_camera_angles = camera_angles;
    }

    if (e.type == SDL_EVENT_MOUSE_WHEEL && !io.WantCaptureMouse) {
        float amount = e.wheel.y;
        camera_radius += amount * 0.4f;
        camera_radius = glm::clamp(camera_radius, 2.0f, 10000.0f);
    }
}

void GUI::Clear() {
    vkDeviceWaitIdle(gfx.GetCoreCtx().device);
    ui.Clear(gfx);
    renderer.Clear(gfx);
    simulation->Clear(gfx.GetCoreCtx());
    gfx.Clear();
}

}  // namespace vfs
