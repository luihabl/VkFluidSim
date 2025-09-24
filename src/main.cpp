#include "gui/gui.h"
#include "platform.h"

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

    auto app = vfs::GUI{};

    platform.Init({
        .name = "Vulkan fluid sim",
        .size = {1200, 700},
        .init = [&app](auto& p) { app.Init(p); },
        .update = [&app](auto& p) { app.Update(p); },
        .clean = [&app](auto& p) { app.Clear(); },
        .handler = [&app](auto& p, auto& e) { app.HandleEvent(p, e); },
        .resources_path = GetResourcesFolder(argc, argv),
    });

    Platform::Info::SetPlatformInstance(&platform);

    platform.Run();
}
