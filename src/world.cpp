#include "world.h"

#include <glm/ext.hpp>
#include <random>

#include "gfx/mesh.h"
#include "pipeline.h"
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
    comp_pipeline.Init(gfx.GetCoreCtx());

    SetBox(17, 9);

    comp_pipeline.SetUniformData({
        .dt = 1.0f / 120.0f,
        .g = 0.0f,
        .mass = 1.0f,
        .damping_factor = 0.05f,
        .target_density = 10.0f,
        .pressure_multiplier = 500.0f,
        .smoothing_radius = 0.35f,
        .box = box,
    });

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

void World::SetBox(float w, float h) {
    float win_w = scale * Platform::Info::GetConfig()->w;
    float win_h = scale * Platform::Info::GetConfig()->h;

    box.x = win_w / 2.0 - w / 2.0;
    box.y = win_h / 2.0 - h / 2.0;
    box.z = w;
    box.w = h;
}

void World::SetInitialData() {
    gfx::CPUMesh mesh;

    float w = Platform::Info::GetConfig()->w;
    float h = Platform::Info::GetConfig()->h;

    DrawCircleFill(mesh, glm::vec3(0.0f), 0.05f, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), 50);

    test_mesh = gfx::UploadMesh(gfx, mesh);

    auto sz = sizeof(ComputePipeline::DataPoint) * n_particles;

    b1 = gfx::Buffer::Create(
        gfx.GetCoreCtx(), sz,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    b2 = gfx::Buffer::Create(
        gfx.GetCoreCtx(), sz,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    in_buf = &b1;
    out_buf = &b2;

    auto staging = gfx::Buffer::Create(gfx.GetCoreCtx(), sz, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VMA_MEMORY_USAGE_CPU_ONLY);

    void* data = staging.Map();

    // for (int i = 0; i < sz; i++) {
    //     ((char*)data)[i] = 0;
    // }

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution dist;

    auto* dp = (ComputePipeline::DataPoint*)data;
    for (int i = 0; i < n_particles; i++) {
        float r1 = dist(rng);
        float r2 = dist(rng);

        float x = box.x + r1 * box.z;
        float y = box.y + r2 * box.w;
        dp[i].x = glm::vec2{x, y};
        dp[i].v = glm::vec2{-1.0f + 2.0f * r1, -1.0f + 2.0f * r2};
    }

    gfx.ImmediateSubmit([&](VkCommandBuffer cmd) {
        auto cpy_info = VkBufferCopy{
            .dstOffset = 0,
            .srcOffset = 0,
            .size = sz,
        };

        vkCmdCopyBuffer(cmd, staging.buffer, b1.buffer, 1, &cpy_info);
        vkCmdCopyBuffer(cmd, staging.buffer, b2.buffer, 1, &cpy_info);
    });

    staging.Destroy();
}

void World::HandleEvent(Platform& platform, const SDL_Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_Q) {
        platform.ScheduleQuit();
    }
}

void World::Update(Platform& platform) {
    static int i = 0;

    auto cmd = gfx.BeginFrame();

    auto tr = glm::ortho(0.0f, (float)platform.GetConfig().w, 0.0f, (float)platform.GetConfig().h,
                         -1.0f, 1.0f);

    tr = glm::scale(tr, glm::vec3(1 / scale, 1 / scale, 1.0f));

    std::swap(in_buf, out_buf);

    comp_pipeline.Compute(cmd, gfx, *in_buf, *out_buf);

    auto mem_barrier = VkMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
    };

    auto dep_info = VkDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pMemoryBarriers = &mem_barrier,
        .memoryBarrierCount = 1,

    };

    vkCmdPipelineBarrier2(cmd, &dep_info);

    renderer.Draw(gfx, cmd, test_mesh, *out_buf, tr, n_particles);

    gfx.EndFrame(cmd, renderer.GetDrawImage());
}

void World::Clear() {
    vkDeviceWaitIdle(gfx.GetCoreCtx().device);
    comp_pipeline.Clear(gfx.GetCoreCtx());
    b1.Destroy();
    b2.Destroy();
    gfx::DestroyMesh(gfx, test_mesh);
    renderer.Clear(gfx);
    gfx.Clear();
}

}  // namespace vfs