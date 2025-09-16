#include "world_3d.h"

#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_keycode.h"
#include "gfx/common.h"
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
    
    auto screen_size = Platform::Info::GetScreenSize();
    camera.SetPerspective(90.0f, 0.1f, 100000.0f, (float)screen_size.x / screen_size.y);
    camera.SetPosition(gfx::axis::BACK * 20.0f);
    
    
}

void World3D::Update(Platform& platform) {
    {
        constexpr float delta = 0.5f;
        glm::vec3 offset{0.0f};

        auto* keyboard = SDL_GetKeyboardState(nullptr);
        if (keyboard[SDL_SCANCODE_A]) {
            offset.x += delta;
        }
        if (keyboard[SDL_SCANCODE_D]) {
            offset.x -= delta;
        }
        if (keyboard[SDL_SCANCODE_W]) {
            offset.y += delta;
        }
        if (keyboard[SDL_SCANCODE_S]) {
            offset.y -= delta;
        }

        auto pos = camera.GetPosition() + offset;
        camera.SetPosition(pos);
        camera.SetTarget({0.0f, 0.0f, 0.0f});
    }

    auto cmd = gfx.BeginFrame();

    renderer.Draw(gfx, cmd, camera.GetViewProj());

    gfx.EndFrame(cmd, renderer.GetDrawImage());
}

void World3D::HandleEvent(Platform& platform, const Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_Q) {
        platform.ScheduleQuit();
    }
}

void World3D::Clear() {
    vkDeviceWaitIdle(gfx.GetCoreCtx().device);
    renderer.Clear(gfx);
    gfx.Clear();
}

}  // namespace vfs
