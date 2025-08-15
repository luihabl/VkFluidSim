#include <cstdio>

#include "SDL3/SDL_events.h"
#include "gfx/gfx.h"
#include "platform.h"
#include "world.h"

int main() {
    auto platform = vfs::Platform{};

    auto world = vfs::World{};

    platform.Init({
        .name = "Vulkan fluid sim",
        .w = 1600,
        .h = 800,
        .init = [&world](vfs::Platform& p) { world.Init(p); },
        .update = [&world](vfs::Platform& p) { world.Update(p); },
        .clean = [&world](vfs::Platform& p) { world.Clean(); },
        .handler = [&world](vfs::Platform& p, SDL_Event& e) { world.HandleEvent(p, e); },
    });

    platform.Run();
}
