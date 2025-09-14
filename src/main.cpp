#include "platform.h"
#include "world.h"

std::filesystem::path GetResourcesFolder(int argc, char* argv[]) {
    if (argc > 1) {
        return argv[1];
    }

    if (const char* env = std::getenv("VK_FLUID_SIM_RESOURCES_PATH")) {
        return env;
    }

    auto p = std::filesystem::path(argv[0]);
    return p.parent_path() / "assets";
}

int main(int argc, char* argv[]) {
    using namespace vfs;

    auto platform = vfs::Platform{};

    auto world = vfs::World{};

    platform.Init({
        .name = "Vulkan fluid sim",
        .size = {1200, 700},
        .init = [&world](auto& p) { world.Init(p); },
        .update = [&world](auto& p) { world.Update(p); },
        .clean = [&world](auto& p) { world.Clear(); },
        .handler = [&world](auto& p, auto& e) { world.HandleEvent(p, e); },
        .resources_path = GetResourcesFolder(argc, argv),
    });

    Platform::Info::SetPlatformInstance(&platform);

    platform.Run();
}
