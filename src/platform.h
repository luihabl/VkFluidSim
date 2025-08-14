#pragma once

#include <SDL3/SDL.h>

#include <functional>
#include <string>

namespace vfs {

struct Platform {
    using EventHandler = std::function<void(Platform*, SDL_Event*)>;
    using UpdateFunc = std::function<void(Platform*)>;

    SDL_Window* window;
    std::string name;
    int w, h;
    bool quit = false;

    void Init(int w, int h, const char* name);
    void Run(EventHandler&& handler, UpdateFunc&& update);
    void Clean();
};

}  // namespace vfs