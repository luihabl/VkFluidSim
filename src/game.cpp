#include "game.h"

namespace vfs {
void Game::Init(Platform& platform) {
    gfx.Init({
        .name = "Vulkan fluid sim",
        .window = platform.window,
        .validation_layers = true,
    });
}

void Game::HandleEvent(Platform& platform, const SDL_Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_Q) {
        platform.quit = true;
    }
}

void Game::Update(Platform& platform) {}

void Game::Clean() {
    gfx.Clean();
}

}  // namespace vfs