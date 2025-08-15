#include "world.h"

namespace vfs {
void World::Init(Platform& platform) {
    gfx.Init({
        .name = "Vulkan fluid sim",
        .window = platform.window,
        .validation_layers = true,
    });
}

void World::HandleEvent(Platform& platform, const SDL_Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_Q) {
        platform.quit = true;
    }
}

void World::Update(Platform& platform) {}

void World::Clean() {
    gfx.Clean();
}

}  // namespace vfs