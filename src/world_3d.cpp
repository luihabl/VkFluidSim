#include "world_3d.h"

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
    renderer.GetTransform().SetScale({10.0f, 10.0f, 1.0f});
}

void World3D::Update(Platform& platform) {
    auto cmd = gfx.BeginFrame();

    renderer.Draw(gfx, cmd, camera.GetViewProj());

    gfx.EndFrame(cmd, renderer.GetDrawImage());
}

void World3D::HandleEvent(Platform& platform, const Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_Q) {
        platform.ScheduleQuit();
    }

    if (e.type == SDL_EVENT_KEY_DOWN) {
        glm::vec3 offset{0.0f};
        constexpr float delta = 0.5f;
        if (e.key.key == SDLK_A) {
            offset.x += delta;
        }

        if (e.key.key == SDLK_D) {
            offset.x -= delta;
        }

        if (e.key.key == SDLK_W) {
            offset.y += delta;
        }

        if (e.key.key == SDLK_S) {
            offset.y -= delta;
        }

        auto pos = camera.GetPosition() + offset;
        camera.SetPosition(pos);
    }
}

void World3D::Clear() {
    vkDeviceWaitIdle(gfx.GetCoreCtx().device);
    renderer.Clear(gfx);
    gfx.Clear();
}

}  // namespace vfs
