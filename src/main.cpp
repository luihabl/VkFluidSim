#include <argparse/argparse.hpp>

#include "gui/gui.h"
#include "platform.h"

int main(int argc, char* argv[]) {
    using namespace vfs;

    auto input_file = std::string{};
    argparse::ArgumentParser arg_parser("VkFluidSim");
    arg_parser.add_argument("--input", "-i")
        .help("JSON file describing the simulation scene")
        .nargs(1)
        .store_into(input_file);

    auto resources = std::string{};
    if (const char* env = std::getenv("VK_FLUID_SIM_RESOURCES_PATH")) {
        resources = env;
    } else {
        auto p = std::filesystem::path(argv[0]);
        resources = p.parent_path() / "assets";
    }

    arg_parser.add_argument("--resources", "-r")
        .help("path to resource folder")
        .nargs(1)
        .default_value(resources)
        .store_into(resources);

    arg_parser.parse_args(argc, argv);

    auto platform = vfs::Platform{};
    auto app = vfs::GUI{};

    platform.Init({
        .name = "VkFluidSim",
        .size = {1200, 700},
        .init = [&app, input_file](auto& p) { app.Init(p, input_file); },
        .update = [&app](auto& p) { app.Update(p); },
        .clean = [&app](auto& p) { app.Clear(); },
        .handler = [&app](auto& p, auto& e) { app.HandleEvent(p, e); },
        .resources_path = resources,
    });

    Platform::Info::SetPlatformInstance(&platform);

    platform.Run();
}
