#include "platform.h"

#include <fmt/core.h>
#include <fmt/std.h>

#include "SDL3/SDL_version.h"

namespace vfs {

void Platform::Init(int w_in, int h_in, const char* name_in) {
    w = w_in;
    h = h_in;
    name = name_in;

    SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow(name.c_str(), w, h, flags);

    fmt::println("{}", name);

    fmt::println("SDL {}.{}.{}", SDL_VERSIONNUM_MAJOR(SDL_VERSION),
                 SDL_VERSIONNUM_MINOR(SDL_VERSION), SDL_VERSIONNUM_MICRO(SDL_VERSION));
}

void Platform::Run(EventHandler&& handler, UpdateFunc&& update) {
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_EVENT_QUIT)
                quit = true;

            handler(this, &e);
        }

        update(this);
    }
}

void Platform::Clean() {}

}  // namespace vfs