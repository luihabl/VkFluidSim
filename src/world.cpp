#include "world.h"

#include "gfx/mesh.h"

namespace vfs {
void World::Init(Platform& platform) {
    gfx.Init({
        .name = "Vulkan fluid sim",
        .window = platform.GetWindow(),
        .validation_layers = true,
    });

    auto ext = gfx.GetSwapchainExtent();
    renderer.Init(gfx, ext.width, ext.height);

    SetInitialData();
}

void World::SetInitialData() {
    gfx::CPUMesh mesh;

    mesh.vertices.resize(4);
    mesh.vertices[0].pos = {0.5, -0.5, 0};
    mesh.vertices[1].pos = {0.5, 0.5, 0};
    mesh.vertices[2].pos = {-0.5, -0.5, 0};
    mesh.vertices[3].pos = {-0.5, 0.5, 0};

    mesh.vertices[0].color = {0, 0, 0, 1};
    mesh.vertices[1].color = {0.5, 0.5, 0.5, 1};
    mesh.vertices[2].color = {1, 0, 0, 1};
    mesh.vertices[3].color = {0, 1, 0, 1};

    mesh.indices.resize(6);
    mesh.indices[0] = 0;
    mesh.indices[1] = 1;
    mesh.indices[2] = 2;

    mesh.indices[3] = 2;
    mesh.indices[4] = 1;
    mesh.indices[5] = 3;

    test_mesh = gfx::UploadMesh(gfx, mesh);
}

void World::HandleEvent(Platform& platform, const SDL_Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_Q) {
        platform.ScheduleQuit();
    }
}

void World::Update(Platform& platform) {
    static int i = 0;

    auto cmd = gfx.BeginFrame();

    renderer.Draw(gfx, cmd, test_mesh);

    gfx.EndFrame(cmd, renderer.GetDrawImage());
}

void World::Clean() {
    vkDeviceWaitIdle(gfx.GetCoreCtx().device);
    gfx::DestroyMesh(gfx, test_mesh);
    renderer.Clean(gfx);
    gfx.Clean();
}

}  // namespace vfs