#include <cstdio>

#include "SDL3/SDL_events.h"
#include "gfx/gfx.h"
#include "platform.h"

int main() {
    auto platform = vfs::Platform{};
    platform.Init(1600, 800, "Vulkan fluid sim");

    auto gfx = gfx::Device{};
    gfx.Init({.name = "vulkan fluid sim", .validation_layers = true, .window = platform.window});

    platform.Run(
        [](vfs::Platform* platform, SDL_Event* e) {
            if (e->type == SDL_EVENT_KEY_DOWN && e->key.key == SDLK_Q) {
                platform->quit = true;
            }
        },
        [](vfs::Platform* platform) {

        });
    gfx.Clean();
}
