#include "platform.h"
#include "world.h"

int main() {
    auto platform = vfs::Platform{};

    auto world = vfs::World{};

    platform.Init({
        .name = "Vulkan fluid sim",
        .w = 1600,
        .h = 800,
        .init = [&world](auto& p) { world.Init(p); },
        .update = [&world](auto& p) { world.Update(p); },
        .clean = [&world](auto& p) { world.Clean(); },
        .handler = [&world](auto& p, auto& e) { world.HandleEvent(p, e); },
    });

    platform.Run();
}
