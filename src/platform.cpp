#include "platform.h"

#include <fmt/core.h>
#include <fmt/std.h>

#include "SDL3/SDL_version.h"

namespace vfs {

void Platform::Init(Config&& config_) {
    config = std::move(config_);

    SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow(config.name.c_str(), config.w, config.h, flags);

    fmt::println("{}", config.name);

    fmt::println("SDL {}.{}.{}", SDL_VERSIONNUM_MAJOR(SDL_VERSION),
                 SDL_VERSIONNUM_MINOR(SDL_VERSION), SDL_VERSIONNUM_MICRO(SDL_VERSION));
}

void Platform::Run() {
    if (config.init)
        config.init(*this);

    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_EVENT_QUIT)
                quit = true;
            if (config.handler)
                config.handler(*this, e);
        }
        if (config.update)
            config.update(*this);
    }

    if (config.clean)
        config.clean(*this);
}

void Platform::Clean() {}

}  // namespace vfs