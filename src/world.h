#pragma once

#include "gfx/gfx.h"
#include "gfx/mesh.h"
#include "platform.h"
#include "renderer.h"

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
    Renderer renderer;

    gfx::GPUMesh test_mesh;
    void SetInitialData();
};
}  // namespace vfs