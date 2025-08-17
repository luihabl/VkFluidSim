#pragma once

#include <SDL3/SDL.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

#include "SDL3/SDL_video.h"

namespace vfs {

class Platform {
public:
    using EventHandler = std::function<void(Platform&, SDL_Event&)>;
    using PlatformFunc = std::function<void(Platform&)>;
    using Path = std::filesystem::path;

    struct Config {
        int w;
        int h;
        std::string name;
        EventHandler handler;
        PlatformFunc init;
        PlatformFunc update;
        PlatformFunc clean;
        Path resources_path = ".";
    };

    struct Info {
        static void SetPlatformInstance(Platform* platform);
        static Path ResourcePath(const char* resource);
        static Path ResourceFolder();
        static const Config* GetConfig();
    };

    void Init(Config&& config);
    void Run();
    void Clean();
    Path ResourcePath(const char* resource) const;

    SDL_Window* GetWindow() const { return window; }
    const Config& GetConfig() const { return config; }
    void ScheduleQuit() { quit = true; }

private:
    SDL_Window* window;
    Config config;
    bool quit = false;
};

}  // namespace vfs