#include <cstdio>

#include "SDL3/SDL_events.h"
#include "game.h"
#include "gfx/gfx.h"
#include "platform.h"

int main() {
    auto platform = vfs::Platform{};

    auto game = vfs::Game{};

    platform.Init({
        .name = "Vulkan fluid sim",
        .w = 1600,
        .h = 800,
        .init = [&game](vfs::Platform& p) { game.Init(p); },
        .update = [&game](vfs::Platform& p) { game.Update(p); },
        .clean = [&game](vfs::Platform& p) { game.Clean(); },
        .handler = [&game](vfs::Platform& p, SDL_Event& e) { game.HandleEvent(p, e); },
    });

    platform.Run();
}
