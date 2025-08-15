#pragma once
#include "gfx/gfx.h"
#include "platform.h"
namespace vfs {
class World {
public:
    using Event = SDL_Event;

    void Init(Platform& platform);
    void Update(Platform& platform);
    void HandleEvent(Platform& platform, const Event& e);
    void Clean();

private:
    gfx::Device gfx;
};
}  // namespace vfs