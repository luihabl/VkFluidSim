#pragma once

#include <SDL3/SDL.h>

#include <functional>
#include <string>

namespace vfs {

struct Platform {
    using EventHandler = std::function<void(Platform&, SDL_Event&)>;
    using PlatformFunc = std::function<void(Platform&)>;

    struct Config {
        int w;
        int h;
        std::string name;
        EventHandler handler;
        PlatformFunc init;
        PlatformFunc update;
        PlatformFunc clean;
    };

    SDL_Window* window;
    Config config;
    bool quit = false;

    void Init(Config&& config);
    void Run();
    void Clean();
};

}  // namespace vfs