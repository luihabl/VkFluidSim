#include "world_3d.h"

#include <glm/common.hpp>
#include <glm/ext.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/trigonometric.hpp>

#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_keycode.h"
#include "gfx/common.h"
#include "imgui.h"
#include "platform.h"

namespace vfs {

struct PushConstants3D {
    glm::mat4 view_proj;
    VkDeviceAddress positions;
    VkDeviceAddress velocities;
};

void World3D::Init(Platform& platform) {
    gfx.Init({
        .name = "Vulkan fluid sim 3D",
        .window = platform.GetWindow(),
        .validation_layers = true,
    });

    auto ext = gfx.GetSwapchainExtent();
    renderer.Init(gfx, ext.width, ext.height);

    ui.Init(gfx);

    auto screen_size = Platform::Info::GetScreenSize();
    camera.SetPerspective(90.0f, 0.1f, 100000.0f, (float)screen_size.x / screen_size.y);
    camera.SetPosition(gfx::axis::BACK * 20.0f);
    camera_angles = {0.0f, -90.0f, 0.0f};
}

void World3D::Update(Platform& platform) {
    {
        constexpr float delta = 0.5f;

        auto* keyboard = SDL_GetKeyboardState(nullptr);
        if (keyboard[SDL_SCANCODE_A]) {
            camera_angles.y += delta;
        }
        if (keyboard[SDL_SCANCODE_D]) {
            camera_angles.y -= delta;
        }
        if (keyboard[SDL_SCANCODE_W]) {
            camera_angles.x += delta;
        }
        if (keyboard[SDL_SCANCODE_S]) {
            camera_angles.x -= delta;
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

    auto cmd = gfx.BeginFrame();

    renderer.Draw(gfx, cmd, camera.GetViewProj());
    DrawUI(cmd);

    gfx.EndFrame(cmd, renderer.GetDrawImage());
}

void World3D::DrawUI(VkCommandBuffer cmd) {
    ui.BeginDraw(gfx, cmd, renderer.GetDrawImage());

    if (ImGui::Begin("Control")) {
#define TEXTV3(str, prop)                                     \
    {                                                         \
        auto a = prop;                                        \
        ImGui::Text(str ": %.2f, %.2f, %.2f", a.x, a.y, a.z); \
    }
        auto pos = camera.GetPosition();
        TEXTV3("Camera position", camera.GetPosition());
        TEXTV3("Global up (+y)", gfx::axis::UP);
        TEXTV3("Global left (+x)", gfx::axis::LEFT);
        TEXTV3("Global front (+z)", gfx::axis::FRONT);

        auto angles = glm::eulerAngles(camera.GetRotation());

        ImGui::Text("pitch: %.2f deg", glm::degrees(angles.x));
        ImGui::Text("yaw: %.2f deg", glm::degrees(angles.y));
        ImGui::Text("roll: %.2f deg", glm::degrees(angles.z));
    }

    ImGui::End();

    ui.EndDraw(cmd);
}

void World3D::HandleEvent(Platform& platform, const Event& e) {
    ui.HandleEvent(e);

    if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_Q) {
        platform.ScheduleQuit();
    }
}

void World3D::Clear() {
    vkDeviceWaitIdle(gfx.GetCoreCtx().device);
    ui.Clear(gfx);
    renderer.Clear(gfx);
    gfx.Clear();
}

}  // namespace vfs
