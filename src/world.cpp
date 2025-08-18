#include "world.h"

#include <glm/ext.hpp>

#include "gfx/mesh.h"
#include "platform.h"

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

void DrawCircleFill(gfx::CPUMesh& mesh,
                    const glm::vec3& center,
                    float radius,
                    const glm::vec4& color,
                    int steps) {
    float cx = center.x;
    float cy = center.y;

    for (int i = 0; i < steps; i++) {
        float angle0 = (float)i * 2 * M_PI / (float)steps;
        float angle1 = (float)(i + 1) * 2 * M_PI / (float)steps;

        auto p0 = glm::vec3(cx, cy, 0.0f);
        auto p1 = glm::vec3(cx + radius * sinf(angle0), cy + radius * cosf(angle0), 0.0f);
        auto p2 = glm::vec3(cx + radius * sinf(angle1), cy + radius * cosf(angle1), 0.0f);

        const unsigned int n = (unsigned int)mesh.vertices.size();
        mesh.indices.insert(mesh.indices.end(), {n + 0, n + 2, n + 1});
        mesh.vertices.insert(mesh.vertices.end(), {{.pos = p0, .color = color},
                                                   {.pos = p1, .color = color},
                                                   {.pos = p2, .color = color}});
    }
}

void World::SetInitialData() {
    gfx::CPUMesh mesh;

    float w = Platform::Info::GetConfig()->w;
    float h = Platform::Info::GetConfig()->h;

    DrawCircleFill(mesh, glm::vec3(w / 2, h / 2, 0.0f), 10.0f, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
                   50);

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

    auto proj = glm::ortho(0.0f, (float)platform.GetConfig().w, 0.0f, (float)platform.GetConfig().h,
                           0.0f, 1.0f);

    static float t = 0.0f;
    auto transform = glm::translate(proj, glm::vec3(500.0f * std::sin(t), 0.0f, 0.0f));
    t += 0.01f;

    renderer.Draw(gfx, cmd, test_mesh, transform);

    gfx.EndFrame(cmd, renderer.GetDrawImage());
}

void World::Clean() {
    vkDeviceWaitIdle(gfx.GetCoreCtx().device);
    gfx::DestroyMesh(gfx, test_mesh);
    renderer.Clean(gfx);
    gfx.Clean();
}

}  // namespace vfs