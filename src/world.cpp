#include "world.h"

namespace vfs {
void World::Init(Platform& platform) {
    gfx.Init({
        .name = "Vulkan fluid sim",
        .window = platform.window,
        .validation_layers = true,
    });

    auto ext = gfx.GetSwapchainExtent();
    renderer.Init(gfx, ext.width, ext.height);
}

void World::HandleEvent(Platform& platform, const SDL_Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_Q) {
        platform.quit = true;
    }
}

void World::Update(Platform& platform) {
    static int i = 0;

    auto cmd = gfx.BeginFrame();

    renderer.Draw(gfx, cmd);

    gfx.EndFrame(cmd, renderer.GetDrawImage());
}

void World::Clean() {
    vkDeviceWaitIdle(gfx.GetCoreCtx().device);
    renderer.Clean(gfx);
    gfx.Clean();
}

}  // namespace vfs